#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"	// for ssprintf, arraylen

#include "io/ITGIO.h"
#include "arch/USB/USBDriver_Impl.h"

CString ITGIO::m_sInputError;
int ITGIO::m_iInputErrorCount = 0;

// control message timeout, in microseconds (so, 1 ms)
const int REQ_TIMEOUT = 1000;

// weirdly, the ITG2 r21 binary checks for three PIDs; we'll do that too.
const short ITGIO_VENDOR_ID = 0x07C0;
const short ITGIO_PRODUCT_ID[3] = { 0x1501, 0x1582, 0x1584 };

// convenience constant for how many PIDs we need to check
const unsigned NUM_PRODUCT_IDS = ARRAYLEN( ITGIO_PRODUCT_ID );

bool ITGIO::DeviceMatches( int iVID, int iPID )
{
	if( iVID != ITGIO_VENDOR_ID )
		return false;

	for( unsigned i = 0; i < NUM_PRODUCT_IDS; ++i )
		if( iPID == ITGIO_PRODUCT_ID[i] )
			return true;

	return false;
}

bool ITGIO::Open()
{
	/* we don't really care which PID works, just if it does */
	for( unsigned i = 0; i < NUM_PRODUCT_IDS; ++i )
		if( OpenInternal(ITGIO_VENDOR_ID, ITGIO_PRODUCT_ID[i]) )
			return true;

	return false;
}

bool ITGIO::Read( uint32_t *pData )
{
	/* XXX: magic number left over from the ITG disassembly */
	int iExpected = 4;

	int iResult = m_pDriver->ControlMessage(
		USB_DIR_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		HID_GET_REPORT, HID_IFACE_IN, 0, (char*)pData, iExpected,
		REQ_TIMEOUT );

	return iResult == iExpected;
}

bool ITGIO::Write( const uint32_t iData )
{
	/* XXX: magic number left over from the ITG disassembly */
	int iExpected = 4;

	int iResult = m_pDriver->ControlMessage(
		USB_DIR_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		HID_SET_REPORT, HID_IFACE_OUT, 0, (char*)&iData, iExpected,
		REQ_TIMEOUT );


	return iResult == iExpected;
}

void ITGIO::Reconnect()
{
	LOG->Debug( "Attempting to reconnect ITGIO." );

	/* set a message that the game loop can catch and display */
	m_sInputError = ssprintf( "I/O error: %s", m_pDriver->GetError() );

	Close();

	// attempt to reconnect every 0.1 seconds
	while( !Open() )
	{
		++m_iInputErrorCount;
		usleep(100000);
	}

	m_sInputError = "";
}

/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
