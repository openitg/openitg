#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "iButton.h"

extern "C" {
#include "ibutton/shaib.h"
}

// owAcquireEx is stupid and takes a char * when it really won't modify it
char serial_port[] =
#ifdef WIN32
	"COM1"
#else
	"/dev/ttyS0"
#endif
;

bool iButton::GetAESKey(const uchar *subkey, uchar *output)
{
	uchar firstDataPage[32], firstScratchPad[32];
	SHACopr copr;

	memcpy(firstDataPage, subkey, 32);

	if ((copr.portnum = owAcquireEx(serial_port)) == -1)
	{
		LOG->Warn( "GetAESKey(): failed to acquire port." );
		return false;
	}

	FindNewSHA(copr.portnum, copr.devAN, TRUE);
	owSerialNum(copr.portnum, copr.devAN, FALSE);

	WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
	memset(firstScratchPad, '\0', 32);
	memcpy(firstScratchPad+8, subkey+32, 15);

	WriteScratchpadSHA18(copr.portnum, 0, firstScratchPad, 32, 1);
	SHAFunction18(copr.portnum, 0xC3, 0, 1);
	ReadScratchpadSHA18(copr.portnum, 0, 0, firstScratchPad, 1);

	memset(firstDataPage, '\0', 32);
	WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
	memcpy(output, firstScratchPad+8, 24);
	owRelease(copr.portnum);	

	return true;
}

CString iButton::GetSerialNumber()
{
	SHACopr copr;
	uchar spBuf[32];

	if( (copr.portnum = owAcquireEx(serial_port)) == -1 )
	{
		LOG->Warn("GetSerialNumber(): failed to acquire port.");
		return CString();
	}

	FindNewSHA(copr.portnum, copr.devAN, true);
	ReadAuthPageSHA18(copr.portnum, 9, spBuf, NULL, false);
	owRelease(copr.portnum);

	CString sNewSerial = (char*)spBuf;
	TrimLeft(sNewSerial);
	TrimRight(sNewSerial);
	
	return sNewSerial;
}

/*
 * (c) 2008 BoXoRRoXoRs
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
