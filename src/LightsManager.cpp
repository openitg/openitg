#include "global.h"
#include "GameState.h"
#include "GameManager.h"
#include "InputMapper.h"
#include "PrefsManager.h"
#include "CommonMetrics.h"
#include "Actor.h"
#include "Style.h"

#include "arch/Lights/LightsDriver.h"

/* UGLY: to maintain compatibility, we need to use 'LightsDriver',
 * even though our system allows more than one to be instantiated... */
static Preference<CString> g_sLightsDrivers( "LightsDriver", "" ); // "" = DEFAULT_LIGHTS_DRIVER

Preference<float>	g_fLightsFalloffSeconds( "LightsFalloffSeconds", 0.1f );
Preference<float>	g_fLightsAheadSeconds( "LightsAheadSeconds", 0.05f );
Preference<bool>	g_bBlinkGameplayButtonLightsOnNote( "BlinkGameplayButtonLightsOnNote", false );


static const CString CabinetLightNames[] = {
	"MarqueeUpLeft",
	"MarqueeUpRight",
	"MarqueeLrLeft",
	"MarqueeLrRight",
	"ButtonsLeft",
	"ButtonsRight",
	"BassLeft",
	"BassRight",
};
XToString( CabinetLight, NUM_CABINET_LIGHTS );
StringToX( CabinetLight );

static const CString LightsModeNames[] = {
	"Attract",
	"Joining",
	"Menu",
	"Menu (cabinet)",
	"Menu (bass)",
	"Demonstration",
	"Gameplay",
	"Stage",
	"Cleared",
	"TestAutoCycle",
	"TestManualCycle",
};
XToString( LightsMode, NUM_LIGHTS_MODES );

static void GetUsedGameInputs( vector<GameInput> &vGameInputsOut )
{
	vGameInputsOut.clear();

	set<GameInput> vGIs;
	vector<const Style*> vStyles;
	GAMEMAN->GetStylesForGame( GAMESTATE->m_pCurGame, vStyles );
	FOREACH( const Style*, vStyles, style )
	{
		bool bFound = find( STEPS_TYPES_TO_SHOW.GetValue().begin(), STEPS_TYPES_TO_SHOW.GetValue().end(), (*style)->m_StepsType ) != STEPS_TYPES_TO_SHOW.GetValue().end();
		if( !bFound )
			continue;
		FOREACH_PlayerNumber( pn )
		{
			for( int i=0; i<(*style)->m_iColsPerPlayer; i++ )
			{
				StyleInput si( pn, i );
				GameInput gi = (*style)->StyleInputToGameInput( si );
				if( gi.IsValid() )
				{
					vGIs.insert( gi );
				}
			}
		}
	}
	FOREACHS_CONST( GameInput, vGIs, gi )
		vGameInputsOut.push_back( *gi );
}

LightsManager*	LIGHTSMAN = NULL;	// global and accessable from anywhere in our program

LightsManager::LightsManager()
{
	ZERO( m_fSecsLeftInCabinetLightBlink );
	ZERO( m_fSecsLeftInGameButtonBlink );
	ZERO( m_fActorLights );
	ZERO( m_fSecsLeftInActorLightBlink );
	m_iQueuedCoinCounterPulses = 0;
	m_CoinCounterTimer.SetZero();

	m_LightsMode = LIGHTSMODE_JOINING;

	LightsDriver::Create( g_sLightsDrivers, m_vpDrivers );

	m_fTestAutoCycleCurrentIndex = 0;
	m_clTestManualCycleCurrent = LIGHT_INVALID;
	m_iControllerTestManualCycleCurrent = -1;
}

LightsManager::~LightsManager()
{
	CHECKPOINT_M("Deleting drivers");
	FOREACH( LightsDriver*, m_vpDrivers, iter )
		SAFE_DELETE( *iter );
	m_vpDrivers.clear();

	CHECKPOINT_M("Drivers deleted.");
}

// XXX: make themable
static const float g_fLightEffectRiseSeconds = 0.075f;
static const float g_fLightEffectFalloffSeconds = 0.35f;
static const float g_fCoinPulseTime = 0.100f;

void LightsManager::BlinkActorLight( CabinetLight cl )
{
	m_fSecsLeftInActorLightBlink[cl] = g_fLightEffectRiseSeconds;
}

float LightsManager::GetActorLightLatencySeconds() const
{
	return g_fLightEffectRiseSeconds;
}

