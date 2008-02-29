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

#if defined(UNIX)
#include "io/USBDevice.h"
#else
#include "io/USBDevice_Libusb.h"
#endif

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

	m_Title.LoadFromFont( THEME->GetPathF( "ScreenArcadeDiagnostics", "text" ) );
	m_Title.SetName( "Title" );

	// You can use some #define'd macros in ActorUtil.h for these. -- Vyhd
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
	this->PlayCommand( "Refresh" ); // This updates our uptime.

	/* Any reason this was at the end? If so, sorry for moving it. */
	Screen::Update( fDeltaTime );

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

	/* You can just access the device directly through the vector.
	 * No need to create a new device. :) -- Vyhd */
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
