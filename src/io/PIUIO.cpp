#include "global.h"
#include "RageLog.h"
#include "USBDevice.h"
#include "PIUIO.h"

bool PIUIO::Matches( int idVendor, int idProduct )
{
	if( idVendor == 0x547 && idProduct == 0x1002 )
		return true;

	LOG->Trace( "Not a match. idVendor %u, idProduct %u", idVendor, idProduct );

	return false;
}

bool PIUIO::Read( u_int32_t *pData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(m_pHandle, 192, 174, 0, 0, (char *)pData, 8, 10000);
		if( iResult == 8 ) // all data read
			break;

		// all data not read
		LOG->Trace( "Device error: %s", usb_strerror() );
		Close();
	
		while( !Open() )
			usleep( 100000 );
	}

	return true;
}

bool PIUIO::Write( u_int32_t iData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(m_pHandle, 64, 174, 0, 0, (char *)&iData, 8, 10000 );
		
		if( iResult == 8 )
			break;
		
		LOG->Trace( "Device error: %s", usb_strerror() );
		Close();

		while( !Open() )
			usleep( 100000 );
	}

	return true;
}
