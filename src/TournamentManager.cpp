#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "TournamentManager.h"
#include "PrefsManager.h"
#include "ProfileManager.h"
#include "GameState.h"
#include "GameConstantsAndTypes.h"
#include "StageStats.h"
#include "Course.h"
#include "song.h"
#include "Steps.h"
#include "StepsUtil.h"
#include "DateTime.h"
#include "PlayerNumber.h"

TournamentManager* TOURNAMENT = NULL;

TournamentManager::TournamentManager()
{
	Init();
}

void TournamentManager::Init()
{
	m_bTournamentMode = false;

	m_iMeterLimitLow = -1;
	m_iMeterLimitHigh = INT_MAX;
	m_DifficultyLimitLow = DIFFICULTY_BEGINNER;
	m_DifficultyLimitHigh = DIFFICULTY_INVALID;
	m_pCurMatch = NULL;
	m_pCurStage = NULL;
	FOREACH_PlayerNumber( pn )
		m_pCurCompetitor[pn] = NULL;

	m_Round = ROUND_QUALIFIERS;
}

TournamentManager::~TournamentManager()
{
	DumpCompetitors();
	// de-allocate all matches and data
	Reset();
}

void TournamentManager::RemoveStepsOutsideLimits( vector<Steps*> &vpSteps ) const
{
	// don't apply limits if we don't need to
	if( !TOURNAMENT->IsTournamentMode() )
		return;

	ASSERT( m_iMeterLimitLow <= m_iMeterLimitHigh );
	ASSERT( m_DifficultyLimitLow <= m_DifficultyLimitHigh );

	StepsUtil::RemoveStepsOutsideMeterRange( vpSteps, m_iMeterLimitLow, m_iMeterLimitHigh );
	StepsUtil::RemoveStepsOutsideDifficultyRange( vpSteps, m_DifficultyLimitLow, m_DifficultyLimitHigh );
}

bool TournamentManager::HasStepsInsideLimits( Song *pSong ) const
{
	if( !(const TournamentManager *)TOURNAMENT->IsTournamentMode() )
		return true;

	ASSERT( m_iMeterLimitLow <= m_iMeterLimitHigh );
	ASSERT( m_DifficultyLimitLow <= m_DifficultyLimitHigh );

	return pSong->HasStepsWithinMeterAndDifficultyRange( m_iMeterLimitLow, m_iMeterLimitHigh, m_DifficultyLimitLow, m_DifficultyLimitHigh, STEPS_TYPE_DANCE_SINGLE );
}

void TournamentManager::GetCompetitorNames( vector<CString> &vsNames, bool bDisplayIndex )
{
	vsNames.clear();

	// "1: Player 1" vs. "Player 1"
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
	{
		CString sNewLine = bDisplayIndex ? ssprintf("%i: %s", i+1, m_pCompetitors[i]->sDisplayName.c_str()) : m_pCompetitors[i]->sDisplayName;
		LOG->Debug( "Adding line \"%s\" to vsNames.", sNewLine.c_str() );
		vsNames.push_back( sNewLine );
	}
}

int TournamentManager::FindCompetitorIndex( Competitor *cptr )
{
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
		if( &m_pCompetitors[i] == &cptr )
			return i;

	return -1;
}

Competitor *TournamentManager::GetCompetitorByName( CString sName )
{
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
	{
		if( m_pCompetitors[i]->sDisplayName.compare(sName) )
		{
			ssprintf("%s matches %s", m_pCompetitors[i]->sDisplayName.c_str(), sName.c_str() );
			return m_pCompetitors[i];
		}
	}

	return NULL;
}

void TournamentManager::Reset()
{
	for( unsigned i = 0; i < m_pMatches.size(); i++ )
	{
		// matches have pointers to stages - delete them first
		for( unsigned j = 0; j < m_pMatches[i]->vStages.size(); j++ )
			SAFE_DELETE( m_pMatches[i]->vStages[j] );

		SAFE_DELETE( m_pMatches[i] );
	}
	m_pMatches.clear();

	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
		SAFE_DELETE( m_pCompetitors[i] );
	m_pCompetitors.clear();
}

