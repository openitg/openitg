#include "global.h"
#include "RageLog.h"
#include "USBDriver_Impl_Libusb.h"
#include <usb.h>

USBDriver_Impl_Libusb::USBDriver_Impl_Libusb()
{
	usb_init();
	m_pHandle = NULL;
}

USBDriver_Impl_Libusb::~USBDriver_Impl_Libusb()
{
	Close();
}

static struct usb_device *FindDevice( int iVendorID, int iProductID )
{
	for( usb_bus *bus = usb_get_busses(); bus; bus = bus->next )
		for( struct usb_device *dev = bus->devices; dev; dev = dev->next )
			if( iVendorID == dev->descriptor.idVendor && iProductID == dev->descriptor.idProduct )
				return dev;

	// fall through
	LOG->Trace( "FindDevice() found no matches." );
	return NULL;
}


bool USBDriver_Impl_Libusb::Open( int iVendorID, int iProductID )
{
	Close();

	if( usb_find_busses() < 0 )
	{
		LOG->Warn( "Libusb: usb_find_busses: %s", usb_strerror() );
		return false;
	}

	if( usb_find_devices() < 0 )
	{
		LOG->Warn( "Libusb: usb_find_devices: %s", usb_strerror() );
		return false;
	}
	
	struct usb_device *dev = FindDevice( iVendorID, iProductID );

	if( dev == NULL )
	{
		LOG->Warn( "Libusb: no match for %04x, %04x.", iVendorID, iProductID );
		return false;
	}

	m_pHandle = usb_open( dev );

	if( m_pHandle == NULL )
	{
		LOG->Warn( "Libusb: usb_open: %s", usb_strerror() );
		return false;
	}

#ifdef LIBUSB_HAS_DETACH_KERNEL_DRIVER_NP
		// The device may be claimed by a kernel driver. Attempt to reclaim it.
		for( unsigned i = 0; i < dev->config->bNumInterfaces; i++ )
		{
			int iResult = usb_detach_kernel_driver_np( m_pHandle, i );

			switch( iResult )
			{
			case -61:	// "No data available" (no driver attached) - continue
			case 0:		// no error
				continue;
				break;
			default:	// unhandled error
				LOG->Warn( "Libusb: usb_detach_kernel_driver_np: %s", usb_strerror() );
				Close();
				return false;
				break;
			}
		}

#endif

	if ( !SetConfiguration(dev->config->bConfigurationValue) )
	{
		LOG->Warn( "Libusb: usb_set_configuration: %s", usb_strerror() );
		Close();
		return false;
	}
	
	// attempt to claim all interfaces for this device
	for( unsigned i = 0; i < dev->config->bNumInterfaces; i++ )
	{
		if ( !ClaimInterface(i) )
		{
			LOG->Warn( "Libusb: usb_claim_interface(%i): %s", i, usb_strerror() );
			Close();
			return false;
		}
	}

	return true;
}

void USBDriver_Impl_Libusb::Close()
{
	// never opened
	if( m_pHandle == NULL )
		return;

	usb_set_altinterface( m_pHandle, 0 );
	usb_reset( m_pHandle );
	usb_close( m_pHandle );
	m_pHandle = NULL;
}

int USBDriver_Impl_Libusb::ControlMessage( int iType, int iRequest, int iValue, int iIndex, char *pData, int iSize, int iTimeout )
{
	return usb_control_msg( m_pHandle, iType, iRequest, iValue, iIndex, pData, iSize, iTimeout );
}

int USBDriver_Impl_Libusb::BulkRead( int iEndpoint, char *pData, int iSize, int iTimeout )
{
	return usb_bulk_read( m_pHandle, iEndpoint, pData, iSize, iTimeout );
}

int USBDriver_Impl_Libusb::BulkWrite( int iEndpoint, char *pData, int iSize, int iTimeout )
{
	return usb_bulk_write( m_pHandle, iEndpoint, pData, iSize, iTimeout );
}

int USBDriver_Impl_Libusb::InterruptRead( int iEndpoint, char *pData, int iSize, int iTimeout )
{
	return usb_interrupt_read( m_pHandle, iEndpoint, pData, iSize, iTimeout );
}

int USBDriver_Impl_Libusb::InterruptWrite( int iEndpoint,char *pData, int iSize, int iTimeout )
{
	return usb_interrupt_write( m_pHandle, iEndpoint, pData, iSize, iTimeout );
}

bool USBDriver_Impl_Libusb::SetConfiguration( int iConfig )
{
	return usb_set_configuration( m_pHandle, iConfig ) == 0;
}

bool USBDriver_Impl_Libusb::ClaimInterface( int iInterface )
{
	return usb_claim_interface( m_pHandle, iInterface ) == 0;
}

bool USBDriver_Impl_Libusb::ReleaseInterface( int iInterface )
{
	return usb_release_interface( m_pHandle, iInterface ) == 0;
}

const char* USBDriver_Impl_Libusb::GetError() const
{
	return usb_strerror();
}
