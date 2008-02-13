#include <usb.h>

#include "global.h"
#include "RageLog.h"
#include "USBDriver.h"

USBDriver::USBDriver()
{
}

USBDriver::~USBDriver()
{
	Close();
}

// this is overridden by derived USB drivers
bool USBDriver::Matches( int idVendor, int idProduct )
{
	return false;
}

struct usb_device *USBDriver::FindDevice()
{
	for( usb_bus *bus = usb_get_busses(); bus; bus->next )
		for( struct usb_device *dev = bus->devices; dev; dev->next )
			if( Matches(dev->descriptor.idVendor, dev->descriptor.idProduct) )
				return dev;

	// fall through
	return NULL;
}

void USBDriver::Close()
{
	// already closed
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
		LOG->Warn( "usb_find_busses: %s", usb_strerror() );
		return false;
	}

	if( usb_find_devices() < 0 )
	{
		LOG->Warn( "usb_find_devices: %s", usb_strerror() );
		return false;
	}
	
	if( !usb_busses )
	{
		LOG->Warn( "usb_busses: %s", usb_strerror() );
		return false;
	}

	// set the device
	struct usb_device *device = FindDevice();

	if( device == NULL )
	{
		LOG->Warn( "USBDriver: could not set usb_device" );
		return false;
	}
	
	m_pHandle = usb_open( device );
	if( m_pHandle == NULL )
	{
		LOG->Warn( "usb_open: %s", usb_strerror() );
		return false;
	}

	if( usb_set_configuration(m_pHandle, device->config->bConfigurationValue) != 0 )
	{
		LOG->Warn( "usb_set_configuration: %s", usb_strerror() );
		Close();
		return false;
	}

	if( usb_claim_interface(m_pHandle, device->config->interface->altsetting->bInterfaceNumber) != 0 )
	{
#ifdef LIBUSB_HAS_GET_DRIVER_NP
		char sDriver[1025];
		usb_get_driver_np( m_pHandle, device->config->interface->altsetting->bInterfaceNumber, sDriver, 1024 );
		if( sDriver[0] != 0 )
			LOG->Warn( "usb_claim_interface: %s (claimed by %s)", usb_strerror(), sDriver );
		else
#endif
			LOG->Warn( "usb_claim_interface: %s", usb_strerror() );
		
		Close();
		return false;
	}

	m_iInterfaceNumber = device->config->interface->altsetting->bInterfaceNumber;
	
	return true;
}
