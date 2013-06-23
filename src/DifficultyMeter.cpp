#include "global.h"
#include "DifficultyMeter.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "GameState.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "Course.h"
#include "SongManager.h"
#include "ActorUtil.h"
#include "Style.h"
#include "XmlFile.h"

// lua start
LUA_REGISTER_CLASS( DifficultyMeter )
// lua end
REGISTER_ACTOR_CLASS( DifficultyMeter );


DifficultyMeter::DifficultyMeter()
{
}

static CString GetDifficultyCommandName( Difficulty d ) { return "Set"+DifficultyToString(d); }
static CString GetCourseDifficultyCommandName( CourseDifficulty d ) { return "Set"+CourseDifficultyToString(d)+"Course"; }
static const CString DIFFICULTY_COMMAND_NAME_NONE = "SetNone";

/* sID experiment:
 *
 * Names of an actor, "Foo":
 * [Foo]
 * Metric=abc
 *
 * [ScreenSomething]
 * FooP1X=20
 * FooP2Y=30
 *
 * Graphics\Foo under p1
 *
 * We want to call it different things in different contexts: we may only want one
 * set of internal metrics for a given use, but separate metrics for each player at
 * the screen level, and we may or may not want separate names at the asset level.
 *
 * As is, we tend to end up having to either duplicate [Foo] to [FooP1] and [FooP2]
 * or not use m_sName for [Foo], which limits its use.  Let's try using a separate
 * name for internal metrics.  I'm not sure if this will cause more confusion than good,
 * so I'm trying it first in only this object.
 */

void DifficultyMeter::Load( const CString &sType )
{
	/* We can't use global ThemeMetric<CString>s, because we can have multiple
	 * DifficultyMeters on screen at once, with different names. */
	m_iNumFeetInMeter.Load(sType,"NumFeetInMeter");
	m_iMaxFeetInMeter.Load(sType,"MaxFeetInMeter");
	m_iGlowIfMeterGreaterThan.Load(sType,"GlowIfMeterGreaterThan");
	m_bShowFeet.Load(sType,"ShowFeet");
	m_bShowDifficulty.Load(sType,"ShowDifficulty");
	m_bShowMeter.Load(sType,"ShowMeter");
	m_bShowEditDescription.Load(sType,"ShowEditDescription");
	m_bAutoColorFeet.Load(sType,"AutoColorFeet");
	m_sZeroMeterString.Load(sType,"ZeroMeterString");

	if( m_bShowFeet )
	{
		m_textFeet.SetName( "Feet" );
		CString Feet;
		if( !m_bAutoColorFeet )
		{
			for( unsigned i = 0; i < NUM_DIFFICULTIES; ++i )
				Feet += char(i + '0'); // 01234
			Feet += 'X'; // Off
		}
		else
		{
			Feet = "0X";
		}
		int iNumFrames = m_bAutoColorFeet ? 2 : NUM_DIFFICULTIES+1;
		m_textFeet.LoadFromTextureAndChars( THEME->GetPathG(sType,ssprintf("bar %dx1",iNumFrames)), Feet );
		ActorUtil::SetXYAndOnCommand( m_textFeet, sType );
		this->AddChild( &m_textFeet );
	}

	if( m_bShowDifficulty )
	{
		m_Difficulty.Load( THEME->GetPathG(sType,"difficulty") );
		m_Difficulty->SetName( "Difficulty" );
		ActorUtil::SetXYAndOnCommand( m_Difficulty, sType );
		this->AddChild( m_Difficulty );

		// These commands should have been loaded by SetXYAndOnCommand above.
		FOREACH_Difficulty( d )
			ASSERT( m_Difficulty->HasCommand(GetDifficultyCommandName(d)) );
		FOREACH_CourseDifficulty( d )
			ASSERT( m_Difficulty->HasCommand(GetCourseDifficultyCommandName(d)) );
		ASSERT( m_Difficulty->HasCommand(DIFFICULTY_COMMAND_NAME_NONE) );
	}

	if( m_bShowMeter )
	{
		m_textMeter.SetName( "Meter" );
		m_textMeter.LoadFromFont( THEME->GetPathF(sType,"meter") );
		ActorUtil::SetXYAndOnCommand( m_textMeter, sType );
		this->AddChild( &m_textMeter );

		// These commands should have been loaded by SetXYAndOnCommand above.
		FOREACH_Difficulty( d )
			ASSERT( m_textMeter.HasCommand(GetDifficultyCommandName(d)) );
		FOREACH_CourseDifficulty( d )
			ASSERT( m_textMeter.HasCommand(GetCourseDifficultyCommandName(d)) );
		ASSERT( m_textMeter.HasCommand(DIFFICULTY_COMMAND_NAME_NONE) );
	}
	
	if( m_bShowEditDescription )
	{
		m_textEditDescription.SetName( "EditDescription" );
		m_textEditDescription.LoadFromFont( THEME->GetPathF(sType,"EditDescription") );
		ActorUtil::SetXYAndOnCommand( m_textEditDescription, sType );
		this->AddChild( &m_textEditDescription );
	}

	Unset();
}