void LightsManager::Update( float fDeltaTime )
{
	//
	// Update actor effect lights.
	//
	FOREACH_CabinetLight( cl )
	{
		float fTime = fDeltaTime;
		float &fDuration = m_fSecsLeftInActorLightBlink[cl];
		if( fDuration > 0 )
		{
			/* The light has power left.  Brighten it. */
			float fSeconds = min( fDuration, fTime );
			fDuration -= fSeconds;
			fTime -= fSeconds;
			fapproach( m_fActorLights[cl], 1, fSeconds / g_fLightEffectRiseSeconds );
		}

		if( fTime > 0 )
		{
			/* The light is out of power.  Dim it. */
			fapproach( m_fActorLights[cl], 0, fTime / g_fLightEffectFalloffSeconds );
		}

		Actor::SetBGMLight( cl, m_fActorLights[cl] );
	}

	if( !IsEnabled() )
		return;

	// update lights falloff
	{
		FOREACH_CabinetLight( cl )
			fapproach( m_fSecsLeftInCabinetLightBlink[cl], 0, fDeltaTime );
		FOREACH_GameController( gc )
			FOREACH_GameButton( gb )
				fapproach( m_fSecsLeftInGameButtonBlink[gc][gb], 0, fDeltaTime );
	}

	//
	// Set new lights state cabinet lights
	//
	{
		ZERO( m_LightsState.m_bCabinetLights );
		ZERO( m_LightsState.m_bGameButtonLights );
	}

	//
	// Update the counter and pulse if we can
	//
	{
		m_LightsState.m_bCoinCounter = false;
		
		if( !m_CoinCounterTimer.IsZero() )
		{
			float fAgo = m_CoinCounterTimer.Ago();
			if( fAgo < g_fCoinPulseTime )
				m_LightsState.m_bCoinCounter = true;
			else if( fAgo >= g_fCoinPulseTime * 2 )
				m_CoinCounterTimer.SetZero();
		}
		else if( m_iQueuedCoinCounterPulses )
		{
			m_CoinCounterTimer.Touch();
			--m_iQueuedCoinCounterPulses;
		}
	}

	if( m_LightsMode == LIGHTSMODE_TEST_AUTO_CYCLE )
	{
		m_fTestAutoCycleCurrentIndex += fDeltaTime;
		m_fTestAutoCycleCurrentIndex = fmodf( m_fTestAutoCycleCurrentIndex, NUM_CABINET_LIGHTS*100 );
	}

	switch( m_LightsMode )
	{
	case LIGHTSMODE_JOINING:
		{
//			int iBeat = (int)(GAMESTATE->m_fLightSongBeat);
//			bool bBlinkOn = (iBeat%2)==0;

			FOREACH_PlayerNumber( pn )
			{
				if( GAMESTATE->m_bSideIsJoined[pn] )
				{
					m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_UP_LEFT+pn] = true;
					m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_LR_LEFT+pn] = true;
					m_LightsState.m_bCabinetLights[LIGHT_BUTTONS_LEFT+pn] = true;
					m_LightsState.m_bCabinetLights[LIGHT_BASS_LEFT+pn] = true;
				}
			}
		}
		break;
	case LIGHTSMODE_ATTRACT:
		{
			int iSec = (int)RageTimer::GetTimeSinceStartFast();
			int iTopIndex = iSec % 4;
			switch( iTopIndex )
			{
			case 0:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_UP_LEFT]  = true;	break;
			case 1:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_LR_RIGHT] = true;	break;
			case 2:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_UP_RIGHT] = true;	break;
			case 3:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_LR_LEFT]  = true;	break;
			default:	ASSERT(0);
			}

			bool bOn = (iSec%4)==0;
			m_LightsState.m_bCabinetLights[LIGHT_BASS_LEFT]			= bOn;
			m_LightsState.m_bCabinetLights[LIGHT_BASS_RIGHT]		= bOn;
		}
		break;
	case LIGHTSMODE_MENU:
	case LIGHTSMODE_MENU_LIGHTS:
	case LIGHTSMODE_MENU_BASS:
		{
			FOREACH_CabinetLight( cl )
				m_LightsState.m_bCabinetLights[cl] = false;

			int iBeat = (int)(GAMESTATE->m_fLightSongBeat);

			if( m_LightsMode != LIGHTSMODE_MENU_LIGHTS )
			{
				int iTopIndex = iBeat;
				wrap( iTopIndex, 4 );

				switch( iTopIndex )
				{
				case 0:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_UP_LEFT]  = true;	break;
				case 1:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_LR_RIGHT] = true;	break;
				case 2:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_UP_RIGHT] = true;	break;
				case 3:	m_LightsState.m_bCabinetLights[LIGHT_MARQUEE_LR_LEFT]  = true;	break;
				default:	ASSERT(0);
				}
			}

			/* Flash the button lights for active players. */
			bool bBlinkOn = (iBeat%2)==0;

			FOREACH_PlayerNumber( pn )
				if( GAMESTATE->m_bSideIsJoined[pn] )
					m_LightsState.m_bCabinetLights[LIGHT_BUTTONS_LEFT+pn] = bBlinkOn;

			if( m_LightsMode == LIGHTSMODE_MENU )
				break;

			/* UGLY: must be done manually so we can specify */
			/* update bass */
			m_LightsState.m_bCabinetLights[LIGHT_BASS_LEFT] = m_fSecsLeftInCabinetLightBlink[LIGHT_BASS_LEFT] > 0;
			m_LightsState.m_bCabinetLights[LIGHT_BASS_RIGHT] = m_fSecsLeftInCabinetLightBlink[LIGHT_BASS_RIGHT] > 0;

			if( m_LightsMode == LIGHTSMODE_MENU_BASS )
				break;

			FOREACH_CabinetLight( cl )
				m_LightsState.m_bCabinetLights[cl] = m_fSecsLeftInCabinetLightBlink[cl] > 0;
		}
		break;
	case LIGHTSMODE_DEMONSTRATION:
	case LIGHTSMODE_GAMEPLAY:
		FOREACH_CabinetLight( cl )
			m_LightsState.m_bCabinetLights[cl] = m_fSecsLeftInCabinetLightBlink[cl] > 0;
		break;
	case LIGHTSMODE_STAGE:
	case LIGHTSMODE_ALL_CLEARED:
		{
			FOREACH_CabinetLight( cl )
			{
				bool b = true;
				switch( cl )
				{
				case LIGHT_BUTTONS_LEFT:
				case LIGHT_BUTTONS_RIGHT:
					b = false;
					break;
				default:
					b = true;
					break;
				}
				m_LightsState.m_bCabinetLights[cl] = b;
			}
		}
		break;
	case LIGHTSMODE_TEST_AUTO_CYCLE:
		{
			int iSec = GetTestAutoCycleCurrentIndex();
			FOREACH_CabinetLight( cl )
			{
				bool bOn = (iSec%NUM_CABINET_LIGHTS) == cl;
				m_LightsState.m_bCabinetLights[cl] = bOn;
			}
		}
		break;
	case LIGHTSMODE_TEST_MANUAL_CYCLE:
		{
			FOREACH_CabinetLight( cl )
			{
				bool bOn = cl == m_clTestManualCycleCurrent;
				m_LightsState.m_bCabinetLights[cl] = bOn;
			}
		}
		break;
	default:
		ASSERT(0);
	}


	// If not joined, has enough credits, and not too late to join, then 
	// blink the menu buttons rapidly so they'll press Start
	{
		int iBeat = (int)(GAMESTATE->m_fLightSongBeat*4);
		bool bBlinkOn = (iBeat%2)==0;
		FOREACH_PlayerNumber( pn )
		{
			if( !GAMESTATE->m_bSideIsJoined[pn] && GAMESTATE->PlayersCanJoin() && GAMESTATE->EnoughCreditsToJoin() )
			{
				m_LightsState.m_bCabinetLights[LIGHT_BUTTONS_LEFT+pn] = bBlinkOn;
				m_LightsState.m_bGameButtonLights[pn][GAME_BUTTON_START] = bBlinkOn;
			}
		}
	}


	//
	// Update game controller lights
	//
	// FIXME:  Works only with Game==dance
	// FIXME:  lights pads for players who aren't playing
	switch( m_LightsMode )
	{
	case LIGHTSMODE_ATTRACT:
		{
			ZERO( m_LightsState.m_bGameButtonLights );
		}
		break;
	case LIGHTSMODE_ALL_CLEARED:
	case LIGHTSMODE_STAGE:
	case LIGHTSMODE_JOINING:
		{
			FOREACH_GameController( gc )
			{
				bool bOn = GAMESTATE->m_bSideIsJoined[gc];

				FOREACH_GameButton( gb )
					m_LightsState.m_bGameButtonLights[gc][gb] = bOn;
			}
		}
		break;
	case LIGHTSMODE_MENU:
	case LIGHTSMODE_MENU_BASS:
	case LIGHTSMODE_MENU_LIGHTS:
	case LIGHTSMODE_DEMONSTRATION:
	case LIGHTSMODE_GAMEPLAY:
		{
			if( (m_LightsMode == LIGHTSMODE_GAMEPLAY && g_bBlinkGameplayButtonLightsOnNote) || m_LightsMode == LIGHTSMODE_DEMONSTRATION )
			{
				//
				// Blink on notes.
				//
				FOREACH_GameController( gc )
				{
					FOREACH_GameButton( gb )
					{
						m_LightsState.m_bGameButtonLights[gc][gb] = m_fSecsLeftInGameButtonBlink[gc][gb] > 0 ;
					}
				}
			}
			else
			{
				//
				// Blink on button pressess.
				//
				FOREACH_GameController( gc )
				{
					// don't blink unjoined sides
					if( !GAMESTATE->m_bSideIsJoined[gc] )
						continue;

					FOREACH_GameButton_Custom( gb )
					{
						bool bOn = INPUTMAPPER->IsButtonDown( GameInput(gc,gb) );
						m_LightsState.m_bGameButtonLights[gc][gb] = bOn;
					}
				}
			}
		}
		break;
	case LIGHTSMODE_TEST_AUTO_CYCLE:
		{
			int index = GetTestAutoCycleCurrentIndex();

			vector<GameInput> vGI;
			GetUsedGameInputs( vGI );

			wrap( index, vGI.size() );

			ZERO( m_LightsState.m_bGameButtonLights );

			GameController gc = vGI[index].controller;
			GameButton gb = vGI[index].button;
			m_LightsState.m_bGameButtonLights[gc][gb] = true;
		}
		break;
	case LIGHTSMODE_TEST_MANUAL_CYCLE:
		{
			ZERO( m_LightsState.m_bGameButtonLights );

			vector<GameInput> vGI;
			GetUsedGameInputs( vGI );
			if( m_iControllerTestManualCycleCurrent != -1 )
			{
				GameController gc = vGI[m_iControllerTestManualCycleCurrent].controller;
				GameButton gb = vGI[m_iControllerTestManualCycleCurrent].button;
				m_LightsState.m_bGameButtonLights[gc][gb] = true;
			}
		}
		break;
	default:
		ASSERT(0);
	}

	// apply new light values we set above
	FOREACH( LightsDriver*, m_vpDrivers, iter )
		(*iter)->Set( &m_LightsState );
}

