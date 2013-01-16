#include "global.h"
#include "PlayerStageStats.h"
#include "RageLog.h"
#include "ThemeManager.h"
#include "Foreach.h"
#include "LuaFunctions.h"
#include <float.h>
#include "GameState.h"
#include "Course.h"
#include "ScoreKeeperMAX2.h"

#define GRADE_PERCENT_TIER(i)			THEME->GetMetricF("PlayerStageStats",ssprintf("GradePercent%s",GradeToString((Grade)i).c_str()))
#define GRADE_TIER02_IS_ALL_PERFECTS	THEME->GetMetricB("PlayerStageStats","GradeTier02IsAllPerfects")

void PlayerStageStats::Init()
{
	vpPlayedSteps.clear();
	vpPossibleSteps.clear();
	fAliveSeconds = 0;
	bFailed = false;
	bFailedEarlier = false;
	bGaveUp = false;

	bFlag_FFC = true; // you start off every song with an FFC
	bFlag_FEC = false;
	bFlag_FGC = false;
	bFlag_PulsateEnd = false;

	fFullFantasticComboBegin = 0.0f;
	fFullExcellentComboBegin = -1.0f;
	fFullGreatComboBegin = -1.0f;
	fPulsatingComboEnd = -1.0f;

	m_ComboStatus = COMBSTAT_FFC;

	iPossibleDancePoints = iCurPossibleDancePoints = iActualDancePoints = 0;
	iPossibleGradePoints = 0;
	iCurCombo = iMaxCombo = iCurMissCombo = iScore = iBonus = iMaxScore = iCurMaxScore = 0;
	iSongsPassed = iSongsPlayed = 0;
	iTotalError = 0;
	fCaloriesBurned = 0;
	iTotalError = 0;
	fLifeRemainingSeconds = 0;

	ZERO( iTapNoteScores );
	ZERO( iHoldNoteScores );
	radarPossible.Zero();
	radarActual.Zero();

	fFirstSecond = FLT_MAX;
	fLastSecond = 0;
}

void PlayerStageStats::AddStats( const PlayerStageStats& other )
{
	FOREACH_CONST( Steps*, other.vpPlayedSteps, s )
		vpPlayedSteps.push_back( *s );
	FOREACH_CONST( Steps*, other.vpPossibleSteps, s )
		vpPossibleSteps.push_back( *s );
	fAliveSeconds += other.fAliveSeconds;
	bFailed |= other.bFailed;
	bFailedEarlier |= other.bFailedEarlier;
	bGaveUp |= other.bGaveUp;
	iPossibleDancePoints += other.iPossibleDancePoints;
	iActualDancePoints += other.iActualDancePoints;
	iCurPossibleDancePoints += other.iCurPossibleDancePoints;
	iPossibleGradePoints += other.iPossibleGradePoints;
	
/*
	bFlag_FFC |= other.bFlag_FFC;
	bFlag_FEC |= other.bFlag_FEC;
	bFlag_FGC |= other.bFlag_FGC;
	bFlag_PulsateEnd |= other.bFlag_PulsateEnd;

	fFullFantasticComboBegin = other.fFullFantasticComboBegin;
	fFullExcellentComboBegin = other.fFullExcellentComboBegin;
	fFullGreatComboBegin = other.fFullGreatComboBegin;
	fPulsatingComboEnd = other.fPulsatingComboEnd;
*/

	if (GAMESTATE->IsCourseMode()) m_ComboStatus = other.m_ComboStatus;

	for( int t=0; t<NUM_TAP_NOTE_SCORES; t++ )
		iTapNoteScores[t] += other.iTapNoteScores[t];
	for( int h=0; h<NUM_HOLD_NOTE_SCORES; h++ )
		iHoldNoteScores[h] += other.iHoldNoteScores[h];
	iCurCombo += other.iCurCombo;
	iMaxCombo += other.iMaxCombo;
	iCurMissCombo += other.iCurMissCombo;
	iScore += other.iScore;
	iMaxScore += other.iMaxScore;
	iCurMaxScore += other.iCurMaxScore;
	radarPossible += other.radarPossible;
	radarActual += other.radarActual;
	iSongsPassed += other.iSongsPassed;
	iSongsPlayed += other.iSongsPlayed;
	iTotalError += other.iTotalError;
	fCaloriesBurned += other.fCaloriesBurned;
	fLifeRemainingSeconds = other.fLifeRemainingSeconds;	// don't accumulate

	const float fOtherFirstSecond = other.fFirstSecond + fLastSecond;
	const float fOtherLastSecond = other.fLastSecond + fLastSecond;
	fLastSecond = fOtherLastSecond;

	map<float,float>::const_iterator it;
	for( it = other.fLifeRecord.begin(); it != other.fLifeRecord.end(); ++it )
	{
		const float pos = it->first;
		const float life = it->second;
		fLifeRecord[fOtherFirstSecond+pos] = life;
	}

	for( unsigned i=0; i<other.ComboList.size(); ++i )
	{
		const Combo_t &combo = other.ComboList[i];

		Combo_t newcombo(combo);
		newcombo.fStartSecond += fOtherFirstSecond;
		ComboList.push_back( newcombo );
	}

	/* Merge identical combos.  This normally only happens in course mode, when a combo
	 * continues between songs. */
	for( unsigned i=1; i<ComboList.size(); ++i )
	{
		Combo_t &prevcombo = ComboList[i-1];
		Combo_t &combo = ComboList[i];
		const float PrevComboEnd = prevcombo.fStartSecond + prevcombo.fSizeSeconds;
		const float ThisComboStart = combo.fStartSecond;
		if( fabsf(PrevComboEnd - ThisComboStart) > 0.001 )
			continue;

		/* These are really the same combo. */
		prevcombo.fSizeSeconds += combo.fSizeSeconds;
		prevcombo.cnt += combo.cnt;
		ComboList.erase( ComboList.begin()+i );
		--i;
	}
}

