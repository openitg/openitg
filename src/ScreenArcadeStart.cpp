/* Basic screen functions */
#include "global.h"
#include "ScreenArcadeStart.h"

/* diagnostics functions */
#include "DiagnosticsUtil.h"

/* USBDevice tests */
#include "io/USBDevice.h"

/* Input board loading/testing */

/* InputHandler loading */
#include "RageInput.h"
#include "InputMapper.h"
#include "arch/InputHandler/InputHandler_Iow.h"
#include "arch/InputHandler/InputHandler_PIUIO.h"
#include "arch/InputHandler/InputHandler_MiniMaid.h"
#include "arch/InputHandler/InputHandler_P3IO.h"

#define NEXT_SCREEN	THEME->GetMetric( m_sName, "NextScreen" )

/* automatic warning dismissal */
const float TIMEOUT = 15.0f;

/* time between validation attempts, which can be expensive */
const float UPDATE_TIME = 1.0f;

REGISTER_SCREEN_CLASS( ScreenArcadeStart );

ScreenArcadeStart::ScreenArcadeStart( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadeStart::ScreenArcadeStart()" );
}

void ScreenArcadeStart::Init()
{
	ScreenWithMenuElements::Init();

	/* Use LIGHTSMODE_JOINING to force all lights off except selection buttons. */
	LIGHTSMAN->SetLightsMode( LIGHTSMODE_JOINING );

	m_Error.LoadFromFont( THEME->GetPathF( "ScreenArcadeStart", "error" ) );
	m_Error.SetName( "Error" );
	SET_XY_AND_ON_COMMAND( m_Error );
	this->AddChild( &m_Error );

	this->SortByDrawOrder();

	/* Initialize the timeout, skew by the tween time */
	m_fTimeout = TIMEOUT + this->GetTweenTimeLeft();

	/* are there any errors with loading the I/O board? */
	m_bHandlerLoaded = LoadHandler();
}

ScreenArcadeStart::~ScreenArcadeStart()
{
	LOG->Trace( "ScreenArcadeStart::~ScreenArcadeStart()" );
}

void ScreenArcadeStart::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );

	/* update the error text as needed */
	m_Error.SetText( m_sMessage );

	/* if everything's good, fake a Start press. It's a bit
	 * hacky, but we can keep all our logic in one place. */
	if( m_fTimeout < 0 || (m_bHandlerLoaded && m_bHubConnected) )
		MenuStart( PLAYER_INVALID );

	/* If we have no handler, we can't play. If we can play, and
	 * don't know it, the user can hit START. Don't time out. */
	if( !m_bHandlerLoaded )
		return;

	m_fTimeout -= fDeltaTime;

	/* Check for the USB hub. Has it been connected? */
	m_bHubConnected = CheckForHub();
}

void ScreenArcadeStart::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default handler
}

void ScreenArcadeStart::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToNextScreen:
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
		break;
	}
}

void ScreenArcadeStart::MenuStart( PlayerNumber pn )
{
	if( !IsTransitioning() )
	{
		// nothing set by any I/O handlers, so set one now
		if( DiagnosticsUtil::GetInputType() == "" )
			DiagnosticsUtil::SetInputType( "Home" );

		this->PlayCommand( "Off" );
		StartTransitioning( SM_GoToNextScreen );
	}
}

void ScreenArcadeStart::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

bool ScreenArcadeStart::CheckForHub()
{
	/* make sure we never add the tween time into the timeout text. */
	float fTimer = m_fTimeout;
	CLAMP( fTimer, 0.0f, TIMEOUT);

	if( !DiagnosticsUtil::HubIsConnected() )
	{
		m_sMessage = ssprintf(
			"The memory card hub is not connected.\n"
			"Please consult the service manual for details.\n\n"
			"Connect the USB hub or press START to continue,\n"
			"or wait %.0f seconds for this warning to automatically dismiss.",
			fTimer );

		return false;
	}

	return true;
}

bool ScreenArcadeStart::LoadHandler()
{
	// this makes it so much easier to keep track of. --Vyhd
	enum Board { BOARD_NONE, BOARD_ITGIO, BOARD_PIUIO, BOARD_MINIMAID, BOARD_P3IO };
	Board iBoard = BOARD_NONE;

	{
		vector<USBDevice> vDevices;
		GetUSBDeviceList( vDevices );

		for( unsigned i = 0; i < vDevices.size(); i++ )
		{
			if( vDevices[i].IsITGIO() )
				iBoard = BOARD_ITGIO;
			else if( vDevices[i].IsPIUIO() )
				iBoard = BOARD_PIUIO;
			else if( vDevices[i].IsMiniMaid() )
				iBoard = BOARD_MINIMAID;
			else if( vDevices[i].IsP3IO() )
				iBoard = BOARD_P3IO;

			// early abort if we found something
			if( iBoard != BOARD_NONE )
				break;
		}
	}

	USBDriver *pDriver;

	if( iBoard == BOARD_ITGIO )
		pDriver = new ITGIO;
	else if( iBoard == BOARD_PIUIO )
		pDriver = new PIUIO;
	else if( iBoard == BOARD_MINIMAID )
		pDriver = new MiniMaid;
	else
#ifdef ITG_ARCADE
	{
		m_sMessage = "The input/lights controller is not connected or is not receiving power.\n\nPlease consult the service manual.";
		return false;
	}
#else
	{
		/* Return true if PC, even though it doesn't load. */
		LOG->Warn( "ScreenArcadeStart: I/O board not found. Continuing anyway..." );
		return true;
	}
#endif

	/* Attempt a connection */
	if( !pDriver->Open() )
	{
		m_sMessage = "The input/lights controller could not be initialized.\n\nPlease consult the service manual.";

		SAFE_DELETE( pDriver );
		return false;
	}

	pDriver->Close();
	SAFE_DELETE( pDriver );

	if( iBoard == BOARD_ITGIO )
		INPUTMAN->AddHandler( new InputHandler_Iow );
	else if( iBoard == BOARD_PIUIO )
		INPUTMAN->AddHandler( new InputHandler_PIUIO );
	else if( iBoard == BOARD_MINIMAID )
		INPUTMAN->AddHandler( new InputHandler_MiniMaid );
	else
		ASSERT(0);

	LOG->Trace( "Remapping joysticks after loading driver." );

	INPUTMAPPER->AutoMapJoysticksForCurrentGame();
	INPUTMAPPER->SaveMappingsToDisk();

	return true;
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
