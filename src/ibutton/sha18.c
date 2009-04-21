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
// sha18.c - Low-level memory and SHA functions for the DS1963S.
//
// Version: 2.10
//

#include "ownet.h"
#include "shaib.h"

//--------------------------------------------------------------------------
// Attempt to erase the scratchpad of the specified SHA iButton for DS1963S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - value for TA2/TA1 (Note: TA2/TA1 aren't always set by
//                 Erase Scratchpad command.)
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Returns: TRUE, success, scratchpad erased
//          FALSE, failure to erase scratchpad
//
SMALLINT EraseScratchpadSHA18(int portnum, int address, SMALLINT resume)
{
   uchar send_block[50];
   int num_verf;
   short send_cnt=0;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   // change number of verification bytes if in overdrive
   num_verf = (in_overdrive[portnum&0x0FF]) ? 6 : 2;

   // erase scratchpad command
   send_block[send_cnt++] = CMD_ERASE_SCRATCHPAD;
   // TA1
   send_block[send_cnt++] = (uchar)(address & 0xFF);
   // TA2
   send_block[send_cnt++] = (uchar)((address >> 8) & 0xFF);

   // now add the verification bytes
   //for (i = 0; i < num_verf; i++)
   //   send_block[send_cnt++] = 0xFF;
   memset(&send_block[send_cnt], 0x0FF, num_verf);
   send_cnt += num_verf;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // check verification
   OWASSERT( ((send_block[send_cnt-1] & 0xF0) == 0x50) ||
             ((send_block[send_cnt-1] & 0xF0) == 0xA0),
             OWERROR_NO_COMPLETION_BYTE, FALSE);

   return TRUE;
}

//----------------------------------------------------------------------
// Read the scratchpad with CRC16 verification for DS1963S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - pointer to address that is read from scratchpad
// 'es'          - pointer to offset byte read from scratchpad
// 'data'        - pointer data buffer read from scratchpad
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - scratch read, address, es, and data returned
//         FALSE - error reading scratch, device not present
//
//
SMALLINT ReadScratchpadSHA18(int portnum, int* address, uchar* es,
                           uchar* data, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[40];
   SMALLINT i;
   ushort lastcrc16;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
      resume = 1; // for addition later
   }

   // read scratchpad command
   send_block[send_cnt++] = CMD_READ_SCRATCHPAD;

   // now add the read bytes for data bytes and crc16
   //for (i = 0; i < 37; i++)
   //   send_block[send_cnt++] = 0xFF;
   memset(&send_block[send_cnt], 0x0FF, 37);
   send_cnt += 37;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // calculate CRC16 of result
   setcrc16(portnum,0);
   for (i = resume; i < send_cnt ; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );

   // copy data to return buffers
   if(address)
      *address = (send_block[2+resume] << 8) | send_block[1+resume];
   if(es)
      *es = send_block[3+resume];

   memcpy(data, &send_block[4+resume], 32);

   // success
   return TRUE;
}