Grade PlayerStageStats::GetGradeFromPercent( float fPercent )
{
	Grade grade = GRADE_FAILED;

	FOREACH_Grade(g)
	{
		if( fPercent >= GRADE_PERCENT_TIER(g) )
		{
			grade = g;
			break;
		}
	}
	return grade;
}

Grade PlayerStageStats::GetGrade() const
{
	if( bFailedEarlier )
		return GRADE_FAILED;

	/* XXX: This entire calculation should be in ScoreKeeper, but final evaluation
	 * is tricky since at that point the ScoreKeepers no longer exist. */
	float fActual = 0;

	bool bIsBeginner = false;
	if( vpPlayedSteps.size() && !GAMESTATE->IsCourseMode() )
		bIsBeginner = vpPlayedSteps[0]->GetDifficulty() == DIFFICULTY_BEGINNER;

	FOREACH_TapNoteScore( tns )
	{
		int iTapScoreValue = ScoreKeeperMAX2::TapNoteScoreToGradePoints( tns, bIsBeginner );
		fActual += iTapNoteScores[tns] * iTapScoreValue;
		LOG->Trace( "GetGrade actual: %i * %i", iTapNoteScores[tns], iTapScoreValue );
	}

	FOREACH_HoldNoteScore( hns )
	{
		int iHoldScoreValue = ScoreKeeperMAX2::HoldNoteScoreToGradePoints( hns, bIsBeginner );
		fActual += iHoldNoteScores[hns] * iHoldScoreValue;
		LOG->Trace( "GetGrade actual: %i * %i", iHoldNoteScores[hns], iHoldScoreValue );
	}

	LOG->Trace( "GetGrade: fActual: %f, fPossible: %d", fActual, iPossibleGradePoints );


	float fPercent = (iPossibleGradePoints == 0) ? 0 : fActual / iPossibleGradePoints;

	Grade grade = GetGradeFromPercent( fPercent );

	LOG->Trace( "GetGrade: Grade: %s, %i", GradeToString(grade).c_str(), GRADE_TIER02_IS_ALL_PERFECTS );
	if( GRADE_TIER02_IS_ALL_PERFECTS )
	{
		if(	iTapNoteScores[TNS_MARVELOUS] > 0 &&
			iTapNoteScores[TNS_PERFECT] == 0 &&
			iTapNoteScores[TNS_GREAT] == 0 &&
			iTapNoteScores[TNS_GOOD] == 0 &&
			iTapNoteScores[TNS_BOO] == 0 &&
			iTapNoteScores[TNS_MISS] == 0 &&
			iTapNoteScores[TNS_HIT_MINE] == 0 &&
			iHoldNoteScores[HNS_NG] == 0 )
			return GRADE_TIER01;

		if( iTapNoteScores[TNS_PERFECT] > 0 &&
			iTapNoteScores[TNS_GREAT] == 0 &&
			iTapNoteScores[TNS_GOOD] == 0 &&
			iTapNoteScores[TNS_BOO] == 0 &&
			iTapNoteScores[TNS_MISS] == 0 &&
			iTapNoteScores[TNS_HIT_MINE] == 0 &&
			iHoldNoteScores[HNS_NG] == 0 )
			return GRADE_TIER02;

		return max( grade, GRADE_TIER03 );
	}

	return grade;
}

