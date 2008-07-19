#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "PrefsManager.h" // for m_bDebugUSBInput
#include "ScreenManager.h" // XXX
#include "LightsManager.h"
#include "DiagnosticsUtil.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_Iow.h"

extern LightsState g_LightsState;

// Iow lights thread is disabled for now: causes major, major input problems

InputHandler_Iow::InputHandler_Iow()
{
	m_bShutdown = false;
	DiagnosticsUtil::SetInputType("ITGIO");

	if( !IOBoard.Open() )
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

	LOG->Trace( "Shutting down Iow threads..." );

	if( InputThread.IsCreated() )
		InputThread.Wait();

	LOG->Trace( "Iow threads shut down." );

	/* Reset all lights to off and close it */
	IOBoard.Write( 0 );
	IOBoard.Close();
}

void InputHandler_Iow::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_ITGIO) );
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
//		IOBoard.Write( 0xFFFFFFFF );
		IOBoard.Write( 0xFFFF0000 | m_iWriteData );

		IOBoard.Read( &m_iReadData );

		// ITGIO opens high - flip the bit values
		m_iReadData = ~m_iReadData;

		if( SCREENMAN )
			SCREENMAN->SystemMessageNoAnimate( ssprintf( "%#2x", m_iReadData ) );

		HandleInput();
	}
}

void InputHandler_Iow::HandleInput()
{
	uint32_t i = 1; // convenience hack

	// filter out the data we've written

	static const uint32_t iInputBits[NUM_IO_BUTTONS] = {
	/* Player 1 - left, right, up, down */
	(i << 18), (i << 19), (i << 16), (i << 17),

	/* Player 1 - Select (unused), Start, MenuLeft, MenuRight */
	0, (i << 20), (i << 21), (i << 22),

	/* Player 2 - left, right, up, down */
	(i << 25), (i << 26), (i << 23), (i << 24),

	/* Player 2 - Select (unused), Start, MenuLeft, MenuRight */
	0, (i << 27), (i << 28), (i << 29),

	/* Service, coin insert */
	// XXX: service is untested
	(i << 30), (i << 31)	};

	if( PREFSMAN->m_bDebugUSBInput && (m_iReadData != 0) )
	{
		if( LOG )
			LOG->Info( "Input: %i", m_iReadData );

		CString sInputs;

		for( unsigned x = 0; x < 32; x++ )
		{
			/* the bit we expect isn't in the data */
			if( !(m_iReadData & (i << x)) )
				continue;

			if( sInputs == "" )
				sInputs = ssprintf( "Inputs: %i", x );
			else
				sInputs += ssprintf( ", %i", x );
		}

		if( LOG )
			LOG->Info( sInputs );
	}

	InputDevice id = DEVICE_ITGIO;

	for( int iButton = 0; iButton < NUM_IO_BUTTONS; iButton++ )
	{
		DeviceInput di(id, iButton);

		if( InputThread.IsCreated() )
			di.ts.Touch();

		ButtonPressed( di, (m_iReadData & iInputBits[iButton]) );
	}
}

/* Requires "LightsDriver=ext" */
void InputHandler_Iow::UpdateLights()
{
	static const uint32_t iCabinetBits[NUM_CABINET_LIGHTS] = {
	/* Upper-left, upper-right, lower-left, lower-right marquee */
	(1 << 8), (1 << 10), (1 << 9), (1 << 11),

	/* P1 select, P2 select, both bass */
	(1 << 13), (1 << 12), (1 << 15), (1 << 15)	};

	static const uint32_t iPadBits[2][4] =
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
		FOREACH_ENUM( GameButton, 4, gb )
			if( g_LightsState.m_bGameButtonLights[gc][gb] )
				m_iWriteData |= iPadBits[gc][gb];

	if( m_iWriteData != m_iLastWrite )
		LOG->Trace( "Iow lights: setting %i", m_iWriteData );
}
