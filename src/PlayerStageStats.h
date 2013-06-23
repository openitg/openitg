/* PlayerStageStats - Contains statistics for one stage of play - either one song, or a whole course. */

#ifndef PlayerStageStats_H
#define PlayerStageStats_H

#include "Grade.h"
#include "RadarValues.h"
#include <map>
class Steps;
struct lua_State;

// OpenITG: added 11/13/08
enum ComboStatus {
        COMBSTAT_FFC = 0, // Full Fantastic Combo
        COMBSTAT_FEC, // Full Excellent Combo
        COMBSTAT_FGC, // Full Great Combo (if implemented in theme)
		COMBSTAT_NONE,
        NUM_COMBSTAT,
        COMBSTAT_INVALID
};

struct PlayerStageStats
{
	PlayerStageStats() { Init(); }
	void Init();

	void AddStats( const PlayerStageStats& other );		// accumulate

	static Grade GetGradeFromPercent( float fPercent );
	Grade GetGrade() const;
	float GetPercentDancePoints() const;
	float GetCurMaxPercentDancePoints() const;

	vector<Steps*>  vpPlayedSteps;
	vector<Steps*>  vpPossibleSteps;
	float	fAliveSeconds;		// how far into the music did they last before failing?  Updated by Gameplay, scaled by music rate.

	/* Set if the player actually failed at any point during the song.  This is always
	 * false in FAIL_OFF.  If recovery is enabled and two players are playing,
	 * this is only set if both players were failing at the same time. */
	bool	bFailed;

	/* This indicates whether the player bottomed out his bar/ran out of lives at some
	 * point during the song.  It's set in all fail modes. */
	bool	bFailedEarlier;
	bool	bGaveUp;	// exited gameplay by giving up
	int		iPossibleDancePoints;
	int		iCurPossibleDancePoints;
	int		iActualDancePoints;
	int		iPossibleGradePoints;
	int		iTapNoteScores[NUM_TAP_NOTE_SCORES];
	int		iHoldNoteScores[NUM_HOLD_NOTE_SCORES];
	int		iCurCombo;
	int		iMaxCombo;
	int		iCurMissCombo;
	int		iScore;
	int		iCurMaxScore;
	int		iMaxScore;
	int		iBonus;  // bonus to be added on screeneval
	RadarValues	radarPossible;	// filled in by ScreenGameplay on start of notes
	RadarValues radarActual;
	/* The number of songs played and passed, respectively. */
	int		iSongsPassed;
	int		iSongsPlayed;
	int		iTotalError;
	float	fLifeRemainingSeconds;	// used in survival

	// OpenITG: used for colorized life graphs
	bool	bFlag_FFC, bFlag_FEC, bFlag_FGC, bFlag_PulsateEnd;
	float	fFullFantasticComboBegin;
	float	fFullExcellentComboBegin;
	float	fFullGreatComboBegin;
	float	fPulsatingComboEnd;

	ComboStatus m_ComboStatus;

	// workout
	float	fCaloriesBurned;

	map<float,float>	fLifeRecord;
	void	SetLifeRecordAt( float fLife, float fStepsSecond, TapNoteScore note = TNS_NONE );
	void	GetLifeRecord( float *fLifeOut, int iNumSamples, float fStepsEndSecond ) const;
	float	GetLifeRecordAt( float fStepsSecond ) const;
	float	GetLifeRecordLerpAt( float fStepsSecond ) const;
	float	GetCurrentLife() const;

	struct Combo_t
	{
		/* Start and size of this combo, in the same scale as the combo list mapping and
		 * the life record. */
		float fStartSecond, fSizeSeconds;

		/* Combo size, in steps. */
		int cnt;

		/* Size of the combo that didn't come from this stage (rollover from the last song). 
		 * (This is a subset of cnt.) */
		int rollover;

		/* Get the size of the combo that came from this song. */
		int GetStageCnt() const { return cnt - rollover; }

		Combo_t(): fStartSecond(0), fSizeSeconds(0), cnt(0), rollover(0) { }
		bool IsZero() const { return fStartSecond < 0; }
	};
	vector<Combo_t> ComboList;
	float fFirstSecond;
	float fLastSecond;

	int		GetComboAtStartOfStage() const;
	bool	FullComboOfScore( TapNoteScore tnsAllGreaterOrEqual ) const;
	bool	FullCombo() const { return FullComboOfScore(TNS_GREAT); }
	bool	SingleDigitsOfScore( TapNoteScore tnsAllGreaterOrEqual ) const;
	bool	OneOfScore( TapNoteScore tnsAllGreaterOrEqual ) const;
	int		GetTotalTaps() const;
	float	GetPercentageOfTaps( TapNoteScore tns ) const;
	void	UpdateComboList( float fSecond, bool rollover );
	Combo_t GetMaxCombo() const;

	float GetSurvivalSeconds() { return fAliveSeconds + fLifeRemainingSeconds; }

	// Lua
	void PushSelf( lua_State *L );
};


#endif

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
