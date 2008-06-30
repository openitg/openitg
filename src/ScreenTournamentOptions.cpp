#include "global.h"
#include "RageLog.h"
#include "TournamentManager.h"
#include "ScreenManager.h"
#include "ScreenTextEntry.h"
#include "ScreenMiniMenu.h"
#include "ScreenTournamentOptions.h"
#include "PlayerNumber.h"

#define PREV_SCREEN		THEME->GetMetric( m_sName, "PrevScreen" )
#define NEXT_SCREEN		THEME->GetMetric( m_sName, "NextScreen" )

REGISTER_SCREEN_CLASS( ScreenTournamentOptions );

enum {
	PO_ADD_PLAYER,
	PO_REMOVE_PLAYER,
	PO_SET_QUALIFY_SONG,
	NUM_TOURNAMENT_OPTIONS_LINES,
};	

OptionRowDefinition g_TournamentOptionsLines[NUM_TOURNAMENT_OPTIONS_LINES] = {
	OptionRowDefinition( "Add competitor", true, "Press START" ),
	OptionRowDefinition( "Remove competitor", true ),
	OptionRowDefinition( "Set qualifier song", true ),
//	OptionRowDefinition( "", true ),
};

AutoScreenMessage( SM_DoneCreating );
AutoScreenMessage( SM_DoneDeleting );

ScreenTournamentOptions::ScreenTournamentOptions( CString sClassName ) : ScreenOptions( sClassName )
{
	LOG->Debug( "ScreenTournamentOptions::ScreenTournamentOptions()" );
}

void ScreenTournamentOptions::Init()
{
	ScreenOptions::Init();

	g_TournamentOptionsLines[PO_ADD_PLAYER].choices.clear();
	g_TournamentOptionsLines[PO_ADD_PLAYER].choices.push_back( "Press START" );
	
	// enable all lines for all players
	for( unsigned i = 0; i < NUM_TOURNAMENT_OPTIONS_LINES; i++ )
		FOREACH_PlayerNumber( pn )
			g_TournamentOptionsLines[i].m_vEnabledForPlayers.insert( pn );

	vector<OptionRowDefinition> vDefs( &g_TournamentOptionsLines[0], &g_TournamentOptionsLines[ARRAYSIZE(g_TournamentOptionsLines)] );
	vector<OptionRowHandler*> vHands( vDefs.size(), NULL );
	SetNavigation( NAV_THREE_KEY );
	InitMenu( INPUTMODE_SHARE_CURSOR, vDefs, vHands );
}

void ScreenTournamentOptions::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
}

void ScreenTournamentOptions::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_DoneCreating )
	{
		if( !ScreenTextEntry::s_bCancelledLast && ScreenTextEntry::s_sLastAnswer != "" )
		{
			CString sError;
			bool bResult = TOURNAMENT->RegisterCompetitor( ScreenTextEntry::s_sLastAnswer, "BLAH", sError );

			if( !bResult )
				SCREENMAN->SystemMessage( sError );
		}
	}
}

void ScreenTournamentOptions::MenuStart( PlayerNumber pn )
{
	switch( GetCurrentRow() )
	{
	case PO_ADD_PLAYER:
		SCREENMAN->TextEntry( SM_DoneCreating, "Enter the player's name", "", 12 );
		break;
	}	
}

void ScreenTournamentOptions::MenuBack( PlayerNumber pn )
{
}

void ScreenTournamentOptions::ImportOptions( int row, const vector<PlayerNumber> &vpns )
{
	
}

void ScreenTournamentOptions::ExportOptions( int row, const vector<PlayerNumber> &vpns )
{
}

void ScreenTournamentOptions::GoToPrevScreen()
{
	SCREENMAN->SetNewScreen( PREV_SCREEN );	
}

void ScreenTournamentOptions::GoToNextScreen()
{
	SCREENMAN->SetNewScreen( NEXT_SCREEN );
}
