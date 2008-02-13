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
//  linuxlnk.C - COM functions required by MLANLL.C, MLANTRNU, MLANNETU.C and
//           MLanFile.C for MLANU to communicate with the DS2480 based
//           Universal Serial Adapter 'U'.  Fill in the platform specific code.
//
//  Version: 1.02
//
//  History: 1.00 -> 1.01  Added function msDelay.
//
//           1.01 -> 1.02  Changed to generic OpenCOM/CloseCOM for easier
//                         use with other platforms.
//

//--------------------------------------------------------------------------
// Copyright (C) 1998 Andrea Chambers and University of Newcastle upon Tyne,
// All Rights Reserved.
//--------------------------------------------------------------------------
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
// IN NO EVENT SHALL THE UNIVERSITY OF NEWCASTLE UPON TYNE OR ANDREA CHAMBERS
// BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH
// THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//---------------------------------------------------------------------------
//
//  LinuxLNK.C - COM functions required by MLANLLU.C, MLANTRNU.C, MLANNETU.C
//             and MLanFile.C for MLANU to communicate with the DS2480 based
//             Universal Serial Adapter 'U'.  Platform specific code.
//
//  Version: 2.01
//  History: 1.02 -> 1.03  modifications by David Smiczek
//                         Changed to use generic OpenCOM/CloseCOM
//                         Pass port name to OpenCOM instead of hard coded
//                         Changed msDelay to handle long delays
//                         Reformatted to look like 'TODO.C'
//                         Added #include "ds2480.h" to use constants.
//                         Added function SetBaudCOM()
//                         Added function msGettick()
//                         Removed delay from WriteCOM(), used tcdrain()
//                         Added wait for byte available with timeout using
//                          select() in ReadCOM()
//
//           1.03 -> 2.00  Support for multiple ports. Include "ownet.h". Use
//                         'uchar'.  Reorder functions. Provide correct
//                         return values to OpenCOM.  Replace 'makeraw' call.
//                         Should now be POSIX.
//           2.00 -> 2.01  Added support for owError library.
//

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <time.h>
#include <termios.h>
#include <errno.h>
#include <sys/time.h>

#include "ds2480.h"
#include "ownet.h"

// LinuxLNK global
int fd[MAX_PORTNUM];
SMALLINT fd_init;
struct termios origterm;


//---------------------------------------------------------------------------
// Attempt to open a com port.  Keep the handle in ComID.
// Set the starting baud rate to 9600.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number provided will
//               be used to indicate the port number desired when calling
//               all other functions in this library.
//
//
// Returns: the port number if it was succesful otherwise -1
//
int OpenCOMEx(char *port_zstr)
{
   int i;
   int portnum;

   if(!fd_init)
   {
      for(i=0; i<MAX_PORTNUM; i++)
         fd[i] = 0;
      fd_init = 1;
   }

   // check to find first available handle slot
   for(portnum = 0; portnum<MAX_PORTNUM; portnum++)
   {
      if(!fd[portnum])
         break;
   }
   OWASSERT( portnum<MAX_PORTNUM, OWERROR_PORTNUM_ERROR, -1 );

   if(!OpenCOM(portnum, port_zstr))
   {
      return -1;
   }

   return portnum;
}

//---------------------------------------------------------------------------
// Attempt to open a com port.
// Set the starting baud rate to 9600.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number provided will
//               be used to indicate the port number desired when calling
//               all other functions in this library.
//
// 'port_zstr' - zero terminate port name.  For this platform
//               use format COMX where X is the port number.
//
//
// Returns: TRUE(1)  - success, COM port opened
//          FALSE(0) - failure, could not open specified port
//
SMALLINT OpenCOM(int portnum, char *port_zstr)
{
   struct termios t;               // see man termios - declared as above
   int rc;

   if(!fd_init)
   {
      int i;
      for(i=0; i<MAX_PORTNUM; i++)
         fd[i] = 0;
      fd_init = 1;
   }

   OWASSERT( portnum<MAX_PORTNUM && portnum>=0 && !fd[portnum],
             OWERROR_PORTNUM_ERROR, FALSE );

   fd[portnum] = open(port_zstr, O_RDWR|O_NONBLOCK);
   if (fd[portnum]<0)
   {
      OWERROR(OWERROR_GET_SYSTEM_RESOURCE_FAILED);
      return FALSE;  // changed (2.00), used to return fd;
   }
   rc = tcgetattr (fd[portnum], &t);
   if (rc < 0)
   {
      int tmp;
      tmp = errno;
      close(fd[portnum]);
      errno = tmp;
      OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
      return FALSE; // changed (2.00), used to return rc;
   }

   cfsetospeed(&t, B9600);
   cfsetispeed (&t, B9600);

   // Get terminal parameters. (2.00) removed raw
   tcgetattr(fd[portnum],&t);
   // Save original settings.
   origterm = t;

   // Set to non-canonical mode, and no RTS/CTS handshaking
   t.c_iflag &= ~(BRKINT|ICRNL|IGNCR|INLCR|INPCK|ISTRIP|IXON|IXOFF|PARMRK);
   t.c_iflag |= IGNBRK|IGNPAR;
   t.c_oflag &= ~(OPOST);
   t.c_cflag &= ~(CRTSCTS|CSIZE|HUPCL|PARENB);
   t.c_cflag |= (CLOCAL|CS8|CREAD);
   t.c_lflag &= ~(ECHO|ECHOE|ECHOK|ECHONL|ICANON|IEXTEN|ISIG);
   t.c_cc[VMIN] = 0;
   t.c_cc[VTIME] = 3;

   rc = tcsetattr(fd[portnum], TCSAFLUSH, &t);
   tcflush(fd[portnum],TCIOFLUSH);

   if (rc < 0)
   {
      int tmp;
      tmp = errno;
      close(fd[portnum]);
      errno = tmp;
      OWERROR(OWERROR_SYSTEM_RESOURCE_INIT_FAILED);
      return FALSE; // changed (2.00), used to return rc;
   }

   return TRUE; // changed (2.00), used to return fd;
}


