#include "global.h"
#include "io/USBDriver.h"
#include "RageLog.h"
#include <usb.h>

USBDriver::USBDriver() { m_pHandle = NULL; }

USBDriver::~USBDriver() { Close(); }

struct usb_device *USBDriver::FindDevice( usb_bus *usb_busses )
{
	LOG->Trace( "USBDriver::FindDevice()." );

	for( usb_bus *bus = usb_busses; bus; bus = bus->next )
		for( struct usb_device *dev = bus->devices; dev; dev = dev->next )
			if( Matches(dev->descriptor.idVendor, dev->descriptor.idProduct) )
			{
				LOG->Trace( "FindDevice() got a match." );
				return dev;
			}

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
	
	if( !usb_busses )
		return false;

	for (usb_bus *bus = usb_busses; bus; bus = bus->next)
	{
		for (struct usb_device *dev = bus->devices; dev; dev = dev->next)
		{
			if ( Matches(dev->descriptor.idVendor, dev->descriptor.idProduct) )
			{
				// if we can't open it, try the next device
				m_pHandle = usb_open(dev);
				if (m_pHandle)
				{
					if ( usb_set_configuration(m_pHandle, dev->config->bConfigurationValue) < 0 )
					{
						LOG->Warn( "USBDriver::Open(): usb_set_configuration: %s", usb_strerror() );
						Close();
						return false;
					}
					if ( usb_claim_interface(m_pHandle, dev->config->interface->altsetting->bInterfaceNumber) < 0 )
					{
						LOG->Warn( "USBDriver::Open(): usb_claim_interface: %s", usb_strerror() );
						Close();
						return false;
					}
					m_iInterfaceNum = dev->config->interface->altsetting->bInterfaceNumber;
					m_iIdVendor = dev->descriptor.idVendor;
					m_iIdProduct = dev->descriptor.idProduct;
					return true;
				}
				else
				{
					LOG->Warn( "USBDriver::Open(): usb_open: %s", usb_strerror() );
				}
			}
		}
	}

	// nothing usable found
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
