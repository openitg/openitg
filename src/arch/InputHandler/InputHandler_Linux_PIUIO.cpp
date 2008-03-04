#include "global.h"
#include "RageLog.h"
#include "RageException.h"
#include "RageUtil.h"
#include "RageInput.h" // for g_sInputType

#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h" // needed for g_LightsState
#include "InputHandler_Linux_PIUIO.h"

LightsState g_LightsState;
extern CString g_sInputType;

InputHandler_Linux_PIUIO::InputHandler_Linux_PIUIO()
{
	m_bShutdown = false;

	// device found and set
	if( IOBoard.Open() )
	{
		LOG->Trace( "Opened PIUIO board." );
		m_bFoundDevice = true;

		InputThread.SetName( "PIUIO thread" );
		InputThread.Create( InputThread_Start, this );
	}
	else
	{
		/* We can't accept input, so why bother? */
		sm_crash( "Failed to connect to PIUIO board." );
	}

	g_sInputType = "PIUIO";
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

	IOBoard.Close();
}

void InputHandler_Linux_PIUIO::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_PIUIO) );
		vDescriptionsOut.push_back( "ITGIO|PIUIO" );
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
		/* Figure out the lights and write them */
		UpdateLights();
		IOBoard.Write( m_iLightData );

		/* Get the data and send it to RageInput */
		IOBoard.Read( &m_iInputData );
		HandleInput();

		// give up 0.01 sec per read for other events -
		// this may need adjusting for sync/lag fixing
		usleep( 10000 );
	}
}

void InputHandler_Linux_PIUIO::HandleInput()
{
/* Enable as needed for raw input.. */
#if 0
	if( m_iInputData != 0 && ( m_iInputData != m_iLastInputData ) )
	{
		LOG->Info( "Input: %i", m_iInputData );
	}

	m_iLastInputData;

	return;
#endif

/* Enable as needed for input logging... */
#if 1
	if( m_iInputData != 0 && ( m_iInputData != m_iLastInputData ) )
	{
		CString sInputs;
		
		for( unsigned x = 0; x < 64; x++ )
		{
			// bitwise AND sieve - PIUIO is open high
			if( (m_iInputData & (1 << x)) )
				continue;

			if( sInputs == "" )
				sInputs	= ssprintf( "Inputs: (1 << %i)", x );
			else
				sInputs += ssprintf( ", (1 << %i)", x );
		}

		if( LOG )
			LOG->Info( sInputs );
	}
	return;
#endif	

	static const uint64_t iInputBits[NUM_IO_BUTTONS] = {
	/* Player 1 */	
	//Left arrow, left/right/up/down sensors
	(1 << 2), // + (1 << x) + (1 << x) + (1 << x),

	// Right arrow, left/right/up/down sensors
	(1 << 3), // + (1 << x) + (1 << x) + (1 << x),
	
	// Up arrow, left/right/up/down sensors
	(1 << 0), // + (1 << x) + (1 << x) + (1 << x),
	
	// Down arrow, left/right/up/down sensors
	(1 << 1), // + (1 << x) + (1 << x) + (1 << x),

	// Select, Start, MenuLeft, MenuRight
	(1 << 21), (1 << 20), (1 << 22), (1 << 23),

	/* Player 2 */
	// Left arrow, left/right/up/down sensors
	(1 << 18), // + (1 << x) + (1 << x) + (1 << x),

	// Right arrow, left/right/up/down sensors
	(1 << 19), // + (1 << x) + (1 << x) + (1 << x),
	
	// Up arrow, left/right/up/down sensors
	(1 << 16), // + (1 << x) + (1 << x) + (1 << x),
	
	// Down arrow, left/right/up/down sensors
	(1 << 17), // + (1 << x) + (1 << x) + (1 << x),

	// Select, Start, MenuLeft, MenuRight
	(1 << 5), (1 << 4), (1 << 6), (1 << 7),

	/* General input */
	// Service button, Coin event
	(1 << 9), (1 << 20) // (1 << 20) should be P1 start, won't trigger
	};

	InputDevice id = DEVICE_PIUIO;

	for( int iButton = 0; iButton < NUM_IO_BUTTONS; iButton++ )
	{
		DeviceInput di(id, iButton);

		/* If we're in a thread, our timestamp is accurate */
		if( InputThread.IsCreated() )
			di.ts.Touch();

		ButtonPressed( di, !(m_iInputData & iInputBits[iButton]) );
	}
}

CString SensorDescriptions[] = {"right", "left", "bottom", "top"};

CString GetSensorDescription(int bits) {
	if (bits == 0) return "";
	CStringArray retSensors;
	for(int i = 0; i < 3; i++) {
		if (bits & (1 << i)) retSensors.push_back(SensorDescriptions[i]);
	}
	return join(", ", retSensors);
}

/* Requires "LightsDriver=ext" */
void InputHandler_Linux_PIUIO::UpdateLights()
{
	static const uint32_t iCabinetBits[NUM_CABINET_LIGHTS] = 
	{ (1 << 23), (1 << 26), (1 << 25), (1 << 24), 0, 0, (1 << 10), (1 << 10) };

	static const uint32_t iPadBits[2][4] = 
	{
		{ (1 << 20), (1 << 21), (1 << 18), (1 << 19) },	/* Player 1 */
		{ (1 << 4), (1 << 5), (1 << 2), (1 << 3) }	/* Player 2 */
	};

	// reset
	m_iLightData = 0;

	// update marquee lights
	FOREACH_CabinetLight( cl )
		if( g_LightsState.m_bCabinetLights[cl] )
			m_iLightData |= iCabinetBits[cl];

	// update the four pad lights on both game controllers
	FOREACH_GameController( gc )
		FOREACH_ENUM( GameButton, 4, gb )
			if( g_LightsState.m_bGameButtonLights[gc][gb] )
				m_iLightData |= iPadBits[gc][gb];

	/* Debugging purposes */
	if( m_iLastLightData != m_iLightData )
		LOG->Trace( "UpdateLights: %u", m_iLightData );

	m_iLastLightData = m_iLightData;
}

/*
 * (c) 2008 BoXoRRoXoRs
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
