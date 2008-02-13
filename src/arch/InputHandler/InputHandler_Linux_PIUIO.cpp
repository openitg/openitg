#include "global.h"
#include "RageLog.h"

#include "LightsManager.h"
#include "PrefsManager.h"
#include "InputHandler_Linux_PIUIO.h"

InputHandler_Linux_PIUIO::InputHandler_Linux_PIUIO()
{
	m_bShutdown = false;

	// initialise m_pDevice

	// device found and opened
	if( m_pDevice.Open() )
	{
		InputThread.SetName( "PIUIO thread" );
		InputThread.Create( InputThread_Start, this );
	}
}

InputHandler_Linux_PIUIO::~InputHandler_Linux_PIUIO()
{
	if( InputThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down PIUIO thread..." );
		InputThread.Wait();
		LOG->Trace( "PIUIO thread shut down." );
	}

	// remember to delete device handler when finished
}



bool InputHandler_Linux_PIUIO::DeviceMatches( int idVendor, int idProduct )
{
	if( idVendor == 1351 && idProduct == 4098 )
		return true;

	return false;
}

bool InputHandler_Linux_PIUIO::Read( u_int32_t *pData )
{
	int iResult;

	while( 1 )
	{
		iResult = usb_control_msg(device, 192, 174, 0, 0, *pData, 8, 10000);
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

bool InputHandler_Linux_PIUIO::Write( u_int32_t iData )
{
	int iResult;

	while( 1 )
	{
		// XXX: this can't be right...but we'll worry later.
		iResult = usb_control_msg(device, 64, 174, 0, 0, iData, 8, 10000 );
		
		if( iResult == 8 )
			break;
		
		LOG->Trace( "Device error: %s", usb_strerror() );
		Close();

		while( !Open() )
			usleep( 100000 );
	}

	return true;
}

void InputHandler_Linux_PIUIO::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
//		vDevicesOut.push_back( In
		vDescriptionsOut.push_back( "Input/lights controller" );
	}
}

int InputHandler_Linux_PIUIO::InputThread_Start( void *p )
{
	((InputHandler_Linux_PIUIO *) p)->InputThreadMain();
	return 0;
}

void InputHandler_Linux_PIUIO::InputThreadMain()
{

}

void InputHandler_Linux_PIUIO::UpdateLights()
{
	// XXX: need to check LightsDriver somehow
	ASSERT( g_LightsState );
}
