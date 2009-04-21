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
//  ds2480ut.c - DS2480B utility functions.
//
//  Version: 2.01
//
//  History: 1.00 -> 1.01  Default PDSRC changed from 0.83 to 1.37V/us
//                         in DS2480Detect. Changed to use msDelay instead
//                         of Delay.
//           1.01 -> 1.02  Changed global declarations from 'uchar' to 'int'.
//                         Changed DSO/WORT from 7 to 10us in DS2480Detect.
//           1.02 -> 1.03  Removed caps in #includes for Linux capatibility
//           1.03 -> 2.00  Changed 'MLan' to 'ow'. Added support for
//                         multiple ports.  Changed W1LT to 8us.
//           2.00 -> 2.01 Added error handling. Added circular-include check.
//           2.01 -> 2.10 Added raw memory error handling and SMALLINT
//           2.10 -> 3.00 Added memory bank functionality
//                        Added file I/O operations
//

#include "ownet.h"
#include "ds2480.h"

// global DS2480B state
SMALLINT ULevel[MAX_PORTNUM]; // current DS2480B 1-Wire Net level
SMALLINT UBaud[MAX_PORTNUM];  // current DS2480B baud rate
SMALLINT UMode[MAX_PORTNUM];  // current DS2480B command or data mode state
SMALLINT USpeed[MAX_PORTNUM]; // current DS2480B 1-Wire Net communication speed
SMALLINT UVersion[MAX_PORTNUM]; // current DS2480B version 

//---------------------------------------------------------------------------
// Attempt to resyc and detect a DS2480B
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                OpenCOM to indicate the port number.
//
// Returns:  TRUE  - DS2480B detected successfully
//           FALSE - Could not detect DS2480B
//
SMALLINT DS2480Detect(int portnum)
{
   uchar sendpacket[10],readbuffer[10];
   uchar sendlen=0;

   // reset modes
   UMode[portnum] = MODSEL_COMMAND;
   UBaud[portnum] = PARMSET_9600;
   USpeed[portnum] = SPEEDSEL_FLEX;

   // set the baud rate to 9600
   SetBaudCOM(portnum,(uchar)UBaud[portnum]);

   // send a break to reset the DS2480
   BreakCOM(portnum);

   // delay to let line settle
   msDelay(2);

   // flush the buffers
   FlushCOM(portnum);

   // send the timing byte
   sendpacket[0] = 0xC1;
   if (WriteCOM(portnum,1,sendpacket) != 1)
   {
      OWERROR(OWERROR_WRITECOM_FAILED);
      return FALSE;
   }

   // delay to let line settle
   msDelay(4);

   // set the FLEX configuration parameters
   // default PDSRC = 1.37Vus
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_SLEW | PARMSET_Slew1p37Vus;
   // default W1LT = 10us
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_WRITE1LOW | PARMSET_Write10us;
   // default DSO/WORT = 8us
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_SAMPLEOFFSET | PARMSET_SampOff8us;

   // construct the command to read the baud rate (to test command block)
   sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3);

   // also do 1 bit operation (to test 1-Wire block)
   sendpacket[sendlen++] = CMD_COMM | FUNCTSEL_BIT | UBaud[portnum] | BITPOL_ONE;

   // flush the buffers
   FlushCOM(portnum);

   // send the packet
   if (WriteCOM(portnum,sendlen,sendpacket))
   {
      // read back the response
      if (ReadCOM(portnum,5,readbuffer) == 5)
      {
         // look at the baud rate and bit operation
         // to see if the response makes sense
         if (((readbuffer[3] & 0xF1) == 0x00) &&
             ((readbuffer[3] & 0x0E) == UBaud[portnum]) &&
             ((readbuffer[4] & 0xF0) == 0x90) &&
             ((readbuffer[4] & 0x0C) == UBaud[portnum]))
            return TRUE;
         else
            OWERROR(OWERROR_DS2480_BAD_RESPONSE);
      }
      else
         OWERROR(OWERROR_READCOM_FAILED);
   }
   else
      OWERROR(OWERROR_WRITECOM_FAILED);

   return FALSE;
}

//---------------------------------------------------------------------------
// Change the DS2480B from the current baud rate to the new baud rate.
//
// 'portnum' - number 0 to MAX_PORTNUM-1.  This number was provided to
//             OpenCOM to indicate the port number.
// 'newbaud' - the new baud rate to change to, defined as:
//               PARMSET_9600     0x00
//               PARMSET_19200    0x02
//               PARMSET_57600    0x04
//               PARMSET_115200   0x06
//
// Returns:  current DS2480B baud rate.
//
SMALLINT DS2480ChangeBaud(int portnum, uchar newbaud)
{
   uchar rt=FALSE;
   uchar readbuffer[5],sendpacket[5],sendpacket2[5];
   uchar sendlen=0,sendlen2=0;

   // see if diffenent then current baud rate
   if (UBaud[portnum] == newbaud)
      return UBaud[portnum];
   else
   {
      // build the command packet
      // check if correct mode
      if (UMode[portnum] != MODSEL_COMMAND)
      {
         UMode[portnum] = MODSEL_COMMAND;
         sendpacket[sendlen++] = MODE_COMMAND;
      }
      // build the command
      sendpacket[sendlen++] = CMD_CONFIG | PARMSEL_BAUDRATE | newbaud;

      // flush the buffers
      FlushCOM(portnum);

      // send the packet
      if (!WriteCOM(portnum,sendlen,sendpacket))
      {
         OWERROR(OWERROR_WRITECOM_FAILED);
         rt = FALSE;
      }
      else
      {
         // make sure buffer is flushed
         msDelay(5);

         // change our baud rate
         SetBaudCOM(portnum,newbaud);
         UBaud[portnum] = newbaud;

         // wait for things to settle
         msDelay(5);

         // build a command packet to read back baud rate
         sendpacket2[sendlen2++] = CMD_CONFIG | PARMSEL_PARMREAD | (PARMSEL_BAUDRATE >> 3);

         // flush the buffers
         FlushCOM(portnum);

         // send the packet
         if (WriteCOM(portnum,sendlen2,sendpacket2))
         {
            // read back the 1 byte response
            if (ReadCOM(portnum,1,readbuffer) == 1)
            {
               // verify correct baud
               if (((readbuffer[0] & 0x0E) == (sendpacket[sendlen-1] & 0x0E)))
                  rt = TRUE;
               else
                  OWERROR(OWERROR_DS2480_WRONG_BAUD);
            }
            else
               OWERROR(OWERROR_READCOM_FAILED);
         }
         else
            OWERROR(OWERROR_WRITECOM_FAILED);
      }
   }

   // if lost communication with DS2480 then reset
   if (rt != TRUE)
      DS2480Detect(portnum);

   return UBaud[portnum];
}
