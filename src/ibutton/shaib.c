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
// shaibutton.c - Protocol-level functions as well as useful utility
//                functions for sha applications.
//
// Version: 2.10
//

#include "ownet.h"
#include "shaib.h"

//Global
SMALLINT in_overdrive[MAX_PORTNUM];

//---------------------------------------------------------------------
// Uses File I/O API to find the coprocessor with a specific
// coprocessor file name.  Usually 'COPR.0'.
//
// 'copr'   - Structure for holding coprocessor information
// 'fe'     - pointer to file entry structure, with proper filename.
//
// Returns: TRUE, found a valid coprocessor
//          FALSE, no coprocessor is present
//
/*
SMALLINT FindCoprSHA(SHACopr* copr, FileEntry* fe)
{
   SMALLINT FoundCopr = FALSE;
   int data;

   // now get all the SHA iButton parts until we find
   // one that has the right file on it.
   if(FindNewSHA(copr->portnum, copr->devAN, TRUE))
   {
      do
      {
         short handle;

         if(owOpenFile(copr->portnum, copr->devAN, fe, &handle))
         {
            int length = fe->NumPgs << 5;
            uchar* raw = malloc(length*5);

            if(owReadFile(copr->portnum, copr->devAN,
                          handle, raw,
                          length, &length))
            {
               if(!owCloseFile(copr->portnum, copr->devAN, handle))
                  return FALSE;
               if(raw != 0)
                  data = GetCoprFromRawData(copr, raw, length);
               FoundCopr = TRUE;
            }

            free(raw);
         }
      }
      while(!FoundCopr && FindNewSHA(copr->portnum, copr->devAN, FALSE));
   }

   return FoundCopr;
}
*/
//---------------------------------------------------------------------
// Extracts coprocessor configuration information from raw data.
// Returns the last unaccessed index.
//
// 'copr'   - Structure for holding coprocessor information
// 'raw'    - Raw bytes, usually from file stored on disk or on the
//            coprocessor itself.
// 'length' - The length of coprocessor data.
//
int GetCoprFromRawData(SHACopr* copr, uchar* raw, int length)
{
   int i, namelen, siglen, auxlen;
   //copy in the name of the user's service file
   memcpy(copr->serviceFilename, raw, 5);

   //various page numbers
   copr->signPageNumber = raw[5];
   copr->authPageNumber = raw[6];
   copr->wspcPageNumber = raw[7];

   //version information
   copr->versionNumber = raw[8];

   //skip 4 bytes for date info;
   //get bind data, bind code, and signing challenge
   memcpy(copr->bindData, &raw[13], 32);
   memcpy(copr->bindCode, &raw[45], 7);
   memcpy(copr->signChlg, &raw[52], 3);

   namelen = raw[55];
   siglen = raw[56];
   auxlen = raw[57];

   // read in provider name as null-terminated string
   copr->providerName = malloc(namelen+1);
   memcpy(copr->providerName, &raw[58], namelen);
   copr->providerName[namelen] = '\0';

   // get initial signature
   memcpy(copr->initSignature, &raw[58+namelen],
            (siglen>20?20:siglen) );

   // read in auxilliary data as null-terminated string
   copr->auxilliaryData = malloc(auxlen+1);
   memcpy(copr->auxilliaryData,
            &raw[58+namelen+siglen], auxlen);
   copr->auxilliaryData[auxlen] = '\0';

   //encryption code
   i = 58+namelen+siglen+auxlen;
   copr->encCode = raw[i];

   //ds1961S compatibility flag - optional
   if((length>i+1) && (raw[i+1]!=0))
      copr->ds1961Scompatible = TRUE;
   else
      copr->ds1961Scompatible = FALSE;

   return i+2;
}

