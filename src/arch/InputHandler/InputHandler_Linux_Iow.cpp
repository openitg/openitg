#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageInput.h" // for g_sInputType
#include "PrefsManager.h" // for m_bDebugUSBInput
#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_Linux_Iow.h"

extern LightsState g_LightsState;
extern CString g_sInputType;

// Iow lights thread is disabled for now: causes major, major input problems

InputHandler_Linux_Iow::InputHandler_Linux_Iow()
{
	m_bShutdown = false;

	if( !IOBoard.Open() )
	{
		LOG->Warn( "OpenITG could not establish a connection with ITGIO." );
		return;
	}

	LOG->Trace( "Opened ITGIO board." );
	m_bFoundDevice = true;

	InputThread.SetName( "Iow thread" );
//	LightsThread.SetName( "Iow lights thread" );
	InputThread.Create( InputThread_Start, this );
//	LightsThread.Create( LightsThread_Start, this );

	g_sInputType = "ITGIO";
}

InputHandler_Linux_Iow::~InputHandler_Linux_Iow()
{
	if( !InputThread.IsCreated() )
		return;

	m_bShutdown = true;

	LOG->Trace( "Shutting down Iow threads..." );

	if( InputThread.IsCreated() )
		InputThread.Wait();

	if( LightsThread.IsCreated() )
		LightsThread.Wait();

	LOG->Trace( "Iow threads shut down" );

	/* Reset all lights to off and close it */
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

int InputHandler_Linux_Iow::LightsThread_Start( void *p )
{
	((InputHandler_Linux_Iow *) p)->LightsThreadMain();
	return 0;
}

void InputHandler_Linux_Iow::InputThreadMain()
{
	while( !m_bShutdown )
	{
		IOBoard.Read( &m_iInputData );

		m_iInputData = ~m_iInputData;

		HandleInput();

//		UpdateLights();
//		IOBoard.Write( m_iWriteData );

		// give up 0.008 sec for other events -
		// this may need adjusting for lag/offsets
		usleep( 8000 );
	}
}

void InputHandler_Linux_Iow::LightsThreadMain()
{
/*
	while( !m_bShutdown )
	{
		// give up 0.01 sec for other events -
		// this may need adjusting for lag/offsets
		usleep( 10000 );
	}
*/
}

/* Thanks, ITG-IO documentation. ):< */
void InputHandler_Linux_Iow::HandleInput()
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
	// FIXME: Service is obviously wrong
	(i << 1), (i << 31)	};

	if( (m_iInputData != 0) )
	{
		if( LOG )
			LOG->Info( "Input: %i", m_iInputData );

		CString sInputs;

		for( unsigned x = 0; x < 32; x++ )
		{
			/* the bit we expect isn't in the data */
//			if( (m_iInputData & (i << x)) )
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
void InputHandler_Linux_Iow::UpdateLights()
{
	/* FIXME: none of these are tested, and it's likely none are right. */
	static const uint32_t iCabinetBits[NUM_CABINET_LIGHTS] = {
	/* Upper-left, upper-right, lower-left, lower-right marquee */
	(1 << 24), (1 << 26), (1 << 25), (1 << 27),

	/* P1 select, P2 select, both bass */
	(1 << 29), (1 << 28), (1 << 31), (1 << 31) };

	static const uint32_t iPadBits[2][4] =
	{
		/* Left, right, up, down */
		{ (1 << 17), (1 << 16), (1 << 19), (1 << 18) },	/* Player 1 */
		{ (1 << 21), (1 << 20), (1 << 23), (1 << 22) }	/* Player 2 */
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
}
