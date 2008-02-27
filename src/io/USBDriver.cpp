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
