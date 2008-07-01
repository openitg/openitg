/* A singleton used to handle tournaments and stat-saving */

#ifndef TOURNAMENT_MANAGER_H
#define TOURNAMENT_MANAGER_H

#include "ProfileManager.h"
#include "GameConstantsAndTypes.h"
#include "Difficulty.h"
#include "StageStats.h"
#include "PlayerNumber.h"
#include <vector>

class Song;
class Steps;
struct lua_State;

const unsigned TOURNAMENT_MAX_PLAYERS = 256;

// a struct that holds the saved data of a tournament match;
// this is sort of a watered-down PlayerStageStats
struct TournamentMatch
{
	CString sPlayer[NUM_PLAYERS];
	int iSeedIndex[NUM_PLAYERS];

	// score data
	int iActualPoints[NUM_PLAYERS];
	int iPossiblePoints[NUM_PLAYERS];
	float fPercentPoints[NUM_PLAYERS];
	int iTapNoteScores[NUM_PLAYERS][NUM_TAP_NOTE_SCORES];
	int iHoldNoteScores[NUM_PLAYERS][NUM_HOLD_NOTE_SCORES];

	// general statistics
	CString sGroup, sTitle;
	CString sTimePlayed;
	CString sDifficulty;
	CString sRound; // the round of the tournament
	CString sStage; // the sta

	PlayerNumber winner;
};

// a player in the tournament format
struct Competitor
{
	CString sDisplayName, sHighScoreName;
	
	// i prefer keeping these unsigned so we don't
	// run into floating-point-related problems ~ Vyhd
	unsigned iSeedScorePossible;
	unsigned iSeedScoreActual;
	unsigned iSeedIndex; 
};

class TournamentManager
{
public:
	TournamentManager();
	~TournamentManager();

	// currently used data
	Competitor *m_pCurCompetitor[NUM_PLAYERS];
	TournamentMatch *m_pCurMatch;

	void SetMeterLimitLow( int iLow );
	void SetMeterLimitHigh( int iHigh );
	void SetDifficultyLimitLow( Difficulty dLow );
	void SetDifficultyLimitHigh( Difficulty dHigh );

	void StartMatch(); 	// initialise and save date, song, etc.
	void CancelMatch();	// delete the data and prepare for a new match	
	void FinishMatch( StageStats &stats ); // save end-of-game data
	
	bool IsTournamentMode();

	// some utilities to cleanly check against tournament limits
	void RemoveStepsOutsideLimits( vector<Steps*> &vpSteps );
	bool HasStepsInsideLimits( Song *pSong ) const;

	TournamentRound GetCurrentRound() { return m_Round; }
	unsigned GetNumCompetitors() { return m_pCompetitors.size(); }

	// vector search-and-find functions
	int FindIndexOfCompetitor( Competitor *cptr );
	Competitor *FindCompetitorByName( CString sName );

	// attempt to add a new, unique competitor
	bool RegisterCompetitor( CString sDisplayName, CString sHighScoreName, CString &sError );

	void Reset();
	void PushSelf( lua_State *L );

	// debugging
	void DumpMatches();
	void DumpCompetitors();
private:
	// limitations on the playable songs + steps
	int m_iMeterLimitLow, m_iMeterLimitHigh;
	Difficulty m_DifficultyLimitLow, m_DifficultyLimitHigh;

	// the current round the tournament is in
	TournamentRound m_Round;

	vector<TournamentMatch*> m_pMatches;
	vector<Competitor*> m_pCompetitors;
};

// global and accessible from anywhere in our program
extern TournamentManager*	TOURNAMENT;

#endif

/*
 * (c) 2003-2004 Chris Danford
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