float PlayerStageStats::GetPercentDancePoints() const
{
	if( iPossibleDancePoints == 0 )
		return 0; // div/0

	if( iActualDancePoints == iPossibleDancePoints )
		return 1;	// correct for rounding error

	/* This can happen in battle, with transform attacks. */
	//ASSERT_M( iActualDancePoints <= iPossibleDancePoints, ssprintf("%i/%i", iActualDancePoints, iPossibleDancePoints) );

	float fPercentDancePoints =  iActualDancePoints / (float)iPossibleDancePoints;
	
	return fPercentDancePoints;
}

float PlayerStageStats::GetCurMaxPercentDancePoints() const
{
	if ( iPossibleDancePoints == 0 )
		return 0; // div/0

	if ( iCurPossibleDancePoints == iPossibleDancePoints )
		return 1; // correct for rounding error

	float fCurMaxPercentDancePoints = iCurPossibleDancePoints / (float)iPossibleDancePoints;

	return fCurMaxPercentDancePoints;
}

void PlayerStageStats::SetLifeRecordAt( float fLife, float fStepsSecond, TapNoteScore note )
{
	// Don't save life stats in endless courses, or could run OOM in a few hours.
	if( GAMESTATE->m_pCurCourse && GAMESTATE->m_pCurCourse->IsEndless() )
		return;
	
	if( fStepsSecond < 0 )
		return;

	fFirstSecond = min( fStepsSecond, fFirstSecond );
	fLastSecond = max( fStepsSecond, fLastSecond );
	//LOG->Trace( "fLastSecond = %f", fLastSecond );

	// fSecond will always be greater than any value already in the map.
	fLifeRecord[fStepsSecond] = fLife;

	// ColorizedLifeGraph: moved 5/16/09
	switch( note )
	{
	case TNS_PERFECT:
		if ( m_ComboStatus == COMBSTAT_FFC )
		{
			fFullExcellentComboBegin = fStepsSecond;
			bFlag_FEC = true;
			m_ComboStatus = COMBSTAT_FEC;
		}
		break;
	case TNS_GREAT:
		if ( m_ComboStatus < COMBSTAT_FGC )
		{
			fFullGreatComboBegin = fStepsSecond;
			bFlag_FGC = true;
			m_ComboStatus = COMBSTAT_FGC;
		}
		break;
	case TNS_GOOD:
	case TNS_BOO:
	case TNS_MISS:
		if ( m_ComboStatus < COMBSTAT_NONE )
		{
			bFlag_PulsateEnd = true;
			fPulsatingComboEnd = fStepsSecond;
			m_ComboStatus = COMBSTAT_NONE;
		}
		break;
	}

	//
	// Memory optimization:
	// If we have three consecutive records A, B, and C all with the same fLife,
	// we can eliminate record B without losing data.  Only check the last three 
	// records in the map since we're only inserting at the end, and we know all 
	// earlier redundant records have already been removed.
	//
	map<float,float>::iterator C = fLifeRecord.end();
	--C;
	if( C == fLifeRecord.begin() ) // no earlier records left
		return;

	map<float,float>::iterator B = C;
	--B;
	if( B == fLifeRecord.begin() ) // no earlier records left
		return;

	map<float,float>::iterator A = B;
	--A;

	if( A->second == B->second && B->second == C->second )
		fLifeRecord.erase(B);
}

float PlayerStageStats::GetLifeRecordAt( float fStepsSecond ) const
{
	if( fLifeRecord.empty() )
		return 0;
	
	/* Find the first element whose key is greater than k. */
	map<float,float>::const_iterator it = fLifeRecord.upper_bound( fStepsSecond );

	/* Find the last element whose key is less than or equal to k. */
	if( it != fLifeRecord.begin() )
		--it;

	return it->second;

}

float PlayerStageStats::GetLifeRecordLerpAt( float fStepsSecond ) const
{
	if( fLifeRecord.empty() )
		return 0;
	
	/* Find the first element whose key is greater than k. */
	map<float,float>::const_iterator later = fLifeRecord.upper_bound( fStepsSecond );

	/* Find the last element whose key is less than or equal to k. */
	map<float,float>::const_iterator earlier = later;
	if( earlier != fLifeRecord.begin() )
		--earlier;

	if( later == fLifeRecord.end() )
		return earlier->second;

	if( earlier->first == later->first )	// two samples from the same time.  Don't divide by zero in SCALE
		return earlier->second;

	/* earlier <= pos <= later */
	return SCALE( fStepsSecond, earlier->first, later->first, earlier->second, later->second );
}


