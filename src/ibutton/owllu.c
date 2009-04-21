//---------------------------------------------------------------------------
// Copyright (C) 2000 Dallas Semiconductor Corporation, All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL DALLAS SEMICONDUCTOR BE LIABLE FOR ANY CLAIM, DAMAGES
// OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// Except as contained in this notice, the name of Dallas Semiconductor
// shall not be used except as stated in the Dallas Semiconductor
// Branding Policy.
//---------------------------------------------------------------------------
//
//  owLLU.C - Link Layer 1-Wire Net functions using the DS2480/DS2480B (U)
//            serial interface chip.
//
//  Version: 3.00
//
//  History: 1.00 -> 1.01  DS2480 version number now ignored in
//                         owTouchReset.
//           1.02 -> 1.03  Removed caps in #includes for Linux capatibility
//                         Removed #include <windows.h>
//                         Add #include "ownet.h" to define TRUE,FALSE
//           1.03 -> 2.00  Changed 'MLan' to 'ow'. Added support for
//                         multiple ports.
//           2.00 -> 2.01 Added error handling. Added circular-include check.
//           2.01 -> 2.10 Added raw memory error handling and SMALLINT
//           2.10 -> 3.00 Added memory bank functionality
//                        Added file I/O operations
//                        Added owReadBitPower and owWriteBytePower
//                        Added support for THE LINK
//                        Updated owLevel to match AN192
//

#include "ownet.h"
#include "ds2480.h"

int dodebug=0;

// external globals
extern SMALLINT ULevel[MAX_PORTNUM]; // current DS2480B 1-Wire Net level
extern SMALLINT UBaud[MAX_PORTNUM];  // current DS2480B baud rate
extern SMALLINT UMode[MAX_PORTNUM];  // current DS2480B command or data mode state
extern SMALLINT USpeed[MAX_PORTNUM]; // current DS2480B 1-Wire Net communication speed
extern SMALLINT UVersion[MAX_PORTNUM]; // current DS2480B version

// new global for DS1994/DS2404/DS1427.  If TRUE, puts a delay in owTouchReset to compensate for alarming clocks.
SMALLINT FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE = FALSE; // default owTouchReset to quickest response.

// local varable flag, true if program voltage available
static SMALLINT ProgramAvailable[MAX_PORTNUM];