void LightsManager::BlinkCabinetLight( CabinetLight cl, float fLength )
{
	m_fSecsLeftInCabinetLightBlink[cl] = (fLength == 0) ? g_fLightsFalloffSeconds : fLength;
}

void LightsManager::BlinkGameButton( GameInput gi, float fLength )
{
	m_fSecsLeftInGameButtonBlink[gi.controller][gi.button] = (fLength == 0) ? g_fLightsFalloffSeconds : fLength;
}

void LightsManager::SetLightsMode( LightsMode lm )
{
	m_LightsMode = lm;
}

LightsMode LightsManager::GetLightsMode()
{
	return m_LightsMode;
}

void LightsManager::ChangeTestCabinetLight( int iDir )
{
	m_iControllerTestManualCycleCurrent = -1;

	m_clTestManualCycleCurrent = (CabinetLight)(m_clTestManualCycleCurrent+iDir);

	int cablight = (int)m_clTestManualCycleCurrent;
	wrap( cablight, NUM_CABINET_LIGHTS );
	m_clTestManualCycleCurrent = (CabinetLight)cablight;
}

void LightsManager::ChangeTestGameButtonLight( int iDir )
{
	m_clTestManualCycleCurrent = LIGHT_INVALID;
	
	vector<GameInput> vGI;
	GetUsedGameInputs( vGI );

	m_iControllerTestManualCycleCurrent += iDir;
	wrap( m_iControllerTestManualCycleCurrent, vGI.size() );
}

CabinetLight LightsManager::GetFirstLitCabinetLight()
{
	FOREACH_CabinetLight( cl )
	{
		if( m_LightsState.m_bCabinetLights[cl] )
			return cl;
	}
	return LIGHT_INVALID;
}

void LightsManager::GetFirstLitGameButtonLight( GameController &gcOut, GameButton &gbOut )
{
	FOREACH_GameController( gc )
	{
		FOREACH_GameButton( gb )
		{
			if( m_LightsState.m_bGameButtonLights[gc][gb] )
			{
				gcOut = gc;
				gbOut = gb;
				return;
			}
		}
	}
	gcOut = GAME_CONTROLLER_INVALID;
	gbOut = GAME_BUTTON_INVALID;
}

bool LightsManager::IsEnabled() const
{
	return m_vpDrivers.size() > 1 || PREFSMAN->m_bDebugLights;
}


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
