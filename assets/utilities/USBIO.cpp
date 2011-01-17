#include <cstdio>
#include <usb.h>
#include "USBIO.h"

/*
 * Global functions
 */

// probes for a board type as given.
USBDriver *USBIO::TryBoardType( Board type )
{
	USBDriver *pDriver = NULL;
	switch( type )
	{
	case BOARD_ITGIO:		pDriver = new ITGIO;	break;
	case BOARD_PIUIO:		pDriver = new PIUIO;	break;
	case BOARD_PACDRIVE:	pDriver = new PacDrive;	break;
	}

	// this board couldn't be opened - delete the object
	if( !pDriver->Open() )
	{
		delete( pDriver );
		pDriver = NULL;
	}
	else
	{
		// close the board, so it's ready for whatever else.
		pDriver->Close();
	}

	// we'll return the object if it could open.
	// if not, we return NULL, as set above.
	return pDriver;
}

// probes for all known board types
USBDriver *USBIO::FindBoard( Board &type )
{
	USBDriver *pDriver;
	for( Board b = BOARD_ITGIO; b < NUM_BOARDS; b = (Board)(b+1) )
	{
		pDriver = TryBoardType( b );

		if( pDriver == NULL )
			continue;

		type = b;
		break;
	}

	if( pDriver == NULL )
		type = BOARD_INVALID;

	// if this ends up NULL, the calling program must handle it.
	return pDriver;
}

void USBIO::MaskOutput( Board type, uint32_t *pData )
{
	switch( type )
	{
	// the first 16 seem to enable input.
	case BOARD_ITGIO:		*pData |= 0xFFFF0000;	break;

	// avoid pulsing the coin counter and missetting the sensors
	case BOARD_PIUIO:		*pData &= 0xE7FCFFFC;	break;

	// I'm not sure what happens when we write past 16, but don't do it.
	case BOARD_PACDRIVE:	*pData &= 0xFFFF0000;	break;
	}
}

/*
 * USBDriver
 */

USBDriver::USBDriver()
{
	m_pHandle = NULL;
}

USBDriver::~USBDriver()
{
	Close();
}

bool USBDriver::Matches( int idVendor, int idProduct ) const
{
	return false;
}

struct usb_device *USBDriver::FindDevice()
{
	for( usb_bus *bus = usb_get_busses(); bus; bus = bus->next )
		for( struct usb_device *dev = bus->devices; dev; dev = dev->next )
			if( this->Matches(dev->descriptor.idVendor, dev->descriptor.idProduct) )
				return dev;

	// fall through
	return NULL;
}

void USBDriver::Close()
{
	// never opened
	if( m_pHandle == NULL )
		return;

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
		printf( "USBDriver::Open(): usb_find_busses: %s\n", usb_strerror() );
		return false;
	}

	if( usb_find_devices() < 0 )
	{
		printf( "USBDriver::Open(): usb_find_devices: %s\n", usb_strerror() );
		return false;
	}
	
	struct usb_device *dev = FindDevice();

	if( dev == NULL )
		return false;

	m_pHandle = usb_open( dev );

	if( m_pHandle == NULL )
	{
		printf( "USBDriver::Open(): usb_open: %s\n", usb_strerror() );
		return false;
	}

// The device may be claimed by a kernel driver. Attempt to reclaim it.
// This macro is self-porting, so no ifdef LINUX should be needed.
#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
		for( unsigned i = 0; i < dev->config->bNumInterfaces; i++ )
		{
			int iResult = usb_detach_kernel_driver_np( m_pHandle, i );

			switch( iResult )
			{
			// no kernel drivers needed detached ("No data available") - not an error
			case -61:
			// no error
			case 0:
				continue;
				break;
			default:
				printf( "USBDriver::Open(): usb_detach_kernel_driver_np: %s\n", usb_strerror() );
				return false;
				break;
			}
		}
#endif

	if ( usb_set_configuration(m_pHandle, dev->config->bConfigurationValue) < 0 )
	{
		printf( "USBDriver::Open(): usb_set_configuration: %s\n", usb_strerror() );
		Close();
		return false;
	}
	
	// claim all interfaces for this device
	for( unsigned i = 0; i < dev->config->bNumInterfaces; i++ )
	{
		if ( usb_claim_interface(m_pHandle, i) < 0 )
		{
			printf( "USBDriver::Open(): usb_claim_interface(%i): %s\n", i, usb_strerror() );
			Close();
			return false;
		}
	}

	return true;
}

/*
 * ITGIO
 */

bool ITGIO::DeviceMatches( int idVendor, int idProduct )
{
	if ( idVendor == 0x7c0 )
	{
		if ( idProduct == 0x1501 || idProduct == 0x1582 || idProduct == 0x1584 )
			return true;
	}

	return false;
}

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
	printf( "ITG I/O error: %s\n", usb_strerror() );
	Close();

	printf( "Attempting to reconnect...\n" );

	while( !Open() )
		sleep( 1 );

	printf( "Successfully reconnected.\n" );
}

/*
 * PIUIO
 */

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

		Reconnect();
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
		
		Reconnect();
	}

	return true;
}

bool PIUIO::BulkReadWrite( uint32_t *pData )
{
	int iResult;

	while ( 1 )
	{
		// XXX: what are the reason(s) for the first 2 numbers being 0?
		iResult = usb_control_msg(m_pHandle, 0, 0, 0, 0, (char*)(pData), 32, 10011);

		if ( iResult == 32 )
			break;

		Reconnect();
	}
	return true;
}

void PIUIO::Reconnect()
{
		printf( "PIU I/O error: %s\n", usb_strerror() );
		Close();

		printf( "Attempting to reconnect...\n" );

		while( !Open() )
			sleep( 1 );

		printf( "Successfully reconnected.\n" );
}

/*
 * PacDrive
 */

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
bool PacDrive::Write( uint32_t iData )
{
	// output is within the first 16 bits - accept a
	// 16-bit arg and cast it, for simplicity's sake.

	int iReturn = usb_control_msg( m_pHandle, USB_ENDPOINT_OUT | USB_TYPE_CLASS | USB_RECIP_INTERFACE,
		HID_SET_REPORT, HID_IFACE_OUT, 0, (char *)&iData, 4, 10000 );

	if( iReturn == 4 )
		return true;

	printf( "PacDrive writing failed, returned %i: %s\n", iReturn, usb_strerror() );

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
