#include "global.h"
#include "InputMapper.h"
#include "SongManager.h"
#include "GameState.h"
#include "ScoreKeeperMAX2.h"
#include "RageLog.h"
#include "RageDisplay.h"
#include "CombinedLifeMeter.h"
#include "PlayerAI.h"
#include "NoteField.h"
#include "NoteFieldPositioning.h"
#include "NoteDataUtil.h"
#include "ScreenGameplay.h" /* for SM_ComboStopped */
#include "ArrowEffects.h"
#include "Game.h"
#include "ScreenDimensions.h"
#include "PlayerState.h"
#include "GameSoundManager.h"
#include "Style.h"
#include "ProfileManager.h"
#include "StatsManager.h"

CString JUDGMENT_X_NAME( size_t p, size_t both_sides )		{ return "JudgmentXOffset" + (both_sides ? CString("BothSides") : ssprintf("OneSideP%d",int(p+1)) ); }
CString COMBO_X_NAME( size_t p, size_t both_sides )			{ return "ComboXOffset" + (both_sides ? CString("BothSides") : ssprintf("OneSideP%d",int(p+1)) ); }
CString ATTACK_DISPLAY_X_NAME( size_t p, size_t both_sides ){ return "AttackDisplayXOffset" + (both_sides ? CString("BothSides") : ssprintf("OneSideP%d",int(p+1)) ); }

/* Distance to search for a note in Step(), in seconds. */
static const float StepSearchDistance = 1.0f;

enum HoldWindow { HW_OK, HW_Roll };
enum TapWindow { TW_Marvelous, TW_Perfect, TW_Great, TW_Good, TW_Boo, TW_Mine, TW_Attack };


float AdjustedWindowTap( TapWindow tw, float fTimingScale, bool bIsPlayingBeginner )
{
	float fSecs = 0;
	switch( tw )
	{
	case TW_Marvelous:	fSecs = PREFSMAN->m_fJudgeWindowSecondsMarvelous;	break;
	case TW_Perfect:	fSecs = PREFSMAN->m_fJudgeWindowSecondsPerfect;		break;
	case TW_Great:		fSecs = PREFSMAN->m_fJudgeWindowSecondsGreat;		break;
	case TW_Good:		fSecs = PREFSMAN->m_fJudgeWindowSecondsGood;		break;
	case TW_Boo:		fSecs = PREFSMAN->m_fJudgeWindowSecondsBoo;		break;
	case TW_Mine:		fSecs = PREFSMAN->m_fJudgeWindowSecondsMine;		break;
	case TW_Attack:		fSecs = PREFSMAN->m_fJudgeWindowSecondsAttack;		break;
	default:	ASSERT(0);
	}

	fSecs *= PREFSMAN->m_fJudgeWindowScale;
	fSecs += PREFSMAN->m_fJudgeWindowAdd;

	// apply this last, so it overlays on the above scaling
	fSecs *= fTimingScale;

	if( bIsPlayingBeginner && PREFSMAN->m_bMercifulBeginner && tw==TW_Boo )
		fSecs += 0.5f;
	return fSecs;
}

float AdjustedWindowHold( HoldWindow hw, bool bIsPlayingBeginner )
{
	float fSecs = 0;
	switch( hw )
	{
	case HW_OK:	fSecs = PREFSMAN->m_fJudgeWindowSecondsOK;	break;
	case HW_Roll:	fSecs = PREFSMAN->m_fJudgeWindowSecondsRoll;	break;
	default:	ASSERT(0);
	}

	fSecs *= PREFSMAN->m_fJudgeWindowScale;
	fSecs += PREFSMAN->m_fJudgeWindowAdd;

	return fSecs;
}

#define ADJUSTED_WINDOW_TAP( tw )	AdjustedWindowTap( tw, m_pPlayerState->m_CurrentPlayerOptions.m_fTimingScale, IsPlayingBeginner() )
#define ADJUSTED_WINDOW_HOLD( hw )	AdjustedWindowHold( hw, IsPlayingBeginner() )

Player::Player()
{
	m_pPlayerState = NULL;
	m_pPlayerStageStats = NULL;
	m_fNoteFieldHeight = 0;

	m_pLifeMeter = NULL;
	m_pCombinedLifeMeter = NULL;
	m_pScoreDisplay = NULL;
	m_pSecondaryScoreDisplay = NULL;
	m_pPrimaryScoreKeeper = NULL;
	m_pSecondaryScoreKeeper = NULL;
	m_pInventory = NULL;
	
	m_bPaused = false;
	m_iOffsetSample = 0;

	this->AddChild( &m_Judgment );
	this->AddChild( &m_Combo );
	this->AddChild( &m_AttackDisplay );
	for( int c=0; c<MAX_NOTE_TRACKS; c++ )
		this->AddChild( &m_HoldJudgment[c] );

	PlayerAI::InitFromDisk();

	m_pNoteField = new NoteField;

	this->SubscribeToMessage( MESSAGE_AUTOSYNC_CHANGED );
}

Player::~Player()
{
	delete m_pNoteField;
}

/* Init() does the expensive stuff: load sounds and note skins.  Load() just loads a NoteData. */
void Player::Init(
	const CString &sType,
	PlayerState* pPlayerState, 
	PlayerStageStats* pPlayerStageStats,
	LifeMeter* pLM, 
	CombinedLifeMeter* pCombinedLM, 
	ScoreDisplay* pScoreDisplay, 
	ScoreDisplay* pSecondaryScoreDisplay, 
	Inventory* pInventory, 
	ScoreKeeper* pPrimaryScoreKeeper, 
	ScoreKeeper* pSecondaryScoreKeeper )
{
	GRAY_ARROWS_Y_STANDARD.Load(sType,"ReceptorArrowsYStandard");
	GRAY_ARROWS_Y_REVERSE.Load(sType,"ReceptorArrowsYReverse");
	JUDGMENT_X.Load(sType,JUDGMENT_X_NAME,NUM_PLAYERS,2);
	JUDGMENT_Y.Load(sType,"JudgmentY");
	JUDGMENT_Y_REVERSE.Load(sType,"JudgmentYReverse");
	JUDGMENT_CENTERED_ADDY.Load(sType,"JudgmentCenteredAddY");
	JUDGMENT_CENTERED_ADDY_REVERSE.Load(sType,"JudgmentCenteredAddYReverse");
	COMBO_X.Load(sType,COMBO_X_NAME,NUM_PLAYERS,2);
	COMBO_Y.Load(sType,"ComboY");
	COMBO_Y_REVERSE.Load(sType,"ComboYReverse");
	COMBO_CENTERED_ADDY.Load(sType,"ComboCenteredAddY");
	COMBO_CENTERED_ADDY_REVERSE.Load(sType,"ComboCenteredAddYReverse");
	ATTACK_DISPLAY_X.Load(sType,ATTACK_DISPLAY_X_NAME,NUM_PLAYERS,2);
	ATTACK_DISPLAY_Y.Load(sType,"AttackDisplayY");
	ATTACK_DISPLAY_Y_REVERSE.Load(sType,"AttackDisplayYReverse");
	HOLD_JUDGMENT_Y_STANDARD.Load(sType,"HoldJudgmentYStandard");
	HOLD_JUDGMENT_Y_REVERSE.Load(sType,"HoldJudgmentYReverse");
	BRIGHT_GHOST_COMBO_THRESHOLD.Load(sType,"BrightGhostComboThreshold");
	TAP_JUDGMENTS_UNDER_FIELD.Load(sType,"TapJudgmentsUnderField");
	HOLD_JUDGMENTS_UNDER_FIELD.Load(sType,"HoldJudgmentsUnderField");
	START_DRAWING_AT_PIXELS.Load(sType,"StartDrawingAtPixels");
	STOP_DRAWING_AT_PIXELS.Load(sType,"StopDrawingAtPixels");
	MAX_PRO_TIMING_ERROR.Load(sType,"MaxProTimingError");

	m_pPlayerState = pPlayerState;
	m_pPlayerStageStats = pPlayerStageStats;
	m_pLifeMeter = pLM;
	m_pCombinedLifeMeter = pCombinedLM;
	m_pScoreDisplay = pScoreDisplay;
	m_pSecondaryScoreDisplay = pSecondaryScoreDisplay;
	m_pInventory = pInventory;
	m_pPrimaryScoreKeeper = pPrimaryScoreKeeper;
	m_pSecondaryScoreKeeper = pSecondaryScoreKeeper;

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	m_sMessageToSendOnStep = ssprintf("StepP%d",pn+1);


	m_soundMine.Load( THEME->GetPathS(sType,"mine"), true );

	/* Attacks can be launched in course modes and in battle modes.  They both come
	 * here to play, but allow loading a different sound for different modes. */
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_RAVE:
	case PLAY_MODE_BATTLE:
		m_soundAttackLaunch.Load( THEME->GetPathS(sType,"battle attack launch"), true );
		m_soundAttackEnding.Load( THEME->GetPathS(sType,"battle attack ending"), true );
		break;
	default:
		m_soundAttackLaunch.Load( THEME->GetPathS(sType,"course attack launch"), true );
		m_soundAttackEnding.Load( THEME->GetPathS(sType,"course attack ending"), true );
		break;
	}

	// calculate M-mod speed here, so we can adjust properly on a per-song basis.
	// XXX: can we find a better location for this?
	if( m_pPlayerState->m_StoredPlayerOptions.m_fMaxScrollBPM != 0 )
	{
		DisplayBpms bpms;

		if( GAMESTATE->IsCourseMode() )
		{
			ASSERT( GAMESTATE->m_pCurTrail[pn] );
			GAMESTATE->m_pCurTrail[pn]->GetDisplayBpms( bpms );
		}
		else
		{
			ASSERT( GAMESTATE->m_pCurSong );
			GAMESTATE->m_pCurSong->GetDisplayBpms( bpms );
		}

		float fMaxBPM = 0;

		// all BPMs are listed and available, so try them first.
		// get the maximum listed value for the song or course.
		// if the BPMs are < 0, reset and get the actual values.
		if( !bpms.IsSecret() )
		{
			fMaxBPM = bpms.GetMax();
			fMaxBPM = max( 0, fMaxBPM );
		}

		// we can't rely on the displayed BPMs, so manually calculate.
		if( fMaxBPM == 0 )
		{
			float fThrowAway = 0;

			if( GAMESTATE->IsCourseMode() )
			{
				FOREACH_CONST( TrailEntry, GAMESTATE->m_pCurTrail[pn]->m_vEntries, e )
				{
					float fMaxForEntry;
					e->pSong->m_Timing.GetActualBPM( fThrowAway, fMaxForEntry );
					fMaxBPM = max( fMaxForEntry, fMaxBPM );
				}
			}
			else
			{
				GAMESTATE->m_pCurSong->m_Timing.GetActualBPM( fThrowAway, fMaxBPM );
			}
		}

		ASSERT( fMaxBPM > 0 );

		// set an X-mod equal to Mnum / fMaxBPM (e.g. M600 with 150 becomes 4x)
		m_pPlayerState->m_StoredPlayerOptions.m_fScrollSpeed =
			( m_pPlayerState->m_StoredPlayerOptions.m_fMaxScrollBPM / fMaxBPM );
	}

	RageSoundParams p;
	GameSoundManager::SetPlayerBalance( pn, p );

	m_soundMine.SetParams( p );
	m_soundAttackLaunch.SetParams( p );
	m_soundAttackEnding.SetParams( p );

	m_Combo.SetName( "Combo" );
	m_Combo.Load( m_pPlayerState, m_pPlayerStageStats );
	ActorUtil::OnCommand( m_Combo, sType );

	m_Judgment.SetName( "Judgment" );
	m_Judgment.Load( IsPlayingBeginner() );
	ActorUtil::OnCommand( m_Judgment, sType );

	m_fNoteFieldHeight = GRAY_ARROWS_Y_REVERSE-GRAY_ARROWS_Y_STANDARD;
	m_pNoteField->Init( m_pPlayerState, m_fNoteFieldHeight );
}