void DifficultyMeter::LoadFromNode( const CString& sDir, const XNode* pNode )
{
	ActorFrame::LoadFromNode( sDir, pNode );

	CString s;
	pNode->GetAttrValue( "Type", s );
	ASSERT( s.size() );
	Load( s );
}

void DifficultyMeter::SetFromGameState( PlayerNumber pn )
{
	if( GAMESTATE->IsCourseMode() )
	{
		const Trail* pTrail = GAMESTATE->m_pCurTrail[pn];
		if( pTrail )
			SetFromTrail( pTrail );
		else
			SetFromMeterAndCourseDifficulty( 0, GAMESTATE->m_PreferredCourseDifficulty[pn] );
	}
	else
	{
		const Steps* pSteps = GAMESTATE->m_pCurSteps[pn];
		if( pSteps )
			SetFromSteps( pSteps );
		else
			SetFromMeterAndDifficulty( 0, GAMESTATE->m_PreferredDifficulty[pn] );
	}
}

void DifficultyMeter::SetFromSteps( const Steps* pSteps )
{
	if( pSteps == NULL )
	{
		Unset();
		return;
	}

	Difficulty dc = pSteps->GetDifficulty();
	SetInternal( pSteps->GetMeter(), dc, GetDifficultyCommandName(dc), dc == DIFFICULTY_EDIT ? pSteps->GetDescription() : CString() );
}

void DifficultyMeter::SetFromTrail( const Trail* pTrail )
{
	if( pTrail == NULL )
	{
		Unset();
		return;
	}

	CourseDifficulty cd = pTrail->m_CourseDifficulty;
	SetInternal( pTrail->GetMeter(), cd, GetCourseDifficultyCommandName( cd ), CString() );
}

void DifficultyMeter::Unset()
{
	SetInternal( 0, DIFFICULTY_EDIT, DIFFICULTY_COMMAND_NAME_NONE, CString() );
}

void DifficultyMeter::SetFromMeterAndDifficulty( int iMeter, Difficulty dc )
{
	SetInternal( 0, dc, GetDifficultyCommandName(dc), CString() );
}

void DifficultyMeter::SetFromMeterAndCourseDifficulty( int iMeter, CourseDifficulty cd )
{
	SetInternal( 0, cd, GetCourseDifficultyCommandName(cd), CString() );
}

void DifficultyMeter::SetInternal( int iMeter, Difficulty dc, const CString &sDifficultyCommand, const CString &sDescription )
{
	if( m_bShowFeet )
	{
		char on = '0';
		char off = 'X';
		if( !m_bAutoColorFeet )
			on = char(dc + '0');

		CString sNewText;
		int iNumOn = min( (int)m_iMaxFeetInMeter, iMeter );
		sNewText.insert( sNewText.end(), iNumOn, on );
		int iNumOff = max( 0, m_iNumFeetInMeter-iNumOn );
		sNewText.insert( sNewText.end(), iNumOff, off );

		m_textFeet.SetText( sNewText );
		if( m_bAutoColorFeet )
			m_textFeet.SetDiffuse( SONGMAN->GetDifficultyColor(dc) );

		if( iMeter > m_iGlowIfMeterGreaterThan )
			m_textFeet.SetEffectGlowShift();
		else
			m_textFeet.SetEffectNone();
	}

	if( m_bShowMeter )
	{
		if( iMeter == 0 )	// Unset calls with this
		{
			m_textMeter.SetText( m_sZeroMeterString );
		}
		else
		{
			const CString sMeter = ssprintf( "%i", iMeter );
			m_textMeter.SetText( sMeter );
		}

		m_textMeter.PlayCommand( "TextChanged" );
	}

	if( m_bShowEditDescription )
	{
		if( dc == DIFFICULTY_EDIT )
		{
			m_textEditDescription.SetVisible( true );
			m_textEditDescription.SetText( sDescription );
		}
		else
		{
			m_textEditDescription.SetVisible( false );
		}
	}

	if( m_sCurDifficultyCommand == sDifficultyCommand )
		return;
	m_sCurDifficultyCommand = sDifficultyCommand;

	if( m_bShowDifficulty )
		m_Difficulty->PlayCommand( sDifficultyCommand );
	if( m_bShowMeter )
		m_textMeter.PlayCommand( sDifficultyCommand );
}

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