//---------------------------------------------------------------------
// Uses File I/O API to find the user token with a specific
// service file name.  Usually 'DSLM.102'.
//
// 'user'       - Structure for holding user token information
// 'fe'         - pointer to file entry structure, with proper
//                service filename.
// 'doBlocking' - if TRUE, method blocks until a user token is found.
//
// Returns: TRUE, found a valid user token
//          FALSE, no user token is present
//
/*
SMALLINT FindUserSHA(SHAUser* user, FileEntry* fe,
                     SMALLINT doBlocking)
{
   SMALLINT FoundUser = FALSE;

   for(;;)
   {
      // now get all the SHA iButton parts until we find
      // one that has the right file on it.
      while(!FoundUser && FindNewSHA(user->portnum, user->devAN, FALSE))
      {
         short handle;

         if(owOpenFile(user->portnum, user->devAN, fe, &handle))
         {
            user->accountPageNumber = fe->Spage;
            FoundUser = TRUE;
            if(!owCloseFile(user->portnum, user->devAN, handle))
               return FALSE;
         }
      }

      if(FoundUser)
         return TRUE;
      else if(!doBlocking)
         return FALSE;
   }
}
*/

//final buffer for tracking list of known SHA parts
//only holds the CRC for each part
static uchar ListOfKnownSHA[MAX_PORTNUM][MAX_SHA_IBUTTONS];
static int listIndex = 0;
//intermediate buffer for tracking list of known SHA parts
static uchar listBuffer[MAX_PORTNUM][MAX_SHA_IBUTTONS];
static int indexBuffer = 0;
//holds last state of FindNewSHA
static SMALLINT needsFirst = FALSE;

//---------------------------------------------------------------------
// Finds new SHA iButtons on the given port.  Uses 'triple-buffer'
// technique to insure it only finds 'new' buttons.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
// 'devAN'       - pointer to buffer for device address
// 'resetList'   - if TRUE, final buffer is cleared.
//
// Returns: TRUE, found a new SHA iButton.
//          FALSE, no new buttons are present.
//
SMALLINT FindNewSHA(int portnum, uchar* devAN, SMALLINT resetList)
{
   uchar ROM[8];
   uchar tempList[MAX_SHA_IBUTTONS] = {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
                                       0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00};
   int tempIndex = 0, i;
   SMALLINT hasDevices = TRUE, completeList = FALSE;

   // force back to standard speed
   if(MODE_NORMAL != owSpeed(portnum,MODE_NORMAL))
   {
      OWERROR(OWERROR_LEVEL_FAILED);
      return FALSE;
   }

   in_overdrive[portnum&0x0FF] = FALSE;

   // get first device in list with the specified family
   if(needsFirst || resetList)
   {
      hasDevices = owFirst(portnum, TRUE, FALSE);
      completeList = TRUE;
      if(resetList)
         listIndex = 0;
   }
   else
   {
      hasDevices = owNext(portnum, TRUE, FALSE);
   }

   if(hasDevices)
   {
      do
      {
         // verify correct device type
         owSerialNum(portnum, ROM, TRUE);
         tempList[tempIndex++] = ROM[7];

         // compare crc to complete list of ROMs
         for(i=0; i<listIndex; i++)
         {
            if(ROM[7] == ListOfKnownSHA[portnum&0x0FF][i])
               break;
         }

         // found no match;
         if(i==listIndex)
         {
            // check if correct type and not copr_rom
            if ((SHA_FAMILY_CODE   == (ROM[0] & 0x7F)) ||
                (SHA33_FAMILY_CODE == (ROM[0] & 0x7F)))
            {
               // save the ROM to the return buffer
               owSerialNum(portnum,devAN,TRUE);
               ListOfKnownSHA[portnum&0x0FF][listIndex++] = devAN[7];
               return TRUE;
            }
         }
      }
      while(owNext(portnum, TRUE, FALSE));
   }

   // depleted the list, start over from beginning next time
   needsFirst = TRUE;

   if(completeList)
   {
      // known list is triple-buffered, listBuffer is intermediate
      // buffer.  tempList is the immediate buffer, while
      // ListOfKnownSHA is the final buffer.
      if( (memcmp(tempList, listBuffer[portnum&0x0FF], MAX_SHA_IBUTTONS)==0) &&
          tempIndex == indexBuffer )
      {
         memcpy(ListOfKnownSHA[portnum&0x0FF], tempList, MAX_SHA_IBUTTONS);
         listIndex = tempIndex;
      }
      else
      {
         memcpy(listBuffer[portnum&0x0FF], tempList, MAX_SHA_IBUTTONS);
         indexBuffer = tempIndex;
      }

   }
   return FALSE;
}


