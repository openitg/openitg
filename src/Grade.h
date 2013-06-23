/* Grade - Mark the player receives after clearing a song. */

#ifndef GRADE_H
#define GRADE_H

#include "RageUtil.h"

#define NUM_GRADE_TIERS 20
enum Grade 
{ 
	GRADE_TIER01,	// = AAAA
	GRADE_TIER02,	// = AAA
	GRADE_TIER03,	// = AA
	GRADE_TIER04,	// = A
	GRADE_TIER05,	// = B
	GRADE_TIER06,	// = C
	GRADE_TIER07,	// = D
	GRADE_TIER08,
	GRADE_TIER09,
	GRADE_TIER10,
	GRADE_TIER11,
	GRADE_TIER12,
	GRADE_TIER13,
	GRADE_TIER14,
	GRADE_TIER15,
	GRADE_TIER16,
	GRADE_TIER17,
	GRADE_TIER18,
	GRADE_TIER19,
	GRADE_TIER20,
	GRADE_FAILED,	// = E
	NUM_GRADES, 
	GRADE_NO_DATA,	// ~GRADE_INVALID
};

/* This is in the header so the test sets don't require Grade.cpp (through PrefsManager),
 * since that pulls in ThemeManager. */
static inline CString GradeToString( Grade g )
{
        // string is meant to be human readable
        switch( g )
        {
        case GRADE_NO_DATA:     return "NoData";
        case GRADE_FAILED:      return "Failed";
        default:
                return ssprintf("Tier%02d",g+1);
        }
}

CString GradeToOldString( Grade g );	// "AAA", "B", etc for backward compatibility
CString GradeToThemedString( Grade g );
Grade StringToGrade( const CString &s );
#define FOREACH_Grade( g ) FOREACH_ENUM( Grade, NUM_GRADES, g )
#define FOREACH_UsedGrade( g ) FOREACH_ENUM( Grade, THEME->GetMetricI("PlayerStageStats","NumGradeTiersUsed"), g )

#endif

/*
 * (c) 2001-2004 Chris Danford
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