void Player::Load( const NoteData& noteData )
{
	m_LastTapNoteScore = TNS_NONE;

	m_iRowLastCrossed = BeatToNoteRowNotRounded( GAMESTATE->m_fSongBeat ) - 1;	// why this?
	m_iMineRowLastCrossed = BeatToNoteRowNotRounded( GAMESTATE->m_fSongBeat ) - 1;	// why this?

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	GAMESTATE->ResetNoteSkinsForPlayer( pn );

	// init steps
	m_NoteData.Init();
	bool bOniDead = GAMESTATE->m_SongOptions.m_LifeType == SongOptions::LIFE_BATTERY  &&  
		(m_pPlayerStageStats == NULL || m_pPlayerStageStats->bFailed);
	if( !bOniDead )
		m_NoteData.CopyAll( noteData );

	/* The editor reuses Players ... so we really need to make sure everything
	 * is reset and not tweening.  Perhaps ActorFrame should recurse to subactors;
	 * then we could just this->StopTweening()? -glenn */
	m_Judgment.StopTweening();
//	m_Combo.Reset();				// don't reset combos between songs in a course!
	if( m_pPlayerStageStats )
		m_Combo.SetCombo( m_pPlayerStageStats->iCurCombo, m_pPlayerStageStats->iCurMissCombo );	// combo can persist between songs and games
	m_AttackDisplay.Init( m_pPlayerState );
	m_Judgment.Reset();

	/* Don't re-init this; that'll reload graphics.  Add a separate Reset() call
	 * if some ScoreDisplays need it. */
//	if( m_pScore )
//		m_pScore->Init( pn );

	/* Apply transforms. */
	NoteDataUtil::TransformNoteData( m_NoteData, GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions, GAMESTATE->GetCurrentStyle()->m_StepsType );
	
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_RAVE:
	case PLAY_MODE_BATTLE:
		{
			// ugly, ugly, ugly.  Works only w/ dance.
			NoteDataUtil::TransformNoteData( m_NoteData, GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions, GAMESTATE->GetCurrentStyle()->m_StepsType );
			
			// shuffle either p1 or p2
			static int count = 0;
			switch( count )
			{
			case 0:
			case 3:
				NoteDataUtil::Turn( m_NoteData, STEPS_TYPE_DANCE_SINGLE, NoteDataUtil::left);
				break;
			case 1:
			case 2:
				NoteDataUtil::Turn( m_NoteData, STEPS_TYPE_DANCE_SINGLE, NoteDataUtil::right);
				break;
			default:
				ASSERT(0);
			}
			count++;
			count %= 4;
		}
		break;
	}

	int iStartDrawingAtPixels = GAMESTATE->m_bEditing ? -100 : START_DRAWING_AT_PIXELS;
	int iStopDrawingAtPixels = GAMESTATE->m_bEditing ? 400 : STOP_DRAWING_AT_PIXELS;

	float fNoteFieldMiddle = (GRAY_ARROWS_Y_STANDARD+GRAY_ARROWS_Y_REVERSE)/2;
	
	if( m_pNoteField )
		m_pNoteField->SetY( fNoteFieldMiddle );
	m_pNoteField->Load( &m_NoteData, iStartDrawingAtPixels, iStopDrawingAtPixels );

	const bool bReverse = GAMESTATE->m_pPlayerState[pn]->m_PlayerOptions.GetReversePercentForColumn(0) == 1;
	bool bPlayerUsingBothSides = GAMESTATE->GetCurrentStyle()->m_StyleType==ONE_PLAYER_TWO_SIDES;
	m_Combo.SetX( COMBO_X.GetValue(pn,bPlayerUsingBothSides) );
	m_Combo.SetY( bReverse ? COMBO_Y_REVERSE : COMBO_Y );
	m_AttackDisplay.SetX( ATTACK_DISPLAY_X.GetValue(pn,bPlayerUsingBothSides) - 40 );
	// set this in Update //m_AttackDisplay.SetY( bReverse ? ATTACK_DISPLAY_Y_REVERSE : ATTACK_DISPLAY_Y );
	m_Judgment.SetX( JUDGMENT_X.GetValue(pn,bPlayerUsingBothSides) );
	// set this in Update //m_Judgment.SetY( bReverse ? JUDGMENT_Y_REVERSE : JUDGMENT_Y );

	// Need to set Y positions of all these elements in Update since
	// they change depending on PlayerOptions.

	//
	// Load keysounds.  If sounds are already loaded (as in the editor), don't reload them.
	// XXX: the editor will load several duplicate copies (in each NoteField), and each
	// player will load duplicate sounds.  Does this belong somewhere else (perhaps in
	// a separate object, used alongside ScreenGameplay::m_pSoundMusic and ScreenEdit::m_pSoundMusic?)
	// We don't have to load separate copies to set player fade: always make a copy, and set the
	// fade on the copy.
	//
	const Song* pSong = GAMESTATE->m_pCurSong;
	CString sSongDir = pSong->GetSongDir();
	m_vKeysounds.resize( pSong->m_vsKeysoundFile.size() );

	RageSoundParams p;
	GameSoundManager::SetPlayerBalance( pn, p );
	for( unsigned i=0; i<m_vKeysounds.size(); i++ )
	{
		 CString sKeysoundFilePath = sSongDir + pSong->m_vsKeysoundFile[i];
		 RageSound& sound = m_vKeysounds[i];
		 if( sound.GetLoadedFilePath() != sKeysoundFilePath )
			sound.Load( sKeysoundFilePath, true );
		 sound.SetParams( p );
	}
}

