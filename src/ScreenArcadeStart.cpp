/* Basic screen functions */
#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ScreenArcadeStart.h"
#include "ScreenManager.h"
#include "ActorFrame.h"

/* I'd like to include these as needed only */
//#include "GameState.h"
//#include "GameSoundManager.h"
//#include "ThemeManager.h"
//#include "Game.h"
//#include "ScreenDimensions.h"
//#include "GameManager.h"
//#include "PrefsManager.h"
//#include "ActorUtil.h"

/* Serial number functions */
#include "MiscITG.h"

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

#define NEXT_SCREEN	THEME->GetMetric( m_sName, "NextScreen" );

REGISTER_SCREEN_CLASS( ScreenArcadeStart );

ScreenArcadeStart::ScreenArcadeStart( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadeStart::ScreenArcadeStart()" );
}

void ScreenArcadeStart::Init()
{
	CString sGameSerial = GetSerialNumber();

	ScreenWithMenuElements::Init();

	m_Error.LoadFromFont( THEME->GetPathF( "ScreenArcadeStart", "error" ) );
	m_Error.SetName( "Error" );
	SET_XY_AND_ON_COMMAND( m_Error );
	this->AddChild( &m_Error );

	this->SortByDrawOrder();

	CString sError = "";

	if( !LoadHandler(sError) )
	{
		m_Error.SetText( sError );
		return;
	}

	if( !Refresh(sError) )
	{
		m_Error.SetText( sError );
		return;
	}

	if( sError.empty() )
		LOG->Trace( "All OK" );
}

ScreenArcadeStart::~ScreenArcadeStart()
{
	LOG->Trace( "ScreenArcadeStart::~ScreenArcadeStart()" );

	if( m_bFatalError )
		HOOKS->SystemReboot();
}

void ScreenArcadeStart::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
}

void ScreenArcadeStart::DrawPrimitives()
{
	Screen::DrawPrimitives();
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
		SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		break;
	}
}

void ScreenArcadeStart::MenuStart( PlayerNumber pn )
{
	MenuBack(pn);
}

bool ScreenArcadeStart::Refresh( CString &sMessage )
{
	vector<USBDevice> vDevices;
	GetUSBDeviceList( vDevices );
}

bool ScreenArcadeStart::LoadHandler( CString &sMessage )
{
	USBDriver *pDriver;

	bIsKit = false;
	/* Default to PIUIO if this isn't a kit, at least for now */
	if( IsAFile( "/rootfs/itgdata/K" )
		bIsKit = true;

	if( bIsKit )
		pDriver = new ITGIO;
	else
		pDriver = new PIUIO;
	
	/* Attempt a connection */
	if( !pDriver->Open() )
	{	
		sMessage = "The input/lights controller could not be initialized.\n\nPlease consult the service manual.";

		delete pDriver;
		return false;
	}

	pDriver->Close();
	delete pDriver;

	if( bIsKit )
		INPUTMAN->AddHandler( new InputHandler_Iow );
	else
		INPUTMAN->AddHandler( new InputHandler_PIUIO );

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
