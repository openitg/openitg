#include "global.h"
#include "RageLog.h"

#include "io/ITGIO.h"

CString ITGIO::m_sInputError;
int ITGIO::m_iInputErrorCount = 0;

bool ITGIO::DeviceMatches( int idVendor, int idProduct )
{
	if ( idVendor == 0x7c0 )
	{
		if ( idProduct == 0x1501 || idProduct == 0x1582 || idProduct == 0x1584 )
			return true;
	}

	return false;
}

// redirect for USBDriver's Open() code
bool ITGIO::Matches( int idVendor, int idProduct ) const
{
	return ITGIO::DeviceMatches( idVendor, idProduct );
}

bool ITGIO::Read( uint32_t *pData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(m_pHandle, USB_ENDPOINT_IN | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			HID_GET_REPORT, HID_IFACE_IN, 0, (char *)pData, 4, 1000 );
		if( iResult == 4 ) // all data read
			break;

		// all data not read
		Reconnect();
		//Write( [obj+12] );
	}

	return true;
}

bool ITGIO::Write( uint32_t iData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(m_pHandle, USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
			HID_SET_REPORT, HID_IFACE_OUT, 0, (char *)&iData, 4, 1000 );
	
		if( iResult == 4 ) // all data read
			break;
		
		Reconnect();
	}

	return true;
}

void ITGIO::Reconnect()
{
	LOG->Warn( "Attempting to reconnect ITGIO." );

	m_sInputError = "I/O error";
	Close();

	// attempt to reconnect every 0.1 seconds
	do
	{
		m_iInputErrorCount++;
		usleep(100000);
	}
	while( !Open() );

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
