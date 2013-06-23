#include "global.h"
#include "CourseContentsList.h"
#include "RageUtil.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "PrefsManager.h"
#include "Course.h"
#include "SongManager.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "GameState.h"
#include "Style.h"
#include "RageTexture.h"
#include "CourseEntryDisplay.h"

const int MAX_VISIBLE_ITEMS = 5;
const int MAX_ITEMS = MAX_VISIBLE_ITEMS+2;

CourseContentsList::CourseContentsList()
{
	for( int i=0; i<MAX_ITEMS; i++ )
		m_vpDisplay.push_back( new CourseEntryDisplay );
}

CourseContentsList::~CourseContentsList()
{
	FOREACH( CourseEntryDisplay*, m_vpDisplay, d )
		delete *d;
	m_vpDisplay.clear();
}

void CourseContentsList::Load()
{
	FOREACH( CourseEntryDisplay*, m_vpDisplay, d )
	{
		(*d)->SetName( "CourseEntryDisplay" );
		(*d)->Load();
		(*d)->SetUseZBuffer( true );
	}
}

void CourseContentsList::SetFromGameState()
{
	RemoveAllChildren();

	// FIXME: Is there a better way to handle when players don't have 
	// the same number of TrailEntries?
	// They have to have the same number, and of the same songs, or gameplay
	// isn't going to line up.
	Trail* pMasterTrail = GAMESTATE->m_pCurTrail[GAMESTATE->m_MasterPlayerNumber];
	if( pMasterTrail == NULL )
		return;
	unsigned uNumEntriesToShow = min( pMasterTrail->m_vEntries.size(), m_vpDisplay.size() );

	for( int i=0; i<(int)uNumEntriesToShow; i++ )
	{
		CourseEntryDisplay &d = *m_vpDisplay[i];

		this->AddChild( &d );

		const TrailEntry* pte[NUM_PLAYERS];
		ZERO( pte );

		FOREACH_EnabledPlayer(pn)
		{
			Trail* pTrail = GAMESTATE->m_pCurTrail[pn];
			if( pTrail == NULL )
				continue;	// skip
			if( unsigned(i) < pTrail->m_vEntries.size() )
				pte[pn] = &pTrail->m_vEntries[i];
		}
		d.LoadFromTrailEntry( (int)(truncf(m_fItemAtPosition0InList))+i+1, pte );
	}

	bool bLoop = pMasterTrail->m_vEntries.size() > uNumEntriesToShow;

	this->Load2( 
		(float)MAX_VISIBLE_ITEMS,
		m_vpDisplay[0]->GetUnzoomedWidth(),
		m_vpDisplay[0]->GetUnzoomedHeight(),
		bLoop,
		0.7f,
		0.7f );

	this->SetCurrentAndDestinationItem( (MAX_VISIBLE_ITEMS-1)/2 );
	if( bLoop )
	{
		SetPauseCountdownSeconds( 1.5f );
		this->SetDestinationItem( MAX_ITEMS+1 );	// loop forever
	}
}

void CourseContentsList::TweenInAfterChangedCourse()
{
	/*
	m_fItemAtTopOfList = 0;
	m_fTimeUntilScroll = 3;

	for( int i=0; i<m_fItemAtPosition0InList; i++ )
	{
		CourseEntryDisplay& display = m_CourseContentDisplays[i];

		display.StopTweening();
		display.SetXY( 0, -((MAX_VISIBLE_CONTENTS-1)/2) * float(CONTENTS_BAR_HEIGHT) );
		display.BeginTweening( i*0.1f );
		display.SetY( (-(MAX_VISIBLE_CONTENTS-1)/2 + i) * float(CONTENTS_BAR_HEIGHT) );
	}
	*/
}

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
