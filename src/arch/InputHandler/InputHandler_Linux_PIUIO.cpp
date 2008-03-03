#include "global.h"
#include "RageLog.h"
#include "RageException.h"
#include "RageUtil.h"

#include "ScreenManager.h" // for SCREENMAN->SystemMessageNoAnimate
#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h" // needed for g_LightsState
#include "InputHandler_Linux_PIUIO.h"

LightsState g_LightsState;

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
		/* Figure out the lights and write them */
		UpdateLights();
		IOBoard.Write( m_iLightData );

		/* Get the data and send it to RageInput */
		IOBoard.Read( &m_iInputData );
		HandleInput();

		usleep( 10000 ); // avoid unnecessary lag during testing
	}
}

void InputHandler_Linux_PIUIO::HandleInput()
{
	if( (m_iInputData != 0) && (m_iLastInputData != m_iInputData) )
	{
		CString sInputs;
		
		for( unsigned x = 0; x < 32; x++ )
		{
			// bitwise AND comparison
			if( !(m_iInputData & (1 << x)) )
				continue;

			if( sInputs == "" )
				sInputs	= ssprintf( "Inputs: (1 << %i)", x );
			else
				sInputs += ssprintf( ", (1 << %i)", x );
		}

		SCREENMAN->SystemMessageNoAnimate( sInputs );
	}

	m_iLastInputData = m_iInputData;

// The decompile is unreadable on this. I'm gonna brute-force it with the above.
#if 0
	static const int iInputBits[NUM_IO_BUTTONS] = {
	/* P1 pad - Left, Right, Up, Down */
	(1 << xx), (1 << xx), (1 << xx), (1 << xx),
	/* P1 control - Select, Start, MenuLeft, MenuRight */
	(1 << xx), (1 << xx), (1 << xx), (1 << xx),

	/* P2 pad - Left, Right, Up, Down */
	(1 << xx), (1 << xx), (1 << xx), (1 << xx),
	/* P2 control - Select, Start, MenuLeft, MenuRight */
	(1 << xx), (1 << xx), (1 << xx), (1 << xx),

	/* General - Service button, Coin event */
	(1 << xx), (1 << xx), };

	InputDevice id = DEVICE_PIUIO;

	for( int iButton = 0; iButton < NUM_IO_BUTTONS; iButton++ )
	{
		DeviceInput di(id, iButton);

		/* If we're in a thread, our timestamp is accurate */
		if( InputThread.IsCreated() )
			di.ts.Touch();

		ButtonPressed( di, !( m_iInputData & iInputBits[iButton]);
	}
#endif // 0
}

/* Requires "LightsDriver=ext" */
void InputHandler_Linux_PIUIO::UpdateLights()
{
	/* CONFIRMED BYTES
	 * ---------------
	 *
	 * Cabinet Lighting:
	 *
	 * Upper-left:	(????)			LL marquee:	(1 << 24)
	 * Upper-right:	(1 << 23)		LR marquee:	(1 << 25)
	 * Bass lights:	(1 << 10)

	 * P1 lighting:
	 *
	 * P1-Left:	????			P1-Right:	????
	 * P1-Up:	????			P1-Down:	????

	 * P2 lighting:
	 * 
	 * P2-Left:	(1 << 4)		P2-Right:	(1 << 5)
	 * P2-Up:	(1 << 2)		P2-Down:	(1 << 3)
	 */

	static const int iCabinetBits[NUM_CABINET_LIGHTS] = 
	{ (1 << 26), (1 << 23), (1 << 24), (1 << 25), 0, 0, (1 << 10), (1 << 10) };

	static const int iPadBits[2][4] = 
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

	// lights updating isn't too important...leave a lot of process time.
	usleep( 10000 );
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
