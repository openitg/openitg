#include "global.h"
#include "RageLog.h"

#include "io/PIUIO.h"

bool PIUIO::DeviceMatches( int idVendor, int idProduct )
{
	if( idVendor == 0x547 && idProduct == 0x1002 )
		return true;

	return false;
}

bool PIUIO::Matches( int idVendor, int idProduct ) const
{
	return PIUIO::DeviceMatches( idVendor, idProduct );
}

bool PIUIO::Read( uint32_t *pData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(m_pHandle, USB_ENDPOINT_IN | USB_TYPE_VENDOR, 0xAE, 0, 0, (char *)pData, 8, 10000);
		if( iResult == 8 ) // all data read
			break;

		// all data not read
		LOG->Warn( "PIUIO device error: %s", usb_strerror() );
		Close();
	
		while( !Open() )
			usleep( 100000 );
	}

	return true;
}

bool PIUIO::Write( uint32_t iData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(m_pHandle, USB_ENDPOINT_OUT | USB_TYPE_VENDOR, 0xAE, 0, 0, (char *)&iData, 8, 10000 );
		
		if( iResult == 8 )
			break;
		
		LOG->Warn( "PIUIO device error: %s", usb_strerror() );
		Close();

		while( !Open() )
			usleep( 100000 );
	}

	return true;
}

bool PIUIO::BulkReadWrite( uint32_t *pData )
{
	int iResult;

	while ( 1 )
	{
		// this is caught by the r16 kernel hack, using '10011' as
		// a sentinel. the rest of the USB parameters aren't used.
		iResult = usb_control_msg(m_pHandle, 0, 0, 0, 0, (char*)(pData), 32, 10011);

		if ( iResult == 32 )
			break;

		LOG->Warn( "PIUIO bulk comm error: %s", usb_strerror() );
		Close();
		
		while( !Open() )
			usleep( 100000 );
	}
	return true;
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
