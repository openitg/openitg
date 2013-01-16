/* PercentageDisplay - An Actor that displays a percentage. */

#ifndef PERCENTAGE_DISPLAY_H
#define PERCENTAGE_DISPLAY_H

#include "ActorFrame.h"
#include "PlayerNumber.h"
#include "BitmapText.h"
#include "StageStats.h"
#include "ThemeMetric.h"

struct PlayerState;

class PercentageDisplay: public ActorFrame
{
public:
	PercentageDisplay();
	void Load( PlayerNumber pn, PlayerStageStats *pSource, const CString &sMetricsGroup, bool bAutoRefresh );
	void Update( float fDeltaTime );
	void TweenOffScreen();

	static CString FormatPercentScore( float fPercentDancePoints );

private:
	ThemeMetric<int> DANCE_POINT_DIGITS;
	ThemeMetric<bool> PERCENT_USE_REMAINDER;
	ThemeMetric<bool> APPLY_SCORE_DISPLAY_OPTIONS;

	void Refresh();
	PlayerNumber m_PlayerNumber;
	PlayerStageStats *m_pSource;
	bool m_bAutoRefresh;
	int m_Last;
	int m_LastMax;
	BitmapText	m_textPercent;
	BitmapText	m_textPercentRemainder;
};

#endif

/*
 * (c) 2001-2003 Chris Danford
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
