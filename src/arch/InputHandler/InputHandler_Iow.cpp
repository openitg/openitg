#include "global.h"
#include "RageLog.h"
#include "DiagnosticsUtil.h"

#include "arch/ArchHooks/ArchHooks.h"

// required I/O routines
#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_Iow.h"

// debug stuff
#include "RageUtil.h"
#include "ScreenManager.h"

REGISTER_INPUT_HANDLER( Iow );

bool InputHandler_Iow::s_bInitialized = false;

InputHandler_Iow::InputHandler_Iow()
{
	if( s_bInitialized )
	{
		LOG->Warn( "InputHandler_Iow: Redundant driver loaded. Disabling..." );
		return;
	}

	// attempt to open the I/O device
	if( !Board.Open() )
	{
		LOG->Warn( "InputHandler_Iow: could not establish a connection with the I/O device." );
		return;
	}

	LOG->Trace( "Opened ITGIO board." );

	s_bInitialized = true;
	m_bFoundDevice = true;
	m_bShutdown = false;

	DiagnosticsUtil::SetInputType("ITGIO");

	// set any alternate lights mappings, if they exist
	SetLightsMappings();

	// report every 5000 updates
	m_DebugTimer.SetName( "ITGIO" );
	m_DebugTimer.AutoReport( false );
	m_DebugTimer.SetInterval( 5000 );

	InputThread.SetName( "Iow thread" );
	InputThread.Create( InputThread_Start, this );
}

InputHandler_Iow::~InputHandler_Iow()
{
	if( !InputThread.IsCreated() )
		return;

	m_bShutdown = true;
	m_DebugTimer.Report();

	LOG->Trace( "Shutting down Iow thread..." );
	InputThread.Wait();
	LOG->Trace( "Iow thread shut down." );

	/* Reset all lights to off and close it */
	if( m_bFoundDevice )
	{
		Board.Write( 0 );
		Board.Close();

		s_bInitialized = false;
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

void InputHandler_Iow::SetLightsMappings()
{
	uint32_t iCabinetLights[NUM_CABINET_LIGHTS] = 
	{
		/* Upper-left, upper-right, lower-left, lower-right marquee */
		(1 << 8), (1 << 10), (1 << 9), (1 << 11),

		/* P1 select, P2 select, both bass */
		(1 << 13), (1 << 12), (1 << 15), (1 << 15)
	};

	uint32_t iGameLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS] =
	{
		/* Left, right, up, down */
		{ (1 << 1), (1 << 0), (1 << 3), (1 << 2) }, /* Player 1 */
		{ (1 << 5), (1 << 4), (1 << 7), (1 << 6) }, /* Player 2 */
	};

	m_LightsMappings.SetCabinetLights( iCabinetLights );
	m_LightsMappings.SetGameLights( iGameLights[GAME_CONTROLLER_1],
		iGameLights[GAME_CONTROLLER_2] );

	// if there are any alternate mappings, set them here now
	LightsMapper::LoadMappings( "ITGIO", m_LightsMappings );
}

int InputHandler_Iow::InputThread_Start( void *p )
{
	((InputHandler_Iow *) p)->InputThreadMain();
	return 0;
}

void InputHandler_Iow::InputThreadMain()
{
	// boost this thread priority past the priority of the binary;
	// if we don't, we might lose input data (e.g. coins) during loads.
	HOOKS->BoostThreadPriority();

	while( !m_bShutdown )
	{
		m_DebugTimer.StartUpdate();

		UpdateLights();

		/* XXX: the first 16 bits seem to manually trigger inputs;
		 * ITGIO opens high, so writing a 0 bit counts as a press.
		 * I have no idea what use that could possibly be, but we need
		 * to write 0xFFFF0000 to not trigger input on a write... */

		Board.Write( 0xFFFF0000 | m_iWriteData );

		// ITGIO opens high - flip the bit values
		Board.Read( &m_iReadData );
		m_iReadData = ~m_iReadData;

		HandleInput();

		m_DebugTimer.EndUpdate();

		if( g_bDebugInputDrivers && m_DebugTimer.TimeToReport() && SCREENMAN )
			SCREENMAN->SystemMessageNoAnimate( BitsToString(m_iReadData) );
	}

	HOOKS->UnBoostThreadPriority();
}

void InputHandler_Iow::HandleInput()
{
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

void InputHandler_Iow::UpdateLights()
{
	static const LightsState *m_LightsState = LightsDriver_External::Get();

	ZERO( m_iWriteData );

	// update cabinet lighting
	FOREACH_CabinetLight( cl )
		if( m_LightsState->m_bCabinetLights[cl] )
			m_iWriteData |= m_LightsMappings.m_iCabinetLights[cl];

	FOREACH_GameController( gc )
		FOREACH_GameButton( gb )
			if( m_LightsState->m_bGameButtonLights[gc][gb] )
				m_iWriteData |= m_LightsMappings.m_iGameLights[gc][gb];

	m_iWriteData |= m_LightsState->m_bCoinCounter ?
		m_LightsMappings.m_iCoinCounterOn : m_LightsMappings.m_iCoinCounterOff;
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
