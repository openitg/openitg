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

AutoScreenMessage( SM_BackFromRegisterMenu );

AutoScreenMessage( SM_SetDisplayName );
AutoScreenMessage( SM_SetScoreName );
AutoScreenMessage( SM_SetSeed );
AutoScreenMessage( SM_RegisterPlayer );

// globals to store current player data
CString g_sCurPlayerName = "";
CString g_sCurScoreName = "";
unsigned g_iCurSeedIndex = 0;
int g_CurPlayerIndex = -1;

enum TournamentOptionsLine
{
	PO_ADD_PLAYER,
	PO_REMOVE_PLAYER,
	PO_SET_QUALIFY_SONG,
	NUM_TOURNAMENT_OPTIONS_LINES,
};	

OptionRowDefinition g_TournamentOptionsLines[NUM_TOURNAMENT_OPTIONS_LINES] = {
	OptionRowDefinition( "AddPlayer", true, "Press START" ),
	OptionRowDefinition( "ModifyPlayer", true ),
	OptionRowDefinition( "SetQualifier", true )
};

enum RegisterMenuChoice
{
	set_player_name,
	set_highscore_name,
	set_seed_number,
	exit_and_register,
	exit_and_delete,
	exit_and_cancel,
	NUM_REGISTER_MENU_CHOICES
};

/* Row definitions. UGLY: MenuRow assumes Edit mode, but this isn't. We'll fix this up sometime...  */
MenuRow set_player_name_row	= 	MenuRow( set_player_name,	"Set player name",	true, EDIT_MODE_PRACTICE, 0 );
MenuRow set_highscore_name_row	=	MenuRow( set_highscore_name,	"Set highscore name",	true, EDIT_MODE_PRACTICE, 0 );
MenuRow set_seed_number_row	=	MenuRow( set_seed_number,	"Set seed (optional)",	true, EDIT_MODE_PRACTICE, 0 );
MenuRow exit_and_register_row	=	MenuRow( exit_and_register,	"Finish registration",	(!g_sCurPlayerName.empty() && !g_sCurScoreName.empty()), EDIT_MODE_PRACTICE, 0 );
MenuRow exit_and_delete_row	=	MenuRow( exit_and_delete,	"Delete registration",	true, EDIT_MODE_PRACTICE, 0 );
MenuRow exit_and_cancel_row	=	MenuRow( exit_and_cancel,	"Cancel registration",	true, EDIT_MODE_PRACTICE, 0 );

static Menu g_RegisterMenu
(
"ScreenMiniMenuRegisterMenu",
set_player_name_row,
set_highscore_name_row,
set_seed_number_row,
exit_and_register_row,
exit_and_cancel_row
);

// this is used to cancel registrations
static Menu g_RegisterModifyMenu
(
	"ScreenMiniMenuRegisterModifyMenu",
	set_player_name_row,
	set_highscore_name_row,
	set_seed_number_row,
	exit_and_register_row,
	exit_and_delete_row
);

// helper functions

void RefreshRegisterMenuText()
{
	g_RegisterMenu.rows[set_player_name].choices.resize(1);
	g_RegisterMenu.rows[set_player_name].choices[0] = g_sCurPlayerName;
	g_RegisterMenu.rows[set_highscore_name].choices.resize(1);
	g_RegisterMenu.rows[set_highscore_name].choices[0] = g_sCurScoreName;
	g_RegisterMenu.rows[set_seed_number].choices.resize(1);
	g_RegisterMenu.rows[set_seed_number].choices[0] = ssprintf( "%i", g_iCurSeedIndex );
}

void RefreshRegisterModifyMenuText()
{
}

void LoadCompetitor( Competitor *cptr )
{
	g_sCurPlayerName = cptr->sDisplayName;
	g_sCurScoreName = cptr->sHighScoreName;
	g_iCurSeedIndex = cptr->iSeedIndex;
	g_CurPlayerIndex = TOURNAMENT->FindCompetitorIndex( cptr );
}

void ResetPlayerData()
{
	g_sCurPlayerName = "";
	g_sCurScoreName = "";
	g_iCurSeedIndex = 0;
	g_CurPlayerIndex = -1;
}

ScreenTournamentOptions::ScreenTournamentOptions( CString sClassName ) : ScreenOptions( sClassName )
{
	LOG->Debug( "ScreenTournamentOptions::ScreenTournamentOptions()" );
}

