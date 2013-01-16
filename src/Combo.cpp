#include "global.h"
#include "Combo.h"
#include "StatsManager.h"
#include "GameState.h"
#include "song.h"

Combo::Combo()
{
	m_iLastSeenCombo = -1;
	m_pPlayerState = NULL;
	m_pPlayerStageStats = NULL;

	this->SubscribeToMessage( MESSAGE_BEAT_CROSSED );
}

void Combo::Load( PlayerState *pPlayerState, PlayerStageStats *pPlayerStageStats )
{
	ASSERT( m_SubActors.empty() );	// don't load twice

	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;

	SHOW_COMBO_AT		.Load(m_sName,"ShowComboAt");
	LABEL_X				.Load(m_sName,"LabelX");
	LABEL_Y				.Load(m_sName,"LabelY");
	LABEL_ON_COMMAND	.Load(m_sName,"LabelOnCommand");
	NUMBER_X			.Load(m_sName,"NumberX");
	NUMBER_Y			.Load(m_sName,"NumberY");
	NUMBER_ON_COMMAND	.Load(m_sName,"NumberOnCommand");
	NUMBER_MIN_ZOOM		.Load(m_sName,"NumberMinZoom");
	NUMBER_MAX_ZOOM		.Load(m_sName,"NumberMaxZoom");
	NUMBER_MAX_ZOOM_AT	.Load(m_sName,"NumberMaxZoomAt");
	PULSE_COMMAND		.Load(m_sName,"PulseCommand");
	FULL_COMBO_GREATS_COMMAND		.Load(m_sName,"FullComboGreatsCommand");
	FULL_COMBO_PERFECTS_COMMAND		.Load(m_sName,"FullComboPerfectsCommand");
	FULL_COMBO_MARVELOUSES_COMMAND	.Load(m_sName,"FullComboMarvelousesCommand");
	FULL_COMBO_BROKEN_COMMAND		.Load(m_sName,"FullComboBrokenCommand");
	MISS_COMBO_COMMAND				.Load(m_sName,"MissComboCommand");
	SHOW_MISS_COMBO					.Load(m_sName,"ShowMissCombo");
	
	m_spr100Milestone.Load( THEME->GetPathG(m_sName,"100milestone") );
	this->AddChild( m_spr100Milestone );

	m_spr1000Milestone.Load( THEME->GetPathG(m_sName,"1000milestone") );
	this->AddChild( m_spr1000Milestone );

	m_sprComboLabel.Load( THEME->GetPathG(m_sName,"label") );
	m_sprComboLabel->StopAnimating();
	m_sprComboLabel->SetXY( LABEL_X, LABEL_Y );
	m_sprComboLabel->RunCommands( LABEL_ON_COMMAND );
	m_sprComboLabel->SetHidden( true );
	this->AddChild( m_sprComboLabel );

	m_sprMissesLabel.Load( THEME->GetPathG(m_sName,"misses") );
	m_sprMissesLabel->StopAnimating();
	m_sprMissesLabel->SetXY( LABEL_X, LABEL_Y );
	m_sprMissesLabel->RunCommands( LABEL_ON_COMMAND );
	m_sprMissesLabel->SetHidden( true );
	this->AddChild( m_sprMissesLabel );

	m_textNumber.LoadFromFont( THEME->GetPathF(m_sName,"numbers") );
	m_textNumber.SetXY( NUMBER_X, NUMBER_Y );
	m_textNumber.RunCommands( NUMBER_ON_COMMAND );
	m_textNumber.SetHidden( true );
	this->AddChild( &m_textNumber );
}