void Player::Update( float fDeltaTime )
{
	//LOG->Trace( "Player::Update(%f)", fDeltaTime );

	if( GAMESTATE->m_pCurSong==NULL )
		return;

	ActorFrame::Update( fDeltaTime );

	if( m_pPlayerState->m_bAttackBeganThisUpdate )
		m_soundAttackLaunch.Play();
	if( m_pPlayerState->m_bAttackEndedThisUpdate )
		m_soundAttackEnding.Play();


	const float fSongBeat = GAMESTATE->m_fSongBeat;
	const int iSongRow = BeatToNoteRow( fSongBeat );

	if( m_pNoteField )
		m_pNoteField->Update( fDeltaTime );

	//
	// Update Y positions
	//
	{
		for( int c=0; c<GAMESTATE->GetCurrentStyle()->m_iColsPerPlayer; c++ )
		{
			float fPercentReverse = m_pPlayerState->m_CurrentPlayerOptions.GetReversePercentForColumn(c);
			float fHoldJudgeYPos = SCALE( fPercentReverse, 0.f, 1.f, HOLD_JUDGMENT_Y_STANDARD, HOLD_JUDGMENT_Y_REVERSE );
//			float fGrayYPos = SCALE( fPercentReverse, 0.f, 1.f, GRAY_ARROWS_Y_STANDARD, GRAY_ARROWS_Y_REVERSE );

			const float fX = ArrowEffects::GetXPos( m_pPlayerState, c, 0 );
			const float fZ = ArrowEffects::GetZPos( m_pPlayerState, c, 0 );

			m_HoldJudgment[c].SetX( fX );
			m_HoldJudgment[c].SetY( fHoldJudgeYPos );
			m_HoldJudgment[c].SetZ( fZ );
		}
	}

	// NoteField accounts for reverse on its own now.
	//if( m_pNoteField )
	//	m_pNoteField->SetY( fGrayYPos );

	const bool bReverse = m_pPlayerState->m_PlayerOptions.GetReversePercentForColumn(0) == 1;
	float fPercentCentered = m_pPlayerState->m_CurrentPlayerOptions.m_fScrolls[PlayerOptions::SCROLL_CENTERED];
	m_Combo.SetY( 
		bReverse ? 
		COMBO_Y_REVERSE + fPercentCentered * COMBO_CENTERED_ADDY_REVERSE : 
		COMBO_Y + fPercentCentered * COMBO_CENTERED_ADDY );
	m_Judgment.SetY(
		bReverse ? 
		JUDGMENT_Y_REVERSE + fPercentCentered * JUDGMENT_CENTERED_ADDY_REVERSE : 
		JUDGMENT_Y + fPercentCentered * JUDGMENT_CENTERED_ADDY );


	float fMiniPercent = m_pPlayerState->m_CurrentPlayerOptions.m_fEffects[PlayerOptions::EFFECT_MINI];
	float fNoteFieldZoom = 1 - fMiniPercent*0.5f;
	float fJudgmentZoom = 1 - fMiniPercent*0.25f;
	if( m_pNoteField )
		m_pNoteField->SetZoom( fNoteFieldZoom );
	m_Judgment.SetZoom( fJudgmentZoom );

	// If we're paused, don't update tap or hold note logic, so hold notes can be released
	// during pause.
	if( m_bPaused )
		return;

	//
	// Check for TapNote misses
	//
	UpdateTapNotesMissedOlderThan( GetMaxStepDistanceSeconds() );

	//
	// update pressed flag
	//
	const int iNumCols = GAMESTATE->GetCurrentStyle()->m_iColsPerPlayer;
	ASSERT_M( iNumCols < MAX_COLS_PER_PLAYER, ssprintf("%i >= %i", iNumCols, MAX_COLS_PER_PLAYER) );
	for( int col=0; col < iNumCols; ++col )
	{
		// TODO: Remove use of PlayerNumber.
		PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

		CHECKPOINT_M( ssprintf("%i %i", col, iNumCols) );
		const StyleInput StyleI( pn, col );
		const GameInput GameI = GAMESTATE->GetCurrentStyle()->StyleInputToGameInput( StyleI );
		bool bIsHoldingButton = INPUTMAPPER->IsButtonDown( GameI );
		// TODO: Make this work for non-human-controlled players
		if( bIsHoldingButton && !GAMESTATE->m_bDemonstrationOrJukebox && m_pPlayerState->m_PlayerController==PC_HUMAN )
			if( m_pNoteField )
				m_pNoteField->SetPressed( col );
	}
	

	//
	// handle Autoplay for rolls
	//
	if( m_pPlayerState->m_PlayerController != PC_HUMAN )
	{
		for( int iTrack=0; iTrack<m_NoteData.GetNumTracks(); ++iTrack )
		{
			// TODO: Make the CPU miss sometimes.
			int iHeadRow;
			if( !m_NoteData.IsHoldNoteAtBeat(iTrack, iSongRow, &iHeadRow) )
				continue;

			const TapNote &tn = m_NoteData.GetTapNote( iTrack, iHeadRow );
			if( tn.type != TapNote::hold_head || tn.subType != TapNote::hold_head_roll )
				continue;
			if( tn.HoldResult.hns != HNS_NONE )
				continue;
			if( tn.HoldResult.fLife >= 0.5f )
				continue;

			RageTimer now;
			Step( iTrack, now, false );
		}
	}


	//
	// update HoldNotes logic
	//
	for( int iTrack=0; iTrack<m_NoteData.GetNumTracks(); ++iTrack )
	{
		// Since this is being called every frame, let's not check the whole array every time.
		// Instead, only check 1 beat back.  Even 1 is overkill.
		const int iStartCheckingAt = max( 0, iSongRow-BeatToNoteRow(1) );
		NoteData::iterator begin, end;
		m_NoteData.GetTapNoteRangeInclusive( iTrack, iStartCheckingAt, iSongRow+1, begin, end );
		for( ; begin != end; ++begin )
		{
			TapNote &tn = begin->second;
			if( tn.type != TapNote::hold_head )
				continue;
			int iRow = begin->first;

			// set hold flags so NoteField can do intelligent drawing
			tn.HoldResult.bHeld = false;
			tn.HoldResult.bActive = false;

			HoldNoteScore hns = tn.HoldResult.hns;
			if( hns != HNS_NONE )	// if this HoldNote already has a result
				continue;	// we don't need to update the logic for this one

			// TODO: Remove use of PlayerNumber.
			PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

			// if they got a bad score or haven't stepped on the corresponding tap yet
			const TapNoteScore tns = tn.result.tns;
			const bool bSteppedOnTapNote = tns != TNS_NONE  &&  tns != TNS_MISS;	// did they step on the start of this hold?

			bool bIsHoldingButton = false;
			if( m_pPlayerState->m_PlayerController != PC_HUMAN )
			{
				// TODO: Make the CPU miss sometimes.
				bIsHoldingButton = true;
			}
			else
			{
				const StyleInput StyleI( pn, iTrack );
				const GameInput GameI = GAMESTATE->GetCurrentStyle()->StyleInputToGameInput( StyleI );
				bIsHoldingButton = INPUTMAPPER->IsButtonDown( GameI );
			}

			int iEndRow = iRow + tn.iDuration;

			float fLife = tn.HoldResult.fLife;
			if( bSteppedOnTapNote && fLife != 0 )
			{
				/* This hold note is not judged and we stepped on its head.  Update iLastHeldRow.
				 * Do this even if we're a little beyond the end of the hold note, to make sure
				 * iLastHeldRow is clamped to iEndRow if the hold note is held all the way. */
				tn.HoldResult.iLastHeldRow = min( iSongRow, iEndRow );
			}

			// If the song beat is in the range of this hold:
			if( iRow <= iSongRow && iRow <= iEndRow )
			{
				switch( tn.subType )
				{
				case TapNote::hold_head_hold:
					// set hold flag so NoteField can do intelligent drawing
					tn.HoldResult.bHeld = bIsHoldingButton && bSteppedOnTapNote;
					tn.HoldResult.bActive = bSteppedOnTapNote;

					if( bSteppedOnTapNote && bIsHoldingButton )
					{
						// Increase life
						fLife = 1;
					}
					else
					{
						// Decrease life
						fLife -= fDeltaTime/ADJUSTED_WINDOW_HOLD(HW_OK);
						fLife = max( fLife, 0 );	// clamp
					}
					break;
				case TapNote::hold_head_roll:
					tn.HoldResult.bHeld = true;
					tn.HoldResult.bActive = bSteppedOnTapNote;

					// give positive life in Step(), not here.

					// Decrease life
					fLife -= fDeltaTime/ADJUSTED_WINDOW_HOLD(HW_Roll);
					fLife = max( fLife, 0 );	// clamp
					break;
				default:
					ASSERT(0);
				}
			}

			/* check for NG.  If the head was missed completely, don't count an NG. */
			if( bSteppedOnTapNote && fLife == 0 )	// the player has not pressed the button for a long time!
				hns = HNS_NG;

			// check for OK
			if( iSongRow >= iEndRow && bSteppedOnTapNote && fLife > 0 )	// if this HoldNote is in the past
			{
				fLife = 1;
				hns = HNS_OK;
				bool bBright = m_pPlayerStageStats && m_pPlayerStageStats->iCurCombo>(int)BRIGHT_GHOST_COMBO_THRESHOLD;
				if( m_pNoteField )
					m_pNoteField->DidHoldNote( iTrack, HNS_OK, bBright );	// bright ghost flash
			}

			if( hns != HNS_NONE )
			{
				/* this note has been judged */
				HandleHoldScore( hns, tns );
				m_HoldJudgment[iTrack].SetHoldJudgment( hns );

				int ms_error = (hns == HNS_OK)? 0:MAX_PRO_TIMING_ERROR;

				if( m_pPlayerStageStats )
					m_pPlayerStageStats->iTotalError += ms_error;
			}

			tn.HoldResult.fLife = fLife;
			tn.HoldResult.hns = hns;
		}
	}

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	{
		// Why was this originally "BeatToNoteRowNotRounded"?  It should be rounded.  -Chris
		/* We want to send the crossed row message exactly when we cross the row--not
		 * .5 before the row.  Use a very slow song (around 2 BPM) as a test case: without
		 * rounding, autoplay steps early. -glenn */
		const int iRowNow = BeatToNoteRowNotRounded( GAMESTATE->m_fSongBeat );
		if( iRowNow >= 0 )
		{
			// for each index we crossed since the last update
			if( GAMESTATE->IsPlayerEnabled(pn) )
				FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( m_NoteData, r, m_iRowLastCrossed, iRowNow+1 )
					CrossedRow( r );
			m_iRowLastCrossed = iRowNow+1;
		}
	}

	{
		// TRICKY: 
		float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
		fPositionSeconds -= PREFSMAN->m_fPadStickSeconds;
		const float fSongBeat = GAMESTATE->m_pCurSong ? GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds ) : 0;
		const int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
		if( iRowNow >= 0 )
		{
			// for each index we crossed since the last update
			if( GAMESTATE->IsPlayerEnabled(pn) )
				FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( m_NoteData, r, m_iMineRowLastCrossed, iRowNow+1 )
					CrossedMineRow( r );
			m_iMineRowLastCrossed = iRowNow+1;
		}
	}

	// process transforms that are waiting to be applied
	ApplyWaitingTransforms();
}