void PlayerStageStats::GetLifeRecord( float *fLifeOut, int iNumSamples, float fStepsEndSecond ) const
{
	for( int i = 0; i < iNumSamples; ++i )
	{
		float from = SCALE( i, 0, (float)iNumSamples, 0.0f, fStepsEndSecond );
		fLifeOut[i] = GetLifeRecordLerpAt( from );
	}
}

float PlayerStageStats::GetCurrentLife() const
{
	if( fLifeRecord.empty() )
		return 0;
	map<float,float>::const_iterator iter = fLifeRecord.end();
	--iter; 
	return iter->second;
}

/* If "rollover" is true, we're being called before gameplay begins, so we can record
 * the amount of the first combo that comes from the previous song. */
void PlayerStageStats::UpdateComboList( float fSecond, bool rollover )
{
	// Don't save combo stats in endless courses, or could run OOM in a few hours.
	if( GAMESTATE->m_pCurCourse && GAMESTATE->m_pCurCourse->IsEndless() )
		return;
	
	if( fSecond < 0 )
		return;

	if( !rollover )
	{
		fFirstSecond = min( fSecond, fFirstSecond );
		fLastSecond = max( fSecond, fLastSecond );
		//LOG->Trace( "fLastSecond = %f", fLastSecond );
	}

	int cnt = iCurCombo;
	if( !cnt )
		return; /* no combo */

	if( ComboList.size() == 0 || ComboList.back().cnt >= cnt )
	{
		/* If the previous combo (if any) starts on -9999, then we rolled over some
		 * combo, but missed the first step.  Remove it. */
		if( ComboList.size() && ComboList.back().fStartSecond == -9999 )
			ComboList.erase( ComboList.begin()+ComboList.size()-1, ComboList.end() );

		/* This is a new combo. */
		Combo_t NewCombo;
		/* "start" is the position that the combo started within this song.  If we're
		 * recording rollover, the combo hasn't started yet (within this song), so put
		 * a placeholder in and set it on the next call.  (Otherwise, start will be less
		 * than fFirstPos.) */
		if( rollover )
			NewCombo.fStartSecond = -9999;
		else
			NewCombo.fStartSecond = fSecond;
		ComboList.push_back( NewCombo );
	}

	Combo_t &combo = ComboList.back();
	combo.fSizeSeconds = fSecond - combo.fStartSecond;
	combo.cnt = cnt;
	if( !rollover && combo.fStartSecond == -9999 )
		combo.fStartSecond = fSecond;

	if( rollover )
		combo.rollover = cnt;
}

/* This returns the largest combo contained within the song, as if
 * m_bComboContinuesBetweenSongs is turned off. */
PlayerStageStats::Combo_t PlayerStageStats::GetMaxCombo() const
{
	if( ComboList.size() == 0 )
		return Combo_t();

	int m = 0;
	for( unsigned i = 1; i < ComboList.size(); ++i )
	{
		if( ComboList[i].cnt > ComboList[m].cnt )
			m = i;
	}

	return ComboList[m];
}

int	PlayerStageStats::GetComboAtStartOfStage() const
{
	if( ComboList.empty() )
		return 0;
	else
		return ComboList[0].rollover;
}

bool PlayerStageStats::FullComboOfScore( TapNoteScore tnsAllGreaterOrEqual ) const
{
	ASSERT( tnsAllGreaterOrEqual >= TNS_GREAT );
	ASSERT( tnsAllGreaterOrEqual <= TNS_MARVELOUS );

	// If missed any holds, then it's not a full combo
	if( iHoldNoteScores[HNS_NG] > 0 )
		return false;

	// If has any of the judgments below, then not a full combo
	for( int i=TNS_MISS; i<tnsAllGreaterOrEqual; i++ )
	{
		if( iTapNoteScores[i] > 0 )
			return false;
	}

	// If has at least one of the judgments equal to or above, then is a full combo.
	for( int i=tnsAllGreaterOrEqual; i<NUM_TAP_NOTE_SCORES; i++ )
	{
		if( iTapNoteScores[i] > 0 )
			return true;
	}

	return false;
}

bool PlayerStageStats::SingleDigitsOfScore( TapNoteScore tnsAllGreaterOrEqual ) const
{
	return FullComboOfScore( tnsAllGreaterOrEqual ) &&
		iTapNoteScores[tnsAllGreaterOrEqual] < 10;
}

bool PlayerStageStats::OneOfScore( TapNoteScore tnsAllGreaterOrEqual ) const
{
	return FullComboOfScore( tnsAllGreaterOrEqual ) &&
		iTapNoteScores[tnsAllGreaterOrEqual] == 1;
}

