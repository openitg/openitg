#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageInput.h" // for g_sInputType
#include "PrefsManager.h" // for m_bDebugUSBInput
#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_Iow.h"

extern LightsState g_LightsState;
extern CString g_sInputType;

// Iow lights thread is disabled for now: causes major, major input problems

InputHandler_Iow::InputHandler_Iow()
{
	m_bShutdown = false;

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
	LightsThread.SetName( "Iow lights thread" );
	InputThread.Create( InputThread_Start, this );
	LightsThread.Create( LightsThread_Start, this );

	g_sInputType = "ITGIO";
}

InputHandler_Iow::~InputHandler_Iow()
{
	if( !InputThread.IsCreated() )
		return;

	m_bShutdown = true;

	LOG->Trace( "Shutting down Iow threads..." );

	if( InputThread.IsCreated() )
		InputThread.Wait();

	if( LightsThread.IsCreated() )
		LightsThread.Wait();

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
		vDescriptionsOut.push_back( "ITGIO|PIUIO" );
	}
}

int InputHandler_Iow::InputThread_Start( void *p )
{
	((InputHandler_Iow *) p)->InputThreadMain();
	return 0;
}

int InputHandler_Iow::LightsThread_Start( void *p )
{
	((InputHandler_Iow *) p)->LightsThreadMain();
	return 0;
}

void InputHandler_Iow::InputThreadMain()
{
	while( !m_bShutdown )
	{
		IOBoard.Read( &m_iInputData );

		m_iInputData = ~m_iInputData;

		HandleInput();

//		UpdateLights();
//		IOBoard.Write( m_iWriteData );
	}
}

void InputHandler_Iow::LightsThreadMain()
{
	while( !m_bShutdown )
	{
		UpdateLights();
		// TEST: Do we need to re-write input?
		IOBoard.Write( m_iWriteData );
//		IOBoard.Write( m_iInputData | m_iWriteData );
	}
}

void InputHandler_Iow::HandleInput()
{
	uint32_t i = 1; // convenience hack

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

	if( PREFSMAN->m_bDebugUSBInput && (m_iInputData != 0) )
	{
		if( LOG )
			LOG->Info( "Input: %i", m_iInputData );

		CString sInputs;

		for( unsigned x = 0; x < 32; x++ )
		{
			/* the bit we expect isn't in the data */
			if( !(m_iInputData & (i << x)) )
				continue;

			if( sInputs == "" )
				sInputs = ssprintf( "Inputs: (1 << %i)", x );
			else
				sInputs += ssprintf( ", (1 << %i)", x );
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

		ButtonPressed( di, (m_iInputData & iInputBits[iButton]) );
	}
}

/* Requires "LightsDriver=ext" */
void InputHandler_Iow::UpdateLights()
{
	/* To do:
	 - Does Iow crash if lights never change? [possible Write() problem]
	 - See if this can accidentally set input states
	*/

	// We don't know these anyway...no loss wiping them.
	// TEST: does the input problem occur if we write all 0s?
	static const uint32_t iCabinetBits[NUM_CABINET_LIGHTS] = {
	/* Upper-left, upper-right, lower-left, lower-right marquee */
	0, 0, 0, 0,

	/* P1 select, P2 select, both bass */
	0, 0, 0, 0	};

	static const uint32_t iPadBits[2][4] =
	{
		/* Left, right, up, down */
		{ 0, 0, 0, 0 }, /* Player 1 */
		{ 0, 0, 0, 0 }, /* Player 2 */
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
