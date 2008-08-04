#include "global.h"
#include "RageLog.h"
#include "io/USBDriver.h"

#include <usb.h>

USBDriver::USBDriver()
{
	m_pHandle = NULL;
}

USBDriver::~USBDriver()
{
	Close();
}

/* sorry, but...Visual C++ wants it this way. */
bool USBDriver::Matches( int idVendor, int idProduct ) const
{
	return false;
}

struct usb_device *USBDriver::FindDevice()
{
	LOG->Trace( "USBDriver::FindDevice()" );

	for( usb_bus *bus = usb_get_busses(); bus; bus = bus->next )
		for( struct usb_device *dev = bus->devices; dev; dev = dev->next )
			if( this->Matches(dev->descriptor.idVendor, dev->descriptor.idProduct) )
				return dev;

	// fall through
	LOG->Trace( "FindDevice() found no matches." );
	return NULL;
}

void USBDriver::Close()
{
	// never opened
	if( m_pHandle == NULL )
		return;

	LOG->Trace( "USBDriver::Close()" );

	usb_set_altinterface( m_pHandle, 0 );
	usb_reset( m_pHandle );
	usb_close( m_pHandle );
	m_pHandle = NULL;
}

bool USBDriver::Open()
{
	Close();

	usb_init();

	if( usb_find_busses() < 0 )
	{
		LOG->Warn( "USBDriver::Open(): usb_find_busses: %s", usb_strerror() );
		return false;
	}

	if( usb_find_devices() < 0 )
	{
		LOG->Warn( "USBDriver::Open(): usb_find_devices: %s", usb_strerror() );
		return false;
	}
	
	struct usb_device *dev = FindDevice();

	if( dev == NULL )
	{
		LOG->Warn( "USBDriver::Open(): no device found." );
		return false;
	}

	m_pHandle = usb_open( dev );

	if( m_pHandle == NULL )
	{
		LOG->Warn( "USBDriver::Open(): usb_open: %s", usb_strerror() );
		return false;
	}

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP

		// The device may be claimed by a kernel driver. Attempt to reclaim it.
		// This macro is self-porting, so no ifdef LINUX should be needed.
		LOG->Debug( "USBDriver::Open(): attempting to detach kernel drivers..." );

		for( unsigned i = 0; i < dev->config->bNumInterfaces; i++ )
		{
			int iResult = usb_detach_kernel_driver_np( m_pHandle, i );

			LOG->Debug( "iResult: %i", iResult );

			// ignore "No data available" - means there was no driver claiming it
			if( iResult != 0 && iResult != -61 )
			{
				LOG->Warn( "USBDriver::Open(): usb_detach_kernel_driver_np: %s", usb_strerror() );
				return false;
			}
		}

#endif

	if ( usb_set_configuration(m_pHandle, dev->config->bConfigurationValue) < 0 )
	{
		LOG->Warn( "USBDriver::Open(): usb_set_configuration: %s", usb_strerror() );
		Close();
		return false;
	}
	
	// claim all interfaces for this device
	for( unsigned i = 0; i < dev->config->bNumInterfaces; i++ )
	{
		if ( usb_claim_interface(m_pHandle, i) < 0 )
		{
			LOG->Warn( "USBDriver::Open(): usb_claim_interface(%i): %s", i, usb_strerror() );
			Close();
			return false;
		}
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