int PlayerStageStats::GetTotalTaps() const
{
	int iTotalTaps = 0;
	for( int i=TNS_MISS; i<NUM_TAP_NOTE_SCORES; i++ )
	{
		iTotalTaps += iTapNoteScores[i];
	}
	return iTotalTaps;
}

float PlayerStageStats::GetPercentageOfTaps( TapNoteScore tns ) const
{
	int iTotalTaps = 0;
	for( int i=TNS_MISS; i<NUM_TAP_NOTE_SCORES; i++ )
	{
		iTotalTaps += iTapNoteScores[i];
	}
	return iTapNoteScores[tns] / (float)iTotalTaps;
}


LuaFunction( GetGradeFromPercent,	PlayerStageStats::GetGradeFromPercent( FArg(1) ) )


#include "LuaBinding.h"

template<class T>
class LunaPlayerStageStats: public Luna<T>
{
public:
	LunaPlayerStageStats() { LUA->Register( Register ); }

	static int GetCaloriesBurned( T* p, lua_State *L )			{ lua_pushnumber(L, p->fCaloriesBurned ); return 1; }
	static int GetLifeRemainingSeconds( T* p, lua_State *L )	{ lua_pushnumber(L, p->fLifeRemainingSeconds); return 1; }
	static int GetSurvivalSeconds( T* p, lua_State *L )			{ lua_pushnumber(L, p->GetSurvivalSeconds()); return 1; }
	static int FullCombo( T* p, lua_State *L )					{ lua_pushnumber(L, p->FullCombo()); return 1; }
	static int MaxCombo( T* p, lua_State *L )					{ lua_pushnumber(L, p->GetMaxCombo().cnt); return 1; }
	static int GetCurrentCombo( T* p, lua_State *L )			{ lua_pushnumber(L, p->iCurCombo); return 1; }
	static int GetGrade( T* p, lua_State *L )					{ lua_pushnumber(L, p->GetGrade()); return 1; }
	static int GetScore( T* p, lua_State *L )					{ lua_pushnumber(L, p->iScore); return 1; }
	static int GetTapNoteScores( T* p, lua_State *L )			{ lua_pushnumber(L, p->iTapNoteScores[(TapNoteScore)IArg(1)]); return 1; }
	static int GetHoldNoteScores( T* p, lua_State *L )			{ lua_pushnumber(L, p->iHoldNoteScores[(HoldNoteScore)IArg(1)]); return 1; }
	static int GetCurrentLife( T* p, lua_State *L )				{ lua_pushnumber(L, p->GetCurrentLife()); return 1; }
	static int GetActualDancePoints( T* p, lua_State *L )		{ lua_pushnumber(L, p->iActualDancePoints); return 1; }
	static int GetPossibleDancePoints( T* p, lua_State *L )		{ lua_pushnumber(L, p->iPossibleDancePoints); return 1; }
	static int GetPercentDancePoints( T* p, lua_State *L )		{ lua_pushnumber(L, p->GetPercentDancePoints()); return 1; }

	static int GetPlayedSteps( T* p, lua_State *L )
	{
		lua_newtable(L);
		for( int i = 0; i < (int) p->vpPlayedSteps.size(); ++i )
		{
			p->vpPlayedSteps[i]->PushSelf(L);
			lua_rawseti( L, -2, i+1 );
		}
		return 1;
	}
	static int GetPossibleSteps( T* p, lua_State *L )
	{
		lua_newtable(L);
		for( int i = 0; i < (int) p->vpPossibleSteps.size(); ++i )
		{
			p->vpPossibleSteps[i]->PushSelf(L);
			lua_rawseti( L, -2, i+1 );
		}
		return 1;
	}

	static void Register( lua_State *L )
	{
		ADD_METHOD( GetCaloriesBurned )
		ADD_METHOD( GetLifeRemainingSeconds )
		ADD_METHOD( GetSurvivalSeconds )
		ADD_METHOD( FullCombo )
		ADD_METHOD( MaxCombo )
		ADD_METHOD( GetCurrentCombo )
		ADD_METHOD( GetGrade )
		ADD_METHOD( GetScore )
		ADD_METHOD( GetTapNoteScores )
		ADD_METHOD( GetHoldNoteScores )
		ADD_METHOD( GetCurrentLife )
		ADD_METHOD( GetActualDancePoints )
		ADD_METHOD( GetPossibleDancePoints )
		ADD_METHOD( GetPercentDancePoints )
		ADD_METHOD( GetPlayedSteps )
		ADD_METHOD( GetPossibleSteps )
		Luna<T>::Register( L );
	}
};

LUA_REGISTER_CLASS( PlayerStageStats )


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