void ScreenTournamentOptions::Init()
{
	ScreenOptions::Init();

	// enable all lines for all players
	for( unsigned i = 0; i < NUM_TOURNAMENT_OPTIONS_LINES; i++ )
		FOREACH_PlayerNumber( pn )
			g_TournamentOptionsLines[i].m_vEnabledForPlayers.insert( pn );

	g_TournamentOptionsLines[PO_ADD_PLAYER].choices.clear();
	g_TournamentOptionsLines[PO_ADD_PLAYER].choices.push_back( "Press START" );
	g_TournamentOptionsLines[PO_REMOVE_PLAYER].choices.clear();
	g_TournamentOptionsLines[PO_REMOVE_PLAYER].choices.push_back( "-NO ENTRIES-" );
	g_TournamentOptionsLines[PO_SET_QUALIFY_SONG].choices.clear();
	g_TournamentOptionsLines[PO_SET_QUALIFY_SONG].choices.push_back( "-NO ENTRIES-" );
	
	vector<OptionRowDefinition> vDefs( &g_TournamentOptionsLines[0], &g_TournamentOptionsLines[ARRAYSIZE(g_TournamentOptionsLines)] );
	vector<OptionRowHandler*> vHands( vDefs.size(), NULL );
	InitMenu( INPUTMODE_SHARE_CURSOR, vDefs, vHands );
}

void ScreenTournamentOptions::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	CHECKPOINT_M( "Input start" );
	ScreenOptions::Input( DeviceI, type, GameI, MenuI, StyleI );
	CHECKPOINT_M( "Input end" );
}

void ScreenTournamentOptions::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_BackFromRegisterMenu )
	{
		LOG->Debug( "Row code: %i", ScreenMiniMenu::s_iLastRowCode );
		switch( ScreenMiniMenu::s_iLastRowCode )
		{	
		case set_player_name:
			SCREENMAN->TextEntry( SM_SetDisplayName, ssprintf( "Player %u's name:", TOURNAMENT->GetNumCompetitors()+1), "", 12 );
			break;
		case set_highscore_name:
			SCREENMAN->TextEntry( SM_SetScoreName, ssprintf( "Highscore name for %s:", g_sCurPlayerName.c_str()),  "", 4 );
			break;
		case set_seed_number:
			SCREENMAN->TextEntry( SM_SetSeed, ssprintf( "Seed index for %s:", g_sCurPlayerName.c_str()), "", 3 );
			break;
		case exit_and_register:
			{
				CString sError;
				bool bRegistered = TOURNAMENT->RegisterCompetitor( g_sCurPlayerName, g_sCurScoreName, g_iCurSeedIndex, sError );

				if( bRegistered )
					ResetPlayerData(); // we're registered - reset and prepare for new data	
				else
					SCREENMAN->SystemMessage( ssprintf( "Registration error: %s", sError.c_str()) );
			}
			break;
		case exit_and_delete:
			{
				// TODO: implement a smart vector deletion function
			}
			break;
		case exit_and_cancel:
			ResetPlayerData();
			break;
		}
	}
	else if( SM == SM_SetDisplayName )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
			g_sCurPlayerName = ScreenTextEntry::s_sLastAnswer;
	}
	else if( SM == SM_SetScoreName )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
			g_sCurScoreName = ScreenTextEntry::s_sLastAnswer;
		g_sCurScoreName.MakeUpper();
	}
	else if( SM == SM_SetSeed )
	{
		if( !ScreenTextEntry::s_bCancelledLast )
			g_iCurSeedIndex = (int) strtof( ScreenTextEntry::s_sLastAnswer, NULL );
	}
	else if( SM == SM_GoToNextScreen || SM == SM_GoToPrevScreen )
	{
		SCREENMAN->SetNewScreen( PREV_SCREEN );
	}
}

void ScreenTournamentOptions::MenuStart( PlayerNumber pn, const InputEventType type )
{
	if( type != IET_FIRST_PRESS )
		return;

	switch( GetCurrentRow() )
	{
	case PO_ADD_PLAYER:
		{
			RefreshRegisterMenuText();
			SCREENMAN->MiniMenu( &g_RegisterMenu, SM_BackFromRegisterMenu );
		}
		break;
	default:
		ScreenOptions::MenuStart( pn, type );
	}
}

void ScreenTournamentOptions::MenuBack( PlayerNumber pn, const InputEventType type )
{
	TOURNAMENT->DumpCompetitors();
	if( !IsTransitioning() )
	{
		this->PlayCommand( "Off" );
		this->StartTransitioning( SM_GoToPrevScreen );
	}
}

void ScreenTournamentOptions::ImportOptions( int row, const vector<PlayerNumber> &vpns )
{
	LOG->Debug( "ScreenTournamentOptions::ImportOptions( %i, vpns )", row );

	switch( row )
	{
	case PO_REMOVE_PLAYER:
		{
			LOG->Debug( "Getting indexes and names." );
			TOURNAMENT->GetCompetitorNames( g_TournamentOptionsLines[PO_REMOVE_PLAYER].choices, true );

			for( unsigned i = 0; i < g_TournamentOptionsLines[PO_REMOVE_PLAYER].choices.size(); i++ )
				ssprintf( "Choice %i: %s", i+1, g_TournamentOptionsLines[PO_REMOVE_PLAYER].choices[i].c_str() );	
		}
		break;
	}			
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
