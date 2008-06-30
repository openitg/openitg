/* A singleton used to handle tournaments and stat-saving */

#ifndef TOURNAMENT_MANAGER_H
#define TOURNAMENT_MANAGER_H

#include "ProfileManager.h"
#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"

const unsigned TOURNAMENT_MAX_PLAYERS = 256;

// a struct that holds the saved data of a tournament match
// this is sort of a watered-down PlayerStageStats
struct TournamentMatch
{
	CString sPlayer[NUM_PLAYERS];

	// score data
	int iActualDancePoints[NUM_PLAYERS];
	int iPossibleDancePoints[NUM_PLAYERS];
	int iTapNoteScores[NUM_TAP_NOTE_SCORES][NUM_PLAYERS];
	int iHoldNoteScores[NUM_HOLD_NOTE_SCORES][NUM_PLAYERS];
};

class TournamentManager
{
public:
	TournamentManager();
	~TournamentManager();

	bool IsTournamentMode();
private:
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
