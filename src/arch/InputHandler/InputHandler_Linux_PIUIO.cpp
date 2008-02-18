#include "global.h"
#include "RageLog.h"
#include "RageException.h"

#include "PrefsManager.h"
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
		UpdateLights();
		IOBoard.Write( m_iLightData );
	}
}

/* Requires "LightsDriver=ext" */
void InputHandler_Linux_PIUIO::UpdateLights()
{
	/* From the decompile: */
#if 0
  if ( LightsDriver_External__g_LightState[9] )
    *(_WORD *)(a2 + 2) |= 0x20u;
  if ( LightsDriver_External__g_LightState[8] )
    *(_WORD *)(a2 + 2) |= 0x10u;
  if ( LightsDriver_External__g_LightState[11] )
    *(_WORD *)(a2 + 2) |= 8u;
  if ( LightsDriver_External__g_LightState[10] )
    *(_WORD *)(a2 + 2) |= 4u;
  if ( LightsDriver_External__g_LightState[29] )
    *(_WORD *)a2 |= 0x20u;
  if ( LightsDriver_External__g_LightState[28] )
    *(_WORD *)a2 |= 0x10u;
  if ( LightsDriver_External__g_LightState[31] )
    *(_WORD *)a2 |= 8u;
  if ( LightsDriver_External__g_LightState[30] )
    *(_WORD *)a2 |= 4u;
  if ( (_BYTE)LightsDriver_External__g_LightState )
    *(_WORD *)(a2 + 2) |= 0x80u;
  if ( LightsDriver_External__g_LightState[2] )
    *(_WORD *)(a2 + 2) |= 0x200u;
  if ( LightsDriver_External__g_LightState[1] )
    *(_WORD *)(a2 + 2) |= 0x400u;
  if ( LightsDriver_External__g_LightState[3] )
    *(_WORD *)(a2 + 2) |= 0x100u;
  if ( LightsDriver_External__g_LightState[6] )
    *(_WORD *)a2 |= 0x400u;
  if ( LightsDriver_External__g_LightState[7] )
    *(_WORD *)a2 |= 0x400u;
#endif

	static const int iCabinetBits[NUM_CABINET_LIGHTS-1] = 
	{ (1 << 22), (1 << 25), (1 << 24), (1 << 23), 0, 0, (1 << 10) };

	static const int iPlayerBits[2][4] = 
	{
		{ (1 << 19), (1 << 20), (1 << 17), (1 << 18) }, /* Player 1 */
		{ (1 << 4), (1 << 5), (1 << 2), (1 << 3) }	/* Player 2 */
	};

	// reset
	m_iLightData = 0;

	// there's one trigger for both bass lights, but we can only add
	// the value once - otherwise, it'll mess with our bits. Set left
	// if right is true, so we can iterate to NUM_CABINET_LIGHTS-1 only.

	if( g_LightsState.m_bCabinetLights[LIGHT_BASS_RIGHT] )
		g_LightsState.m_bCabinetLights[LIGHT_BASS_LEFT] = true;

	// update marquee lights
	FOREACH_ENUM( CabinetLight, NUM_CABINET_LIGHTS-1, cl )
		if( g_LightsState.m_bCabinetLights[cl] )
			m_iLightData += iCabinetBits[cl];

	// update the four pad lights on both game controllers
	FOREACH_GameController( gc )
		FOREACH_ENUM( GameButton, 4, gb )
			if( g_LightsState.m_bGameButtonLights[gc][gb] )
				m_iLightData += iPlayerBits[gc][gb];

	LOG->Trace( "UpdateLights: %u", m_iLightData );

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