//----------------------------------------------------------------------
// Write the scratchpad with CRC16 verification for DS1963S.  The data
// is padded until the offset is 0x1F so that the CRC16 is retrieved.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - address to write data to
// 'data'        - data to write
// 'data_len'    - number of bytes of data to write
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - write to scratch verified
//         FALSE - error writing scratch, device not present, or HIDE
//                 flag is in incorrect state for address being written.
//
SMALLINT WriteScratchpadSHA18(int portnum, int address, uchar *data,
                              SMALLINT data_len, SMALLINT resume)
{
   uchar send_block[50];
   short send_cnt=0,i;
   ushort lastcrc16;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   setcrc16(portnum,0);
   // write scratchpad command
   send_block[send_cnt] = CMD_WRITE_SCRATCHPAD;
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   // address 1
   send_block[send_cnt] = (uchar)(address & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   // address 2
   send_block[send_cnt] = (uchar)((address >> 8) & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   // data
   for (i = 0; i < data_len; i++)
   {
      send_block[send_cnt] = data[i];
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   }
   // pad if needed
   for (i = 0; i < (0x1F - ((address + data_len - 1) & 0x1F)); i++)
   {
      send_block[send_cnt] = 0xFF;
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   }
   // CRC16
   send_block[send_cnt++] = 0xFF;
   send_block[send_cnt++] = 0xFF;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // perform CRC16 of last 2 byte in packet
   for (i = send_cnt - 2; i < send_cnt; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );

   // success
   return TRUE;
}


//----------------------------------------------------------------------
// Copy the scratchpad with verification for DS1963S.  Assume that the
// data was padded to get the CRC16 verification on write scratchpad.
// This will result in the 'es' byte to be 0x1F.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'address'     - address of destination
// 'len'         - length of data
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - copy scratch verified
//         FALSE - error during copy scratch, device not present, or HIDE
//                 flag is in incorrect state for address being written.
//
SMALLINT CopyScratchpadSHA18(int portnum, int address,
                             SMALLINT len, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[10];
   int num_verf;
   uchar es = (address + len - 1) & 0x1F;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   // change number of verification bytes if in overdrive
   num_verf = (in_overdrive[portnum&0x0FF]) ? 4 : 2;

   // copy scratchpad command
   send_block[send_cnt++] = CMD_COPY_SCRATCHPAD;
   // address 1
   send_block[send_cnt++] = (uchar)(address & 0xFF);
   // address 2
   send_block[send_cnt++] = (uchar)((address >> 8) & 0xFF);
   // es
   send_block[send_cnt++] = es;

   // verification bytes
   //for (i = 0; i < num_verf; i++)
   //   send_block[send_cnt++] = 0xFF;
   memset(&send_block[send_cnt], 0x0FF, num_verf);
   send_cnt += num_verf;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // check verification
   OWASSERT( ((send_block[send_cnt-1] & 0xF0) == 0x50) ||
             ((send_block[send_cnt-1] & 0xF0) == 0xA0),
             OWERROR_NO_COMPLETION_BYTE, FALSE );

   return TRUE;
}

//----------------------------------------------------------------------
// Perform a scratchpad match using provided data. Fixed data length
// of 20 bytes.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'data'        - data to use in match scratch operation
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - valid match
//         FALSE - no match or device not present
//
SMALLINT MatchScratchpadSHA18(int portnum, uchar* data, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[50];
   int i;
   ushort lastcrc16;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   setcrc16(portnum,0);
   // match scratchpad command
   send_block[send_cnt] = CMD_MATCH_SCRATCHPAD;
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

   // send 20 data bytes
   for (i = 0; i < 20; i++)
   {
      send_block[send_cnt] = data[i];
      lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   }
   // send two crc bytes and verification byte
   //for (i = 0; i < 3; i++)
   //   send_block[send_cnt++] = 0xFF;
   memset(&send_block[send_cnt], 0x0FF, 3);
   send_cnt += 3;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // check the CRC
   for (i = (send_cnt - 3); i < (send_cnt - 1); i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );

   // check verification
   if(send_block[send_cnt - 1] != (uchar)0xFF)
      return TRUE;
   else
      return FALSE;
}

//----------------------------------------------------------------------
// Read Memory Page for DS1963S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'pagenum'     - page number to do a read authenticate
// 'data'        - buffer to read into from page
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - Read successfull
//         FALSE - error occurred during read.
//
SMALLINT ReadMemoryPageSHA18(int portnum, SMALLINT pagenum,
						   uchar* data, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[36];
   int address = pagenum << 5;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, -1 );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   // create the send block
   // Read Memory command
   send_block[send_cnt++] = CMD_READ_MEMORY;
   // TA1
   send_block[send_cnt++] = (uchar)(address & 0xFF);
   // TA2
   send_block[send_cnt++] = (uchar)((address >> 8) & 0xFF);

   // now add the read bytes for data bytes
   memset(&send_block[send_cnt], 0x0FF, 32);
   send_cnt += 32;

   // now send the block
   OWASSERT( owBlock(portnum,TRUE,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   // transfer the results
   memcpy(data, &send_block[send_cnt-32], 32);

   return TRUE;
}

//----------------------------------------------------------------------
// Read Authenticated Page for DS1963S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'pagenum'     - page number to do a read authenticate
// 'data'        - buffer to read into from page
// 'sign'        - buffer for storing sha computation
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: Value of write cycle counter for the page
//         -1 for error
//
int ReadAuthPageSHA18(int portnum, SMALLINT pagenum, uchar* data,
                    uchar* sign, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[56];
   short num_verf;
   SMALLINT i;
   ushort lastcrc16;
   int address = pagenum*32, wcc = -1;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, -1 );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   //seed the crc
   setcrc16(portnum,0);

   // change number of verification bytes if in overdrive
   num_verf = (in_overdrive[portnum&0x0FF]) ? 10 : 2;

   // create the send block
   // Read Authenticated Page command
   send_block[send_cnt] = CMD_READ_AUTH_PAGE;
   lastcrc16 = docrc16(portnum, send_block[send_cnt++]);

   // TA1
   send_block[send_cnt] = (uchar)(address & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

   // TA2
   send_block[send_cnt] = (uchar)((address >> 8) & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

   // now add the read bytes for data bytes, counter, and crc16, verification
   //for (i = 0; i < (42 + num_verf); i++)
   //   send_block[send_cnt++] = 0xFF;
   memset(&send_block[send_cnt], 0x0FF, 42+num_verf);
   send_cnt += 42+num_verf;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, -1 );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // check the CRC
   for (i = resume?4:3; i < (send_cnt - num_verf); i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, -1);

   // check verification
   OWASSERT( ((send_block[send_cnt-1] & 0xF0) == 0x50) ||
             ((send_block[send_cnt-1] & 0xF0) == 0xA0),
             OWERROR_NO_COMPLETION_BYTE, -1);

   // transfer results
   //cnt = 0;
   //for (i = send_cnt - 42 - num_verf; i < (send_cnt - 10 - num_verf); i++)
   //   data[cnt++] = send_block[i];
   memcpy(data, &send_block[send_cnt-42-num_verf], 32);

   wcc = BytesToInt(&send_block[send_cnt-10-num_verf], 4);

   if(sign!=NULL)
   {
      OWASSERT( ReadScratchpadSHA18(portnum, 0, 0, send_block, TRUE),
                OWERROR_READ_SCRATCHPAD_FAILED, FALSE );

      //for(i=0; i<20; i++)
      //   sign[i] = send_block[i+8];
      memcpy(sign, &send_block[8], 20);
   }

   return wcc;
}

//----------------------------------------------------------------------
// Write Data Page for DS1963S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'pagenum'     - page number to write to
// 'data'        - buffer to write into page
// 'resume'      - if true, device access is resumed using the RESUME
//                 ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                 used along with the device's entire address number.
//
// Return: TRUE - Write successfull
//         FALSE - error occurred during write.
//
SMALLINT WriteDataPageSHA18(int portnum, SMALLINT pagenum,
                          uchar* data, SMALLINT resume)
{
   uchar buffer[32];
   int addr = pagenum << 5, addr_buff;
   uchar es = 0;

   OWASSERT( EraseScratchpadSHA18(portnum, addr, resume),
             OWERROR_ERASE_SCRATCHPAD_FAILED, FALSE );

   OWASSERT( WriteScratchpadSHA18(portnum, addr, data, 32, TRUE),
              OWERROR_WRITE_SCRATCHPAD_FAILED, FALSE );

   OWASSERT( ReadScratchpadSHA18(portnum, &addr_buff, &es, buffer, TRUE),
             OWERROR_READ_SCRATCHPAD_FAILED, FALSE );

   // verify that what we read is exactly what we wrote
   OWASSERT( (addr == addr_buff) && (es == 0x1F)
                 && (memcmp(buffer, data, 32)==0),
             OWERROR_READ_SCRATCHPAD_FAILED, FALSE );

   OWASSERT( CopyScratchpadSHA18(portnum, addr, 32, TRUE),
             OWERROR_COPY_SCRATCHPAD_FAILED, FALSE );

   return TRUE;

}

//----------------------------------------------------------------------
// Compute sha command based on control_byte and page address.
//
// 'portnum'      - number 0 to MAX_PORTNUM-1.  This number is provided to
//                  indicate the symbolic port number.
// 'control_byte' - control byte used in sha operation
// 'address'      - address used in compute sha operation
// 'resume'       - if true, device access is resumed using the RESUME
//                  ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                  used along with the device's entire address number.
//
// Return: TRUE - compute sha finished
//         FALSE - CRC error, device not present
//
SMALLINT SHAFunction18(int portnum, uchar control_byte,
                    int address, SMALLINT resume)
{
   short send_cnt=0;
   uchar send_block[18];
   int i,num_verf;
   ushort lastcrc16;

   if(!resume)
   {
      // access the device
      OWASSERT( SelectSHA(portnum), OWERROR_ACCESS_FAILED, FALSE );
   }
   else
   {
      // transmit RESUME command
      send_block[send_cnt++] = ROM_CMD_RESUME;
   }

   setcrc16(portnum,0);
   // change number of verification bytes if in overdrive
   num_verf = (in_overdrive[portnum&0x0FF]) ? 10 : 2;

   // Compute SHA Command
   send_block[send_cnt] = CMD_COMPUTE_SHA;
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   // address 1
   send_block[send_cnt] = (uchar)(address & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   // address 2
   send_block[send_cnt] = (uchar)((address >> 8) & 0xFF);
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);
   // control byte
   send_block[send_cnt] = control_byte;
   lastcrc16 = docrc16(portnum,send_block[send_cnt++]);

   // now read bytes crc16, and verification
   //for (i = 0; i < 2 + num_verf; i++)
   //   send_block[send_cnt++] = 0xFF;
   memset(&send_block[send_cnt], 0x0FF, 2+num_verf);
   send_cnt += 2+num_verf;

   // now send the block
   OWASSERT( owBlock(portnum,resume,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // check the CRC
   for (i = resume?5:4; i < (send_cnt - num_verf); i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );

   // check verification
   OWASSERT( ((send_block[send_cnt-1] & 0xF0) == 0x50) ||
             ((send_block[send_cnt-1] & 0xF0) == 0xA0),
             OWERROR_NO_COMPLETION_BYTE, FALSE );

   return TRUE;
}

//----------------------------------------------------------------------
// Copies hidden scratchpad data into specified secret.  Resume command
// is used by default.
//
// 'portnum'      - number 0 to MAX_PORTNUM-1.  This number is provided to
//                  indicate the symbolic port number.
// 'secretnum'    - secret number to replace with scratchpad data.
//
// Return: TRUE - copy secret succeeded
//         FALSE - error or device not present
//
SMALLINT CopySecretSHA18(int portnum, SMALLINT secretnum)
{
   // change number of verification bytes if in overdrive
   SMALLINT num_verf = (in_overdrive[portnum&0x0FF]) ? 10 : 2;

   // each page has 4 secrets, so look at 2 LS bits to
   // determine offset in the page.
   SMALLINT secret_offset = (secretnum&3) << 3;

   // secrets 0-3 are stored starting at address 0200h
   // and 4-7 are stored starting at address 0220h.
   int address = (secretnum<4 ? 0x0200 : 0x0220)
                 + secret_offset;

   SMALLINT length = 32 - secret_offset;
   SMALLINT send_cnt = 0, i;
   uchar send_block[38];
   ushort lastcrc16;

   //Since other functions must be called before this one
   //that are communicating with the button, resume is assumed.
   send_block[send_cnt++] = ROM_CMD_RESUME;
   send_block[send_cnt++] = CMD_WRITE_SCRATCHPAD;
   send_block[send_cnt++] = (uchar)address;
   send_block[send_cnt++] = (uchar)(address>>8);

   //for(i=0; i<length+2; i++)
   //   send_block[send_cnt++] = (uchar)0x0FF;
   memset(&send_block[send_cnt], 0x0FF, 2+length);
   send_cnt += 2+length;


   // now send the block
   OWASSERT( owBlock(portnum,TRUE,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // calculate CRC16 of result
   setcrc16(portnum,0);
   for (i = 1; i < send_cnt ; i++)
      lastcrc16 = docrc16(portnum,send_block[i]);

   // verify CRC16 is correct
   OWASSERT( lastcrc16==0xB001, OWERROR_CRC_FAILED, FALSE );

   // Now read TA1/TA2 and ES, but not rest of data;
   send_cnt = 1;
   send_block[send_cnt++] = CMD_READ_SCRATCHPAD;

   //for(i=0; i<3; i++)
   //   send_block[send_cnt++] = (uchar)0x0FF;
   memset(&send_block[send_cnt], 0x0FF, 3);
   send_cnt += 3;

   // now send the block to get TA1/TA2 and ES
   OWASSERT( owBlock(portnum,TRUE,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // Use the same buffer to call copyScratchpad with proper
   // authorization bytes.
   send_block[1] = CMD_COPY_SCRATCHPAD;

   //for(i=0; i<num_verf; i++)
   //   send_block[send_cnt++] = (uchar)0x0FF;
   memset(&send_block[send_cnt], 0x0FF, num_verf);
   send_cnt += num_verf;

   // now send the block
   OWASSERT( owBlock(portnum,TRUE,send_block,send_cnt),
             OWERROR_BLOCK_FAILED, FALSE );

   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      debugout(send_block,send_cnt);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // check verification
   OWASSERT( ((send_block[send_cnt-1] & 0xF0) == 0x50) ||
             ((send_block[send_cnt-1] & 0xF0) == 0xA0),
             OWERROR_NO_COMPLETION_BYTE, FALSE );

   return TRUE;
}

//----------------------------------------------------------------------
// Installs new system secret for DS1931S.  input_secret must be
// divisible by 47.  Then, each block of 47 is split up with 32 bytes
// written to a data page and 15 bytes are written to the scratchpad.
// Then, Compute first secret is called (or compute next if the secret
// is divisible by 47 more than once).
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'pagenum'       - page number to do a read authenticate
// 'secretnum'     - destination secret for computation results
// 'input_secret'  - the input secret buffer used.
// 'secret_length' - the length of the input secret buffer, divisibly
//                   by 47.
// 'resume'        - if true, device access is resumed using the RESUME
//                   ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                   used along with the device's entire address number.
//
// Return: TRUE - Install successfull
//         FALSE - error occurred during secret installation.
//
SMALLINT InstallSystemSecret18(int portnum, SMALLINT pagenum,
                               SMALLINT secretnum, uchar* secret,
                               int secret_length, SMALLINT resume)
{
   int addr = pagenum << 5;
   int offset = 0, bytes_left = 0;
   uchar data[32],scratchpad[32];

   for(offset=0; offset<secret_length; offset+=47)
   {
      // clear the buffer
      memset(data, 0x0FF, 32);
      memset(scratchpad, 0x0FF, 32);

      // Determine the amount of bytes remaining to be installed.
      bytes_left = secret_length - offset;

      // copy secret data into page buffer and scratchpad buffer
      memcpy(data, &secret[offset], (bytes_left<32?bytes_left:32));
      if(bytes_left>32)
      {
         memcpy(&scratchpad[8], &secret[offset+32],
                (bytes_left<47?bytes_left-32:15));
      }

      // write secret data into data page
      OWASSERT( WriteDataPageSHA18(portnum, pagenum, data, resume),
                 OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );

      // write secret data into scratchpad
      OWASSERT( WriteScratchpadSHA18(portnum, addr, scratchpad, 32, TRUE),
                 OWERROR_WRITE_SCRATCHPAD_FAILED, FALSE );

      // perform secret computation
      OWASSERT( SHAFunction18(portnum,
                 (uchar)(offset==0 ? SHA_COMPUTE_FIRST_SECRET
                                   : SHA_COMPUTE_NEXT_SECRET), addr, TRUE),
                OWERROR_SHA_FUNCTION_FAILED, FALSE );

      // copy the resulting secret into secret location
      OWASSERT( CopySecretSHA18(portnum, secretnum),
                OWERROR_COPY_SECRET_FAILED, FALSE);

      resume = TRUE;
   }

   return TRUE;
}

//----------------------------------------------------------------------
// Binds unique secret to DS1963S.  bindData must be 32 bytes and
// bindCode must be 15 bytes.
//
// 'portnum'       - number 0 to MAX_PORTNUM-1.  This number is provided to
//                   indicate the symbolic port number.
// 'pagenum'       - page number to do a read authenticate
// 'secretnum'     - destination secret for computation results
// 'bindData'      - the input data written to the data page for unique
//                   secret computation.
// 'bindCode'      - the input data written to the scratchpad for unique
//                   secret computation.
// 'resume'        - if true, device access is resumed using the RESUME
//                   ROM command (0xA5).  Otherwise, a a MATCH ROM is
//                   used along with the device's entire address number.
//
// Return: TRUE - bind successfull
//         FALSE - error occurred during secret installation.
//
SMALLINT BindSecretToiButton18(int portnum, SMALLINT pagenum,
                               SMALLINT secretnum,
                               uchar* bindData, uchar* bindCode,
                               SMALLINT resume)
{
   int addr = pagenum << 5;
   uchar scratchpad[32];

   memset(scratchpad, 0x00, 32);
   memcpy(&scratchpad[8], bindCode, 15);

   // write secret data into data page
   OWASSERT( WriteDataPageSHA18(portnum, pagenum, bindData, resume),
              OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );

   // write secret data into scratchpad
   OWASSERT( WriteScratchpadSHA18(portnum, addr, scratchpad, 32, TRUE),
              OWERROR_WRITE_SCRATCHPAD_FAILED, FALSE );

   // perform secret computation
   OWASSERT( SHAFunction18(portnum, (uchar)SHA_COMPUTE_NEXT_SECRET,
                         addr, TRUE),
             OWERROR_SHA_FUNCTION_FAILED, FALSE );

   // copy the resulting secret into secret location
   OWASSERT( CopySecretSHA18(portnum, secretnum),
             OWERROR_COPY_SECRET_FAILED, FALSE);

   return TRUE;
}