bool TournamentManager::RegisterCompetitor( CString sDisplayName, CString sHighScoreName, unsigned iSeed, CString &sError )
{
	if( !this->IsTournamentMode() )
		return false;

	Competitor *cptr = new Competitor;

	// default to Player xx if no display is given
	// do we need to re-enforce the display name length here?
	if( sDisplayName.empty() )
		cptr->sDisplayName = ssprintf( "Player %i", m_pCompetitors.size()+1 );

#if 0
	// make sure we don't have any naming conflicts
	Competitor *conflict = FindCompetitorByName(cptr->sDisplayName);

	if( conflict != NULL )
	{
		sError = ssprintf( "duplicate name." );
		delete cptr; return false;
	}
#endif

	cptr->sDisplayName = sDisplayName;
	cptr->iSeedIndex = iSeed;

	// do we need to re-enforce the high score name length here?
	cptr->sHighScoreName = sHighScoreName;

	m_pCompetitors.push_back( cptr );

	DumpCompetitors();

	return true;
}

void TournamentManager::StartMatch()
{
	if( !this->IsTournamentMode() )
		return;

	LOG->Debug( "TournamentManager::StartMatch()" );
	ASSERT( m_pCurMatch == NULL );
	ASSERT( m_pCurStage == NULL );

	TournamentMatch *match = new TournamentMatch;
	TournamentStage *stage = new TournamentStage;

	// fill in new match data
	match->sRound = TournamentRoundToString( m_Round );

	FOREACH_PlayerNumber( pn )
	{
		match->sPlayer[pn] = PROFILEMAN->GetPlayerName(pn);
//		match->iSeedIndex[pn] = m_pCurCompetitor[pn]->iSeedIndex;
	}

	// fill in new stage data
	stage->sTimePlayed = DateTime::GetNowDateTime().GetString();

	stage->bIsSong = GAMESTATE->m_PlayMode == PLAY_MODE_REGULAR || GAMESTATE->m_PlayMode == PLAY_MODE_RAVE;

	if( stage->bIsSong )
	{
		// fill in song data
		stage->sGroup = GAMESTATE->m_pCurSong->m_sGroupName;
		stage->sTitle = GAMESTATE->m_pCurSong->GetDisplayFullTitle();
		stage->sDifficulty = DifficultyToString(GAMESTATE->m_pCurSteps[0]->GetDifficulty());
	}
	else
	{
		// fill in course data
		stage->sGroup = GAMESTATE->m_pCurCourse->m_sGroupName;
		stage->sTitle = GAMESTATE->m_pCurCourse->GetDisplayFullTitle();
		stage->sDifficulty = CourseDifficultyToString(GAMESTATE->m_pCurTrail[0]->m_CourseDifficulty);
	}

	m_pCurMatch = match;
	m_pCurStage = stage;
}

// we don't want to save this data if the player backed out.
// delete the allocated data and re-set for a new stage.
void TournamentManager::CancelStage()
{
	if( m_pCurStage == NULL )
		return;

	LOG->Debug( "TournamentManager::CancelStage()" );

	SAFE_DELETE( m_pCurStage );
}

void TournamentManager::CancelMatch()
{
	if( m_pCurMatch == NULL )
		return;

	LOG->Debug( "TournamentManager::CancelMatch()" );

	// free the stages we've played in this match
	for( unsigned i = 0; i < m_pCurMatch->vStages.size(); i++ )
		SAFE_DELETE( m_pCurMatch->vStages[i] );

	SAFE_DELETE( m_pCurMatch );
}

// save all scores and game data from the stats given
void TournamentManager::FinishStage( StageStats &stats )
{
	if( !this->IsTournamentMode() )
		return;

	LOG->Debug( "TournamentManager::FinishStage()" );

	FOREACH_PlayerNumber( pn )
	{
		// fill in score data
		m_pCurStage->iActualPoints[pn] = stats.m_player[pn].iActualDancePoints;
		m_pCurStage->iPossiblePoints[pn] = stats.m_player[pn].iPossibleDancePoints;

		// truncate the rest to avoid rounding errors
		m_pCurStage->fPercentPoints[pn] = ftruncf( stats.m_player[pn].GetPercentDancePoints(), 0.0001 );

		FOREACH_TapNoteScore( tns )
			m_pCurStage->iTapNoteScores[pn][tns] = stats.m_player[pn].iTapNoteScores[tns];
		FOREACH_HoldNoteScore( hns )
			m_pCurStage->iHoldNoteScores[pn][hns] = stats.m_player[pn].iHoldNoteScores[hns];

		// neither player won
		if( !stats.OnePassed() )
			m_pCurStage->iWinner = -1;
	}

	m_pCurMatch->vStages.push_back( m_pCurStage );
	m_pCurStage = NULL;
}