void Player::ApplyWaitingTransforms()
{
	for( unsigned j=0; j<m_pPlayerState->m_ModsToApply.size(); j++ )
	{
		const Attack &mod = m_pPlayerState->m_ModsToApply[j];
		PlayerOptions po;
		/* Should this default to "" always? need it blank so we know if mod.sModifier
		 * changes the note skin. */
		po.m_sNoteSkin = "";
		po.FromString( mod.sModifiers );

		float fStartBeat, fEndBeat;
		mod.GetAttackBeats( GAMESTATE->m_pCurSong, m_pPlayerState, fStartBeat, fEndBeat );
		fEndBeat = min( fEndBeat, m_NoteData.GetLastBeat() );

		LOG->Trace( "Applying transform '%s' from %f to %f to '%s'", mod.sModifiers.c_str(), fStartBeat, fEndBeat,
			GAMESTATE->m_pCurSong->GetTranslitMainTitle().c_str() );
		if( po.m_sNoteSkin != "" )
			GAMESTATE->SetNoteSkinForBeatRange( m_pPlayerState, po.m_sNoteSkin, fStartBeat, fEndBeat );

		NoteDataUtil::TransformNoteData( m_NoteData, po, GAMESTATE->GetCurrentStyle()->m_StepsType, BeatToNoteRow(fStartBeat), BeatToNoteRow(fEndBeat) );
	}
	m_pPlayerState->m_ModsToApply.clear();

	// Cache used NoteSkins now, not on the next update.
	m_pNoteField->RefreshBeatToNoteSkin();
}

void Player::DrawPrimitives()
{
	if( m_pNoteField == NULL )
		return;

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	// May have both players in doubles (for battle play); only draw primary player.
	if( GAMESTATE->GetCurrentStyle()->m_StyleType == ONE_PLAYER_TWO_SIDES  &&
		pn != GAMESTATE->m_MasterPlayerNumber )
		return;


	// Draw these below everything else.
	if( m_pPlayerState->m_PlayerOptions.m_fBlind == 0 )
		m_Combo.Draw();

	m_AttackDisplay.Draw();

	if( TAP_JUDGMENTS_UNDER_FIELD )
		DrawTapJudgments();

	if( HOLD_JUDGMENTS_UNDER_FIELD )
		DrawHoldJudgments();

	float fTilt = m_pPlayerState->m_CurrentPlayerOptions.m_fPerspectiveTilt;
	float fSkew = m_pPlayerState->m_CurrentPlayerOptions.m_fSkew;
	bool bReverse = m_pPlayerState->m_CurrentPlayerOptions.GetReversePercentForColumn(0)>0.5;


	DISPLAY->CameraPushMatrix();
	DISPLAY->PushMatrix();

	float fCenterY = this->GetY()+(GRAY_ARROWS_Y_STANDARD+GRAY_ARROWS_Y_REVERSE)/2;

	DISPLAY->LoadMenuPerspective( 45, SCALE(fSkew,0.f,1.f,this->GetX(),SCREEN_CENTER_X), fCenterY );

	float fOriginalY = 	m_pNoteField->GetY();

	float fTiltDegrees = SCALE(fTilt,-1.f,+1.f,+30,-30) * (bReverse?-1:1);


	float fZoom = SCALE( m_pPlayerState->m_CurrentPlayerOptions.m_fEffects[PlayerOptions::EFFECT_MINI], 0.f, 1.f, 1.f, 0.5f );
	if( fTilt > 0 )
		fZoom *= SCALE( fTilt, 0.f, 1.f, 1.f, 0.9f );
	else
		fZoom *= SCALE( fTilt, 0.f, -1.f, 1.f, 0.9f );

	float fYOffset;
	if( fTilt > 0 )
		fYOffset = SCALE( fTilt, 0.f, 1.f, 0.f, -45.f ) * (bReverse?-1:1);
	else
		fYOffset = SCALE( fTilt, 0.f, -1.f, 0.f, -20.f ) * (bReverse?-1:1);

	m_pNoteField->SetY( fOriginalY + fYOffset );
	m_pNoteField->SetZoom( fZoom );
	m_pNoteField->SetRotationX( fTiltDegrees );
	m_pNoteField->Draw();

	m_pNoteField->SetY( fOriginalY );

	DISPLAY->CameraPopMatrix();
	DISPLAY->PopMatrix();


	if( !(bool)TAP_JUDGMENTS_UNDER_FIELD )
		DrawTapJudgments();

	if( !(bool)TAP_JUDGMENTS_UNDER_FIELD )
		DrawHoldJudgments();
}

void Player::DrawTapJudgments()
{
	if( m_pPlayerState->m_PlayerOptions.m_fBlind > 0 )
		return;

	m_Judgment.Draw();
}

void Player::DrawHoldJudgments()
{
	if( m_pPlayerState->m_PlayerOptions.m_fBlind > 0 )
		return;

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	for( int c=0; c<m_NoteData.GetNumTracks(); c++ )
	{
		NoteFieldMode::BeginDrawTrack( pn, c );

		m_HoldJudgment[c].Draw();

		NoteFieldMode::EndDrawTrack(c);
	}
}

int Player::GetClosestNoteDirectional( int col, int iStartRow, int iEndRow, bool bAllowGraded, bool bForward ) const
{
	NoteData::const_iterator begin, end;
	m_NoteData.GetTapNoteRange( col, iStartRow, iEndRow, begin, end );

	if( !bForward )
		swap( begin, end );

	while( begin != end )
	{
		if( !bForward )
			--begin;

		/* Is this the row we want? */
		do {
			const TapNote &tn = begin->second;
			if( tn.type == TapNote::empty )
				break;
			if( !bAllowGraded && tn.result.tns != TNS_NONE )
				break;

			return begin->first;
		} while(0);

		if( bForward )
			++begin;
	}

	return -1;
}

/* Find the closest note to fBeat. */
int Player::GetClosestNote( int col, int iNoteRow, int iMaxRowsAhead, int iMaxRowsBehind, bool bAllowGraded ) const
{
	// Start at iIndexStartLookingAt and search outward.
	int iNextIndex = GetClosestNoteDirectional( col, iNoteRow, iNoteRow+iMaxRowsAhead, bAllowGraded, true );
	int iPrevIndex = GetClosestNoteDirectional( col, iNoteRow-iMaxRowsBehind, iNoteRow, bAllowGraded, false );

	if( iNextIndex == -1 && iPrevIndex == -1 )
		return -1;
	if( iNextIndex == -1 )
		return iPrevIndex;
	if( iPrevIndex == -1 )
		return iNextIndex;

	/* Figure out which row is closer. */
	if( abs(iNoteRow-iNextIndex) > abs(iNoteRow-iPrevIndex) )
		return iPrevIndex;
	else
		return iNextIndex;
}


void Player::Step( int col, const RageTimer &tm, bool bHeld )
{
	bool bOniDead = 
		GAMESTATE->m_SongOptions.m_LifeType == SongOptions::LIFE_BATTERY  &&  
		m_pPlayerStageStats  && 
		m_pPlayerStageStats->bFailed;
	if( bOniDead )
		return;	// do nothing

	HandleStep( col, tm, bHeld );

	MESSAGEMAN->Broadcast( m_sMessageToSendOnStep );	
}