//---------------------------------------------------------------------------
// Closes the connection to the port.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
void CloseCOM(int portnum)
{
   // restore tty settings
   tcsetattr(fd[portnum], TCSAFLUSH, &origterm);
   FlushCOM(portnum);
   close(fd[portnum]);
   fd[portnum] = 0;
}


//--------------------------------------------------------------------------
// Write an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number provided will
//               be used to indicate the port number desired when calling
//               all other functions in this library.
// Returns 1 for success and 0 for failure
//
SMALLINT WriteCOM(int portnum, int outlen, uchar *outbuf)
{
   long count = outlen;
   int i = write(fd[portnum], outbuf, outlen);

   tcdrain(fd[portnum]);
   return (i == count);
}


//--------------------------------------------------------------------------
// Read an array of bytes to the COM port, verify that it was
// sent out.  Assume that baud rate has been set.
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
// 'outlen'   - number of bytes to write to COM port
// 'outbuf'   - pointer ot an array of bytes to write
//
// Returns:  TRUE(1)  - success
//           FALSE(0) - failure
//
int ReadCOM(int portnum, int inlen, uchar *inbuf)
{
   fd_set         filedescr;
   struct timeval tval;
   int            cnt;

   // loop to wait until each byte is available and read it
   for (cnt = 0; cnt < inlen; cnt++)
   {
      // set a descriptor to wait for a character available
      FD_ZERO(&filedescr);
      FD_SET(fd[portnum],&filedescr);
      // set timeout to 10ms
      tval.tv_sec = 0;
      tval.tv_usec = 10000;

      // if byte available read or return bytes read
      if (select(fd[portnum]+1,&filedescr,NULL,NULL,&tval) != 0)
      {
         if (read(fd[portnum],&inbuf[cnt],1) != 1)
            return cnt;
      }
      else
         return cnt;
   }

   // success, so return desired length
   return inlen;
}


//---------------------------------------------------------------------------
//  Description:
//     flush the rx and tx buffers
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
void FlushCOM(int portnum)
{
   tcflush(fd[portnum], TCIOFLUSH);
}


//--------------------------------------------------------------------------
//  Description:
//     Send a break on the com port for at least 2 ms
//
// 'portnum'  - number 0 to MAX_PORTNUM-1.  This number was provided to
//              OpenCOM to indicate the port number.
//
void BreakCOM(int portnum)
{
   int duration = 0;              // see man termios break may be
   tcsendbreak(fd[portnum], duration);     // too long
}


//--------------------------------------------------------------------------
// Set the baud rate on the com port.
//
// 'portnum'   - number 0 to MAX_PORTNUM-1.  This number was provided to
//               OpenCOM to indicate the port number.
// 'new_baud'  - new baud rate defined as
// PARMSET_9600     0x00
// PARMSET_19200    0x02
// PARMSET_57600    0x04
// PARMSET_115200   0x06
//
void SetBaudCOM(int portnum, uchar new_baud)
{
   struct termios t;
   int rc;
   speed_t baud;

   // read the attribute structure
   rc = tcgetattr(fd[portnum], &t);
   if (rc < 0)
   {
      close(fd[portnum]);
      return;
   }

   // convert parameter to linux baud rate
   switch(new_baud)
   {
      case PARMSET_9600:
         baud = B9600;
         break;
      case PARMSET_19200:
         baud = B19200;
         break;
      case PARMSET_57600:
         baud = B57600;
         break;
      case PARMSET_115200:
         baud = B115200;
         break;
   }

   // set baud in structure
   cfsetospeed(&t, baud);
   cfsetispeed(&t, baud);

   // change baud on port
   rc = tcsetattr(fd[portnum], TCSAFLUSH, &t);
   if (rc < 0)
      close(fd[portnum]);
}


//--------------------------------------------------------------------------
// Get the current millisecond tick count.  Does not have to represent
// an actual time, it just needs to be an incrementing timer.
//
long msGettick(void)
{
   struct timezone tmzone;
   struct timeval  tmval;
   long ms;

   gettimeofday(&tmval,&tmzone);
   ms = (tmval.tv_sec & 0xFFFF) * 1000 + tmval.tv_usec / 1000;
   return ms;
}


//--------------------------------------------------------------------------
//  Description:
//     Delay for at least 'len' ms
//
void msDelay(int len)
{
   struct timespec s;              // Set aside memory space on the stack

   s.tv_sec = len / 1000;
   s.tv_nsec = (len - (s.tv_sec * 1000)) * 1000000;
   nanosleep(&s, NULL);
}