//-------------------------------------------------------------------------
// Select the current device and attempt overdrive if possible.  Usable
// for both DS1963S and DS1961S.
//
// 'portnum'     - number 0 to MAX_PORTNUM-1.  This number is provided to
//                 indicate the symbolic port number.
//
// Return: TRUE - device selected
//         FALSE - device not select
//
SMALLINT SelectSHA(int portnum)
{
   int rt,cnt=0;

#ifdef __MC68K__
   // Used in when overdrive isn't...Used owVerify for overdrive
   rt = owAccess(portnum);
#else
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//
   #ifdef DEBUG_DUMP
      uchar ROM[8];
      int i, mcnt;
      char msg[255];

      owSerialNum(portnum,ROM, TRUE);
      mcnt = sprintf(msg,"\n  Device select ");
      for (i = 0; i < 8; i++)
         mcnt += sprintf(msg + mcnt, "%02X",ROM[i]);
      mcnt += sprintf(msg + mcnt,"\n");
      printf("%s",msg);
   #endif
   //\\//\\//\\//\\//\\//\\//\\//\\//\\//

   // loop to access the device and optionally do overdrive
   do
   {
      rt = owVerify(portnum,FALSE);

      // check not present
      if (rt != 1)
      {
         // if in overdrive, drop back
         if (in_overdrive[portnum&0x0FF])
         {
            // set to normal speed
            if(MODE_NORMAL == owSpeed(portnum,MODE_NORMAL))
            	in_overdrive[portnum&0x0FF] = FALSE;
         }
      }
      // present but not in overdrive
      else if (!in_overdrive[portnum&0x0FF])
      {
         // put all devices in overdrive
         if (owTouchReset(portnum))
         {
            if (owWriteByte(portnum,0x3C))
            {
               // set to overdrive speed
               if(MODE_OVERDRIVE == owSpeed(portnum,MODE_OVERDRIVE))
               		in_overdrive[portnum&0x0FF] = TRUE;
            }
         }

         rt = 0;
      }
      else
         break;
   }
   while ((rt != 1) && (cnt++ < 3));
#endif

   return rt;
}


//-------------------------------------------------------------------------
// Creates a random challenge using the SHA engine of the given
// coprocessor.
//
// 'copr'      - Structure for holding coprocessor information.
// 'pagenum'   - pagenumber to use for calculation.
// 'chlg'      - 3-byte return buffer for challenge.
// 'offset'    - offset into resulting MAC to pull challenge data from.
//
// Return: TRUE - create challenge succeeded.
//         FALSE - an error occurred.
//
SMALLINT CreateChallenge(SHACopr* copr, SMALLINT pageNum,
                         uchar* chlg, SMALLINT offset)
{
   uchar scratchpad[32];
   int   addr = pageNum << 5;
   uchar es = 0x1F;
   int   start = 8;

   if(offset>0 && offset<17)
      start += offset;

   //set the serial number
   owSerialNum(copr->portnum, copr->devAN, FALSE);

   OWASSERT( EraseScratchpadSHA18(copr->portnum, addr, FALSE),
             OWERROR_ERASE_SCRATCHPAD_FAILED, FALSE );

   OWASSERT( SHAFunction18(copr->portnum, SHA_COMPUTE_CHALLENGE, addr, TRUE),
             OWERROR_SHA_FUNCTION_FAILED, FALSE );

   OWASSERT( ReadScratchpadSHA18(copr->portnum, &addr, &es, scratchpad, TRUE),
             OWERROR_READ_SCRATCHPAD_FAILED, FALSE );

   memcpy(chlg,&scratchpad[start],3);

   return TRUE;
}

