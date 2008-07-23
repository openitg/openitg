#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "PrefsManager.h" // for m_bDebugUSBInput
#include "LightsManager.h"
#include "DiagnosticsUtil.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_Iow.h"

extern LightsState g_LightsState;

const int NUM_PAD_LIGHTS = 4;

// Iow lights thread is disabled for now: causes major, major input problems

InputHandler_Iow::InputHandler_Iow()
{
	m_bShutdown = false;
	DiagnosticsUtil::SetInputType("ITGIO");

	if( !Board.Open() )
	{
		LOG->Warn( "OpenITG could not establish a connection with ITGIO." );
		return;
	}

	LOG->Trace( "Opened ITGIO board." );
	m_bFoundDevice = true;

	/* warn if "ext" isn't enabled */
	if( PREFSMAN->GetLightsDriver().Find("ext") == -1 )
		LOG->Warn( "\"ext\" is not an enabled LightsDriver. The I/O board cannot run lights." );

	InputThread.SetName( "Iow thread" );
	InputThread.Create( InputThread_Start, this );
}

InputHandler_Iow::~InputHandler_Iow()
{
	if( !InputThread.IsCreated() )
		return;

	m_bShutdown = true;

	LOG->Trace( "Shutting down Iow thread..." );
	InputThread.Wait();
	LOG->Trace( "Iow thread shut down." );

	/* Reset all lights to off and close it */
	if( m_bFoundDevice )
	{
		Board.Write( 0 );
		Board.Close();
	}
}

void InputHandler_Iow::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_JOY1) );
		vDescriptionsOut.push_back( "ITGIO" );
	}
}

int InputHandler_Iow::InputThread_Start( void *p )
{
	((InputHandler_Iow *) p)->InputThreadMain();
	return 0;
}

void InputHandler_Iow::InputThreadMain()
{
	while( !m_bShutdown )
	{
		UpdateLights();

		// this appears to be AND'd over input (bits 1-16)
		Board.Write( 0xFFFF0000 | m_iWriteData );

		// ITGIO opens high - flip the bit values
		Board.Read( &m_iReadData );
		m_iReadData = ~m_iReadData;

		HandleInput();
	}
}

void InputHandler_Iow::HandleInput()
{
	uint32_t i = 1; // convenience hack

	// filter out the data we've written
	if( PREFSMAN->m_bDebugUSBInput && (m_iReadData != 0) )
	{
		CString sInputs;

		for( unsigned x = 0; x < 16; x++ )
		{
			/* the bit we expect isn't in the data */
			if( !(m_iReadData & (i << (31-x))) )
				continue;

			if( sInputs == "" )
				sInputs = ssprintf( "Inputs: %i", x );
			else
				sInputs += ssprintf( ", %i", x );
		}

		if( LOG )
			LOG->Info( sInputs );
	}

	DeviceInput di = DeviceInput( DEVICE_JOY1, JOY_1 );

	// ITGIO only reads the first 16 bits
	for( int iButton = 0; iButton < 16; iButton++ )
	{
		di.button = JOY_1+iButton;

		if( InputThread.IsCreated() )
			di.ts.Touch();

		ButtonPressed( di, m_iReadData & (1 << (31-iButton)) );
	}
}

/* Requires "LightsDriver=ext" */
void InputHandler_Iow::UpdateLights()
{
	static const uint32_t iCabinetBits[NUM_CABINET_LIGHTS] =
	{
		/* Upper-left, upper-right, lower-left, lower-right marquee */
		(1 << 8), (1 << 10), (1 << 9), (1 << 11),

		/* P1 select, P2 select, both bass */
		(1 << 13), (1 << 12), (1 << 15), (1 << 15)
	};

	static const uint32_t iPadBits[MAX_GAME_CONTROLLERS][NUM_PAD_LIGHTS] =
	{
		/* Left, right, up, down */
		{ (1 << 1), (1 << 0), (1 << 3), (1 << 2) }, /* Player 1 */
		{ (1 << 5), (1 << 4), (1 << 7), (1 << 6) }, /* Player 2 */
	};

	m_iLastWrite = m_iWriteData;
	m_iWriteData = 0;

	/* Something about the coin counter here */

	// update cabinet lighting
	FOREACH_CabinetLight( cl )
		if( g_LightsState.m_bCabinetLights[cl] )
			m_iWriteData |= iCabinetBits[cl];

	// update the four lights on each pad
	FOREACH_GameController( gc )
		FOREACH_ENUM( GameButton, NUM_PAD_LIGHTS, gb )
			if( g_LightsState.m_bGameButtonLights[gc][gb] )
				m_iWriteData |= iPadBits[gc][gb];

	if( m_iWriteData != m_iLastWrite )
		LOG->Debug( "Iow lights: setting %i", m_iWriteData );
}
