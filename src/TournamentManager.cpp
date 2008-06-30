#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "TournamentManager.h"
#include "ProfileManager.h"
#include "GameState.h" // XXX: needed for course data
#include "GameConstantsAndTypes.h"
#include "StageStats.h"
#include "Course.h"
#include "song.h"
#include "PlayerNumber.h"

TournamentManager* TOURNAMENT = NULL;

TournamentManager::TournamentManager()
{
	LOG->Debug( "TournamentManager::TournamentManager()" );

	m_iMeterLimitLow = 4;
	m_iMeterLimitHigh = 5;
}

TournamentManager::~TournamentManager()
{
	LOG->Debug( "TournamentManager::~TournamentManager()" );
}

bool TournamentManager::IsTournamentMode()
{
	LOG->Debug( "TournamentManager::IsTournamentMode()" );

	return true;
}

int TournamentManager::FindIndexOfCompetitor( Competitor *cptr )
{
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
		if( m_pCompetitors[i]->sDisplayName.compare(cptr->sDisplayName))
			return i;

	return -1;
}

Competitor *TournamentManager::FindCompetitorByName( CString sName )
{
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
		if( m_pCompetitors[i]->sDisplayName.compare(sName) )
			return m_pCompetitors[i];

	return NULL;
}

void TournamentManager::Reset()
{
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
		SAFE_DELETE( m_pCompetitors[i] );
	m_pCompetitors.clear();

	for( unsigned i = 0; i < m_pMatches.size(); i++ )
		SAFE_DELETE( m_pMatches[i] );
	m_pMatches.clear();
}

bool TournamentManager::RegisterCompetitor( CString sDisplayName, CString sHighScoreName, CString &sError )
{
	Competitor *cptr = new Competitor;

	// default to Player xx if no display is given
	// do we need to re-enforce the display name length here?
	if( sDisplayName.empty() )
		cptr->sDisplayName = ssprintf( "Player %i", m_pCompetitors.size()+1 );

	// make sure we don't have any naming conflicts
	Competitor *conflict = FindCompetitorByName(cptr->sDisplayName);

	if( &conflict != NULL )
	{
		sError = ssprintf( "same name as player %i", FindIndexOfCompetitor(conflict)+1 );
		delete cptr; return false;
	}

	cptr->sDisplayName = sDisplayName;

	// do we need to re-enforce the high score name length here?
	cptr->sHighScoreName = sHighScoreName;
	m_pCompetitors.push_back( cptr );

	return true;
}

void TournamentManager::RecordMatch( StageStats &stats )
{
	TournamentMatch *match = new TournamentMatch;

	FOREACH_PlayerNumber( pn )
	{
		match->sPlayer[pn] = PROFILEMAN->GetPlayerName(pn);
		match->iPlayerIndex[pn] = 0; // will make this actually work later

		// fill in score data
		match->iActualDancePoints[pn] = stats.m_player[pn].iActualDancePoints;
		match->iPossibleDancePoints[pn] = stats.m_player[pn].iPossibleDancePoints;
		
		FOREACH_TapNoteScore( tns )
			match->iTapNoteScores[pn][tns] = stats.m_player[pn].iTapNoteScores[tns];
		FOREACH_HoldNoteScore( hns )
			match->iHoldNoteScores[pn][hns] = stats.m_player[pn].iHoldNoteScores[hns];

		if( stats.playMode == PLAY_MODE_REGULAR || stats.playMode == PLAY_MODE_RAVE )
		{
			// fill in song data
			match->sGroup = stats.vpPlayedSongs[0]->m_sGroupName;
			match->sTitle = stats.vpPlayedSongs[0]->GetDisplayFullTitle();
		}
		else
		{
			// fill in course data
			match->sGroup = GAMESTATE->m_pCurCourse->m_sGroupName;
			match->sGroup = GAMESTATE->m_pCurCourse->GetDisplayFullTitle();
		}

		// truncate the rest to avoid rounding errors
		float fPercentP1 = ftruncf( stats.m_player[PLAYER_1].GetPercentDancePoints(), 0.0001 );
		float fPercentP2 = ftruncf( stats.m_player[PLAYER_2].GetPercentDancePoints(), 0.0001 );

		// neither player won
		if( !stats.OnePassed() )
			match->winner = PLAYER_INVALID;

		
	}
}
