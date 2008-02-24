#include "global.h"
#include "ScreenArcadeDiagnostics.h"
#include "ScreenManager.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "GameState.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "Game.h"
#include "ScreenDimensions.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "RageInput.h"
#include "ActorUtil.h"
#include "ActorFrame.h" // We need this to call PlayCommand,Refresh and update the uptime.

REGISTER_SCREEN_CLASS( ScreenArcadeDiagnostics );

ScreenArcadeDiagnostics::ScreenArcadeDiagnostics( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadeDiagnostics::ScreenArcadeDiagnostics()" );
}

void ScreenArcadeDiagnostics::Init()
{
	ScreenWithMenuElements::Init();

	m_USBInfo.LoadFromFont( THEME->GetPathF( "ScreenArcadeDiagnostics", "text" ) );
	m_USBInfo.SetName( "USBInfo" );

	//m_USBInfo.SetZoom( 0.6f );
	//m_USBInfo.SetText("USB Info: ");

	// You can use some #define'd macros in ActorUtil.h for these. -- Vyhd
	SET_XY( m_USBInfo );
	COMMAND( m_USBInfo, "On" );
	
	this->AddChild( &m_USBInfo );
	this->SortByDrawOrder();
}

ScreenArcadeDiagnostics::~ScreenArcadeDiagnostics()
{
	LOG->Trace( "ScreenArcadeDiagnostics::~ScreenArcadeDiagnostics()" );
}

void ScreenArcadeDiagnostics::Update( float fDeltaTime )
{
	this->PlayCommand( "Refresh" ); // This updates our uptime.

	/* Any reason this was at the end? If so, sorry for moving it. */
	Screen::Update( fDeltaTime );

	vector<USBDevice> vDevList;
	GetUSBDeviceList( vDevList );

	if ( vDevList.size() == 0 )
	{
		m_USBInfo.SetText("No USB Devices");
		return;
	}

	/* Nothing's changed, why go through the list? */
	if( vDevList.size() == m_iLastSeenDevices )
		return;

	CString sInfo;

	/* You can just access the device directly through the vector.
	 * No need to create a new device. :) -- Vyhd */
	for (unsigned i = 0; i < vDevList.size(); i++)
	{
		sInfo += vDevList[i].GetDeviceDir() + ":";
		CString sDevInfo = ssprintf("%04X:%04X: %s (%dmA)\n",
			vDevList[i].GetIdVendor(),
			vDevList[i].GetIdProduct(),
			vDevList[i].GetDescription().c_str(),
			vDevList[i].GetMaxPower() );
		sInfo += sDevInfo;
	}

	m_USBInfo.SetText( sInfo );
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
		SCREENMAN->PlayStartSound();
		StartTransitioning( SM_GoToPrevScreen );		
	}
}
