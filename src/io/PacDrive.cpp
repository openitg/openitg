#include "global.h"
#include "RageLog.h"
#include "PacDrive.h"

bool PacDrive::Matches( int idVendor, int idProduct ) const
{
	if( idVendor != 0xd209 )
		return false;

	/* PacDrives have PIDs 1500-1508 */
	if( (idProduct & ~0x07) != 0x1500 )
		return false;

	return true;
}

/* While waiting for this to reconnect, we would likely run into a condition
 * where LightsDriver::Set() is being called constantly and none of the calls
 * actually terminate. If this write fails, assume it's lost and don't reconnect.
 */
bool PacDrive::Write( const uint16_t &iData )
{
	// output is within the first 16 bits - accept a
	// 16-bit arg and cast it, for simplicity's sake.
	uint32_t data = (iData << 16);

	int iReturn = usb_control_msg( m_pHandle, USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		HID_SET_REPORT, HID_IFACE_OUT, 0, (char *)data, 4, 10000 );

	if( iReturn == 4 )
		return true;

	LOG->Warn( "PacDrive writing failed, returned %i: %s", iReturn, usb_strerror() );

	return false;
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
