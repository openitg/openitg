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
#include "StepsUtil.h"
#include "DateTime.h"
#include "PlayerNumber.h"

TournamentManager* TOURNAMENT = NULL;

TournamentManager::TournamentManager()
{
	LOG->Debug( "TournamentManager::TournamentManager()" );

	m_iMeterLimitLow = m_iMeterLimitHigh = -1;
	m_DifficultyLimitLow = m_DifficultyLimitHigh = DIFFICULTY_INVALID;
	m_Round = ROUND_QUALIFIERS;
}

TournamentManager::~TournamentManager()
{
	LOG->Debug( "TournamentManager::~TournamentManager()" );
}

bool TournamentManager::IsTournamentMode()
{
	return true;
}

void TournamentManager::RemoveStepsOutsideLimits( vector<Steps*> &vpSteps )
{
	CHECKPOINT_M( "RemoveSteps" );
	// don't apply limits if we don't need to
	if( !this->IsTournamentMode() )
		return;

	ASSERT( m_iMeterLimitLow <= m_iMeterLimitHigh );
	ASSERT( m_DifficultyLimitLow <= m_DifficultyLimitHigh );

//	StepsUtil::RemoveStepsOutsideMeterRange( vpSteps, m_iMeterLimitLow, m_iMeterLimitHigh );
	StepsUtil::RemoveStepsOutsideDifficultyRange( vpSteps, m_DifficultyLimitLow, m_DifficultyLimitHigh );
	CHECKPOINT_M( "~RemoveSteps" );
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
	LOG->Debug( "TournamentManager::RecordMatch()" );
	TournamentMatch *match = new TournamentMatch;

	match->sTimePlayed = DateTime::GetNowDateTime().GetString();
	match->sRound = TournamentRoundToString( m_Round );

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

		// truncate the rest to avoid rounding errors
		float fPercentP1 = ftruncf( stats.m_player[PLAYER_1].GetPercentDancePoints(), 0.0001 );
		float fPercentP2 = ftruncf( stats.m_player[PLAYER_2].GetPercentDancePoints(), 0.0001 );

		// neither player won
		if( !stats.OnePassed() )
			match->winner = PLAYER_INVALID;
	}

	m_pMatches.push_back( match );
	m_Round = (TournamentRound)((int)m_Round + 1);
}

void TournamentManager::DumpMatches()
{
	for( unsigned i = 0; i < m_pMatches.size(); i++ )
	{
		LOG->Debug( "Match %i: %s vs. %s, points %i vs. %i\non song %s in group %s\nPlayed on %s",
		i+1, m_pMatches[i]->sPlayer[PLAYER_1].c_str(), m_pMatches[i]->sPlayer[PLAYER_2].c_str(),
		m_pMatches[i]->iActualDancePoints[PLAYER_1], m_pMatches[i]->iActualDancePoints[PLAYER_2],
		m_pMatches[i]->sTitle.c_str(), m_pMatches[i]->sGroup.c_str(), m_pMatches[i]->sTimePlayed.c_str() );
	}
}

void TournamentManager::DumpCompetitors()
{
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
	{
		LOG->Debug( "Competitor %i: name %s, high-score name %s, seeded %i of %i", i+1, 
		m_pCompetitors[i]->sDisplayName.c_str(), m_pCompetitors[i]->sHighScoreName.c_str(),
		m_pCompetitors[i]->iSeedIndex, m_pCompetitors.size() );
	}
}

// lua start
#include "LuaBinding.h"

template<class T>
class LunaTournamentManager : public Luna<T>
{
public:
	LunaTournamentManager() { LUA->Register( Register ); }

	static int GetNumCompetitors( T* p, lua_State *L )	{ lua_pushnumber( L, p->GetNumCompetitors() ); return 1; }
	static int GetRound( T* p, lua_State *L )		{ lua_pushnumber( L, (int)p->GetRound() ); return 1; }
	static int SetMeterLimitLow( T* p, lua_State *L )	{ p->m_iMeterLimitLow = IArg(1); return 0; }
	static int SetMeterLimitHigh( T* p, lua_State *L )	{ p->m_iMeterLimitHigh = IArg(1); return 0; }
	static int SetDifficultyLimitLow( T* p, lua_State *L )	{ p->m_DifficultyLimitLow = (Difficulty)IArg(1); return 0; }
	static int SetDifficultyLimitHigh( T* p, lua_State *L )	{ p->m_DifficultyLimitHigh = (Difficulty)IArg(1); return 0; }
	static int DumpMatches( T* p, lua_State *L )		{ p->DumpMatches(); return 1; }
	static int DumpCompetitors( T* p, lua_State *L )	{ p->DumpCompetitors(); return 1; }

	static void Register( lua_State *L )
	{
		ADD_METHOD( GetNumCompetitors )
		ADD_METHOD( GetRound )
		ADD_METHOD( SetMeterLimitLow )
		ADD_METHOD( SetMeterLimitHigh )
		ADD_METHOD( SetDifficultyLimitLow )
		ADD_METHOD( SetDifficultyLimitHigh )
		ADD_METHOD( DumpMatches )

		Luna<T>::Register( L );

		if( TOURNAMENT )
		{
			lua_pushstring(L, "TOURNAMENT");
			TOURNAMENT->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX );
		}
	}
};

LUA_REGISTER_CLASS( TournamentManager )