//--------------------------------------------------------------------------
// Reset all of the devices on the 1-Wire Net and return the result.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                OpenCOM to indicate the port number.
//
// Returns: TRUE(1):  presense pulse(s) detected, device(s) reset
//          FALSE(0): no presense pulses detected
//
// WARNING: Without setting the above global (FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE)
//          to TRUE, this routine will not function correctly on some
//          Alarm reset types of the DS1994/DS1427/DS2404 with
//          Rev 1,2, and 3 of the DS2480/DS2480B.
//
//
SMALLINT owTouchReset(int portnum)
{
   uchar readbuffer[10],sendpacket[10];
   uchar sendlen=0;

   if (dodebug)
      printf("\nRST ");//??????????????

   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   // check if correct mode
   if (UMode[portnum] != MODSEL_COMMAND)
   {
      UMode[portnum] = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }

   // construct the command
   sendpacket[sendlen++] = (uchar)(CMD_COMM | FUNCTSEL_RESET | USpeed[portnum]);

   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the 1 byte response
      if (ReadCOM(portnum,1,readbuffer) == 1)
      {
         // make sure this byte looks like a reset byte
         if (((readbuffer[0] & RB_RESET_MASK) == RB_PRESENCE) ||
             ((readbuffer[0] & RB_RESET_MASK) == RB_ALARMPRESENCE))
         {
            // check if programming voltage available
            ProgramAvailable[portnum] = ((readbuffer[0] & 0x20) == 0x20);
            UVersion[portnum] = (readbuffer[0] & VERSION_MASK);

            // only check for alarm pulse if DS2404 present and not using THE LINK
            if ((FAMILY_CODE_04_ALARM_TOUCHRESET_COMPLIANCE) &&
                (UVersion[portnum] != VER_LINK))
            {
               msDelay(5); // delay 5 ms to give DS1994 enough time
               FlushCOM(portnum);
            }
            return TRUE;
         }
         else
            OWERROR(OWERROR_RESET_FAILED);

      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   // an error occured so re-sync with DS2480
   DS2480Detect(portnum);

   return FALSE;
}

//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and return the
// result 1 bit read from the 1-Wire Net.  The parameter 'sendbit'
// least significant bit is used and the least significant bit
// of the result is the return bit.
//
// 'portnum' - number 0 to MAX_PORTNUM-1.  This number was provided to
//             OpenCOM to indicate the port number.
// 'sendbit' - the least significant bit is the bit to send
//
// Returns: 0:   0 bit read from sendbit
//          1:   1 bit read from sendbit
//
SMALLINT owTouchBit(int portnum, SMALLINT sendbit)
{
   uchar readbuffer[10],sendpacket[10];
   uchar sendlen=0;

   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   // check if correct mode
   if (UMode[portnum] != MODSEL_COMMAND)
   {
      UMode[portnum] = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }

   // construct the command
   sendpacket[sendlen] = (sendbit != 0) ? BITPOL_ONE : BITPOL_ZERO;
   sendpacket[sendlen++] |= CMD_COMM | FUNCTSEL_BIT | USpeed[portnum];

   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the response
      if (ReadCOM(portnum,1,readbuffer) == 1)
      {
         // interpret the response
         if (((readbuffer[0] & 0xE0) == 0x80) &&
             ((readbuffer[0] & RB_BIT_MASK) == RB_BIT_ONE))
            return 1;
         else
            return 0;
      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   // an error occured so re-sync with DS2480
   DS2480Detect(portnum);

   return 0;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  TRUE: bytes written and echo was the same
//           FALSE: echo was not the same
//
SMALLINT owWriteByte(int portnum, SMALLINT sendbyte)
{
   return (owTouchByte(portnum,sendbyte) == (0xff & sendbyte)) ? TRUE : FALSE;
}


//--------------------------------------------------------------------------
// Send 8 bits of read communication to the 1-Wire Net and and return the
// result 8 bits read from the 1-Wire Net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  8 bits read from 1-Wire Net
//
SMALLINT owReadByte(int portnum)
{
   return owTouchByte(portnum,(SMALLINT)0xFF);
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and return the
// result 8 bits read from the 1-Wire Net.  The parameter 'sendbyte'
// least significant 8 bits are used and the least significant 8 bits
// of the result is the return byte.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant byte)
//
// Returns:  8 bits read from sendbyte
//
SMALLINT owTouchByte(int portnum, SMALLINT sendbyte)
{
   uchar readbuffer[10],sendpacket[10];
   uchar sendlen=0;

   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   // check if correct mode
   if (UMode[portnum] != MODSEL_DATA)
   {
      UMode[portnum] = MODSEL_DATA;
      sendpacket[sendlen++] = MODE_DATA;
   }

   // add the byte to send
   sendpacket[sendlen++] = (uchar)sendbyte;

   // check for duplication of data that looks like COMMAND mode
   if (sendbyte ==(SMALLINT)MODE_COMMAND)
      sendpacket[sendlen++] = (uchar)sendbyte;

   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the 1 byte response
      if (ReadCOM(portnum,1,readbuffer) == 1)
      {
         if (dodebug)
            printf("%02X ",readbuffer[0]);//??????????????

          // return the response
          return (int)readbuffer[0];
      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   // an error occured so re-sync with DS2480
   DS2480Detect(portnum);

   return 0;
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net communucation speed.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number was provided to
//               OpenCOM to indicate the port number.
// 'new_speed' - new speed defined as
//                MODE_NORMAL     0x00
//                MODE_OVERDRIVE  0x01
//
// Returns:  current 1-Wire Net speed
//
SMALLINT owSpeed(int portnum, SMALLINT new_speed)
{
   uchar sendpacket[5];
   uchar sendlen=0;
   uchar rt = FALSE;

   // check if change from current mode
   if (((new_speed == MODE_OVERDRIVE) &&
        (USpeed[portnum] != SPEEDSEL_OD)) ||
       ((new_speed == MODE_NORMAL) &&
        (USpeed[portnum] != SPEEDSEL_FLEX)))
   {
      if (new_speed == MODE_OVERDRIVE)
      {
         // check for unsupported mode in THE LINK
         if (UVersion[portnum] == VER_LINK)
            OWERROR(OWERROR_FUNC_NOT_SUP);
         // if overdrive then switch to higher baud
         else if (DS2480ChangeBaud(portnum,MAX_BAUD) == MAX_BAUD)
         {
            USpeed[portnum] = SPEEDSEL_OD;
            rt = TRUE;
         }
      }
      else if (new_speed == MODE_NORMAL)
      {
         // else normal so set to 9600 baud
         if (DS2480ChangeBaud(portnum,PARMSET_9600) == PARMSET_9600)
         {
            USpeed[portnum] = SPEEDSEL_FLEX;
            rt = TRUE;
         }

      }

      // if baud rate is set correctly then change DS2480 speed
      if (rt)
      {
         // check if correct mode
         if (UMode[portnum] != MODSEL_COMMAND)
         {
            UMode[portnum] = MODSEL_COMMAND;
            sendpacket[sendlen++] = MODE_COMMAND;
         }

         // proceed to set the DS2480 communication speed
         sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_SEARCHOFF | USpeed[portnum];

         // send the packet
         if (!WriteCOM(portnum,sendlen,sendpacket))
         {
            OWERROR(OWERROR_WRITECOM_FAILED);
            rt = FALSE;
            // lost communication with DS2480 then reset
            DS2480Detect(portnum);
         }
      }
   }

   // return the current speed
   return (USpeed[portnum] == SPEEDSEL_OD) ? MODE_OVERDRIVE : MODE_NORMAL;
}

//--------------------------------------------------------------------------
// Set the 1-Wire Net line level.  The values for new_level are
// as follows:
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number was provided to
//               OpenCOM to indicate the port number.
// 'new_level' - new level defined as
//                MODE_NORMAL     0x00
//                MODE_STRONG5    0x02
//                MODE_PROGRAM    0x04
//                MODE_BREAK      0x08 (not supported)
//
// Returns:  current 1-Wire Net level
//
SMALLINT owLevel(int portnum, SMALLINT new_level)
{
   uchar sendpacket[10],readbuffer[10];
   uchar sendlen=0;
   uchar rt=FALSE;

   // check if need to change level
   if (new_level != ULevel[portnum])
   {
      // check if correct mode
      if (UMode[portnum] != MODSEL_COMMAND)
      {
         UMode[portnum] = MODSEL_COMMAND;
         sendpacket[sendlen++] = MODE_COMMAND;
      }

      // check if just putting back to normal
      if (new_level == MODE_NORMAL)
      {
         // stop pulse command
         sendpacket[sendlen++] = MODE_STOP_PULSE;

         // add the command to begin the pulse WITHOUT prime
         sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE | BITPOL_5V | PRIME5V_FALSE;

         // stop pulse command
         sendpacket[sendlen++] = MODE_STOP_PULSE;

         // flush the buffers
         FlushCOM(portnum);

         // send the packet
         if (WriteCOM(portnum,sendlen,sendpacket))
         {
            // read back the 2 byte response
            if (ReadCOM(portnum,2,readbuffer) == 2)
            {
               // check response byte
               if (((readbuffer[0] & 0xE0) == 0xE0) &&
                   ((readbuffer[1] & 0xE0) == 0xE0))
               {
                  rt = TRUE;
                  ULevel[portnum] = MODE_NORMAL;
               }
            }
            else
               OWERROR(OWERROR_READCOM_FAILED);
         }
         else
            OWERROR(OWERROR_WRITECOM_FAILED);
      }
      // set new level
      else
      {
         // strong 5 volts
         if (new_level == MODE_STRONG5)
         {
            // set the SPUD time value
            sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite;
            // add the command to begin the pulse
            sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE | BITPOL_5V;
         }
         // 12 volts
         else if (new_level == MODE_PROGRAM)
         {
            // check if programming voltage available
            if (!ProgramAvailable[portnum])
               return MODE_NORMAL;

            // set the PPD time value
            sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_12VPULSE | PARMSET_infinite;
            // add the command to begin the pulse
            sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_CHMOD | SPEEDSEL_PULSE | BITPOL_12V;
         }

         // flush the buffers
         FlushCOM(portnum);

         // send the packet
         if (WriteCOM(portnum,sendlen,sendpacket))
         {
            // read back the 1 byte response from setting time limit
            if (ReadCOM(portnum,1,readbuffer) == 1)
            {
               // check response byte
               if ((readbuffer[0] & 0x81) == 0)
               {
                  ULevel[portnum] = new_level;
                  rt = TRUE;
               }
            }
            else
               OWERROR(OWERROR_READCOM_FAILED);
         }
         else
            OWERROR(OWERROR_WRITECOM_FAILED);
      }

      // if lost communication with DS2480 then reset
      if (rt != TRUE)
         DS2480Detect(portnum);
   }

   // return the current level
   return ULevel[portnum];
}

//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  successful
//           FALSE program voltage not available
//
SMALLINT owProgramPulse(int portnum)
{
   uchar sendpacket[10],readbuffer[10];
   uchar sendlen=0;

   // check if programming voltage available
   if (!ProgramAvailable[portnum])
      return FALSE;

   // make sure normal level
   owLevel(portnum,MODE_NORMAL);

   // check if correct mode
   if (UMode[portnum] != MODSEL_COMMAND)
   {
      UMode[portnum] = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }

   // set the SPUD time value
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_12VPULSE | PARMSET_512us;

   // pulse command
   sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_CHMOD | BITPOL_12V | SPEEDSEL_PULSE;

   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the 2 byte response
      if (ReadCOM(portnum,2,readbuffer) == 2)
      {
         // check response byte
         if (((readbuffer[0] | CMD_CONFIG) ==
                (CMD_CONFIG | PARMSEL_12VPULSE | PARMSET_512us)) &&
             ((readbuffer[1] & 0xFC) ==
                (0xFC & (CMD_COMM | FUNCTSEL_CHMOD | BITPOL_12V | SPEEDSEL_PULSE))))
            return TRUE;
      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   // an error occured so re-sync with DS2480
   DS2480Detect(portnum);

   return FALSE;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant bit)
//
// Returns:  TRUE: bytes written and echo was the same, strong pullup now on
//           FALSE: echo was not the same
//
SMALLINT owWriteBytePower(int portnum, SMALLINT sendbyte)
{
   uchar sendpacket[10],readbuffer[10];
   uchar sendlen=0;
   uchar rt=FALSE;
   uchar i, temp_byte;

   if (dodebug)
      printf("P%02X ",sendbyte);//??????????????

   // check if correct mode
   if (UMode[portnum] != MODSEL_COMMAND)
   {
      UMode[portnum] = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }

   // set the SPUD time value
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite;

   // construct the stream to include 8 bit commands with the last one
   // enabling the strong-pullup
   temp_byte = sendbyte;
   for (i = 0; i < 8; i++)
   {
      sendpacket[sendlen++] = ((temp_byte & 0x01) ? BITPOL_ONE : BITPOL_ZERO)
                              | CMD_COMM | FUNCTSEL_BIT | USpeed[portnum] |
                              ((i == 7) ? PRIME5V_TRUE : PRIME5V_FALSE);
      temp_byte >>= 1;
   }

   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the 9 byte response from setting time limit
      if (ReadCOM(portnum,9,readbuffer) == 9)
      {
         // check response
         if ((readbuffer[0] & 0x81) == 0)
         {
            // indicate the port is now at power delivery
            ULevel[portnum] = MODE_STRONG5;

            // reconstruct the echo byte
            temp_byte = 0;
            for (i = 0; i < 8; i++)
            {
               temp_byte >>= 1;
               temp_byte |= (readbuffer[i + 1] & 0x01) ? 0x80 : 0;
            }

            if (temp_byte == sendbyte)
               rt = TRUE;
         }
      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   // if lost communication with DS2480 then reset
   if (rt != TRUE)
      DS2480Detect(portnum);

   return rt;
}

//--------------------------------------------------------------------------
// Send 8 bits of communication to the 1-Wire Net and verify that the
// 8 bits read from the 1-Wire Net is the same (write operation).
// The parameter 'sendbyte' least significant 8 bits are used.  After the
// 8 bits are sent change the level of the 1-Wire net.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'sendbyte' - 8 bits to send (least significant bit)
//
// Returns:  TRUE: bytes written and echo was the same, strong pullup now on
//           FALSE: echo was not the same
//
SMALLINT owReadBytePower(int portnum)
{
   uchar sendpacket[10],readbuffer[10];
   uchar sendlen=0;
   uchar rt=FALSE;
   uchar i, temp_byte;

   // check if correct mode
   if (UMode[portnum] != MODSEL_COMMAND)
   {
      UMode[portnum] = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }

   // set the SPUD time value
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite;

   // construct the stream to include 8 bit commands with the last one
   // enabling the strong-pullup
   temp_byte = 0xFF;
   for (i = 0; i < 8; i++)
   {
      sendpacket[sendlen++] = ((temp_byte & 0x01) ? BITPOL_ONE : BITPOL_ZERO)
                              | CMD_COMM | FUNCTSEL_BIT | USpeed[portnum] |
                              ((i == 7) ? PRIME5V_TRUE : PRIME5V_FALSE);
      temp_byte >>= 1;
   }

   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the 9 byte response from setting time limit
      if (ReadCOM(portnum,9,readbuffer) == 9)
      {
         // check response
         if ((readbuffer[0] & 0x81) == 0)
         {
            // indicate the port is now at power delivery
            ULevel[portnum] = MODE_STRONG5;

            // reconstruct the return byte
            temp_byte = 0;
            for (i = 0; i < 8; i++)
            {
               temp_byte >>= 1;
               temp_byte |= (readbuffer[i + 1] & 0x01) ? 0x80 : 0;
            }

            rt = TRUE;
         }
      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   // if lost communication with DS2480 then reset
   if (rt != TRUE)
      DS2480Detect(portnum);

   if (dodebug)
      printf("PFF%02X ",temp_byte);//??????????????

   return temp_byte;
}


//--------------------------------------------------------------------------
// Send 1 bit of communication to the 1-Wire Net and verify that the
// response matches the 'applyPowerResponse' bit and apply power delivery
// to the 1-Wire net.  Note that some implementations may apply the power
// first and then turn it off if the response is incorrect.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'applyPowerResponse' - 1 bit response to check, if correct then start
//                        power delivery
//
// Returns:  TRUE: bit written and response correct, strong pullup now on
//           FALSE: response incorrect
//
SMALLINT owReadBitPower(int portnum, SMALLINT applyPowerResponse)
{
   uchar sendpacket[3],readbuffer[3];
   uchar sendlen=0;
   uchar rt=FALSE;

   // check if correct mode
   if (UMode[portnum] != MODSEL_COMMAND)
   {
      UMode[portnum] = MODSEL_COMMAND;
      sendpacket[sendlen++] = MODE_COMMAND;
   }

   // set the SPUD time value
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_5VPULSE | PARMSET_infinite;

   // enabling the strong-pullup after bit
   sendpacket[sendlen++] = BITPOL_ONE
                           | CMD_COMM | FUNCTSEL_BIT | USpeed[portnum] |
                           PRIME5V_TRUE;
   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the 2 byte response from setting time limit
      if (ReadCOM(portnum,2,readbuffer) == 2)
      {
         // check response to duration set
         if ((readbuffer[0] & 0x81) == 0)
         {
            // indicate the port is now at power delivery
            ULevel[portnum] = MODE_STRONG5;

            // check the response bit
            if ((readbuffer[1] & 0x01) == applyPowerResponse)
               rt = TRUE;
            else
               owLevel(portnum,MODE_NORMAL);

            return rt;
         }
      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   // if lost communication with DS2480 then reset
   if (rt != TRUE)
      DS2480Detect(portnum);

   return rt;
}

//--------------------------------------------------------------------------
// This procedure indicates whether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive.
//
SMALLINT owHasPowerDelivery(int portnum)
{
   return TRUE;
}

//--------------------------------------------------------------------------
// This procedure indicates wether the adapter can deliver power.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  because all userial adapters have over drive.
//
SMALLINT owHasOverDrive(int portnum)
{
   return TRUE;
}
//--------------------------------------------------------------------------
// This procedure creates a fixed 480 microseconds 12 volt pulse
// on the 1-Wire Net for programming EPROM iButtons.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
// Returns:  TRUE  program volatage available
//           FALSE program voltage not available
SMALLINT owHasProgramPulse(int portnum)
{
   return ProgramAvailable[portnum];
}
