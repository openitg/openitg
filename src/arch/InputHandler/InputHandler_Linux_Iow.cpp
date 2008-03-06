#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageInput.h" // for g_sInputType
#include "RageException.h" // for sm_crash

#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_Linux_Iow.h"

extern LightsState g_LightsState;
extern CString g_sInputType;

InputHandler_Linux_Iow::InputHandler_Linux_Iow()
{
	m_bShutdown = false;

	if( IOBoard.Open() )
	{
		LOG->Trace( "Opened ITGIO board." );
		m_bFoundDevice = true;

		InputThread.SetName( "Iow thread" );
		InputThread.Create( InputThread_Start, this );
	}
	else
	{
		sm_crash( "Could not open ITGIO board." );
	}

	g_sInputType = "ITGIO";
}

InputHandler_Linux_Iow::~InputHandler_Linux_Iow()
{
	if( InputThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down Iow thread..." );
		InputThread.Wait();
		LOG->Trace( "Iow thread shut down" );
	}
	
	/* Reset all lights to off */
	IOBoard.Write( 0 );

	IOBoard.Close();
}

void InputHandler_Linux_Iow::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_ITGIO) );
		vDescriptionsOut.push_back( "ITGIO|PIUIO" );
	}
}

int InputHandler_Linux_Iow::InputThread_Start( void *p )
{
	((InputHandler_Linux_Iow *) p)->InputThreadMain();
	return 0;
}

void InputHandler_Linux_Iow::InputThreadMain()
{
	while( !m_bShutdown )
	{
		UpdateLights();
		IOBoard.Write( m_iLightData );

		IOBoard.Read( &m_iInputData );
		HandleInput();

		// give up 0.01 sec for other events -
		// this may need adjusting for lag/offsets
		usleep( 10000 );
	}
}

/* Thanks, ITG-IO documentation! <3 */
void InputHandler_Linux_Iow::HandleInput()
{
	static const uint16_t iInputBits[NUM_IO_BUTTONS] = {
	/* Player 1 - left, right, up, down */
	(1 << 13), (1 << 12), (1 << 15), (1 << 14),

	/* Player 1 - Select (unused), Start, MenuLeft, MenuRight */
	0, (1 << 11), (1 << 10), (1 << 9),

	/* Player 2 - left, right, up, down */
	(1 << 6), (1 << 5), (1 << 8), (1 << 7),

	/* Player 2 - Select (unused), Start, MenuLeft, MenuRight */
	0, (1 << 4), (1 << 3), (1 << 2),

	/* Service, coin insert */
	(1 << 1), (1 << 0)	};

	InputDevice id = DEVICE_ITGIO;

	for( int iButton = 0; iButton < NUM_IO_BUTTONS; iButton++ )
	{
		DeviceInput di(id, iButton);

		if( InputThread.IsCreated() )
			di.ts.Touch();

		ButtonPressed( di, (m_iInputData & iInputBits[iButton]) );
	}
}

/* Requires "LightsDriver=ext" */
void InputHandler_Linux_Iow::UpdateLights()
{
	static const uint16_t iCabinetBits[NUM_CABINET_LIGHTS] = {
	/* Upper-left, upper-right, lower-left, lower-right marquee */
	(1 << 8), (1 << 10), (1 << 9), (1 << 11),

	/* P1 select, P2 select, both bass */
	(1 << 13), (1 << 12), (1 << 15), (1 << 15) };

	static const uint16_t iPadBits[2][4] =
	{
		/* Left, right, up, down */
		{ (1 << 1), (1 << 0), (1 << 3), (1 << 2) },	/* Player 1 */
		{ (1 << 5), (1 << 4), (1 << 7), (1 << 6) }	/* Player 2 */
	};

	m_iLastWrite = m_iLightData;

	m_iLightData = 0;

	// update cabinet lighting
	FOREACH_CabinetLight( cl )
		if( g_LightsState.m_bCabinetLights[cl] )
			m_iLightData |= iCabinetBits[cl];

	// update the four lights on each pad
	FOREACH_GameController( gc )
		FOREACH_ENUM( GameButton, 4, gb )
			if( g_LightsState.m_bGameButtonLights[gc][gb] )
				m_iLightData |= iPadBits[gc][gb];

	CString sBits;

	for( int x = 0; x < 16; x++ )
		sBits += (m_iLightData & (1 << (16-x))) ? "1" : "0";

	LOG->Trace( "%s", sBits.c_str() );
}
