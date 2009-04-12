/* Basic screen functions */
#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ActorFrame.h"
#include "ThemeManager.h"
#include "ScreenManager.h"
#include "LightsManager.h"
#include "ScreenArcadeStart.h"

/* Serial number functions */
#include "DiagnosticsUtil.h"

/* USBDevice tests */
#include "io/USBDevice.h"

/* Input board loading/testing */
#include "io/USBDriver.h"
#include "io/PIUIO.h"
#include "io/ITGIO.h"

/* InputHandler loading */
#include "RageInput.h"
#include "InputMapper.h"
#include "arch/InputHandler/InputHandler_Iow.h"
#include "arch/InputHandler/InputHandler_PIUIO.h"

#define NEXT_SCREEN	THEME->GetMetric( m_sName, "NextScreen" )

/* automatic warning dismissal */
const float TIMEOUT = 10.0f;

REGISTER_SCREEN_CLASS( ScreenArcadeStart );

ScreenArcadeStart::ScreenArcadeStart( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadeStart::ScreenArcadeStart()" );
}

void ScreenArcadeStart::Init()
{
	ScreenWithMenuElements::Init();

	/* HACK: use LIGHTSMODE_JOINING to force all lights off. */
	LIGHTSMAN->SetLightsMode( LIGHTSMODE_JOINING );

	m_Error.LoadFromFont( THEME->GetPathF( "ScreenArcadeStart", "error" ) );
	m_Error.SetName( "Error" );
	SET_XY_AND_ON_COMMAND( m_Error );
	this->AddChild( &m_Error );

	this->SortByDrawOrder();

	/* are there any errors with loading the I/O board? */
	m_bBoardError = !LoadHandler();

	if( !m_bBoardError )
		m_bUSBError = !Refresh();
}

ScreenArcadeStart::~ScreenArcadeStart()
{
	LOG->Trace( "ScreenArcadeStart::~ScreenArcadeStart()" );
}

void ScreenArcadeStart::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );

	if( m_bFirstUpdate )
		m_Timer.Touch();

	if( m_bBoardError == false )
	{
		m_bUSBError = (Refresh() == false);

		// problem was fixed, or we timed out
		if( m_bUSBError == false || m_Timer.Ago() > TIMEOUT )
		{
			this->PlayCommand( "Off" );
			StartTransitioning( SM_GoToNextScreen );
		}
	}

	m_Error.SetText( m_sMessage );
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
		// nothing set by SAStart, so set it now
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

bool ScreenArcadeStart::Refresh()
{
	float fTimer = (TIMEOUT - m_Timer.Ago());

	// skew this by the tween time, so we stay at 10 until
	// the screen has fully loaded
	if( m_Timer.Ago() < 0.1 )
		fTimer += this->GetTweenTimeLeft();

	CLAMP( fTimer, 0.0f, TIMEOUT);

	if( !DiagnosticsUtil::HubIsConnected() )
	{
		m_sMessage = ssprintf(
			"The memory card hub is not connected.\nPlease consult the service manual for details.\n\n"
			"Connect the USB hub to continue,\nor wait %.0f seconds for this warning to automatically dismiss.",
			fTimer );
		return false;
	}

	return true;
}

bool ScreenArcadeStart::LoadHandler()
{
	// this makes it so much easier to keep track of. --Vyhd
	enum Board { BOARD_NONE, BOARD_ITGIO, BOARD_PIUIO };
	Board iBoard = BOARD_NONE;

	{
		vector<USBDevice> vDevices;
		GetUSBDeviceList( vDevices );

		/* g_sInputType is set by the I/O
		 * drivers - no need to do it here */
		for( unsigned i = 0; i < vDevices.size(); i++ )
		{
			if( vDevices[i].IsITGIO() )
				iBoard = BOARD_ITGIO;
			else if( vDevices[i].IsPIUIO() )
				iBoard = BOARD_PIUIO;

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
		DiagnosticsUtil::SetInputType( "Home" );
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
