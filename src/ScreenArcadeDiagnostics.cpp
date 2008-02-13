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

#include "ActorFrame.h" // We need this to call PlayCommand,Refresh and update the uptime.

REGISTER_SCREEN_CLASS( ScreenArcadeDiagnostics );

ScreenArcadeDiagnostics::ScreenArcadeDiagnostics( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadeDiagnostics::ScreenArcadeDiagnostics()" );
}

void ScreenArcadeDiagnostics::Init()
{
	ScreenWithMenuElements::Init();
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
