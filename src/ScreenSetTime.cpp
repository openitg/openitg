#include "global.h"
#include "ScreenSetTime.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "GameState.h"
#include "GameSoundManager.h"
#include "arch/ArchHooks/ArchHooks.h"
#include <ctime>

static const CString SetTimeSelectionNames[] = {
	"Year", 
	"Month", 
	"Day",
	"Hour", 
	"Minute", 
	"Second", 
};
XToString( SetTimeSelection, NUM_SET_TIME_SELECTIONS );
#define FOREACH_SetTimeSelection( s ) FOREACH_ENUM( SetTimeSelection, NUM_SET_TIME_SELECTIONS, s )

const float g_X[NUM_SET_TIME_SELECTIONS] =
{
	320, 320, 320, 320, 320, 320
};

const float g_Y[NUM_SET_TIME_SELECTIONS] =
{
	140, 180, 220, 260, 300, 340
};

static float GetTitleX( SetTimeSelection s ) { return g_X[s] - 80; }
static float GetTitleY( SetTimeSelection s ) { return g_Y[s]; }
static float GetValueX( SetTimeSelection s ) { return g_X[s] + 80; }
static float GetValueY( SetTimeSelection s ) { return g_Y[s]; }

REGISTER_SCREEN_CLASS( ScreenSetTime );
ScreenSetTime::ScreenSetTime( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenSetTime::ScreenSetTime()" );
}

void ScreenSetTime::Init()
{
	ScreenWithMenuElements::Init();

	m_Selection = hour;

	FOREACH_PlayerNumber( pn )
		GAMESTATE->m_bSideIsJoined[pn] = true;

	FOREACH_SetTimeSelection( s )
	{
		BitmapText &title = m_textTitle[s];
		BitmapText &value = m_textValue[s];

		title.LoadFromFont( THEME->GetPathF("Common","title") );
		title.SetDiffuse( RageColor(1,1,1,1) );
		title.SetText( SetTimeSelectionToString(s) );
		title.SetXY( GetTitleX(s), GetTitleY(s) );
		this->AddChild( &title );

		title.SetDiffuse( RageColor(1,1,1,0) );
		title.BeginTweening( 0.3f, TWEEN_LINEAR );
		title.SetDiffuse( RageColor(1,1,1,1) );

		value.LoadFromFont( THEME->GetPathF("Common","normal") );
		value.SetDiffuse( RageColor(1,1,1,1) );
		value.SetXY( GetValueX(s), GetValueY(s) );
		this->AddChild( &value );

		value.SetDiffuse( RageColor(1,1,1,0) );
		value.BeginTweening( 0.3f, TWEEN_LINEAR );
		value.SetDiffuse( RageColor(1,1,1,1) );
	}

	m_TimeOffset = 0;
	m_Selection = (SetTimeSelection)0;
	ChangeSelection( 0 );

	SOUND->PlayMusic( THEME->GetPathS(m_sName,"music") );

	this->SortByDrawOrder();
}

void ScreenSetTime::Update( float fDelta )
{
	Screen::Update( fDelta );

	time_t iNow = time(NULL);
	iNow += m_TimeOffset;

	tm now;
	localtime_r( &iNow, &now );
	
	int iPrettyHour = now.tm_hour%12;
	if( iPrettyHour == 0 )
		iPrettyHour = 12;
	CString sPrettyHour = ssprintf( "%d %s", iPrettyHour, now.tm_hour>=12 ? "pm" : "am" );

	m_textValue[hour].SetText(	sPrettyHour );
	m_textValue[minute].SetText( ssprintf("%02d",now.tm_min) );
	m_textValue[second].SetText( ssprintf("%02d",now.tm_sec) );
	m_textValue[year].SetText(	ssprintf("%02d",now.tm_year+1900) );
	m_textValue[month].SetText(	ssprintf("%02d",now.tm_mon+1) );
	m_textValue[day].SetText(	ssprintf("%02d",now.tm_mday) );
}

void ScreenSetTime::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenSetTime::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore

	if( IsTransitioning() )
		return;

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default handler
}

