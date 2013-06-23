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
//  owSesU.C - Acquire and release a Session on the 1-Wire Net.
//
//  Version: 2.01
//
//  History: 1.03 -> 2.00  Changed 'MLan' to 'ow'. Added support for
//                         multiple ports.
//          2.00 -> 2.01 Added error handling. Added circular-include check.
//          2.01 -> 2.10 Added raw memory error handling and SMALLINT
//          2.10 -> 3.00 Added memory bank functionality
//                       Added file I/O operations
//

#include "ownet.h"
#include "ds2480.h"

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a com port and a DS2480 based
// adapter.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                OpenCOM to indicate the port number.
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format COMX where X is the port number.
//
// Returns: TRUE - success, COM port opened
//
// exportable functions defined in ownetu.c
SMALLINT owAcquire(int portnum, char *port_zstr)
{
   // attempt to open the communications port
   if (OpenCOM(portnum,port_zstr) < 0)
   {
      OWERROR(OWERROR_OPENCOM_FAILED);
      return FALSE;
   }

   // detect DS2480
   if (!DS2480Detect(portnum))
   {
      CloseCOM(portnum);
      OWERROR(OWERROR_DS2480_NOT_DETECTED);
      return FALSE;
   }

   return TRUE;
}

//---------------------------------------------------------------------------
// Attempt to acquire a 1-Wire net using a com port and a DS2480 based
// adapter.
//
// 'port_zstr'  - zero terminated port name.  For this platform
//                use format COMX where X is the port number.
//
// Returns: valid handle, or -1 if an error occurred
//
// exportable functions defined in ownetu.c
//
int owAcquireEx(char *port_zstr)
{
   int portnum;

   // attempt to open the communications port
   if ((portnum = OpenCOMEx(port_zstr)) < 0)
   {
      OWERROR(OWERROR_OPENCOM_FAILED);
      return -1;
   }

   // detect DS2480
   if (!DS2480Detect(portnum))
   {
      CloseCOM(portnum);
      OWERROR(OWERROR_DS2480_NOT_DETECTED);
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------------------
// Release the previously acquired a 1-Wire net.
//
// 'portnum'    - number 0 to MAX_PORTNUM-1.  This number was provided to
//                OpenCOM to indicate the port number.
//
void owRelease(int portnum)
{
   CloseCOM(portnum);
}
