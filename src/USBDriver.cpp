#include "global.h"
#include <usb.h>

CString USBDevice::GetClassDescription( int iClass )
{
	return "not implemented";	
}

CString USBDevice::GetDescription()
{
	if( IsITGIO() || IsPIUIO() )
		return "Input/lights controller";
	
	return "Description not implemented";
}