void TournamentManager::DumpMatches() const
{
#if 0
	for( unsigned i = 0; i < m_pMatches.size(); i++ )
	{
		LOG->Debug( "Match %i: %s vs. %s, points %i vs. %i\non song %s in group %s\nPlayed on %s",
		i+1, m_pMatches[i]->sPlayer[PLAYER_1].c_str(), m_pMatches[i]->sPlayer[PLAYER_2].c_str(),
		m_pMatches[i]->iActualPoints[PLAYER_1], m_pMatches[i]->iActualPoints[PLAYER_2],
		m_pMatches[i]->sTitle.c_str(), m_pMatches[i]->sGroup.c_str(), m_pMatches[i]->sTimePlayed.c_str() );
	}
#endif
}

void TournamentManager::DumpCompetitors() const
{
	for( unsigned i = 0; i < m_pCompetitors.size(); i++ )
	{
		LOG->Debug( "Competitor %i: name %s, high-score name %s, seeded %i of %i", i+1, 
		m_pCompetitors[i]->sDisplayName.c_str(), m_pCompetitors[i]->sHighScoreName.c_str(),
		m_pCompetitors[i]->iSeedIndex, m_pCompetitors.size() );
	}
}

void TournamentManager::SetMeterLimitLow( int iLow )
{
	m_iMeterLimitLow = iLow;
	CLAMP( m_iMeterLimitLow, 0, m_iMeterLimitHigh );
	LOG->Debug( "m_iMeterLimitLow set to %i", m_iMeterLimitLow );
}

void TournamentManager::SetMeterLimitHigh( int iHigh )
{
	m_iMeterLimitHigh = iHigh;
	CLAMP( m_iMeterLimitHigh, m_iMeterLimitLow, INT_MAX );
	LOG->Debug( "m_iMeterLimitHigh set to %i", m_iMeterLimitHigh );
}

void TournamentManager::SetDifficultyLimitLow( Difficulty dLow )
{
	int iTemp = (int)dLow;
	CLAMP( iTemp, 0, (int)m_DifficultyLimitHigh );
	m_DifficultyLimitLow = (Difficulty)iTemp;
	LOG->Debug( "m_DifficultyLimitLow set to %s", DifficultyToString(m_DifficultyLimitLow).c_str() );
}

void TournamentManager::SetDifficultyLimitHigh( Difficulty dHigh )
{
	int iTemp = (int)dHigh;
	CLAMP( iTemp, (int)m_DifficultyLimitLow, INT_MAX );
	m_DifficultyLimitHigh = (Difficulty)iTemp;
	LOG->Debug( "m_DifficultyLimitHigh set to %s", DifficultyToString(m_DifficultyLimitHigh).c_str() );
}


#if 0
// lua start
#include "LuaBinding.h"

template<class T>
class LunaTournamentManager : public Luna<T>
{
public:
	LunaTournamentManager() { LUA->Register( Register ); }

	static int GetNumCompetitors( T* p, lua_State *L )	{ lua_pushnumber( L, p->GetNumCompetitors() ); return 1; }
	static int GetCurrentRound( T* p, lua_State *L )	{ lua_pushnumber( L, (int)p->GetCurrentRound() ); return 1; }
	static int SetMeterLimitLow( T* p, lua_State *L )	{ p->SetMeterLimitLow(IArg(1)); return 0; }
	static int SetMeterLimitHigh( T* p, lua_State *L )	{ p->SetMeterLimitHigh(IArg(1)); return 0; }
	static int SetDifficultyLimitLow( T* p, lua_State *L )	{ p->SetDifficultyLimitLow((Difficulty)IArg(1)); return 0; }
	static int SetDifficultyLimitHigh( T* p, lua_State *L )	{ p->SetDifficultyLimitHigh((Difficulty)IArg(1)); return 0; }
	static int DumpMatches( T* p, lua_State *L )		{ p->DumpMatches(); return 1; }
	static int DumpCompetitors( T* p, lua_State *L )	{ p->DumpCompetitors(); return 1; }

	static void Register( lua_State *L )
	{
		ADD_METHOD( GetNumCompetitors )
		ADD_METHOD( GetCurrentRound )
		ADD_METHOD( SetMeterLimitLow )
		ADD_METHOD( SetMeterLimitHigh )
		ADD_METHOD( SetDifficultyLimitLow )
		ADD_METHOD( SetDifficultyLimitHigh )
		ADD_METHOD( DumpMatches )
		ADD_METHOD( DumpCompetitors )

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
#endif
