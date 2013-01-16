#include "global.h"
#include "GameState.h"
#include "GameManager.h"
#include "GameSoundManager.h"
#include "LightsManager.h"
#include "PrefsManager.h"
#include "NoteDataUtil.h"
#include "Steps.h"
#include "Game.h"
#include "Style.h"
#include "ScreenPlayLights.h"

AutoScreenMessage( SM_LightsEnded )

#define PREV_SCREEN THEME->GetMetric (m_sName,"PrevScreen")

#define LIGHTS_AUTOGEN_TYPE STEPS_TYPE_DANCE_SINGLE

REGISTER_SCREEN_CLASS( ScreenPlayLights );
ScreenPlayLights::ScreenPlayLights( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
}

void ScreenPlayLights::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_LightsEnded )
		MenuStart( PLAYER_INVALID );

	if( SM == SM_GoToPrevScreen || SM == SM_GoToNextScreen )
	{
		if( m_Music.IsPlaying() )
			m_Music.StopPlaying();

		SCREENMAN->SetNewScreen( PREV_SCREEN );
	}

	Screen::HandleScreenMessage( SM );
}

void ScreenPlayLights::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenPlayLights::Update( float fDeltaTime )
{
	if( m_bFirstUpdate )
	{
		// load lights here.
		LIGHTSMAN->SetLightsMode( LIGHTSMODE_GAMEPLAY );
		LoadLights();

		// load music here.
		m_Music.Load( GAMESTATE->m_pCurSong->m_sGameplayMusic );
		m_Music.StartPlaying();
	}

	UpdateSongPosition( fDeltaTime );

	// skew this a bit, but don't play past the music length,
	float fSecondsToStop = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat( GAMESTATE->m_pCurSong->m_fLastBeat ) + 2;
	CLAMP( fSecondsToStop, 1.0f, GAMESTATE->m_pCurSong->MusicLengthSeconds() - 0.5f );

	/* Make sure we keep going long enough to register a miss for the last note. */
	if( GAMESTATE->m_fMusicSeconds > fSecondsToStop )
		m_Out.StartTransitioning( SM_GoToPrevScreen );

	UpdateLights();
	Screen::Update( fDeltaTime );
}

void ScreenPlayLights::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenPlayLights::UpdateSongPosition( float fDeltaTime )
{
	RageTimer tm;
	const float fSeconds = m_Music.GetPositionSeconds( NULL, &tm );
	const float fAdjust = SOUND->GetFrameTimingAdjustment( fDeltaTime );

	float fSecondsTotal = fSeconds+fAdjust;

	GAMESTATE->UpdateSongPosition( fSecondsTotal, GAMESTATE->m_pCurSong->m_Timing, tm+fAdjust );
}

// this is ripped from ScreenGameplay, with some modifications
void ScreenPlayLights::LoadLights()
{
	ASSERT( GAMESTATE->m_pCurSong );

	// load the current steps into the note data
	FOREACH_EnabledPlayer( p )
		GAMESTATE->m_pCurSteps[p]->GetNoteData( m_NoteData[p] );

	// load a lights-cabinet chart if this song has them
	const Steps *pSteps = GAMESTATE->m_pCurSong->GetClosestNotes( STEPS_TYPE_LIGHTS_CABINET, DIFFICULTY_MEDIUM );

	if( pSteps != NULL )
	{
		pSteps->GetNoteData( m_CabinetNoteData );
		return;
	}

	vector<CString> asDifficulties;
	split( PREFSMAN->m_sLightsStepsDifficulty, ",", asDifficulties );

	Difficulty d1 = DIFFICULTY_INVALID;
	if( asDifficulties.size() > 0 )
		d1 = StringToDifficulty( asDifficulties[0] );

	pSteps = GAMESTATE->m_pCurSong->GetClosestNotes( LIGHTS_AUTOGEN_TYPE, d1 );

	if( pSteps == NULL )
		pSteps = GAMESTATE->m_pCurSong->GetClosestNotes( GAMESTATE->GetCurrentStyle()->m_StepsType, d1 );

	// can't find anything; stop.
	if( pSteps == NULL )
		return;

	NoteData TapNoteData1;
	pSteps->GetNoteData( TapNoteData1 );

	if( asDifficulties.size() > 1 )
	{
		Difficulty d2 = StringToDifficulty( asDifficulties[1] );

		Steps *pSteps2;

		pSteps2 = GAMESTATE->m_pCurSong->GetClosestNotes( LIGHTS_AUTOGEN_TYPE, d2 );

		if( pSteps2 == NULL )
			pSteps2 = GAMESTATE->m_pCurSong->GetClosestNotes( GAMESTATE->GetCurrentStyle()->m_StepsType, d2 );

		// this will fail if pSteps2 is NULL - if pSteps were NULL, we would've returned already.
		if( pSteps != pSteps2 )
		{
			NoteData TapNoteData2;
			pSteps2->GetNoteData( TapNoteData2 );
			NoteDataUtil::LoadTransformedLightsFromTwo( TapNoteData1, TapNoteData2, m_CabinetNoteData );
			return;
		}

		/* fall through */
	}

	NoteDataUtil::LoadTransformedLights( TapNoteData1, m_CabinetNoteData, GameManager::StepsTypeToNumTracks(STEPS_TYPE_LIGHTS_CABINET) );

	LIGHTSMAN->SetLightsMode( LIGHTSMODE_MENU_LIGHTS );
}

