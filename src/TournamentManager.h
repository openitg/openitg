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

// a struct that holds the saved data of a single tournament stage;
// this is sort of a watered-down PlayerStageStats
struct TournamentStage
{
	// score data
	CString sModifiers[NUM_PLAYERS];
	int iActualPoints[NUM_PLAYERS];
	int iPossiblePoints[NUM_PLAYERS]; // this may be different due to insert mods
	float fPercentPoints[NUM_PLAYERS];
	int iTapNoteScores[NUM_PLAYERS][NUM_TAP_NOTE_SCORES];
	int iHoldNoteScores[NUM_PLAYERS][NUM_HOLD_NOTE_SCORES];

	// data on the song or course played
	bool bIsSong;
	CString sGroup, sTitle, sDifficulty, sTimePlayed;

	// corresponds to one seed index, or -1 for a tie
	int iWinner;

	// stage of the match this was played
	Stage stage;
};

// a struct that holds data for an entire match,
// allowing for as many stages as needed
struct TournamentMatch
{
	CString sPlayer[NUM_PLAYERS];
	int iSeedIndex[NUM_PLAYERS];

	// the round this match was held in
	CString sRound;

	// all stages played during this match
	std::vector<TournamentStage*> vStages;
};

// a player in the tournament format
struct Competitor
{
	Competitor(): sDisplayName(""),
	sHighScoreName(""), iSeedScorePossible(0),
	iSeedScoreActual(0), iSeedIndex(0)
	{ }

	CString sDisplayName, sHighScoreName;
	
	// i prefer keeping these unsigned so we don't
	// run into floating-point-related problems ~ Vyhd
	unsigned iSeedScorePossible;
	unsigned iSeedScoreActual;
	unsigned iSeedIndex;

	// holds data on every song played
	vector<Song*> vPlayedSongs;
	// holds data on every match played
	vector<TournamentMatch*> vPlayedMatches;
};

class TournamentManager
{
public:
	TournamentManager();
	~TournamentManager();

	void Init();

	Competitor *m_pCurCompetitor[NUM_PLAYERS];
	TournamentMatch *m_pCurMatch;
	TournamentStage *m_pCurStage;

	void SetMeterLimitLow( int iLow );
	void SetMeterLimitHigh( int iHigh );
	void SetDifficultyLimitLow( Difficulty dLow );
	void SetDifficultyLimitHigh( Difficulty dHigh );

	void StartMatch();
	void CancelMatch();
	void FinishMatch();

	void CancelStage();
	void FinishStage( StageStats &stats );
	
	void StartTournament() { m_bTournamentMode = true; }
	void EndTournament() { m_bTournamentMode = false; }

	bool IsTournamentMode() const { return m_bTournamentMode; }

	void GetCompetitorNames( vector<CString> &vsNames, bool bDisplayIndex = false );
	void GetCompetitorNamesAndIndexes( vector<CString> &vsNames ) { GetCompetitorNames( vsNames, true ); }

	// vector search-and-return functions
	int FindCompetitorIndex( Competitor *cptr );

	Competitor *GetCompetitorByName( CString sName );
	Competitor *GetCompetitorByIndex( unsigned i ) { return m_pCompetitors[i]; }

	bool CompetitorWithNameExists( CString sName );

	bool DeleteCompetitor( Competitor *cptr );
	bool DeleteCompetitorByIndex( unsigned i );

	// some utilities to cleanly check against tournament limits
	void RemoveStepsOutsideLimits( vector<Steps*> &vpSteps ) const;
	bool HasStepsInsideLimits( Song *pSong ) const;

	TournamentRound GetCurrentRound() const { return m_Round; }
	unsigned GetNumCompetitors() const { return m_pCompetitors.size(); }

	// attempt to add a new, unique competitor
	bool RegisterCompetitor( CString sDisplayName, CString sHighScoreName, unsigned iSeed, CString &sError );

	void Reset();
	void PushSelf( lua_State *L );

	// debugging
	void DumpMatches() const;
	void DumpCompetitors() const;
private:
	bool m_bTournamentMode;

	// conditions to win
	bool m_bChoosingPlayerLosesIfBothFail;
	PlayerNumber m_ChoosingPlayer;

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
