#include "global.h"
#include "RageLog.h"

#include "io/PIUIO.h"
#include "arch/USB/USBDriver_Impl.h"

const short PIUIO_VENDOR_ID	= 0x0547;
const short PIUIO_PRODUCT_ID	= 0x1002;

/* proprietary (arbitrary?) request PIUIO requires to handle I/O */
const short PIUIO_CTL_REQ = 0xAE;

/* timeout value for read/writes, in microseconds (so, 10 ms) */
const int REQ_TIMEOUT = 10000;

bool PIUIO::DeviceMatches( int iVID, int iPID )
{
	return iVID == PIUIO_VENDOR_ID && iPID == PIUIO_PRODUCT_ID;
}

bool PIUIO::Open()
{
	return OpenInternal( PIUIO_VENDOR_ID, PIUIO_PRODUCT_ID );
}

bool PIUIO::Read( uint32_t *pData )
{
	/* XXX: magic number left over from the ITG disassembly */
	int iExpected = 8;

	int iResult = m_pDriver->ControlMessage(
		USB_DIR_IN | USB_TYPE_VENDOR, PIUIO_CTL_REQ,
		0, 0, (char*)pData, iExpected, REQ_TIMEOUT );

	return iResult == iExpected;
}

bool PIUIO::Write( const uint32_t iData )
{
	/* XXX: magic number left over from the ITG disassembly */
	int iExpected = 8;

	int iResult = m_pDriver->ControlMessage(
		USB_DIR_OUT | USB_TYPE_VENDOR, PIUIO_CTL_REQ,
		0, 0, (char*)&iData, iExpected, REQ_TIMEOUT );

	return iResult == iExpected;
}

bool PIUIO::BulkReadWrite( uint32_t pData[8] )
{
	/* XXX: magic number left over from the ITG disassembly */
	int iExpected = 32;

	// this is caught by the r16 kernel hack, using '10011' as
	// a sentinel. the rest of the USB parameters aren't used.
	int iResult = m_pDriver->ControlMessage( 0, 0, 0, 0, 
		(char*)pData, iExpected, 10011 );

	return iResult == iExpected;
}

/*
 * Copyright (c) 2008-2011 BoXoRRoXoRs
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
