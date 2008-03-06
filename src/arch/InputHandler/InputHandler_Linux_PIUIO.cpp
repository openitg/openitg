#include "global.h"
#include "RageLog.h"
#include "RageException.h"
#include "RageUtil.h"
#include "RageInput.h" // for g_sInputType

#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_Linux_PIUIO.h"

/* grabbed from LightsDriver_External */
extern LightsState g_LightsState;

/* grabbed from RageInput */
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

	/* Mark the input type, for theme purposes */
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
	uint64_t i = 1; // convenience hack
/* Enable as needed for raw input.. */
#if 0
	if( m_iInputData != 0 && ( m_iInputData != m_iLastInputData ) )
	{
		LOG->Info( "Input: %i", m_iInputData );
	}

	m_iLastInputData = m_iInputData;

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
			if( (m_iInputData & (i << x)) )
				continue;

			if( sInputs == "" )
				sInputs	= ssprintf( "Inputs: (1 << %i)", x );
			else
				sInputs += ssprintf( ", (1 << %i)", x );
		}

		if( LOG )
			LOG->Info( sInputs );
	}
//	return;
#endif	

	static const uint64_t iInputBits[NUM_IO_BUTTONS] = {
	/* Player 1 */	
	//Left arrow
	(i << 2) + (i << 61),
	// Right arrow
	(i << 3),
	// Up arrow
	(i << 0),
	// Down arrow
	(i << 1),
	// Select, Start, MenuLeft, MenuRight
	(i << 5), (i << 4), (i << 6), (i << 7),

	/* Player 2 */
	// Left arrow
	(i << 18),
	// Right arrow
	(i << 19),
	// Up arrow
	(i << 16),
	// Down arrow
	(i << 17),
	// Select, Start, MenuLeft, MenuRight
	(i << 21), (i << 20), (i << 22), (i << 23),

	/* General input */
	// Service button, Coin event
	(i << 9), (i << 5)
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
		/* Lights are Left, Right, Up, Down */
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
	CString sBits;

	for( int x = 0; x < 32; x++ )
		sBits += (m_iLightData & (1 << (32-x))) ? "1" : "0";

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