void Player::HandleStep( int col, const RageTimer &tm, bool bHeld )
{
	//LOG->Trace( "Player::HandlePlayerStep()" );

	ASSERT_M( col >= 0  &&  col <= m_NoteData.GetNumTracks(), ssprintf("%i, %i", col, m_NoteData.GetNumTracks()) );

	//
	// Count calories for this step, unless we're being called because a button is
	// held over a mine.
	//
	if( m_pPlayerStageStats && m_pPlayerState && !bHeld )
	{
		// TODO: remove use of PlayerNumber
		PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
		Profile* pProfile = PROFILEMAN->GetProfile(pn);
		if( pProfile )
		{
			int iNumTracksHeld = 0;
			for( int t=0; t<m_NoteData.GetNumTracks(); t++ )
			{
				const StyleInput StyleI( pn, t );
				const GameInput GameI = GAMESTATE->GetCurrentStyle()->StyleInputToGameInput( StyleI );
				float fSecsHeld = INPUTMAPPER->GetSecsHeld( GameI );
				if( fSecsHeld > 0 )
					iNumTracksHeld++;
			}

			float fCals = 0;
			switch( iNumTracksHeld )
			{
			case 0:
				// autoplay is on, or this is a computer player
				iNumTracksHeld = 1;
				// fall through
			default:
				{
					float fCalsFor100Lbs = SCALE( iNumTracksHeld, 1, 2, 0.029f, 0.193f );
					float fCalsFor200Lbs = SCALE( iNumTracksHeld, 1, 2, 0.052f, 0.334f );
					fCals = SCALE( pProfile->GetCalculatedWeightPounds(), 100.f, 200.f, fCalsFor100Lbs, fCalsFor200Lbs );
				}
				break;
			}

			m_pPlayerStageStats->fCaloriesBurned += fCals;
		}
	}


	float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
	fPositionSeconds -= tm.Ago();
	const float fSongBeat = GAMESTATE->m_pCurSong ? GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds ) : GAMESTATE->m_fSongBeat;



	//
	// Check for step on a TapNote
	//
	const int iStepSearchRows = BeatToNoteRow( StepSearchDistance * GAMESTATE->m_fCurBPS * GAMESTATE->m_SongOptions.m_fMusicRate );
	int iIndexOverlappingNote = GetClosestNote( col, BeatToNoteRow(fSongBeat), 
						   iStepSearchRows, iStepSearchRows, false );
	
	// calculate TapNoteScore
	TapNoteScore score = TNS_NONE;

	if( iIndexOverlappingNote != -1 )
	{
		// compute the score for this hit
		float fNoteOffset;
		{
			const float fStepBeat = NoteRowToBeat( iIndexOverlappingNote );
			const float fStepSeconds = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat(fStepBeat);

			/* We actually stepped on the note this long ago: */
			const float fTimeSinceStep = tm.Ago();

			/* GAMESTATE->m_fMusicSeconds is the music time as of GAMESTATE->m_LastBeatUpdate. Figure
			 * out what the music time is as of now. */
			const float fCurrentMusicSeconds = GAMESTATE->m_fMusicSeconds + (GAMESTATE->m_LastBeatUpdate.Ago()*GAMESTATE->m_SongOptions.m_fMusicRate);

			/* ... which means it happened at this point in the music: */
			const float fMusicSeconds = fCurrentMusicSeconds - fTimeSinceStep * GAMESTATE->m_SongOptions.m_fMusicRate;

			// The offset from the actual step in seconds:
			fNoteOffset = (fStepSeconds - fMusicSeconds) / GAMESTATE->m_SongOptions.m_fMusicRate;	// account for music rate
//			LOG->Trace("step was %.3f ago, music is off by %f: %f vs %f, step was %f off", 
//				fTimeSinceStep, GAMESTATE->m_LastBeatUpdate.Ago()/GAMESTATE->m_SongOptions.m_fMusicRate,
//				fStepSeconds, fMusicSeconds, fNoteOffset );
		}

		const float fSecondsFromPerfect = fabsf( fNoteOffset );


		TapNote tn = m_NoteData.GetTapNote( col, iIndexOverlappingNote );

		switch( m_pPlayerState->m_PlayerController )
		{
		case PC_HUMAN:

			switch( tn.type )
			{
			case TapNote::mine:
				// stepped too close to mine?
				if( fSecondsFromPerfect <= ADJUSTED_WINDOW_TAP(TW_Mine) )
					score = TNS_HIT_MINE;
				break;

			case TapNote::attack:
				if( fSecondsFromPerfect <= ADJUSTED_WINDOW_TAP(TW_Attack) && !tn.result.bHidden )
					score = TNS_PERFECT; /* sentinel */
				break;
			default:
				if(		 fSecondsFromPerfect <= ADJUSTED_WINDOW_TAP(TW_Marvelous) )	score = TNS_MARVELOUS;
				else if( fSecondsFromPerfect <= ADJUSTED_WINDOW_TAP(TW_Perfect) )	score = TNS_PERFECT;
				else if( fSecondsFromPerfect <= ADJUSTED_WINDOW_TAP(TW_Great) )		score = TNS_GREAT;
				else if( fSecondsFromPerfect <= ADJUSTED_WINDOW_TAP(TW_Good) )		score = TNS_GOOD;
				else if( fSecondsFromPerfect <= ADJUSTED_WINDOW_TAP(TW_Boo) )		score = TNS_BOO;
				else	score = TNS_NONE;
				break;
			}
			break;
		
		case PC_CPU:
		case PC_AUTOPLAY:
			score = PlayerAI::GetTapNoteScore( m_pPlayerState );

			// GetTapNoteScore always returns TNS_MARVELOUS in autoplay.
			// If the step is far away, don't judge it.

			if( fSecondsFromPerfect > ADJUSTED_WINDOW_TAP(TW_Boo) )
				score = TNS_NONE;

			// TRICKY:  We're asking the AI to judge mines.  consider TNS_GOOD and below
			// as "mine was hit" and everything else as "mine was avoided"
			/* Make sure we don't execute this code if there's no score. -- Vyhd */
			if( tn.type == TapNote::mine && score != TNS_NONE )
			{
				// The CPU hits a lot of mines.  Only consider hitting the 
				// first mine for a row.  We know we're the first mine if 
				// there are are no mines to the left of us.
				for( int t=0; t<col; t++ )
				{
					if( m_NoteData.GetTapNote(t,iIndexOverlappingNote).type == TapNote::mine )	// there's a mine to the left of us
						return;	// avoid
				}

				// The CPU hits a lot of mines.  Make it less likely to hit 
				// mines that don't have a tap note on the same row.
				bool bTapsOnRow = m_NoteData.IsThereATapOrHoldHeadAtRow( iIndexOverlappingNote );
				TapNoteScore get_to_avoid = bTapsOnRow ? TNS_GREAT : TNS_GOOD;

				if( score >= get_to_avoid )
					return;	// avoided
				else
					score = TNS_HIT_MINE;
			}

			if( tn.type == TapNote::attack && score > TNS_GOOD )
				score = TNS_PERFECT; /* sentinel */

			/* AI will generate misses here.  Don't handle a miss like a regular note because
			 * we want the judgment animation to appear delayed.  Instead, return early if
			 * AI generated a miss, and let UpdateMissedTapNotesOlderThan() detect and handle the 
			 * misses. */
			if( score == TNS_MISS )
				return;

			// Put some small, random amount in fNoteOffset so that demonstration 
			// show a mix of late and early instead of always late.
			fNoteOffset = randomf( -0.1f, 0.1f );

			break;

		default:
			ASSERT(0);
			score = TNS_NONE;
			break;
		}

		if( tn.type == TapNote::attack && score == TNS_PERFECT )
		{
			score = TNS_NONE;	// don't score this as anything

			m_soundAttackLaunch.Play();

			// put attack in effect
			Attack attack(
				ATTACK_LEVEL_1,
				-1,	// now
				tn.fAttackDurationSeconds,
				tn.sAttackModifiers,
				true,
				false
				);

			// TODO: Remove use of PlayerNumber
			PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

			GAMESTATE->LaunchAttack( OPPOSITE_PLAYER[pn], attack );

			// remove all TapAttacks on this row
			for( int t=0; t<m_NoteData.GetNumTracks(); t++ )
			{
				TapNote tn = m_NoteData.GetTapNote(t, iIndexOverlappingNote);
				if( tn.type == TapNote::attack )
				{
					tn.result.bHidden = true;
					m_NoteData.SetTapNote( col, iIndexOverlappingNote, tn );
				}
			}
		}

		if( score == TNS_HIT_MINE )
		{
			if( tn.bKeysound && tn.iKeysoundIndex < (int) m_vKeysounds.size() )
				m_vKeysounds[tn.iKeysoundIndex].Play();
			else
				m_soundMine.Play();

			if( m_pLifeMeter )
				m_pLifeMeter->ChangeLife( score );
			if( m_pScoreDisplay )
				m_pScoreDisplay->OnJudgment( score );
			if( m_pSecondaryScoreDisplay )
				m_pSecondaryScoreDisplay->OnJudgment( score );
			// TODO: Remove use of PlayerNumber
			PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
			if( m_pCombinedLifeMeter )
				m_pCombinedLifeMeter->ChangeLife( pn, score );

			tn.result.bHidden = true;
			m_NoteData.SetTapNote( col, iIndexOverlappingNote, tn );
			if( m_pNoteField )
				m_pNoteField->DidTapNote( col, score, false );
		}

		if( m_pPlayerState->m_PlayerController == PC_HUMAN && score >= TNS_GREAT ) 
			HandleAutosync( fNoteOffset );

		// Do game-specific and mode-specific score mapping.
		score = GAMESTATE->GetCurrentGame()->MapTapNoteScore( score );
		if( score == TNS_MARVELOUS && !GAMESTATE->ShowMarvelous() )
			score = TNS_PERFECT;

		bool bSteppedEarly = -fNoteOffset < 0;
		if( IsPlayingBeginner() && PREFSMAN->m_bMercifulBeginner && score==TNS_BOO && bSteppedEarly )
		{
			m_Judgment.SetJudgment( score, bSteppedEarly );
		}
		else
		{
			tn.result.tns = score;

			if( score != TNS_NONE )
				tn.result.fTapNoteOffset = -fNoteOffset;

			if( score != TNS_NONE && score != TNS_MISS )
			{
				int ms_error = (int) roundf( fSecondsFromPerfect * 1000 );
				ms_error = min( ms_error, MAX_PRO_TIMING_ERROR.GetValue() );

				if( m_pPlayerStageStats )
					m_pPlayerStageStats->iTotalError += ms_error;
			}

			//LOG->Trace("XXX: %i col %i, at %f, music at %f, step was at %f, off by %f",
			//	score, col, fStepSeconds, fCurrentMusicSeconds, fMusicSeconds, fNoteOffset );
	//		LOG->Trace("Note offset: %f (fSecondsFromPerfect = %f), Score: %i", fNoteOffset, fSecondsFromPerfect, score);
			
			m_NoteData.SetTapNote( col, iIndexOverlappingNote, tn );


			// TODO: Remove use of PlayerNumber.
			PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

			// Keep this here so we get the same data as Autosync
			NSMAN->ReportTiming( fNoteOffset,pn );
			
			if( m_pPrimaryScoreKeeper )
				m_pPrimaryScoreKeeper->HandleTapScore( score );
			if( m_pSecondaryScoreKeeper )
				m_pSecondaryScoreKeeper->HandleTapScore( score );

			switch( tn.type )
			{
			case TapNote::tap:
			case TapNote::hold_head:
				// don't judge the row if this note is a mine or tap attack
				if( NoteDataWithScoring::IsRowCompletelyJudged( m_NoteData, iIndexOverlappingNote ) )
					OnRowCompletelyJudged( iIndexOverlappingNote, STATSMAN->m_CurStageStats.fStepsSeconds );
			}

			m_LastTapNoteScore = score;
		}
	}



	if( m_pNoteField )
		m_pNoteField->Step( col, score );

	bool bSteppedOnATap = score != TNS_NONE;

	if( bSteppedOnATap )
	{
		/* Search for keyed sounds separately.  If we can't find a nearby note, search
		 * backwards indefinitely, and ignore grading. */
		if( iIndexOverlappingNote == -1 )
			iIndexOverlappingNote = GetClosestNote( col, BeatToNoteRow(fSongBeat),
							   iStepSearchRows, MAX_NOTE_ROW, true );
		if( iIndexOverlappingNote != -1 )
		{
			TapNote tn = m_NoteData.GetTapNote( col, iIndexOverlappingNote );
			if( tn.bKeysound && tn.iKeysoundIndex < (int) m_vKeysounds.size() )
				m_vKeysounds[tn.iKeysoundIndex].Play();
		}
	}

	

	{
		// Update roll life
		// Let's not check the whole array every time.
		// Instead, only check 1 beat back.  Even 1 is overkill.
		// Just update the life here and let Update judge the roll.
		const int iSongRow = BeatToNoteRow(fSongBeat);
		const int iStartCheckingAt = max( 0, iSongRow-BeatToNoteRow(1) );
		NoteData::iterator begin, end;
		m_NoteData.GetTapNoteRangeInclusive( col, iStartCheckingAt, iSongRow+1, begin, end );
		for( ; begin != end; ++begin )
		{
			TapNote &tn = begin->second;
			if( tn.type != TapNote::hold_head )
				continue;
			int iRow = begin->first;

			HoldNoteScore hns = tn.HoldResult.hns;
			if( hns != HNS_NONE )	// if this HoldNote already has a result
				continue;	// we don't need to update the logic for this one

			// if they got a bad score or haven't stepped on the corresponding tap yet
			const TapNoteScore tns = tn.result.tns;
			const bool bSteppedOnTapNote = tns != TNS_NONE  &&  tns != TNS_MISS;	// did they step on the start of this roll?

			int iEndRow = iRow + tn.iDuration;

			if( bSteppedOnTapNote && tn.HoldResult.fLife != 0 )
			{
				/* This hold note is not judged and we stepped on its head.  Update iLastHeldRow.
				 * Do this even if we're a little beyond the end of the hold note, to make sure
				 * iLastHeldRow is clamped to iEndRow if the hold note is held all the way. */
				tn.HoldResult.iLastHeldRow = min( iSongRow, iEndRow );
			}

			// If the song beat is in the range of this hold:
			if( iRow <= iSongRow && iRow <= iEndRow )
			{
				switch( tn.subType )
				{
				case TapNote::hold_head_hold:
					// this is handled in Update
					break;
				case TapNote::hold_head_roll:
					{
						// Increase life
						tn.HoldResult.fLife = 1;

						bool bBright = m_pPlayerStageStats && m_pPlayerStageStats->iCurCombo>(int)BRIGHT_GHOST_COMBO_THRESHOLD;
						if( m_pNoteField )
							m_pNoteField->DidHoldNote( col, HNS_OK, bBright );
					}
					break;
				default:
					ASSERT(0);
				}
			}
		}
	}
}

