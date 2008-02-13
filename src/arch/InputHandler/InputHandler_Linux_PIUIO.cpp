#include "global.h"
#include "RageLog.h"
#include "RageException.h"

#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h" // needed for g_LightsState
#include "PrefsManager.h"
#include "InputHandler_Linux_PIUIO.h"

LightsState g_LightsState;

InputHandler_Linux_PIUIO::InputHandler_Linux_PIUIO()
{
	m_bShutdown = false;

	// device found and set
	if( IOBoard.Open() )
	{
		LOG->Trace( "Opened I/O board." );
		m_bFoundDevice = true;

		InputThread.SetName( "PIUIO thread" );
		InputThread.Create( InputThread_Start, this );
	}
	else
	{
		sm_crash( "InputHandler_Linux_PIUIO: Failed to open PIU I/O board." );
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

void InputHandler_Linux_PIUIO::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		// currently a dummy number
		vDevicesOut.push_back( InputDevice(DEVICE_PIUIO) );
		vDescriptionsOut.push_back( "PIUIO" );
	}
}

int InputHandler_Linux_PIUIO::InputThread_Start( void *p )
{
	((InputHandler_Linux_PIUIO *) p)->InputThreadMain();
	return 0;
}

void InputHandler_Linux_PIUIO::InputThreadMain()
{
	while( !m_bShutdown )
	{	
		/* For now, we just want to test lights updates. */
		UpdateLights();
		IOBoard.Write( m_iLightData );
	}
}

void InputHandler_Linux_PIUIO::UpdateLights()
{
	// XXX: need to check LightsDriver somehow

	static const int iCabinetData[NUM_CABINET_LIGHTS-1] = 
	{ (1 << 22), (1 << 25), (1 << 24), (1 << 23), 0, 0, (1 << 10) };

	// reset
	m_iLightData = 0;

	// Just do cabinet lights for now and we'll figure the rest later.
	FOREACH_ENUM( CabinetLight, NUM_CABINET_LIGHTS-1, cl )
		if( g_LightsState.m_bCabinetLights[cl] )
			m_iLightData += iCabinetData[cl];
// for reference	
#if 0	
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
#endif
}