//-------------------------------------------------------------------------
// Answers a random challenge by performing an authenticated read of the
// user's account information.
//
// 'user'      - Structure for holding user token information.
// 'chlg'      - 3-byte buffer of challenge data.
//
// Return: the value of the write cycle counter for the account page.
//         or -1 if there is an error
//
/*
int AnswerChallenge(SHAUser* user, uchar* chlg)
{
   int addr = user->accountPageNumber << 5;

   user->writeCycleCounter = -1;
   memcpy(&user->accountFile[20], chlg, 3);

   //set the serial number
   owSerialNum(user->portnum, user->devAN, FALSE);

   if(user->devAN[0]==0x18)
   {
      // for the DS1963S
      OWASSERT( EraseScratchpadSHA18(user->portnum, addr, FALSE),
                OWERROR_ERASE_SCRATCHPAD_FAILED, -1 );

      OWASSERT( WriteScratchpadSHA18(user->portnum, addr,
                                     user->accountFile, 32,
                                     TRUE),
                 OWERROR_WRITE_SCRATCHPAD_FAILED, -1 );

      user->writeCycleCounter =
         ReadAuthPageSHA18(user->portnum,
                           user->accountPageNumber,
                           user->accountFile,
                           user->responseMAC, TRUE);
   }
   else if(user->devAN[0]==0x33||user->devAN[0]==0xB3)
   {
      // for the DS1961S
      OWASSERT( WriteScratchpadSHA33(user->portnum, addr,
                                     &user->accountFile[16],
                                     FALSE),
                 OWERROR_WRITE_SCRATCHPAD_FAILED, -1 );

      user->writeCycleCounter =
         ReadAuthPageSHA33(user->portnum,
                           user->accountPageNumber,
                           user->accountFile,
                           user->responseMAC, TRUE);
   }
   return user->writeCycleCounter;
}
*/
//-------------------------------------------------------------------------
// Verifies the authentication response of a user token.
//
// 'copr'      - Structure for holding coprocessor information.
// 'user'      - Structure for holding user token information.
// 'chlg'      - 3-byte buffer of challenge data.
// 'doBind'    - if true, the user's unique secret is recreated on the
//               coprocessor.  If this function is called multiple times,
//               it is acceptable to skip the bind for all calls after
//               the first on the same user token.
//
// Return: If TRUE, the user's authentication response matched exactly the
//            signature generated by the coprocessor.
//         If FALSE, an error occurred or the signature did not match.
//
SMALLINT VerifyAuthResponse(SHACopr* copr, SHAUser* user,
                            uchar* chlg, SMALLINT doBind)
{
   int addr = copr->wspcPageNumber << 5;
   int wcc = user->writeCycleCounter;
   uchar sign_cmd;
   uchar scratchpad[32];
   uchar fullBindCode[15];

   memset(scratchpad, 0x00, 32);
   memset(fullBindCode, 0x0FF, 15);

   // format the bind code properly
   if(user->devAN[0]==0x18)
   {
      // Format for DS1963S
      memcpy(fullBindCode, copr->bindCode, 4);
      memcpy(&fullBindCode[12], &(copr->bindCode[4]), 3);
      // use ValidateDataPage command
      sign_cmd = SHA_VALIDATE_DATA_PAGE;
      // copy wcc LSB first
      if(!IntToBytes(&scratchpad[8], 4, wcc))
         OWERROR(OWERROR_NO_ERROR_SET);
   }
   else if(user->devAN[0]==0x33||user->devAN[0]==0xB3)
   {
      // Leave bindCode FF for DS1961S
      // Use AuthenticateHost command
      sign_cmd = SHA_AUTHENTICATE_HOST;
      // the user doesn't have a write cycle counter
      memset(&scratchpad[8], 0x0FF, 4);
   }
   else
   {
      OWERROR(OWERROR_WRONG_TYPE);
      return FALSE;
   }

   // the pagenumber
   fullBindCode[4] = (uchar)user->accountPageNumber;
   // and 7 bytes of the address of current device
   memcpy(&fullBindCode[5], user->devAN, 7);

   // get the user address and page num from fullBindCode
   memcpy(&scratchpad[12], &fullBindCode[4], 8);
   // set the same challenge bytes
   memcpy(&scratchpad[20], chlg, 3);

   // set the serial number to that of the coprocessor
   owSerialNum(copr->portnum, copr->devAN, FALSE);

   // install user's unique secret on the wspc secret
   if(doBind)
   {
      OWASSERT( BindSecretToiButton18(copr->portnum,
                                      copr->authPageNumber,
                                      copr->wspcPageNumber&7,
                                      copr->bindData, fullBindCode, FALSE),
                OWERROR_BIND_SECRET_FAILED, FALSE );
      if(user->devAN[0]==0x33||user->devAN[0]==0xB3)
      {
         // also copy the resulting secret into secret location 0, to
         // replace the signing secret.  Necessary for producing the
         // DS1961S's write-authorization MAC.
         OWASSERT( CopySecretSHA18(copr->portnum, 0),
                   OWERROR_COPY_SECRET_FAILED, FALSE);
      }
   }

   // recreate the signature and verify
   OWASSERT( WriteDataPageSHA18(copr->portnum, copr->wspcPageNumber,
                              user->accountFile, doBind),
              OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );

   OWASSERT( WriteScratchpadSHA18(copr->portnum, addr, scratchpad, 32, TRUE),
              OWERROR_WRITE_SCRATCHPAD_FAILED, FALSE );

   OWASSERT( SHAFunction18(copr->portnum, sign_cmd, addr, TRUE),
             OWERROR_SHA_FUNCTION_FAILED, FALSE );

   OWASSERT( MatchScratchpadSHA18(copr->portnum,
                                user->responseMAC, TRUE),
             OWERROR_MATCH_SCRATCHPAD_FAILED, FALSE );

   return TRUE;
}