void ScreenSetTime::ChangeValue( int iDirection )
{
	time_t iNow = time(NULL);
	time_t iAdjusted = iNow + m_TimeOffset;

	tm adjusted;
	localtime_r( &iAdjusted, &adjusted );
	
	//tm now = GetLocalTime();
	switch( m_Selection )
	{
	case hour:		adjusted.tm_hour += iDirection;	break;
	case minute:	adjusted.tm_min += iDirection;	break;
	case second:	adjusted.tm_sec += iDirection;	break;
	case year:		adjusted.tm_year += iDirection;	break;
	case month:		adjusted.tm_mon += iDirection;	break;
	case day:		adjusted.tm_mday += iDirection;	break;
	default:		ASSERT(0);
	}

	/* Normalize: */
	iAdjusted = mktime( &adjusted );

	m_TimeOffset = iAdjusted - iNow;

	SOUND->PlayOnce( THEME->GetPathS("ScreenSetTime","ChangeValue") );
}

void ScreenSetTime::ChangeSelection( int iDirection )
{
	// set new value of m_Selection
	SetTimeSelection OldSelection = m_Selection;
	enum_add<SetTimeSelection>( m_Selection, iDirection );

	int iSelection = (int)m_Selection;
	CLAMP( iSelection, 0, NUM_SET_TIME_SELECTIONS-1 );
	m_Selection = (SetTimeSelection)iSelection;

	if( iDirection != 0 && m_Selection == OldSelection )
		return; // can't move any more

	m_textValue[OldSelection].SetEffectNone();
	m_textValue[m_Selection].SetEffectDiffuseShift( 1.f,
		RageColor(0.3f,0.3f,0.3f,1), 
		RageColor(1,1,1,1) );

	if( iDirection != 0 )
		SOUND->PlayOnce( THEME->GetPathS("ScreenSetTime","ChangeSelection") );
}

void ScreenSetTime::MenuUp( PlayerNumber pn )
{
	ChangeSelection( -1 );
}

void ScreenSetTime::MenuDown( PlayerNumber pn )
{
	ChangeSelection( +1 );
}

void ScreenSetTime::MenuLeft( PlayerNumber pn )
{
	ChangeValue( -1 );
}

void ScreenSetTime::MenuRight( PlayerNumber pn )
{
	ChangeValue( +1 );
}

void ScreenSetTime::MenuStart( PlayerNumber pn )
{
	bool bHoldingLeftAndRight = 
		INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_RIGHT) ) &&
		INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_LEFT) );

	if( bHoldingLeftAndRight )
		ChangeSelection( -1 );
	else if( m_Selection == NUM_SET_TIME_SELECTIONS -1 )	// last row
	{
		/* Save the new time. */
		time_t iNow = time(NULL);
		time_t iAdjusted = iNow + m_TimeOffset;

		tm adjusted;
		localtime_r( &iAdjusted, &adjusted );

		HOOKS->SetTime( adjusted );

		/* We're going to draw a little more while we transition out.  We've already
		 * set the new time; don't over-adjust visually. */
		m_TimeOffset = 0;

		FOREACH_SetTimeSelection( s )
		{
			BitmapText &title = m_textTitle[s];
			title.BeginTweening( 0.3f, TWEEN_LINEAR );
			title.SetDiffuse( RageColor(1,1,1,0) );

			BitmapText &value = m_textValue[s];
			value.BeginTweening( 0.3f, TWEEN_LINEAR );
			value.SetDiffuse( RageColor(1,1,1,0) );
		}

		SOUND->PlayOnce( THEME->GetPathS("Common","start") );
		StartTransitioning( SM_GoToNextScreen );
	}
	else
		ChangeSelection( +1 );
}

void ScreenSetTime::MenuSelect( PlayerNumber pn )
{
	ChangeSelection( -1 );
}

void ScreenSetTime::MenuBack( PlayerNumber pn )
{
	StartTransitioning( SM_GoToPrevScreen );
}

/*
 * (c) 2004 Chris Danford
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