void Player::HandleAutosync(float fNoteOffset)
{
	if( GAMESTATE->m_SongOptions.m_AutosyncType == SongOptions::AUTOSYNC_OFF )
		return;

	m_fOffset[m_iOffsetSample] = fNoteOffset;
	m_iOffsetSample++;

	if( m_iOffsetSample < SAMPLE_COUNT ) 
		return; /* need more */

	const float mean = calc_mean( m_fOffset, m_fOffset+SAMPLE_COUNT );
	const float stddev = calc_stddev( m_fOffset, m_fOffset+SAMPLE_COUNT );

	CString sAutosyncType;
	switch( GAMESTATE->m_SongOptions.m_AutosyncType )
	{
	case SongOptions::AUTOSYNC_SONG:
		sAutosyncType = "Autosync Song";
		break;
	case SongOptions::AUTOSYNC_MACHINE:
		sAutosyncType = "Autosync Machine";
		break;
	default:
		ASSERT(0);
	}

	if( stddev < .03 && stddev < fabsf(mean) )  // If they stepped with less than .03 error
	{
		switch( GAMESTATE->m_SongOptions.m_AutosyncType )
		{
		case SongOptions::AUTOSYNC_SONG:
			GAMESTATE->m_pCurSong->m_Timing.m_fBeat0OffsetInSeconds += mean;
			break;
		case SongOptions::AUTOSYNC_MACHINE:
			PREFSMAN->m_fGlobalOffsetSeconds.Set( PREFSMAN->m_fGlobalOffsetSeconds + mean );
			break;
		default:
			ASSERT(0);
		}

		SCREENMAN->SystemMessage( "Autosync: Correction applied." );
	}
	else
	{
		SCREENMAN->SystemMessage( "Autosync: Correction NOT applied.\n     Timing deviation too high." );
	}

	m_iOffsetSample = 0;
}

void Player::DisplayJudgedRow( int iIndexThatWasSteppedOn, TapNoteScore score, int iTrack )
{
	TapNote tn = m_NoteData.GetTapNote(iTrack, iIndexThatWasSteppedOn);

	// If the score is great or better, remove the note from the screen to 
	// indicate success.  (Or always if blind is on.)
	if( score >= TNS_GREAT || m_pPlayerState->m_PlayerOptions.m_fBlind )
	{
		tn.result.bHidden = true;
		m_NoteData.SetTapNote( iTrack, iIndexThatWasSteppedOn, tn );
	}

	// show the ghost arrow for this column
	bool bBright = m_pPlayerStageStats && m_pPlayerStageStats->iCurCombo>(int)BRIGHT_GHOST_COMBO_THRESHOLD;
	if (m_pPlayerState->m_PlayerOptions.m_fBlind)
	{
		if( m_pNoteField )
			m_pNoteField->DidTapNote( iTrack, TNS_MARVELOUS, bBright );
	}
	else
	{
		bool bBright = m_pPlayerStageStats && m_pPlayerStageStats->iCurCombo>(int)BRIGHT_GHOST_COMBO_THRESHOLD;
		if( m_pNoteField )
			m_pNoteField->DidTapNote( iTrack, score, bBright );
	}
}

