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

	USBInfo.LoadFromFont( THEME->GetPathF( "ScreenArcadeDiagnostics", "text" ) );
	USBInfo.SetName( "USBInfo" );

	// XXX: SOMEONE PLEASE FIX THE TEXT D= --infamouspat
	ActorUtil::SetXY( USBInfo, "ScreenArcadeDiagnostics" );
	USBInfo.SetZoom( 0.6f );
	USBInfo.SetText("USB Info: ");
	USBInfo.AddX(100.0f);
	USBInfo.AddY(25.0f);
	USBInfo.SetVisible(true);

	this->AddChild(&USBInfo);
	this->SortByDrawOrder();
}

ScreenArcadeDiagnostics::~ScreenArcadeDiagnostics()
{
	LOG->Trace( "ScreenArcadeDiagnostics::~ScreenArcadeDiagnostics()" );
}

void ScreenArcadeDiagnostics::Update( float fDeltaTime )
{
	ActorFrame::PlayCommand( "Refresh" ); // This updates our uptime.
	Screen::Update( fDeltaTime );

	vector<USBDevice> vDevList;
	CString sDispInfo = "USB Info:\n";
	GetUSBDeviceList(vDevList);

	if (vDevList.size() == 0)
	{
		USBInfo.SetText("No USB Devices");
		return;
	}

	for (unsigned i = 0; i < vDevList.size(); i++)
	{
		USBDevice nDevice = vDevList[i];
		sDispInfo += ssprintf("\n%s: %.04X:%.04X: %s (%dmA)", 
			nDevice.GetDeviceDir().c_str(),
			nDevice.GetIdVendor(), nDevice.GetIdProduct(),
			nDevice.GetDescription().c_str(), nDevice.GetMaxPower() );
	}
	USBInfo.SetText(sDispInfo);
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