void Combo::SetCombo( int iCombo, int iMisses, float fLastStepsSeconds )
{

	bool bMisses = iMisses > 0;
	int iNum = bMisses ? iMisses : iCombo;

	if( (iNum < (int)SHOW_COMBO_AT)  || 
		(bMisses && !(bool)SHOW_MISS_COMBO) )
	{
		m_sprComboLabel->SetHidden( true );
		m_sprMissesLabel->SetHidden( true );
		m_textNumber.SetHidden( true );
		return;
	}

	if( m_iLastSeenCombo == -1 )	// first update, don't set bIsMilestone=true
		m_iLastSeenCombo = iCombo;

	bool b100Milestone = false;
	bool b1000Milestone = false;
	for( int i=m_iLastSeenCombo+1; i<=iCombo; i++ )
	{
		if( i < 600 )
			b100Milestone |= ((i % 100) == 0);
		else
			b1000Milestone |= ((i % 200) == 0);
	}
	m_iLastSeenCombo = iCombo;

	m_sprComboLabel->SetHidden( bMisses );
	m_sprMissesLabel->SetHidden( !bMisses );
	m_textNumber.SetHidden( false );

	CString txt = ssprintf("%d", iNum);
	
	// Do pulse even if the number isn't changing.

	m_textNumber.SetText( txt );
	float fNumberZoom = SCALE(iNum,0.f,(float)NUMBER_MAX_ZOOM_AT,(float)NUMBER_MIN_ZOOM,(float)NUMBER_MAX_ZOOM);
	CLAMP( fNumberZoom, (float)NUMBER_MIN_ZOOM, (float)NUMBER_MAX_ZOOM );
	m_textNumber.FinishTweening();
	m_textNumber.SetZoom( fNumberZoom );
	m_textNumber.RunCommands( PULSE_COMMAND ); 

	AutoActor &sprLabel = bMisses ? m_sprMissesLabel : m_sprComboLabel;

	sprLabel->FinishTweening();
	sprLabel->RunCommands( PULSE_COMMAND );

	if( b100Milestone )
		m_spr100Milestone->PlayCommand( "Milestone" );
	if( b1000Milestone )
		m_spr1000Milestone->PlayCommand( "Milestone" );

	//SCREENMAN->SystemMessageNoAnimate( ssprintf("m_ComboStatus: %d", m_pPlayerStageStats->m_ComboStatus) );

#if 0
	if ( fLastStepsSeconds != -1.0f )
	{
		if( m_pPlayerStageStats->m_ComboStatus == COMBSTAT_FFC && !m_pPlayerStageStats->FullComboOfScore(TNS_MARVELOUS) )
		{
			if( m_pPlayerStageStats->FullComboOfScore(TNS_PERFECT) )
			{
				m_pPlayerStageStats->fFullExcellentComboBegin = fLastStepsSeconds;
				m_pPlayerStageStats->bFlag_FEC = true;
				m_pPlayerStageStats->m_ComboStatus = COMBSTAT_FEC;
			}
			else if( m_pPlayerStageStats->FullComboOfScore(TNS_GREAT) )
			{
				m_pPlayerStageStats->fFullGreatComboBegin = fLastStepsSeconds;
				m_pPlayerStageStats->bFlag_FGC = true;
				m_pPlayerStageStats->m_ComboStatus = COMBSTAT_FGC;
			}
			else
			{
				m_pPlayerStageStats->fPulsatingComboEnd = fLastStepsSeconds;
				m_pPlayerStageStats->bFlag_PulsateEnd = true;
				m_pPlayerStageStats->m_ComboStatus = COMBSTAT_NONE;
			}
			
		}
		else if( m_pPlayerStageStats->m_ComboStatus == COMBSTAT_FEC && !m_pPlayerStageStats->FullComboOfScore(TNS_PERFECT) )
		{
			ASSERT( !m_pPlayerStageStats->FullComboOfScore(TNS_MARVELOUS) ); // it's not gonna break anything, but it shouldn't happen
	
			if ( m_pPlayerStageStats->FullComboOfScore(TNS_GREAT) )
			{
				m_pPlayerStageStats->fFullGreatComboBegin = fLastStepsSeconds;
				m_pPlayerStageStats->bFlag_FGC = true;
				m_pPlayerStageStats->m_ComboStatus = COMBSTAT_FGC;
			}
			else
			{
				m_pPlayerStageStats->fPulsatingComboEnd = fLastStepsSeconds;
				m_pPlayerStageStats->bFlag_PulsateEnd = true;
				m_pPlayerStageStats->m_ComboStatus = COMBSTAT_NONE;
			}
		}
		else if( m_pPlayerStageStats->m_ComboStatus == COMBSTAT_FGC && !m_pPlayerStageStats->FullComboOfScore(TNS_GREAT) )
		{
			ASSERT( !m_pPlayerStageStats->FullComboOfScore(TNS_MARVELOUS) );
			ASSERT( !m_pPlayerStageStats->FullComboOfScore(TNS_PERFECT) );
	
			m_pPlayerStageStats->fPulsatingComboEnd = fLastStepsSeconds;
			m_pPlayerStageStats->bFlag_PulsateEnd = true;
			m_pPlayerStageStats->m_ComboStatus = COMBSTAT_NONE;
		}
	}
#endif

	// I don't know if this is any faster, but I prefer this code layout.
	// XXX: use a less stupid name.
	const ThemeMetric<apActorCommands> *pToRun = bMisses ? &MISS_COMBO_COMMAND : &FULL_COMBO_BROKEN_COMMAND;

	// don't show a colored combo until 1/4 of the way through the song
	bool bPastMidpoint = GAMESTATE->GetCourseSongIndex()>0 ||
		GAMESTATE->m_fMusicSeconds > GAMESTATE->m_pCurSong->MusicLengthSeconds()/4;

	if( bPastMidpoint )
	{
		if( m_pPlayerStageStats->FullComboOfScore(TNS_MARVELOUS) )
			pToRun = &FULL_COMBO_MARVELOUSES_COMMAND;
		else if( m_pPlayerStageStats->FullComboOfScore(TNS_PERFECT) )
			pToRun = &FULL_COMBO_PERFECTS_COMMAND;
		else if( m_pPlayerStageStats->FullComboOfScore(TNS_GREAT) )
			pToRun = &FULL_COMBO_GREATS_COMMAND;
	}

	// dispatch the commands we're running from earlier
	sprLabel->RunCommands( *pToRun );
	m_textNumber.RunCommands( *pToRun );
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