// this is ripped from ScreenGameplay, with some modifications
void ScreenPlayLights::UpdateLights()
{
	/* this is here instead of LoadLights() in case someone
	 * switches on debug after the screen's already loaded. */
	if( !LIGHTSMAN->IsEnabled() )
		return;

	/* data wasn't loaded */
	if( m_CabinetNoteData.GetNumTracks() == 0 )
		return;

	static bool bBlinkCabinetLight[NUM_CABINET_LIGHTS];
	static bool bBlinkGameButton[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS];
	ZERO( bBlinkCabinetLight );
	ZERO( bBlinkGameButton );

	/* update the lights data */
	{
		const float fSongBeat = GAMESTATE->m_fLightSongBeat;
		const int iSongRow = BeatToNoteRowNotRounded( fSongBeat );

		static int iRowLastCrossed = 0;

		// keep all cabinet lights (except buttons) on if the steps haven't started.
		if( GAMESTATE->m_fSongBeat < GAMESTATE->m_pCurSong->m_fFirstBeat )
		{
			FOREACH_CabinetLight( cl )
			{
				switch( cl )
				{
				case LIGHT_BUTTONS_LEFT:
				case LIGHT_BUTTONS_RIGHT:
					break;
				default:
					bBlinkCabinetLight[cl] = true;
					MESSAGEMAN->Broadcast( CabinetLightToString(cl) + "Hold" );
				}
			}
		}
		else
		{
			FOREACH_CabinetLight( cl )
			{
				FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( m_CabinetNoteData, cl, r, iRowLastCrossed+1, iSongRow+1 )
				{
					if( m_CabinetNoteData.GetTapNote(cl, r).type != TapNote::empty )
					{
						bBlinkCabinetLight[cl] = true;
						MESSAGEMAN->Broadcast( CabinetLightToString(cl) );
					}
				}

				if( m_CabinetNoteData.IsHoldNoteAtBeat(cl, iSongRow) )
				{
					bBlinkCabinetLight[cl] = true;
					MESSAGEMAN->Broadcast( CabinetLightToString(cl) + "Hold" );
				}
			}

			static const Game *pGame = GAMESTATE->GetCurrentGame();
			static const Style *pStyle = GAMESTATE->GetCurrentStyle();

			FOREACH_EnabledPlayer( pn )
			{
				for( int t=0; t< m_NoteData[pn].GetNumTracks(); t++ )
				{
					bool bBlink = false, bHold = false;
					// for each index we crossed since the last update:
					FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( m_NoteData[pn], t, r, iRowLastCrossed+1, iSongRow+1 )
					{
						TapNote tn = m_NoteData[pn].GetTapNote(t,r);

						if( tn.type != TapNote::empty && tn.type != TapNote::mine )
							bBlink = true;
					}

					// check if a hold should be active
					if( m_NoteData[pn].IsHoldNoteAtBeat( t, iSongRow ) )
						bHold = true;

					if( bBlink || bHold )
					{
						GameInput gi = pStyle->StyleInputToGameInput( StyleInput(pn,t) );
						bBlinkGameButton[gi.controller][gi.button] = true;

						// broadcast our messages here
						CString sStr = ssprintf( "%sP%d", pGame->m_szButtonNames[gi.button], gi.controller+1 );
						// hold takes precedence over a blink
						if( bHold )
							MESSAGEMAN->Broadcast( sStr + "Hold" );
						else if( bBlink )
							MESSAGEMAN->Broadcast( sStr );
					}
				}
			}
		}

		iRowLastCrossed = iSongRow;
	}

	/* ...and apply it */
	FOREACH_CabinetLight( cl )
		if( bBlinkCabinetLight[cl] )
			LIGHTSMAN->BlinkCabinetLight( cl );

	FOREACH_GameController( gc )
		FOREACH_GameButton( gb )
			if( bBlinkGameButton[gc][gb] )
				LIGHTSMAN->BlinkGameButton( GameInput(gc,gb) );
}

void ScreenPlayLights::MenuStart( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		this->PlayCommand( "Off" );
		StartTransitioning( SM_GoToPrevScreen );
	}
}

void ScreenPlayLights::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		this->PlayCommand( "Off" );
		StartTransitioning( SM_GoToPrevScreen );
	}
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