// HACK: fStepsSeconds is a timestamp that is passed to SetCombo to accurately
//          tell when the judgement was made for colored life graph rendering
void Player::OnRowCompletelyJudged( int iIndexThatWasSteppedOn, float fStepsSeconds )
{
//	LOG->Trace( "Player::OnRowCompletelyJudged" );
	
	/* Find the minimum score of the row.  This will never be TNS_NONE, since this
	 * function is only called when a row is completed. */
	/* Instead, use the last tap score (ala DDR).  Using the minimum results in 
	 * slightly more harsh scoring than DDR */
	/* I'm not sure this is right, either.  Can you really jump a boo and a perfect
	 * and get scored for a perfect?  (That's so loose, you can gallop jumps.) -glenn */
	/* Instead of grading individual columns, DDR sets a "was pressed recently" 
	 * countdown every time you step on a column.  When you step on the first note of 
	 * the jump, it sets the first "was pressed recently" timer.  Then, when you do 
	 * the 2nd step of the jump, it sets another column's timer then AND's the jump 
	 * columns with the "was pressed recently" columns to see whether or not you hit 
	 * all the columns of the jump.  -Chris */
	const Game *pCurGame = GAMESTATE->m_pCurGame;

	if( !pCurGame->m_bCountNotesSeparately )
	{
		TapNoteResult tnr = NoteDataWithScoring::LastTapNoteResult( m_NoteData, iIndexThatWasSteppedOn );
		TapNoteScore score = tnr.tns;

		ASSERT(score != TNS_NONE);
		ASSERT(score != TNS_HIT_MINE);
		ASSERT(score != TNS_AVOIDED_MINE);

		/* If the whole row was hit with perfects or greats, remove the row
		 * from the NoteField, so it disappears. */

		for( int c=0; c<m_NoteData.GetNumTracks(); c++ )	// for each column
		{
			TapNote tn = m_NoteData.GetTapNote(c, iIndexThatWasSteppedOn);

			if( tn.type == TapNote::empty )	continue; /* no note in this col */
			if( tn.type == TapNote::mine )	continue; /* don't flash on mines b/c they're supposed to be missed */

			DisplayJudgedRow( iIndexThatWasSteppedOn, score, c );
		}

		m_Judgment.SetJudgment( score, tnr.fTapNoteOffset < 0 );
	}
	else
	{
		for( int c=0; c<m_NoteData.GetNumTracks(); c++ )	// for each column
		{
			TapNote tn = m_NoteData.GetTapNote(c, iIndexThatWasSteppedOn);
			TapNoteResult tnr = tn.result;
			TapNoteScore score = tnr.tns;

			if( tn.type == TapNote::empty ) continue; /* no note in this col */
			if( tn.type == TapNote::mine ) continue; /* don't flash on mines b/c they're supposed to be missed */

			DisplayJudgedRow( iIndexThatWasSteppedOn, score, c );

			m_Judgment.SetJudgment( score, tnr.fTapNoteOffset < 0 );
		}
	}

	HandleTapRowScore( iIndexThatWasSteppedOn, fStepsSeconds );	// update score
}


void Player::UpdateTapNotesMissedOlderThan( float fMissIfOlderThanSeconds )
{
	//LOG->Trace( "Steps::UpdateTapNotesMissedOlderThan(%f)", fMissIfOlderThanThisBeat );
	int iMissIfOlderThanThisIndex;
	{
		const float fEarliestTime = GAMESTATE->m_fMusicSeconds - fMissIfOlderThanSeconds;
		bool bFreeze;
		float fMissIfOlderThanThisBeat;
		float fThrowAway;
		GAMESTATE->m_pCurSong->GetBeatAndBPSFromElapsedTime( fEarliestTime, fMissIfOlderThanThisBeat, fThrowAway, bFreeze );

		iMissIfOlderThanThisIndex = BeatToNoteRow( fMissIfOlderThanThisBeat );
		if( bFreeze )
		{
			/* If there is a freeze on iMissIfOlderThanThisIndex, include this index too.
			 * Otherwise we won't show misses for tap notes on freezes until the
			 * freeze finishes. */
			iMissIfOlderThanThisIndex++;
		}
	}

	// Since this is being called every frame, let's not check the whole array every time.
	// Instead, only check 1 beat back.  Even 1 is overkill.
	const int iStartCheckingAt = max( 0, iMissIfOlderThanThisIndex-BeatToNoteRow(1) );

	//LOG->Trace( "iStartCheckingAt: %d   iMissIfOlderThanThisIndex:  %d", iStartCheckingAt, iMissIfOlderThanThisIndex );

	int iNumMissesFound = 0;

	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( m_NoteData, r, iStartCheckingAt, iMissIfOlderThanThisIndex-1 )
	{
		bool MissedNoteOnThisRow = false;
		for( int t=0; t<m_NoteData.GetNumTracks(); t++ )
		{
			/* XXX: cleaner to pick the things we do want to apply misses to, instead of
			 * the things we don't? */
			TapNote tn = m_NoteData.GetTapNote(t, r);
			switch( tn.type )
			{
			case TapNote::empty:
			case TapNote::attack:
				continue; /* no note here */
			}
			if( tn.result.tns != TNS_NONE ) /* note here is already hit */
				continue; 

			if( tn.type == TapNote::mine )
			{
				tn.result.tns =	TNS_AVOIDED_MINE;
			}
			else
			{
				// A normal note.  Penalize for not stepping on it.
				MissedNoteOnThisRow = true;
				tn.result.tns =	TNS_MISS;
				if( m_pPlayerStageStats )
					m_pPlayerStageStats->iTotalError += MAX_PRO_TIMING_ERROR;
			}

			m_NoteData.SetTapNote( t, r, tn );
		}

		if( MissedNoteOnThisRow )
		{
			iNumMissesFound++;
			HandleTapRowScore( r, -1.0f );
		}
	}

	if( iNumMissesFound > 0 )
		m_Judgment.SetJudgment( TNS_MISS, false );
}


void Player::CrossedRow( int iNoteRow )
{
	// If we're doing random vanish, randomise notes on the fly.
	if(m_pPlayerState->m_CurrentPlayerOptions.m_fAppearances[PlayerOptions::APPEARANCE_RANDOMVANISH]==1)
		RandomizeNotes( iNoteRow );

	// check to see if there's a note at the crossed row
	RageTimer now;
	if( m_pPlayerState->m_PlayerController != PC_HUMAN )
	{
		for( int t=0; t<m_NoteData.GetNumTracks(); t++ )
		{
			TapNote tn = m_NoteData.GetTapNote(t, iNoteRow);
			if( tn.type != TapNote::empty && tn.result.tns == TNS_NONE )
				Step( t, now );
		}
	}
}

void Player::CrossedMineRow( int iNoteRow )
{
	// Hold the panel while crossing a mine will cause the mine to explode
	RageTimer now;
	for( int t=0; t<m_NoteData.GetNumTracks(); t++ )
	{
		if( m_NoteData.GetTapNote(t,iNoteRow).type == TapNote::mine )
		{
			// TODO: Remove use of PlayerNumber.
			PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

			const StyleInput StyleI( pn, t );
			const GameInput GameI = GAMESTATE->GetCurrentStyle()->StyleInputToGameInput( StyleI );
			if( PREFSMAN->m_fPadStickSeconds > 0 )
			{
				float fSecsHeld = INPUTMAPPER->GetSecsHeld( GameI );
				if( fSecsHeld >= PREFSMAN->m_fPadStickSeconds )
					Step( t, now+(-PREFSMAN->m_fPadStickSeconds), true );
			}
			else
			{
				bool bIsDown = INPUTMAPPER->IsButtonDown( GameI );
				if( bIsDown )
					Step( t, now, true );
			}
		}
	}
}

void Player::RandomizeNotes( int iNoteRow )
{
	// change the row to look ahead from based upon their speed mod
	/* This is incorrect: if m_fScrollSpeed is 0.5, we'll never change
	 * any odd rows, and if it's 2, we'll shuffle each row twice. */
	int iNewNoteRow = iNoteRow + ROWS_PER_BEAT*2;
	iNewNoteRow = int( iNewNoteRow / m_pPlayerState->m_PlayerOptions.m_fScrollSpeed );

	int iNumOfTracks = m_NoteData.GetNumTracks();
	for( int t=0; t+1 < iNumOfTracks; t++ )
	{
		const int iSwapWith = RandomInt( 0, iNumOfTracks-1 );

		/* Only swap a tap and an empty. */
		const TapNote t1 = m_NoteData.GetTapNote(t, iNewNoteRow);
		if( t1.type != TapNote::tap )
			continue;

		const TapNote t2 = m_NoteData.GetTapNote( iSwapWith, iNewNoteRow );
		if( t2.type != TapNote::empty )
			continue;

		/* Make sure the destination row isn't in the middle of a hold. */
		if( m_NoteData.IsHoldNoteAtBeat(iSwapWith, iNoteRow) )
			continue;
		
		m_NoteData.SetTapNote( t, iNewNoteRow, t2 );
		m_NoteData.SetTapNote( iSwapWith, iNewNoteRow, t1 );
	}
}

