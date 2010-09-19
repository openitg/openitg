#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "InputMapper.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_MK3.h"

// include system-specific low-level calls
#include "arch/ArchHooks/ArchHooks.h"
#include "InputHandler_MK3Helper.h"

REGISTER_INPUT_HANDLER( MK3 );

InputHandler_MK3::InputHandler_MK3()
{
	m_bFoundDevice = false;
	m_bShutdown = false;

	/* Claim both 16-bit output ports and both 16-bit input ports */
	if( !HOOKS->OpenMemoryRange(MK3_OUTPUT_PORT_1, 8) )
	{
		LOG->Warn( "InputHandler_MK3 requires root privileges to run." );
		return;
	}

	// TO DO: figure out what kind of values we read when MK3 isn't
	// initted, and more intelligently decide we have a device there.
	m_bFoundDevice = true;

	SetLightsMappings();

	InputThread.SetName( "MK3 thread" );
	InputThread.Create( InputThread_Start, this );
}

InputHandler_MK3::~InputHandler_MK3()
{
	if( InputThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down MK3 thread..." );
		InputThread.Wait();
		LOG->Trace( "MK3 thread shut down." );
	}

	// reset all lights
	if( m_bFoundDevice )
		MK3::Write( 0 );

	HOOKS->CloseMemoryRange(MK3_OUTPUT_PORT_1, 8);
}

void InputHandler_MK3::SetLightsMappings()
{
	uint32_t iCabinetLights[NUM_CABINET_LIGHTS] = 
	{
		/* UL, UR, LL, LR marquee lights */
		(1 << 23), (1 << 26), (1 << 25), (1 << 24),

		/* selection buttons (not used), bass lights */
		0, 0, (1 << 10), (1 << 10)
	};

	uint32_t iGameLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS] = 
	{
		/* Left, Right, Up, Down */
		{ (1 << 20), (1 << 21), (1 << 18), (1 << 19) },	/* Player 1 */
		{ (1 << 4), (1 << 5), (1 << 2), (1 << 3) }	/* Player 2 */
	};

	uint32_t iCoinCounter[2] = { (1 << 27), (1 << 28) };

	m_LightsMappings.SetCabinetLights( iCabinetLights );
	m_LightsMappings.SetCustomGameLights( iGameLights );
	m_LightsMappings.SetCoinCounter( iCoinCounter );
	
	LightsMapper::LoadMappings( "MK3", m_LightsMappings );
}

void InputHandler_MK3::GetDevicesAndDescriptions( vector<InputDevice> &vDevicesOut, vector<CString> &vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_JOY1) );
		vDescriptionsOut.push_back( "MK3" );
	}
}

void InputHandler_MK3::InputThreadMain()
{
	while( !m_bShutdown )
	{
		/* Figure out the lights states and set them */
		UpdateLights();

		/* Read input and sensors, write the above data */
		HandleInput();
	}
}

void InputHandler_MK3::HandleInput()
{
	ZERO( m_iInputField );
	ZERO( m_iInputData );

	for( uint32_t i = 0; i < 4; i++ )
	{
		// write which sensors to report from
		m_iLightData &= 0xFFFCFFFC;
		m_iLightData |= (i | (i << 16));

		// do one write/read cycle to get this sensor set
		MK3::Write( m_iLightData );
		MK3::Read( &m_iInputData[i] );

		// PIUIO opens high - invert it
		m_iInputData[i] = ~m_iInputData[i];

		// OR it into the input field
		m_iInputField |= m_iInputData[i];
	}

	DeviceInput di(DEVICE_JOY1, JOY_1);

	for( int iBtn = 0; iBtn < 32; iBtn++ )
	{
		di.button = JOY_1+iBtn;

		if( InputThread.IsCreated() )
			di.ts.Touch();

		// TODO: implement per-sensor reporting

		// report state of all button presses
		ButtonPressed( di, (bool)(m_iInputField & (1 << (31-iBtn))) );
	}
}

// TODO: lol
void InputHandler_MK3::UpdateLights()
{
	// set a const pointer to the "ext" LightsState to read from
	static const LightsState *m_LightsState = LightsDriver_External::Get();

	// reset
	ZERO( m_iLightData );

	// update marquee lights
	FOREACH_CabinetLight( cl )
		if( m_LightsState->m_bCabinetLights[cl] )
			m_iLightData |= m_LightsMappings.m_iCabinetLights[cl];

	FOREACH_GameController( gc )
		FOREACH_GameButton( gb )
			if( m_LightsState->m_bGameButtonLights[gc][gb] )
				m_iLightData |= m_LightsMappings.m_iGameLights[gc][gb];

/* doesn't work, as far as I can tell
	m_iLightData |= m_LightsState->m_bCoinCounter ?
		m_LightsMappings.m_iCoinCounterOn : m_LightsMappings.m_iCoinCounterOff;
*/
}

/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
