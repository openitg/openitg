#include "global.h"
#include <usb.h>

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
		return false;

	//XXX: needs filled in
}