void Player::HandleTapRowScore( unsigned row, float fStepsSeconds )
{
	TapNoteScore scoreOfLastTap = NoteDataWithScoring::LastTapNoteResult( m_NoteData, row ).tns;
	int iNumTapsInRow = m_NoteData.GetNumTracksWithTapOrHoldHead(row);
	ASSERT(iNumTapsInRow > 0);

	bool NoCheating = true;
#ifdef DEBUG
	NoCheating = false;
#endif

	if(GAMESTATE->m_bDemonstrationOrJukebox)
		NoCheating = false;
	// don't accumulate points if AutoPlay is on.
	if( NoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
		return;

	/* Update miss combo, and handle "combo stopped" messages. */
	/* When is m_pPlayerStageStats NULL?  Would it be cleaner to pass Player a dummy
	 * PlayerStageStats in this case, instead of having to carefully check for NULL
	 * every time we use it? -glenn */
	int iDummy = 0;
	int &iCurCombo = m_pPlayerStageStats ? m_pPlayerStageStats->iCurCombo : iDummy;
	int &iCurMissCombo = m_pPlayerStageStats ? m_pPlayerStageStats->iCurMissCombo : iDummy;
	switch( scoreOfLastTap )
	{
	case TNS_MARVELOUS:
	case TNS_PERFECT:
	case TNS_GREAT:
		iCurMissCombo = 0;
		SCREENMAN->PostMessageToTopScreen( SM_MissComboAborted, 0 );
		break;

	case TNS_MISS:
		++iCurMissCombo;
		m_LastTapNoteScore = TNS_MISS;

	case TNS_GOOD:
	case TNS_BOO:
		if( iCurCombo > 50 )
			SCREENMAN->PostMessageToTopScreen( SM_ComboStopped, 0 );
		iCurCombo = 0;
		break;
	
	default:
		ASSERT( 0 );
	}

	/* The score keeper updates the hit combo.  Remember the old combo for handling announcers. */
	const int iOldCombo = iCurCombo;

	if( m_pPrimaryScoreKeeper != NULL )
		m_pPrimaryScoreKeeper->HandleTapRowScore( scoreOfLastTap, iNumTapsInRow );
	if( m_pSecondaryScoreKeeper != NULL )
		m_pSecondaryScoreKeeper->HandleTapRowScore( scoreOfLastTap, iNumTapsInRow );

	if( m_pPlayerStageStats )
		m_Combo.SetCombo( m_pPlayerStageStats->iCurCombo, m_pPlayerStageStats->iCurMissCombo, fStepsSeconds );

#define CROSSED( x ) (iOldCombo<x && iCurCombo>=x)
	if ( CROSSED(100) )	
		SCREENMAN->PostMessageToTopScreen( SM_100Combo, 0 );
	else if( CROSSED(200) )	
		SCREENMAN->PostMessageToTopScreen( SM_200Combo, 0 );
	else if( CROSSED(300) )	
		SCREENMAN->PostMessageToTopScreen( SM_300Combo, 0 );
	else if( CROSSED(400) )	
		SCREENMAN->PostMessageToTopScreen( SM_400Combo, 0 );
	else if( CROSSED(500) )	
		SCREENMAN->PostMessageToTopScreen( SM_500Combo, 0 );
	else if( CROSSED(600) )	
		SCREENMAN->PostMessageToTopScreen( SM_600Combo, 0 );
	else if( CROSSED(700) )	
		SCREENMAN->PostMessageToTopScreen( SM_700Combo, 0 );
	else if( CROSSED(800) )	
		SCREENMAN->PostMessageToTopScreen( SM_800Combo, 0 );
	else if( CROSSED(900) )	
		SCREENMAN->PostMessageToTopScreen( SM_900Combo, 0 );
	else if( CROSSED(1000))	
		SCREENMAN->PostMessageToTopScreen( SM_1000Combo, 0 );
	else if( (iOldCombo / 100) < (iCurCombo / 100) && iCurCombo > 1000 )
		SCREENMAN->PostMessageToTopScreen( SM_ComboContinuing, 0 );
#undef CROSSED

	// new max combo
	if( m_pPlayerStageStats )
		m_pPlayerStageStats->iMaxCombo = max(m_pPlayerStageStats->iMaxCombo, iCurCombo);

	/*
	 * Use the real current beat, not the beat we've been passed.  That's because we
	 * want to record the current life/combo to the current time; eg. if it's a MISS,
	 * the beat we're registering is in the past, but the life is changing now.
	 *
	 * We need to include time from previous songs in a course, so we can't use
	 * GAMESTATE->m_fMusicSeconds.  Use fGameplaySeconds instead.
	 */
	if( m_pPlayerStageStats )
		m_pPlayerStageStats->UpdateComboList( STATSMAN->m_CurStageStats.fStepsSeconds, false );

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;
	
	float life = -1;
	if( m_pLifeMeter )
	{
		life = m_pLifeMeter->GetLife();
	}
	else if( m_pCombinedLifeMeter )
	{
		life = GAMESTATE->m_fTugLifePercentP1;
		if( pn == PLAYER_2 )
			life = 1.0f - life;
	}
	if( life != -1 )
		if( m_pPlayerStageStats )
			m_pPlayerStageStats->SetLifeRecordAt( life, STATSMAN->m_CurStageStats.fStepsSeconds, scoreOfLastTap );

	if( m_pScoreDisplay )
	{
		if( m_pPlayerStageStats )
			m_pScoreDisplay->SetScore( m_pPlayerStageStats->iScore );
		m_pScoreDisplay->OnJudgment( scoreOfLastTap );
	}
	if( m_pSecondaryScoreDisplay )
	{
		if( m_pPlayerStageStats )
			m_pSecondaryScoreDisplay->SetScore( m_pPlayerStageStats->iScore );
		m_pSecondaryScoreDisplay->OnJudgment( scoreOfLastTap );
	}

	if( m_pLifeMeter )
	{
		m_pLifeMeter->ChangeLife( scoreOfLastTap );
		m_pLifeMeter->OnDancePointsChange();    // update oni life meter
	}
	if( m_pCombinedLifeMeter )
	{
		m_pCombinedLifeMeter->ChangeLife( pn, scoreOfLastTap );
		m_pCombinedLifeMeter->OnDancePointsChange( pn );    // update oni life meter
	}
}


void Player::HandleHoldScore( HoldNoteScore holdScore, TapNoteScore tapScore )
{
	bool NoCheating = true;
#ifdef DEBUG
	NoCheating = false;
#endif

	if( GAMESTATE->m_bDemonstrationOrJukebox )
		NoCheating = false;
	// don't accumulate points if AutoPlay is on.
	if( NoCheating && m_pPlayerState->m_PlayerController == PC_AUTOPLAY )
		return;

	if(m_pPrimaryScoreKeeper)
		m_pPrimaryScoreKeeper->HandleHoldScore(holdScore, tapScore );
	if(m_pSecondaryScoreKeeper)
		m_pSecondaryScoreKeeper->HandleHoldScore(holdScore, tapScore );

	// TODO: Remove use of PlayerNumber.
	PlayerNumber pn = m_pPlayerState->m_PlayerNumber;

	if( m_pScoreDisplay )
	{
		if( m_pPlayerStageStats ) 
			m_pScoreDisplay->SetScore( m_pPlayerStageStats->iScore );
		m_pScoreDisplay->OnJudgment( holdScore, tapScore );
	}
	if( m_pSecondaryScoreDisplay )
	{
		if( m_pPlayerStageStats ) 
			m_pSecondaryScoreDisplay->SetScore( m_pPlayerStageStats->iScore );
		m_pSecondaryScoreDisplay->OnJudgment( holdScore, tapScore );
	}

	if( m_pLifeMeter ) 
	{
		m_pLifeMeter->ChangeLife( holdScore, tapScore );
		m_pLifeMeter->OnDancePointsChange();
	}
	if( m_pCombinedLifeMeter ) 
	{
		m_pCombinedLifeMeter->ChangeLife( pn, holdScore, tapScore );
		m_pCombinedLifeMeter->OnDancePointsChange( pn );
	}
}

float Player::GetMaxStepDistanceSeconds()
{
	return GAMESTATE->m_SongOptions.m_fMusicRate * ADJUSTED_WINDOW_TAP(TW_Boo);
}

void Player::FadeToFail()
{
	if( m_pNoteField )
		m_pNoteField->FadeToFail();

	// clear miss combo
	m_Combo.SetCombo( 0, 0 );
}

bool Player::IsPlayingBeginner() const
{
	if( m_pPlayerStageStats != NULL && !m_pPlayerStageStats->vpPossibleSteps.empty() )
	{
		Steps *pSteps = m_pPlayerStageStats->vpPossibleSteps[0];
		return pSteps->GetDifficulty() == DIFFICULTY_BEGINNER;
	}
	else
	{
		if( m_pPlayerState->m_PlayerNumber == PLAYER_INVALID )
			return false;
		Steps *pSteps = GAMESTATE->m_pCurSteps[ m_pPlayerState->m_PlayerNumber ];
		return pSteps && pSteps->GetDifficulty() == DIFFICULTY_BEGINNER;
	}	
}

void Player::HandleMessage( const CString& sMessage )
{
	// Reset autosync samples when toggling
	if( sMessage == MessageToString(MESSAGE_AUTOSYNC_CHANGED) )
		m_iOffsetSample = 0;
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
