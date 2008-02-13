#include "global.h"
#include "RageLog.h"

#include "LightsManager.h"
#include "PrefsManager.h"
#include "InputHandler_Linux_PIUIO.h"

InputHandler_Linux_PIUIO::InputHandler_Linux_PIUIO()
{
	m_bShutdown = false;

//	IOBoard.FindDevice

	// device found and set
	if( IOBoard.Open() )
	{
		m_iLightData = 0; // initialise

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
	UpdateLights();
}

void InputHandler_Linux_PIUIO::UpdateLights()
{
	// XXX: need to check LightsDriver somehow
	ASSERT( g_LightsState );

	// lighting needs cleaned up - vector/array map?
	// currently, I just want to get all this outlined.
	// it might even be completely wrong, but at least
	// we'll know what we're doing.

	// probably marquee lights
	if( g_LightsState[0] ) m_iLightData += (1 << 22);
	if( g_LightsState[1] ) m_iLightData += (1 << 25);
	if( g_LightsState[2] ) m_iLightData += (1 << 24);
	if( g_LightsState[3] ) m_iLightData += (1 << 23);

	// probably bass neons
	if( g_LightsState[6] || g_LightsState[7] )
		m_iLightData += (1 << 10);

	// pad lighting P1
	if( g_LightsState[08] ) m_iLightData += (1 << 19);
	if( g_LightsState[09] ) m_iLightData += (1 << 20);
	if( g_LightsState[10] ) m_iLightData += (1 << 17);
	if( g_LightsState[11] ) m_iLightData += (1 << 18);

	// pad lighting P2
	if( g_LightsState[28] ) m_iLightData += (1 << 4);
	if( g_LightsState[29] ) m_iLightData += (1 << 5);
	if( g_LightsState[30] ) m_iLightData += (1 << 2);
	if( g_LightsState[31] ) m_iLightData += (1 << 3);
}
