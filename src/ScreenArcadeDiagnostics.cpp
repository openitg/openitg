#include "global.h"
#include "RageLog.h"
#include "ScreenArcadeDiagnostics.h"
#include "io/USBDevice.h"

REGISTER_SCREEN_CLASS( ScreenArcadeDiagnostics );

// how much time to wait between re-enumerating devices (which is expensive)
const float USB_UPDATE_TIME = 0.5f;

ScreenArcadeDiagnostics::ScreenArcadeDiagnostics( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadeDiagnostics::ScreenArcadeDiagnostics()" );
}

void ScreenArcadeDiagnostics::Init()
{
	ScreenWithMenuElements::Init();

	m_USBInfo.LoadFromFont( THEME->GetPathF( "ScreenArcadeDiagnostics", "text" ) );
	m_USBInfo.SetName( "USBInfo" );

	m_Title.LoadFromFont( THEME->GetPathF( "ScreenArcadeDiagnostics", "text" ) );
	m_Title.SetName( "Title" );

	SET_XY_AND_ON_COMMAND( m_USBInfo );
	SET_XY_AND_ON_COMMAND( m_Title );
	
	this->AddChild( &m_USBInfo );
	this->AddChild( &m_Title );
	this->SortByDrawOrder();
}

ScreenArcadeDiagnostics::~ScreenArcadeDiagnostics()
{
	LOG->Trace( "ScreenArcadeDiagnostics::~ScreenArcadeDiagnostics()" );
}

void ScreenArcadeDiagnostics::Update( float fDeltaTime )
{
	// update the theme elements (uptime, etc.)
	PlayCommand( "Refresh" );

	// only update the USB list once per UPDATE_TIME period.
	// this allows us to keep the screen running smoothly while
	// maintaining a low detection granularity.
	if( m_bFirstUpdate || m_UpdateTimer.Ago() > USB_UPDATE_TIME )
	{
		m_UpdateTimer.Touch();
		UpdateElements();
	}

	Screen::Update( fDeltaTime );
}

void ScreenArcadeDiagnostics::UpdateElements()
{
	// update the USB devices list
	vector<USBDevice> vDevList;
	GetUSBDeviceList( vDevList );

	/* Nothing's changed, why go through the list? */
	if( vDevList.size() == m_iLastSeenDevices )
		return;

	if ( vDevList.size() == 0 )
	{
		m_USBInfo.SetText("No USB Devices");
		m_Title.SetText("");
		m_iLastSeenDevices = 0;
		return;
	}

	CString sInfo, sTitleInfo;

	for (unsigned i = 0; i < vDevList.size(); i++)
	{
		sTitleInfo += vDevList[i].GetDeviceDir() + ":\n";
		CString sDevInfo = ssprintf("%04X:%04X: %s (%dmA)\n",
			vDevList[i].GetIdVendor(),
			vDevList[i].GetIdProduct(),
			vDevList[i].GetDescription().c_str(),
			vDevList[i].GetMaxPower() );
		sInfo += sDevInfo;
	}
	m_iLastSeenDevices = vDevList.size();
	
	m_USBInfo.SetText( sInfo );
	m_Title.SetText( sTitleInfo );
}

void ScreenArcadeDiagnostics::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenArcadeDiagnostics::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default handler
}

void ScreenArcadeDiagnostics::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToNextScreen:
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		break;
	}
}

void ScreenArcadeDiagnostics::MenuStart( PlayerNumber pn )
{
	MenuBack(pn);
}

void ScreenArcadeDiagnostics::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		this->PlayCommand( "Off" );
		StartTransitioning( SM_GoToPrevScreen );
	}
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