//-------------------------------------------------------------------------
// Creates a data signature for the given data buffer using the hardware
// coprocessor.
//
// 'copr'          - Structure for holding coprocessor information.
// 'data'          - data written to the data page to sign
// 'scratchpad'    - data written to the scratchpad to sign
// 'signature'     - data buffer which is either holding the signature that
//                   must match exactly what is generated on the coprocessor
//                   -or- will hold the resulting signature created by the
//                   coprocessor.
// 'readSignature' - implies whether or not the signature buffer
//                   receives the contents of the scratchpad or is used to
//                   match the contents of the scratchpad.  If true,
//                   scratchpad contents are read into the signature buffer.
//
// Return: If TRUE, the user's authentication response matched exactly the
//            signature generated by the coprocessor or the signature was
//            successfully copied into the return buffer.
//         If FALSE, an error occurred or the signature did not match.
//
SMALLINT CreateDataSignature(SHACopr* copr, uchar* data,
                             uchar* scratchpad, uchar* signature,
                             SMALLINT readSignature)
{
   int addr = copr->signPageNumber << 5;

   // set the serial number to that of the coprocessor
   owSerialNum(copr->portnum, copr->devAN, FALSE);

   OWASSERT( WriteDataPageSHA18(copr->portnum, copr->signPageNumber,
                              data, FALSE),
              OWERROR_WRITE_DATA_PAGE_FAILED, FALSE );

   OWASSERT( WriteScratchpadSHA18(copr->portnum, addr,
                                scratchpad, 32, TRUE),
              OWERROR_WRITE_SCRATCHPAD_FAILED, FALSE );

   OWASSERT( SHAFunction18(copr->portnum, SHA_SIGN_DATA_PAGE,
                         addr, TRUE),
             OWERROR_SHA_FUNCTION_FAILED, FALSE );

   if(readSignature)
   {
      OWASSERT( ReadScratchpadSHA18(copr->portnum, 0, 0,
                                  scratchpad, TRUE),
                 OWERROR_READ_SCRATCHPAD_FAILED, FALSE );
   }
   else
   {
      OWASSERT( MatchScratchpadSHA18(copr->portnum,
                                   signature, TRUE),
                 OWERROR_MATCH_SCRATCHPAD_FAILED, FALSE );
   }

   memcpy(signature, &scratchpad[8], 20);

   return TRUE;
}


//-------------------------------------------------------------------------
// Decipher the variable Val into a 'len' byte array to set
// LSB First
//
SMALLINT IntToBytes(uchar* byteArray, int len, unsigned int val)
{
   int i = 0;
   for (; i<len; i++)
   {
      byteArray[i] = (uchar)val;
      val >>= 8;
   }

   if(val==0)
      return TRUE;
   else
      return FALSE;
}

//-------------------------------------------------------------------------
// Decipher the range 'len' of the byte array to an integer
// LSB First
//
int BytesToInt(uchar* byteArray, int len)
{
   unsigned int val = 0;
   int i = (len - 1);
   // Concatanate the byte array into one variable.
   for (; i >= 0; i--)
   {
      val <<= 8;
      val |= (byteArray[i] & 0x00FF);
   }
   return val;
}
