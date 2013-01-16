#include "global.h"
#include "ScreenGameplay.h"
#include "GameManager.h"
#include "RageLog.h"
#include "LifeMeterBar.h"
#include "LifeMeterBattery.h"
#include "LifeMeterTime.h"
#include "GameState.h"
#include "ScoreDisplayNormal.h"
#include "ScoreDisplayPercentage.h"
#include "ScoreDisplayLifeTime.h"
#include "ScoreDisplayOni.h"
#include "ScoreDisplayRave.h"
#include "ScoreKeeperMAX2.h"
#include "ScoreKeeperRave.h"
#include "LyricsLoader.h"
#include "GameSoundManager.h"
#include "CombinedLifeMeterTug.h"
#include "NoteDataUtil.h"
#include "LightsManager.h"
#include "ProfileManager.h"
#include "StatsManager.h"
#include "PlayerAI.h"	// for NUM_SKILL_LEVELS
#include "DancingCharacters.h"
#include "ScreenDimensions.h"
#include "PlayerState.h"
#include "Style.h"
#include "MemoryCardManager.h"
#include "CommonMetrics.h"
#include "Game.h"
#include "RageFileManager.h" // XXX!!! needed for custom song cleanup, remove as soon as we can
#include "AnnouncerManager.h"
#include "Course.h"
#include "Inventory.h"

//
// Defines
//
#define SHOW_LIFE_METER_FOR_DISABLED_PLAYERS	THEME->GetMetricB(m_sName,"ShowLifeMeterForDisabledPlayers")
#define EVAL_ON_FAIL							THEME->GetMetricB(m_sName,"ShowEvaluationOnFail")
#define SHOW_SCORE_IN_RAVE						THEME->GetMetricB(m_sName,"ShowScoreInRave")
#define SONG_POSITION_METER_WIDTH				THEME->GetMetricF(m_sName,"SongPositionMeterWidth")
#define PLAYER_X( p, styleType )				THEME->GetMetricF(m_sName,ssprintf("PlayerP%d%sX",p+1,StyleTypeToString(styleType).c_str()))
#define STOP_COURSE_EARLY						THEME->GetMetricB(m_sName,"StopCourseEarly")	// evaluate this every time it's used
#define COMPARE_SCORES							THEME->GetMetricB(m_sName, "CompareScores" )

// set an adjusted limit based off the song allowance
// r590: made redundant
//#define MAX_CUSTOM_LENGTH ((float)(PREFSMAN->m_iCustomMaxSeconds + 10))

static ThemeMetric<float> INITIAL_BACKGROUND_BRIGHTNESS	("ScreenGameplay","InitialBackgroundBrightness");
static ThemeMetric<float> SECONDS_BETWEEN_COMMENTS	("ScreenGameplay","SecondsBetweenComments");

AutoScreenMessage( SM_PlayGo )

// received while STATE_DANCING
AutoScreenMessage( SM_LoadNextSong )
AutoScreenMessage( SM_StartLoadingNextSong )


// received while STATE_OUTRO
AutoScreenMessage( SM_GoToScreenAfterBack )

AutoScreenMessage( SM_BeginFailed )
AutoScreenMessage( SM_LeaveGameplay )

// received while STATE_INTRO
AutoScreenMessage( SM_StartHereWeGo )
AutoScreenMessage( SM_StopHereWeGo )

static Preference<float> g_fNetStartOffset( "NetworkStartOffset",	-3.0 );
static Preference<float> g_fGiveUpTime( "GiveUpTime", 2.5f );

// define which steps type we autogen lights from
const StepsType LIGHTS_AUTOGEN_TYPE = STEPS_TYPE_DANCE_SINGLE;

REGISTER_SCREEN_CLASS( ScreenGameplay );
ScreenGameplay::ScreenGameplay( CString sName ) : ScreenWithMenuElements(sName)
{
	PLAYER_TYPE.Load( sName, "PlayerType" );
	GIVE_UP_TEXT.Load( sName, "GiveUpText" );
	GIVE_UP_ABORTED_TEXT.Load( sName, "GiveUpAbortedText" );
	MUSIC_FADE_OUT_SECONDS.Load( sName, "MusicFadeOutSeconds" );
	START_GIVES_UP.Load( sName, "StartGivesUp" );
	BACK_GIVES_UP.Load( sName, "BackGivesUp" );
	GIVING_UP_GOES_TO_PREV_SCREEN.Load( sName, "GivingUpGoesToPrevScreen" );
	GIVING_UP_GOES_TO_NEXT_SCREEN.Load( sName, "GivingUpGoesToNextScreen" );
	FAIL_AFTER_30_MISSES.Load( sName, "FailAfter30Misses" );
	USE_FORCED_MODIFIERS_IN_BEGINNER.Load( sName, "UseForcedModifiersInBeginner" );
	FORCED_MODIFIERS_IN_BEGINNER.Load( sName, "ForcedModifiersInBeginner" );
}

void ScreenGameplay::Init()
{
	ScreenWithMenuElements::Init();

	/* Pause MEMCARDMAN.  If a memory card is remove, we don't want to interrupt the
	 * player by making a noise until the game finishes. */
	if( !GAMESTATE->m_bDemonstrationOrJukebox )
	{	
		MEMCARDMAN->PauseMountingThread();
	}

	m_pSoundMusic = NULL;
	m_bPaused = false;
	m_bEasterEgg = false;

	/* We do this ourself. */
	SOUND->HandleSongTimer( false );

	//need to initialize these before checking for demonstration mode
	//otherwise destructor will try to delete possibly invalid pointers

    FOREACH_PlayerNumber(p)
	{
		m_pLifeMeter[p] = NULL;
		m_pPrimaryScoreDisplay[p] = NULL;
		m_pSecondaryScoreDisplay[p] = NULL;
		m_pPrimaryScoreKeeper[p] = NULL;
		m_pSecondaryScoreKeeper[p] = NULL;
		m_pInventory[p] = NULL ;
	}
	m_pCombinedLifeMeter = NULL;

	if( GAMESTATE->m_pCurSong == NULL && GAMESTATE->m_pCurCourse == NULL )
		return;	// ScreenDemonstration will move us to the next screen.  We just need to survive for one update without crashing.

	/* Save selected options before we change them. */
	GAMESTATE->StoreSelectedOptions();

	/* Save settings to the profile now.  Don't do this on extra stages, since the
	 * user doesn't have full control; saving would force profiles to DIFFICULTY_HARD
	 * and save over their default modifiers every time someone got an extra stage.
	 * Do this before course modifiers are set up. */
	if( !GAMESTATE->IsExtraStage() && !GAMESTATE->IsExtraStage2() )
	{
		FOREACH_HumanPlayer( pn )
			GAMESTATE->SaveCurrentSettingsToProfile(pn);
	}

	/* Called once per stage (single song or single course). */
	GAMESTATE->BeginStage();


	// fill in difficulty of CPU players with that of the first human player
	FOREACH_PotentialCpuPlayer(p)
		GAMESTATE->m_pCurSteps[p].Set( GAMESTATE->m_pCurSteps[ GAMESTATE->GetFirstHumanPlayer() ] );

	/* Increment the course play count. */
	if( GAMESTATE->IsCourseMode() && !GAMESTATE->m_bDemonstrationOrJukebox )
		FOREACH_EnabledPlayer(p)
			PROFILEMAN->IncrementCoursePlayCount( GAMESTATE->m_pCurCourse, GAMESTATE->m_pCurTrail[p], p );

	STATSMAN->m_CurStageStats.playMode = GAMESTATE->m_PlayMode;
	STATSMAN->m_CurStageStats.pStyle = GAMESTATE->m_pCurStyle;

	/* Record combo rollover. */
	FOREACH_EnabledPlayer(pn)
		STATSMAN->m_CurStageStats.m_player[pn].UpdateComboList( 0, true );

	if( GAMESTATE->IsExtraStage() )
		STATSMAN->m_CurStageStats.StageType = StageStats::STAGE_EXTRA;
	else if( GAMESTATE->IsExtraStage2() )
		STATSMAN->m_CurStageStats.StageType = StageStats::STAGE_EXTRA2;
	else
		STATSMAN->m_CurStageStats.StageType = StageStats::STAGE_NORMAL;
	
	//
	// Init ScoreKeepers
	//

    FOREACH_EnabledPlayer(p)
	{
        switch( PREFSMAN->m_ScoringType )
		{
		case PrefsManager::SCORING_MAX2:
		case PrefsManager::SCORING_5TH:
			m_pPrimaryScoreKeeper[p] = new ScoreKeeperMAX2( 
				GAMESTATE->m_pPlayerState[p], 
				&STATSMAN->m_CurStageStats.m_player[p] );
			break;
		default: ASSERT(0);
		}

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_RAVE:
			m_pSecondaryScoreKeeper[p] = new ScoreKeeperRave( 
				GAMESTATE->m_pPlayerState[p],
				&STATSMAN->m_CurStageStats.m_player[p] );
			break;
		}
	}

	m_DancingState = STATE_INTRO;

	// Set this in LoadNextSong()
	//m_fTimeLeftBeforeDancingComment = SECONDS_BETWEEN_COMMENTS;
		
	m_bZeroDeltaOnNextUpdate = false;


	m_SongBackground.SetName( "SongBackground" );
	m_SongBackground.SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );
	ON_COMMAND( m_SongBackground );
	this->AddChild( &m_SongBackground );

	m_SongForeground.SetName( "SongForeground" );
	m_SongForeground.SetDrawOrder( DRAW_ORDER_OVERLAY+1 );	// on top of the overlay, but under transitions
	ON_COMMAND( m_SongBackground );
	this->AddChild( &m_SongForeground );

	if( PREFSMAN->m_bShowBeginnerHelper )
	{
		m_BeginnerHelper.SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );
		m_BeginnerHelper.SetXY( SCREEN_CENTER_X, SCREEN_CENTER_Y );
		this->AddChild( &m_BeginnerHelper );
	}
	
	m_sprStaticBackground.SetName( "StaticBG" );
	m_sprStaticBackground.Load( THEME->GetPathG(m_sName,"Static Background") );
	SET_XY( m_sprStaticBackground );
	m_sprStaticBackground.SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );	// behind everything else
	this->AddChild(&m_sprStaticBackground);

	if( !GAMESTATE->m_bDemonstrationOrJukebox )	// only load if we're going to use it
	{
		m_Toasty.Load( THEME->GetPathB(m_sName,"toasty") );
		this->AddChild( &m_Toasty );
	}

    FOREACH_EnabledPlayer(p)
	{
		float fPlayerX = PLAYER_X( p, GAMESTATE->GetCurrentStyle()->m_StyleType );

		/* Perhaps this should be handled better by defining a new
		 * StyleType for ONE_PLAYER_ONE_CREDIT_AND_ONE_COMPUTER,
		 * but for now just ignore SoloSingles when it's Battle or Rave
		 * Mode.  This doesn't begin to address two-player solo (6 arrows) */
		if( PREFSMAN->m_bSoloSingle && 
			GAMESTATE->m_PlayMode != PLAY_MODE_BATTLE &&
			GAMESTATE->m_PlayMode != PLAY_MODE_RAVE &&
			GAMESTATE->GetCurrentStyle()->m_StyleType == ONE_PLAYER_ONE_SIDE )
			fPlayerX = SCREEN_CENTER_X;

		m_Player[p].SetName( ssprintf("PlayerP%i", p+1) );
		m_Player[p].SetXY( fPlayerX, SCREEN_CENTER_Y );
		this->AddChild( &m_Player[p] );
	
		m_sprOniGameOver[p].SetName( ssprintf("OniGameOverP%i",p+1) );
		m_sprOniGameOver[p].Load( THEME->GetPathG(m_sName,"oni gameover") );
		SET_XY_AND_ON_COMMAND( m_sprOniGameOver[p] );
		this->AddChild( &m_sprOniGameOver[p] );
	}

	m_NextSong.Load( THEME->GetPathB(m_sName,"next course song") );
	m_NextSong.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 );
	this->AddChild( &m_NextSong );

	m_SongFinished.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 );
	this->AddChild( &m_SongFinished );


	bool bBattery = GAMESTATE->m_SongOptions.m_LifeType==SongOptions::LIFE_BATTERY;

	//
	// Add LifeFrame
	//
	m_sprLifeFrame.Load( THEME->GetPathG(m_sName,bBattery?"oni life frame":"life frame") );
	m_sprLifeFrame->SetName( "LifeFrame" );
	SET_XY( m_sprLifeFrame );
	this->AddChild( m_sprLifeFrame );

	//
	// Add score frame
	//
	m_sprScoreFrame.Load( THEME->GetPathG(m_sName,bBattery?"oni score frame":"score frame") );
	m_sprScoreFrame.SetName( "ScoreFrame" );
	SET_XY( m_sprScoreFrame );
	this->AddChild( &m_sprScoreFrame );


	//
	// Add combined life meter
	//
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		m_pCombinedLifeMeter = new CombinedLifeMeterTug;
		m_pCombinedLifeMeter->SetName( "CombinedLife" );
		SET_XY( *m_pCombinedLifeMeter );
		this->AddChild( m_pCombinedLifeMeter );		
		break;
	}

	//
	// Before the lifemeter loads, if Networking is required
	// we need to wait, so that there is no Dead On Start issues.
	// if you wait too long at the second checkpoint, you will
	// appear dead when you begin your game.
	//
	NSMAN->StartRequest(0); 



	//
	// Add individual life meter
	//
	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_REGULAR:
	case PLAY_MODE_ONI:
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ENDLESS:
        FOREACH_PlayerNumber(p)
		{
			if( !GAMESTATE->IsPlayerEnabled(p) && !SHOW_LIFE_METER_FOR_DISABLED_PLAYERS )
				continue;	// skip

			switch( GAMESTATE->m_SongOptions.m_LifeType )
			{
			case SongOptions::LIFE_BAR:
				m_pLifeMeter[p] = new LifeMeterBar;
				break;
			case SongOptions::LIFE_BATTERY:
				m_pLifeMeter[p] = new LifeMeterBattery;
				break;
			case SongOptions::LIFE_TIME:
				m_pLifeMeter[p] = new LifeMeterTime;
				break;
			default:
				ASSERT(0);
			}

			m_pLifeMeter[p]->Load( p );
			m_pLifeMeter[p]->SetName( ssprintf("LifeP%d",p+1) );
			SET_XY( *m_pLifeMeter[p] );
			this->AddChild( m_pLifeMeter[p] );		
		}
		break;
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		break;
	}

	m_ShowScoreboard=false;

	//the following is only used in SMLAN/SMOnline
	if( NSMAN->useSMserver && ( !GAMESTATE->PlayerUsingBothSides() ) )
	{
		PlayerNumber pn = GAMESTATE->GetFirstDisabledPlayer();
		if( pn != PLAYER_INVALID )
		{
			FOREACH_NSScoreBoardColumn( col )
			{
				m_Scoreboard[col].LoadFromFont( THEME->GetPathF(m_sName,"scoreboard") );
				m_Scoreboard[col].SetShadowLength( 0 );
				m_Scoreboard[col].SetName( ssprintf("ScoreboardC%iP%i",col+1,pn+1) );
				SET_XY( m_Scoreboard[col] );
				this->AddChild( &m_Scoreboard[col] );
				m_Scoreboard[col].SetText( NSMAN->m_Scoreboard[col] );
				m_Scoreboard[col].SetVertAlign( align_top );
				m_ShowScoreboard = true;
			}
		}
	}

	m_MaxCombo.LoadFromFont( THEME->GetPathF(m_sName,"max combo") );
	m_MaxCombo.SetName( "MaxCombo" );
	SET_XY( m_MaxCombo );
	m_MaxCombo.SetText( ssprintf("%d", STATSMAN->m_CurStageStats.m_player[GAMESTATE->m_MasterPlayerNumber].iMaxCombo) ); // TODO: Make this work for both players
	this->AddChild( &m_MaxCombo );

	/* See if we should be comparing scores during updates: we only do this
	 * if there are two players; the other conditions are set by the themer. */
	m_bCompareScores = COMPARE_SCORES && GAMESTATE->GetCurrentStyle()->m_StyleType == TWO_PLAYERS_TWO_SIDES;

	FOREACH_EnabledPlayer(p)
	{
		//
		// primary score display
		//
		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_REGULAR:
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			if( PREFSMAN->m_bPercentageScoring )
				m_pPrimaryScoreDisplay[p] = new ScoreDisplayPercentage;
			else
				m_pPrimaryScoreDisplay[p] = new ScoreDisplayNormal;
			break;
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:
			if( GAMESTATE->m_SongOptions.m_LifeType == SongOptions::LIFE_TIME )
				m_pPrimaryScoreDisplay[p] = new ScoreDisplayLifeTime;
			else
				m_pPrimaryScoreDisplay[p] = new ScoreDisplayOni;
			break;
		default:
			ASSERT(0);
		}

		m_pPrimaryScoreDisplay[p]->Init( GAMESTATE->m_pPlayerState[p] );
		m_pPrimaryScoreDisplay[p]->SetName( ssprintf("ScoreP%d",p+1) );
		m_pPrimaryScoreDisplay[p]->SetEffectClock( CLOCK_BGM_BEAT );

		/* used for comparing players' scores */
		/* UGLY: we can't use ActorUtil here due to an Actor derivative in a pointer, so manually add it */
		if( m_bCompareScores )
		{
			m_pPrimaryScoreDisplay[p]->AddCommand( "Ahead", THEME->GetMetricA( m_sName, m_pPrimaryScoreDisplay[p]->GetName()+"AheadCommand") );
			m_pPrimaryScoreDisplay[p]->AddCommand( "Behind", THEME->GetMetricA( m_sName, m_pPrimaryScoreDisplay[p]->GetName()+"BehindCommand") );
		}

		SET_XY( *m_pPrimaryScoreDisplay[p] );
		if( GAMESTATE->m_PlayMode != PLAY_MODE_RAVE || SHOW_SCORE_IN_RAVE ) /* XXX: ugly */
			this->AddChild( m_pPrimaryScoreDisplay[p] );

	
		//
		// secondary score display
		//
		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_RAVE:
			m_pSecondaryScoreDisplay[p] = new ScoreDisplayRave;
			break;
		}

		if( m_pSecondaryScoreDisplay[p] )
		{
			m_pSecondaryScoreDisplay[p]->Init( GAMESTATE->m_pPlayerState[p] );
			m_pSecondaryScoreDisplay[p]->SetName( ssprintf("SecondaryScoreP%d",p+1) );
			SET_XY( *m_pSecondaryScoreDisplay[p] );
			this->AddChild( m_pSecondaryScoreDisplay[p] );
		}
	}
	
	//
	// Add stage / SongNumber
	//
	m_sprCourseSongNumber.SetName( "CourseSongNumber" );
	SET_XY( m_sprCourseSongNumber );
	
	FOREACH_EnabledPlayer(p)
	{
		m_textCourseSongNumber[p].LoadFromFont( THEME->GetPathF(m_sName,"song num") );
		m_textCourseSongNumber[p].SetShadowLength( 0 );
		m_textCourseSongNumber[p].SetName( ssprintf("SongNumberP%d",p+1) );
		SET_XY( m_textCourseSongNumber[p] );
		m_textCourseSongNumber[p].SetText( "" );
		m_textCourseSongNumber[p].SetDiffuse( RageColor(0,0.5f,1,1) );	// light blue
	}

	FOREACH_EnabledPlayer(p)
	{
		m_textStepsDescription[p].LoadFromFont( THEME->GetPathF(m_sName,"StepsDescription") );
		m_textStepsDescription[p].SetName( ssprintf("StepsDescriptionP%i",p+1) );
		SET_XY( m_textStepsDescription[p] );
		this->AddChild( &m_textStepsDescription[p] );
	}

	switch( GAMESTATE->m_PlayMode )
	{
	case PLAY_MODE_REGULAR:
	case PLAY_MODE_BATTLE:
	case PLAY_MODE_RAVE:
		break;
	case PLAY_MODE_NONSTOP:
	case PLAY_MODE_ONI:
	case PLAY_MODE_ENDLESS:
		this->AddChild( &m_sprCourseSongNumber );

        FOREACH_EnabledPlayer( p )
			this->AddChild( &m_textCourseSongNumber[p] );
		break;
	default:
		ASSERT(0);	// invalid GameMode
	}


	m_sprStageFrame.Load( THEME->GetPathG(m_sName,"stage frame") );
	m_sprStageFrame->SetName( "StageFrame" );
	SET_XY( m_sprStageFrame );
	this->AddChild( m_sprStageFrame );

	//
	// Player/Song options
	//
    FOREACH_EnabledPlayer(p)
	{
		m_textPlayerOptions[p].LoadFromFont( THEME->GetPathF(m_sName,"player options") );
		m_textPlayerOptions[p].SetShadowLength( 0 );
		m_textPlayerOptions[p].SetName( ssprintf("PlayerOptionsP%d",p+1) );
		SET_XY( m_textPlayerOptions[p] );
		this->AddChild( &m_textPlayerOptions[p] );
	}

	m_textSongOptions.LoadFromFont( THEME->GetPathF(m_sName,"song options") );
	m_textSongOptions.SetShadowLength( 0 );
	m_textSongOptions.SetName( "SongOptions" );
	SET_XY( m_textSongOptions );
	m_textSongOptions.SetText( GAMESTATE->m_SongOptions.GetString() );
	this->AddChild( &m_textSongOptions );

	FOREACH_EnabledPlayer( pn )
	{
		m_ActiveAttackList[pn].LoadFromFont( THEME->GetPathF(m_sName,"ActiveAttackList") );
		m_ActiveAttackList[pn].Init( GAMESTATE->m_pPlayerState[pn] );
		m_ActiveAttackList[pn].SetName( ssprintf("ActiveAttackListP%d",pn+1) );
		SET_XY( m_ActiveAttackList[pn] );
		this->AddChild( &m_ActiveAttackList[pn] );
	}



    FOREACH_EnabledPlayer(p)
	{
		m_DifficultyIcon[p].Load( THEME->GetPathG(m_sName,ssprintf("difficulty icons %dx%d",NUM_PLAYERS,NUM_DIFFICULTIES)) );
		/* Position it in LoadNextSong. */
		this->AddChild( &m_DifficultyIcon[p] );

		m_DifficultyMeter[p].Load( m_sName + ssprintf(" DifficultyMeterP%d",p+1) );
		/* Position it in LoadNextSong. */
		this->AddChild( &m_DifficultyMeter[p] );
	}


	if( PREFSMAN->m_bShowLyrics )
		this->AddChild( &m_LyricDisplay );
	

	m_BPMDisplay.SetName( "BPMDisplay" );
	m_BPMDisplay.Load();
	SET_XY( m_BPMDisplay );
	this->AddChild( &m_BPMDisplay );
	m_fLastBPS = 0;

	ZERO( m_pInventory );
	FOREACH_PlayerNumber(p)
	{
//		switch( GAMESTATE->m_PlayMode )
//		{
//		case PLAY_MODE_BATTLE:
//			m_pInventory[p] = new Inventory;
//			m_pInventory[p]->Load( p );
//			this->AddChild( m_pInventory[p] );
//			break;
//		}
	}

	if( !GAMESTATE->m_bDemonstrationOrJukebox )	// only load if we're going to use it
	{
		m_Ready.Load( THEME->GetPathB(m_sName,"ready") );
		this->AddChild( &m_Ready );

		m_Go.Load( THEME->GetPathB(m_sName,"go") );
		this->AddChild( &m_Go );

		m_Cleared.Load( THEME->GetPathB(m_sName,"cleared") );
		m_Cleared.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 ); // on top of everything else
		this->AddChild( &m_Cleared );

		m_Failed.Load( THEME->GetPathB(m_sName,"failed") );
		m_Failed.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 ); // on top of everything else
		this->AddChild( &m_Failed );

		if( PREFSMAN->m_bAllowExtraStage && GAMESTATE->IsFinalStage() )	// only load if we're going to use it
			m_Extra.Load( THEME->GetPathB(m_sName,"extra1") );
		if( PREFSMAN->m_bAllowExtraStage && GAMESTATE->IsExtraStage() )	// only load if we're going to use it
			m_Extra.Load( THEME->GetPathB(m_sName,"extra2") );
		this->AddChild( &m_Extra );

		// only load if we're going to use it
		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
            FOREACH_PlayerNumber(p)
			{
				m_Win[p].Load( THEME->GetPathB(m_sName,ssprintf("win p%d",p+1)) );
				this->AddChild( &m_Win[p] );
			}
			m_Draw.Load( THEME->GetPathB(m_sName,"draw") );
			this->AddChild( &m_Draw );
			break;
		}

		m_textDebug.LoadFromFont( THEME->GetPathF("Common","normal") );
		m_textDebug.SetName( "Debug" );
		SET_XY( m_textDebug );
		m_textDebug.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 );	// just under transitions, over the foreground
		this->AddChild( &m_textDebug );


		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )	// only load if we're going to use it
		{
			m_textSurviveTime.LoadFromFont( THEME->GetPathF(m_sName,"survive time") );
			m_textSurviveTime.SetShadowLength( 0 );
			m_textSurviveTime.SetName( "SurviveTime" );
			SET_XY( m_textSurviveTime );
			m_textSurviveTime.SetDrawOrder( DRAW_ORDER_TRANSITIONS-1 );
			m_textSurviveTime.SetDiffuse( RageColor(1,1,1,0) );
			this->AddChild( &m_textSurviveTime );
		}
	}
	
	m_SongBackground.Init();

	if( !GAMESTATE->m_bDemonstrationOrJukebox )	// only load if we're going to use it
	{
		m_soundAssistTick.Load(	THEME->GetPathS(m_sName,"assist tick"), true );

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_BATTLE:
			m_soundBattleTrickLevel1.Load(	THEME->GetPathS(m_sName,"battle trick level1"), true );
			m_soundBattleTrickLevel2.Load(	THEME->GetPathS(m_sName,"battle trick level2"), true );
			m_soundBattleTrickLevel3.Load(	THEME->GetPathS(m_sName,"battle trick level3"), true );
			break;
		}
	}

	FOREACH_EnabledPlayer(pn)
		m_Player[pn].Init( 
			PLAYER_TYPE,
			GAMESTATE->m_pPlayerState[pn], 
			&STATSMAN->m_CurStageStats.m_player[pn],
			m_pLifeMeter[pn], 
			m_pCombinedLifeMeter, 
			m_pPrimaryScoreDisplay[pn], 
			m_pSecondaryScoreDisplay[pn], 
			m_pInventory[pn], 
			m_pPrimaryScoreKeeper[pn], 
			m_pSecondaryScoreKeeper[pn] );

	//
	// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
	//
	InitSongQueues();
	FOREACH_EnabledPlayer(pn)
		ASSERT( !m_vpStepsQueue[pn].empty() );

	FOREACH_EnabledPlayer(pn)
	{
		if( m_pPrimaryScoreKeeper[pn] )
			m_pPrimaryScoreKeeper[pn]->Load( m_apSongsQueue, m_vpStepsQueue[pn], m_asModifiersQueue[pn] );
		if( m_pSecondaryScoreKeeper[pn] )
			m_pSecondaryScoreKeeper[pn]->Load( m_apSongsQueue, m_vpStepsQueue[pn], m_asModifiersQueue[pn] );
	}

	/* LoadNextSong first, since that positions some elements which need to be
	 * positioned before we TweenOnScreen. */
	LoadNextSong();

	TweenOnScreen();

	this->SortByDrawOrder();

	m_GiveUpTimer.SetZero();

	// Get the transitions rolling on the first update.
	// We can't do this in the constructor because ScreenGameplay is constructed 
	// in the middle of ScreenStage.
}

//
// fill in m_apSongsQueue, m_vpStepsQueue, m_asModifiersQueue
//
void ScreenGameplay::InitSongQueues()
{
	LOG->Trace("InitSongQueues");
	if( GAMESTATE->IsCourseMode() )
	{
		Course* pCourse = GAMESTATE->m_pCurCourse;
		ASSERT( pCourse );

		m_apSongsQueue.clear();
		PlayerNumber pnMaster = GAMESTATE->m_MasterPlayerNumber;
		Trail *pTrail = GAMESTATE->m_pCurTrail[pnMaster];
		ASSERT( pTrail );
		FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
		{
			ASSERT( e->pSong );
			m_apSongsQueue.push_back( e->pSong );
		}

		FOREACH_EnabledPlayer(p)
		{
			Trail *pTrail = GAMESTATE->m_pCurTrail[p];
			ASSERT( pTrail );

			m_vpStepsQueue[p].clear();
			m_asModifiersQueue[p].clear();
			FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
			{
				ASSERT( e->pSteps );
				m_vpStepsQueue[p].push_back( e->pSteps );
				AttackArray a;
				e->GetAttackArray( a );
				m_asModifiersQueue[p].push_back( a );
			}
		}
	}
	else
	{
		m_apSongsQueue.push_back( GAMESTATE->m_pCurSong );
		FOREACH_PlayerNumber(p)
		{
			Steps *pSteps = GAMESTATE->m_pCurSteps[p];
			m_vpStepsQueue[p].push_back( pSteps );

			AttackArray aa;
			if( pSteps->GetDifficulty() == DIFFICULTY_BEGINNER && (bool)USE_FORCED_MODIFIERS_IN_BEGINNER )
				aa.push_back( Attack(ATTACK_LEVEL_1, 0, 0, FORCED_MODIFIERS_IN_BEGINNER, false, true, false) );	// don't show in AttackList

			m_asModifiersQueue[p].push_back( aa );
		}
	}

	// Fill StageStats
	STATSMAN->m_CurStageStats.vpPossibleSongs = m_apSongsQueue;
	FOREACH_PlayerNumber(p)
		STATSMAN->m_CurStageStats.m_player[p].vpPossibleSteps = m_vpStepsQueue[p];
}

ScreenGameplay::~ScreenGameplay()
{
	if( this->IsFirstUpdate() )
	{
		/* We never received any updates.  That means we were deleted without being
		 * used, and never actually played.  (This can happen when backing out of
		 * ScreenStage.)  Cancel the stage. */
		GAMESTATE->CancelStage();
	}

	LOG->Trace( "ScreenGameplay::~ScreenGameplay()" );
	
	if( !GAMESTATE->m_bDemonstrationOrJukebox )
		MEMCARDMAN->UnPauseMountingThread();

    FOREACH_PlayerNumber(p)
	{
		SAFE_DELETE( m_pLifeMeter[p] );
		SAFE_DELETE( m_pPrimaryScoreDisplay[p] );
		SAFE_DELETE( m_pSecondaryScoreDisplay[p] );
		SAFE_DELETE( m_pSecondaryScoreDisplay[p] );
		SAFE_DELETE( m_pPrimaryScoreKeeper[p] );
		SAFE_DELETE( m_pSecondaryScoreKeeper[p] );
		SAFE_DELETE( m_pInventory[p] );
	}
	SAFE_DELETE( m_pCombinedLifeMeter );
	if( m_pSoundMusic )
		m_pSoundMusic->StopPlaying();

	m_soundAssistTick.StopPlaying(); /* Stop any queued assist ticks. */

	NSMAN->ReportSongOver();

	// erase the last custom song data from memory.
	// XXX: this pulls in FILEMAN, which we really need to get out of here
	if( GAMESTATE->m_pCurSong && GAMESTATE->m_pCurSong->IsCustomSong() && FILEMAN->IsAFile(GAMESTATE->m_pCurSong->m_sGameplayMusic) )
		FILEMAN->Remove( GAMESTATE->m_pCurSong->m_sGameplayMusic );
}

bool ScreenGameplay::IsLastSong()
{
	if( GAMESTATE->m_pCurCourse  &&  GAMESTATE->m_pCurCourse->m_bRepeat )
		return false;
	return GAMESTATE->GetCourseSongIndex() >= (int)m_apSongsQueue.size()-1; // GetCourseSongIndex() is 0-based
}

void ScreenGameplay::SetupSong( int iSongIndex )
{
	FOREACH_EnabledPlayer( p )
	{
		/* This is the first beat that can be changed without it being visible.  Until
		 * we draw for the first time, any beat can be changed. */
		GAMESTATE->m_pPlayerState[p]->m_fLastDrawnBeat = -100;
		GAMESTATE->m_pCurSteps[p].Set( m_vpStepsQueue[p][iSongIndex] );

		/* Load new NoteData into Player.  Do this before 
		 * RebuildPlayerOptionsFromActiveAttacks or else transform mods will get
		 * propogated to GAMESTATE->m_PlayerOptions too early and be double-applied
		 * to the NoteData:
		 * once in Player::Load, then again in Player::ApplyActiveAttacks.  This 
		 * is very bad for transforms like AddMines.
		 */
		NoteData originalNoteData;
		GAMESTATE->m_pCurSteps[p]->GetNoteData( originalNoteData );
		
		const Style* pStyle = GAMESTATE->GetCurrentStyle();
		NoteData ndTransformed;
		pStyle->GetTransformedNoteDataForStyle( p, originalNoteData, ndTransformed );

		// load player
		{
			NoteData nd = ndTransformed;
			NoteDataUtil::RemoveAllTapsOfType( nd, TapNote::autoKeysound );
			m_Player[p].Load( nd );
		}

		// load auto keysounds
		{
			NoteData nd = ndTransformed;
			NoteDataUtil::RemoveAllTapsExceptForType( nd, TapNote::autoKeysound );
			m_AutoKeysounds.Load( p, nd );
		}

		// Put course options into effect.  Do this after Player::Load so
		// that mods aren't double-applied.
		GAMESTATE->m_pPlayerState[p]->m_ModsToApply.clear();
		for( unsigned i=0; i<m_asModifiersQueue[p][iSongIndex].size(); ++i )
		{
			Attack a = m_asModifiersQueue[p][iSongIndex][i];
			if( a.fStartSecond == 0 )
				a.fStartSecond = -1;	// now
			
			GAMESTATE->LaunchAttack( p, a );
			GAMESTATE->m_SongOptions.FromString( a.sModifiers );
		}

		// UGLY: Force updating the BeatToNoteSkin mapping and cache NoteSkins now, or else 
		// we'll do it on the first update and skip.
		m_Player[p].ApplyWaitingTransforms();

		/* Update attack bOn flags. */
		GAMESTATE->Update(0);
		GAMESTATE->RebuildPlayerOptionsFromActiveAttacks( p );

		/* Hack: Course modifiers that are set to start immediately shouldn't tween on. */
		GAMESTATE->m_pPlayerState[p]->m_CurrentPlayerOptions = GAMESTATE->m_pPlayerState[p]->m_PlayerOptions;
	}
}

void ScreenGameplay::LoadCourseSongNumber( int iSongNumber )
{
	if( !GAMESTATE->IsCourseMode() )
		return;
	const CString path = THEME->GetPathG( m_sName, ssprintf("course song %i",iSongNumber+1), true );
	if( path != "" )
		m_sprCourseSongNumber.Load( path );
	else
		m_sprCourseSongNumber.UnloadTexture();
	SCREENMAN->ZeroNextUpdate();
}

void ScreenGameplay::LoadNextSong()
{
	GAMESTATE->ResetMusicStatistics();

	FOREACH_EnabledPlayer( p )
	{
		STATSMAN->m_CurStageStats.m_player[p].iSongsPlayed++;
		m_textCourseSongNumber[p].SetText( ssprintf("%d", STATSMAN->m_CurStageStats.m_player[p].iSongsPlayed) );
	}

	LoadCourseSongNumber( GAMESTATE->GetCourseSongIndex() );

	int iPlaySongIndex = GAMESTATE->GetCourseSongIndex();
	iPlaySongIndex %= m_apSongsQueue.size();
	GAMESTATE->m_pCurSong.Set( m_apSongsQueue[iPlaySongIndex] );
	GAMESTATE->SetSongInProgress( GAMESTATE->m_pCurSong->GetSongDir() );
	STATSMAN->m_CurStageStats.vpPlayedSongs.push_back( GAMESTATE->m_pCurSong );

	// No need to do this here.  We do it in SongFinished().
	//GAMESTATE->RemoveAllActiveAttacks();

	// Restore the player's originally selected options.
	GAMESTATE->RestoreSelectedOptions();

	/* If we're in battery mode, force FailImmediate.  We assume in PlayerMinus::Step that
	 * failed players can't step. */
	if( GAMESTATE->m_SongOptions.m_LifeType == SongOptions::LIFE_BATTERY )
		GAMESTATE->m_SongOptions.m_FailType = SongOptions::FAIL_IMMEDIATE;

	m_textSongOptions.SetText( GAMESTATE->m_SongOptions.GetString() );

	SetupSong( iPlaySongIndex );

	FOREACH_EnabledPlayer( p )
	{
		Song* pSong = GAMESTATE->m_pCurSong;
		Steps* pSteps = GAMESTATE->m_pCurSteps[p];
		STATSMAN->m_CurStageStats.m_player[p].vpPlayedSteps.push_back( pSteps );

		ASSERT( GAMESTATE->m_pCurSteps[p] );
		m_textStepsDescription[p].SetText( GAMESTATE->m_pCurSteps[p]->GetDescription() );

		/* Increment the play count even if the player fails.  (It's still popular,
		 * even if the people playing it aren't good at it.) */
		if( !GAMESTATE->m_bDemonstrationOrJukebox )
			PROFILEMAN->IncrementStepsPlayCount( pSong, pSteps, p );

		m_textPlayerOptions[p].SetText( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.GetString() );
		m_ActiveAttackList[p].Refresh();

		// reset oni game over graphic
		SET_XY_AND_ON_COMMAND( m_sprOniGameOver[p] );

		if( GAMESTATE->m_SongOptions.m_LifeType==SongOptions::LIFE_BATTERY && STATSMAN->m_CurStageStats.m_player[p].bFailed )	// already failed
			ShowOniGameOver(p);

		if( GAMESTATE->m_SongOptions.m_LifeType==SongOptions::LIFE_BAR && 
			GAMESTATE->m_PlayMode == PLAY_MODE_REGULAR && 
			!GAMESTATE->IsEventMode() && 
			!GAMESTATE->m_bDemonstrationOrJukebox )
		{
			m_pLifeMeter[p]->UpdateNonstopLifebar(
				GAMESTATE->GetStageIndex(), 
				PREFSMAN->m_iSongsPerPlay, 
				PREFSMAN->m_iProgressiveStageLifebar);
		}
		if( GAMESTATE->m_SongOptions.m_LifeType==SongOptions::LIFE_BAR && GAMESTATE->m_PlayMode == PLAY_MODE_NONSTOP )
		{
			m_pLifeMeter[p]->UpdateNonstopLifebar(
				GAMESTATE->GetCourseSongIndex(), 
				GAMESTATE->m_pCurCourse->GetEstimatedNumStages(),
				PREFSMAN->m_iProgressiveNonstopLifebar);
		}

		m_DifficultyIcon[p].SetFromSteps( p, GAMESTATE->m_pCurSteps[p] );

		m_DifficultyMeter[p].SetName( m_sName + ssprintf(" DifficultyMeterP%d",p+1) );
		m_DifficultyMeter[p].SetFromSteps( GAMESTATE->m_pCurSteps[p] );

		/* The actual note data for scoring is the base class of Player.  This includes
		 * transforms, like Wide.  Otherwise, the scoring will operate on the wrong data. */
		m_pPrimaryScoreKeeper[p]->OnNextSong( GAMESTATE->GetCourseSongIndex(), GAMESTATE->m_pCurSteps[p], &m_Player[p].m_NoteData );
		if( m_pSecondaryScoreKeeper[p] )
			m_pSecondaryScoreKeeper[p]->OnNextSong( GAMESTATE->GetCourseSongIndex(), GAMESTATE->m_pCurSteps[p], &m_Player[p].m_NoteData );

		if( GAMESTATE->m_bDemonstrationOrJukebox )
		{
			GAMESTATE->m_pPlayerState[p]->m_PlayerController = PC_CPU;
			GAMESTATE->m_pPlayerState[p]->m_iCpuSkill = 5;
		}
		else if( GAMESTATE->IsCpuPlayer(p) )
		{
			GAMESTATE->m_pPlayerState[p]->m_PlayerController = PC_CPU;
			int iMeter = GAMESTATE->m_pCurSteps[p]->GetMeter();
			int iNewSkill = SCALE( iMeter, MIN_METER, MAX_METER, 0, NUM_SKILL_LEVELS-1 );
			/* Watch out: songs aren't actually bound by MAX_METER. */
			iNewSkill = clamp( iNewSkill, 0, NUM_SKILL_LEVELS-1 );
			GAMESTATE->m_pPlayerState[p]->m_iCpuSkill = iNewSkill;
		}
		else
		{
			GAMESTATE->m_pPlayerState[p]->m_PlayerController = PREFSMAN->m_AutoPlay;
		}
	}
	
	const bool bReverse[NUM_PLAYERS] = 
	{
		GAMESTATE->m_pPlayerState[PLAYER_1]->m_PlayerOptions.m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1,
		GAMESTATE->m_pPlayerState[PLAYER_2]->m_PlayerOptions.m_fScrolls[PlayerOptions::SCROLL_REVERSE] == 1
	};

	FOREACH_EnabledPlayer( p )
	{
		m_DifficultyIcon[p].SetName( ssprintf("DifficultyP%d%s",p+1,bReverse[p]?"Reverse":"") );
		SET_XY( m_DifficultyIcon[p] );

		m_DifficultyMeter[p].SetName( ssprintf("DifficultyMeterP%d%s",p+1,bReverse[p]?"Reverse":"") );
		SET_XY( m_DifficultyMeter[p] );
	}

	const bool bBothReverse = bReverse[PLAYER_1] && bReverse[PLAYER_2];
	const bool bOneReverse = !bBothReverse && (bReverse[PLAYER_1] || bReverse[PLAYER_2]);

	/* XXX: We want to put the lyrics out of the way, but it's likely that one
	 * player is in reverse and the other isn't.  What to do? */
	m_LyricDisplay.SetName( ssprintf( "Lyrics%s", bBothReverse? "Reverse": bOneReverse? "OneReverse": "") );
	SET_XY( m_LyricDisplay );

	m_SongFinished.Load( THEME->GetPathB(m_sName,"song finished") );

	// Load lyrics
	// XXX: don't load this here
	LyricsLoader LL;
	if( GAMESTATE->m_pCurSong->HasLyrics()  )
		LL.LoadFromLRCFile(GAMESTATE->m_pCurSong->GetLyricsPath(), *GAMESTATE->m_pCurSong);

	
	/* Set up song-specific graphics. */
	
	
	// Check to see if any players are in beginner mode.
	// Note: steps can be different if turn modifiers are used.
	if( PREFSMAN->m_bShowBeginnerHelper )
	{
		FOREACH_HumanPlayer( p )
		{
			if( GAMESTATE->m_pCurSteps[p]->GetDifficulty() == DIFFICULTY_BEGINNER )
				m_BeginnerHelper.AddPlayer( p, &m_Player[p].m_NoteData );
		}
	}

	m_SongBackground.Unload();

	if( !PREFSMAN->m_bShowBeginnerHelper || !m_BeginnerHelper.Initialize(2) )
	{
		m_BeginnerHelper.SetHidden( true );

		/* BeginnerHelper disabled, or failed to load. */
		m_SongBackground.LoadFromSong( GAMESTATE->m_pCurSong );

		if( !GAMESTATE->m_bDemonstrationOrJukebox )
		{
			/* This will fade from a preset brightness to the actual brightness (based
			 * on prefs and "cover").  The preset brightness may be 0 (to fade from
			 * black), or it might be 1, if the stage screen has the song BG and we're
			 * coming from it (like Pump).  This used to be done in SM_PlayReady, but
			 * that means it's impossible to snap to the new brightness immediately. */
			m_SongBackground.SetBrightness( INITIAL_BACKGROUND_BRIGHTNESS );
			m_SongBackground.FadeToActualBrightness();
		}
	}
	else
	{
		m_BeginnerHelper.SetHidden( false );
	}

	FOREACH_EnabledPlayer(p)
	{
		if( !STATSMAN->m_CurStageStats.m_player[p].bFailed )
		{
			// give a little life back between stages
			if( m_pLifeMeter[p] )
				m_pLifeMeter[p]->OnLoadSong();	
			if( m_pPrimaryScoreDisplay[p] )
				m_pPrimaryScoreDisplay[p]->OnLoadSong();	
			if( m_pSecondaryScoreDisplay[p] )
				m_pSecondaryScoreDisplay[p]->OnLoadSong();	
		}
	}
	if( m_pCombinedLifeMeter )
		m_pCombinedLifeMeter->OnLoadSong();	


	m_SongForeground.LoadFromSong( GAMESTATE->m_pCurSong );

	m_fTimeSinceLastDancingComment = 0;


	/* m_soundMusic and m_SongBackground take a very long time to load,
	 * so cap fDelta at 0 so m_NextSong will show up on screen.
	 * -Chris */
	m_bZeroDeltaOnNextUpdate = true;
	SCREENMAN->ZeroNextUpdate();

	/* Load the music last, since it may start streaming and we don't want the music
	 * to compete with other loading. */

	m_AutoKeysounds.FinishLoading();
	m_pSoundMusic = m_AutoKeysounds.GetSound();
}

void ScreenGameplay::LoadLights()
{
	if( !LIGHTSMAN->IsEnabled() )
		return;

	//
	// First, check if the song has explicit lights
	//
	m_CabinetLightsNoteData.Init();
	ASSERT( GAMESTATE->m_pCurSong );

	const Steps *pSteps = GAMESTATE->m_pCurSong->GetClosestNotes( STEPS_TYPE_LIGHTS_CABINET, DIFFICULTY_MEDIUM );
	if( pSteps != NULL )
	{
		pSteps->GetNoteData( m_CabinetLightsNoteData );
		return;
	}

	//
	// No explicit lights.  Create autogen lights.
	//
	if( PREFSMAN->m_bEasterEggs2 )
	{
		CString sGroup = GAMESTATE->m_pCurSong->m_sGroupName;
		sGroup.MakeLower();

		if( sGroup.find("dance dance revolution") != CString::npos || sGroup.find("ddr") != CString::npos )
		{
			m_bEasterEgg = true;
			pSteps = GAMESTATE->m_pCurSong->GetClosestNotes( STEPS_TYPE_DANCE_SINGLE, DIFFICULTY_MEDIUM );
			NoteData in;
			pSteps->GetNoteData( in );
			NoteDataUtil::LoadTransformedLightsDDR( in, m_CabinetLightsNoteData, GameManager::StepsTypeToNumTracks(STEPS_TYPE_LIGHTS_CABINET) );
			return;
		}
	}		

	CString sDifficulty = PREFSMAN->m_sLightsStepsDifficulty;
	vector<CString> asDifficulties;
	split( sDifficulty, ",", asDifficulties );

	Difficulty d1 = DIFFICULTY_INVALID;
	if( asDifficulties.size() > 0 )
		d1 = StringToDifficulty( asDifficulties[0] );

	pSteps = GAMESTATE->m_pCurSong->GetClosestNotes( LIGHTS_AUTOGEN_TYPE, d1 );

	// One last try...
	if( pSteps == NULL )
		pSteps = GAMESTATE->m_pCurSong->GetClosestNotes( GAMESTATE->GetCurrentStyle()->m_StepsType, d1 );

	// If we can't find anything at all, stop.
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
			NoteDataUtil::LoadTransformedLightsFromTwo( TapNoteData1, TapNoteData2, m_CabinetLightsNoteData );
			return;
		}

		/* fall through */
	}

	NoteDataUtil::LoadTransformedLights( TapNoteData1, m_CabinetLightsNoteData, GameManager::StepsTypeToNumTracks(STEPS_TYPE_LIGHTS_CABINET) );
}

float ScreenGameplay::StartPlayingSong(float MinTimeToNotes, float MinTimeToMusic)
{
	ASSERT(MinTimeToNotes >= 0);
	ASSERT(MinTimeToMusic >= 0);

	/* XXX: We want the first beat *in use*, so we don't delay needlessly. */
	const float fFirstBeat = GAMESTATE->m_pCurSong->m_fFirstBeat;
	const float fFirstSecond = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat( fFirstBeat );
	m_bSongIsCustom = GAMESTATE->m_pCurSong->m_bIsCustomSong;
	float fStartSecond = fFirstSecond - MinTimeToNotes;

	fStartSecond = min(fStartSecond, -MinTimeToMusic);
	
	RageSoundParams p;
	p.AccurateSync = true;
	p.SetPlaybackRate( GAMESTATE->m_SongOptions.m_fMusicRate );
	p.StopMode = RageSoundParams::M_CONTINUE;
	p.m_StartSecond = fStartSecond;

	// Silence music if not playing attract sounds in demonstration.
	if( GAMESTATE->m_bDemonstrationOrJukebox && !GAMESTATE->IsTimeToPlayAttractSounds() )
		p.m_Volume = 0;
	
	ASSERT( !m_pSoundMusic->IsPlaying() );
	m_pSoundMusic->Play( &p );
	if( m_bPaused )
		m_pSoundMusic->Pause( true );

	/* Make sure GAMESTATE->m_fMusicSeconds is set up. */
	GAMESTATE->m_fMusicSeconds = -5000;
	UpdateSongPosition(0);

	ASSERT( GAMESTATE->m_fMusicSeconds > -4000 ); /* make sure the "fake timer" code doesn't trigger */

	/* Return the amount of time until the first beat. */
	return fFirstSecond - fStartSecond;
}


void ScreenGameplay::PauseGame( bool bPause, GameController gc )
{
	if( m_bPaused == bPause )
	{
		LOG->Trace( "ScreenGameplay::PauseGame(%i) received, but already in that state; ignored", bPause );
		return;
	}

	/* Don't pause if we're already tweening out. */
	if( bPause && m_DancingState == STATE_OUTRO )
		return;
	
	AbortGiveUp( false );

	m_bPaused = bPause;
	m_PauseController = gc;

	m_pSoundMusic->Pause( bPause );
	if( bPause )
		this->PlayCommand( "Pause" );
	else
		this->PlayCommand( "Unpause" );

	FOREACH_EnabledPlayer(p)
		m_Player[p].SetPaused( m_bPaused );
}

// XXX: construction zone
void ScreenGameplay::CompareScores()
{
	if( !m_bCompareScores )
		return;

	// keep these in memory, so we don't re-init each time
	static float fPercentP1, fPercentP2;
	static PlayerNumber pn_higher;
	
	// XXX: make this based off something less hard-coded.
	fPercentP1 = ftruncf( STATSMAN->m_CurStageStats.m_player[PLAYER_1].GetPercentDancePoints(), 0.0001f );
	fPercentP2 = ftruncf( STATSMAN->m_CurStageStats.m_player[PLAYER_2].GetPercentDancePoints(), 0.0001f );

	if( fPercentP1 == fPercentP2 )
		pn_higher = PLAYER_INVALID; /* tie sentinel */
	else
		pn_higher = fPercentP1 > fPercentP2 ? PLAYER_1 : PLAYER_2;

	// don't need to update
	if( m_LeadingPlayer == pn_higher )
		return;

	m_LeadingPlayer = pn_higher;

	/* mark each player as ahead */
	if( pn_higher == PLAYER_INVALID )
	{
		FOREACH_EnabledPlayer( p )
			m_pPrimaryScoreDisplay[p]->PlayCommand( "Ahead" );

		return;
	}

	/* set each player accordingly */
	FOREACH_EnabledPlayer( p )
		m_pPrimaryScoreDisplay[p]->PlayCommand( (p == pn_higher) ? "Ahead" : "Behind" );
}

	


// play assist ticks
void ScreenGameplay::PlayTicks()
{
	if( !GAMESTATE->m_SongOptions.m_bAssistTick )
		return;

	/* Sound cards have a latency between when a sample is Play()ed and when the sound
	 * will start coming out the speaker.  Compensate for this by boosting fPositionSeconds
	 * ahead.  This is just to make sure that we request the sound early enough for it to
	 * come out on time; the actual precise timing is handled by SetStartTime. */
	float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
	fPositionSeconds += SOUND->GetPlayLatency() + (float)TICK_EARLY_SECONDS + 0.250f;
	const float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds );

	const int iSongRow = max( 0, BeatToNoteRowNotRounded( fSongBeat ) );
	static int iRowLastCrossed = -1;
	if( iSongRow < iRowLastCrossed )
		iRowLastCrossed = -1;

	int iTickRow = -1;
	// for each index we crossed since the last update:
	FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( m_Player[GAMESTATE->m_MasterPlayerNumber].m_NoteData, r, iRowLastCrossed+1, iSongRow+1 )
		if( m_Player[GAMESTATE->m_MasterPlayerNumber].m_NoteData.IsThereATapOrHoldHeadAtRow( r ) )
			iTickRow = r;

	iRowLastCrossed = iSongRow;

	if( iTickRow != -1 )
	{
		const float fTickBeat = NoteRowToBeat( iTickRow );
		const float fTickSecond = GAMESTATE->m_pCurSong->m_Timing.GetElapsedTimeFromBeat( fTickBeat );
		float fSecondsUntil = fTickSecond - GAMESTATE->m_fMusicSeconds;
		fSecondsUntil /= GAMESTATE->m_SongOptions.m_fMusicRate; /* 2x music rate means the time until the tick is halved */

		RageSoundParams p;
		p.StartTime = GAMESTATE->m_LastBeatUpdate + (fSecondsUntil - (float)TICK_EARLY_SECONDS);
		m_soundAssistTick.Play( &p );
	}
}

/* Play announcer "type" if it's been at least fSeconds since the last announcer. */
void ScreenGameplay::PlayAnnouncer( CString type, float fSeconds )
{
	if( GAMESTATE->m_fOpponentHealthPercent == 0 )
		return; // Shut the announcer up

	/* Don't play in demonstration. */
	if( GAMESTATE->m_bDemonstrationOrJukebox )
		return;

	/* Don't play before the first beat, or after we're finished. */
	if( m_DancingState != STATE_DANCING )
		return;
	if( GAMESTATE->m_pCurSong == NULL  ||	// this will be true on ScreenDemonstration sometimes
		GAMESTATE->m_fSongBeat < GAMESTATE->m_pCurSong->m_fFirstBeat )
		return;


	if( m_fTimeSinceLastDancingComment < fSeconds )
		return;
	m_fTimeSinceLastDancingComment = 0;

	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( type ));

	if( m_pCombinedLifeMeter )
		m_pCombinedLifeMeter->OnTaunt();
}

void ScreenGameplay::UpdateSongPosition( float fDeltaTime )
{
	if( !m_pSoundMusic->IsPlaying() )
		return;

	RageTimer tm;
	const float fSeconds = m_pSoundMusic->GetPositionSeconds( NULL, &tm );
	const float fAdjust = SOUND->GetFrameTimingAdjustment( fDeltaTime );

	float fSecondsTotal = fSeconds+fAdjust; 

	GAMESTATE->UpdateSongPosition( fSecondsTotal, GAMESTATE->m_pCurSong->m_Timing, tm+fAdjust );
}

void ScreenGameplay::Update( float fDeltaTime )
{
	if( GAMESTATE->m_pCurSong == NULL  )
	{
		/* ScreenDemonstration will move us to the next screen.  We just need to
		 * survive for one update without crashing.  We need to call Screen::Update
		 * to make sure we receive the next-screen message. */
		Screen::Update( fDeltaTime );
		return;
	}

	if( m_bFirstUpdate )
	{
		// load the lights and manually update, then set the state below,
		// so we avoid skipping between the stage and gameplay.
		LoadLights();
		UpdateLights();

		SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay intro" ));	// crowd cheer

		//
		// Get the transitions rolling
		//
		if( GAMESTATE->m_bDemonstrationOrJukebox )
		{
			StartPlayingSong( 0, 0 );	// *kick* (no transitions)
		}
		else if ( NSMAN->useSMserver )
		{
			//If we're using networking, we must not have any
			//delay.  If we do this can cause inconsistancy
			//on different computers and differet themes

			StartPlayingSong( 0, 0 );
			m_pSoundMusic->Stop();

			float startOffset = g_fNetStartOffset;

			NSMAN->StartRequest(1); 

			RageSoundParams p;
			p.AccurateSync = true;
			p.SetPlaybackRate( 1.0 );	//Force 1.0 playback speed
			p.StopMode = RageSoundParams::M_CONTINUE;
			p.m_StartSecond = startOffset;
			m_pSoundMusic->Play( &p );

			UpdateSongPosition(0);
		}
		else
		{
			float fMinTimeToMusic = m_In.GetLengthSeconds();	// start of m_Ready
			float fMinTimeToNotes = fMinTimeToMusic + m_Ready.GetLengthSeconds() + m_Go.GetLengthSeconds()+2;	// end of Go

			/*
			 * Tell the music to start, but don't actually make any noise for
			 * at least 2.5 (or 1.5) seconds.  (This is so we scroll on screen smoothly.)
			 *
			 * This is only a minimum: the music might be started later, to meet
			 * the minimum-time-to-notes value.  If you're writing song data,
			 * and you want to make sure we get ideal timing here, make sure there's
			 * a bit of space at the beginning of the music with no steps. 
			 */

			/*float delay =*/ StartPlayingSong( fMinTimeToNotes, fMinTimeToMusic );
		}

		if( GAMESTATE->m_bDemonstrationOrJukebox )
			LIGHTSMAN->SetLightsMode( LIGHTSMODE_DEMONSTRATION );
		else
			LIGHTSMAN->SetLightsMode( LIGHTSMODE_GAMEPLAY );
	}


	UpdateSongPosition( fDeltaTime );

	if( m_bZeroDeltaOnNextUpdate )
	{
		Screen::Update( 0 );
		m_bZeroDeltaOnNextUpdate = false;
	}
	else
	{
		Screen::Update( fDeltaTime );
	}

	/* This happens if ScreenDemonstration::HandleScreenMessage sets a new screen when
	 * PREFSMAN->m_bDelayedScreenLoad. */
	if( GAMESTATE->m_pCurSong == NULL )
		return;
	/* This can happen if ScreenDemonstration::HandleScreenMessage sets a new screen when
	 * !PREFSMAN->m_bDelayedScreenLoad.  (The new screen was loaded when we called Screen::Update,
	 * and the ctor might set a new GAMESTATE->m_pCurSong, so the above check can fail.) */
	if( SCREENMAN->GetTopScreen() != this )
		return;

	/* Update actors when paused, but never move on to another state. */
	if( m_bPaused )
		return;

	if( GAMESTATE->m_MasterPlayerNumber != PLAYER_INVALID && !m_MaxCombo.GetHidden() )
		m_MaxCombo.SetText( ssprintf("%d", STATSMAN->m_CurStageStats.m_player[GAMESTATE->m_MasterPlayerNumber].iMaxCombo) ); /* MAKE THIS WORK FOR BOTH PLAYERS! */
	
	//LOG->Trace( "m_fOffsetInBeats = %f, m_fBeatsPerSecond = %f, m_Music.GetPositionSeconds = %f", m_fOffsetInBeats, m_fBeatsPerSecond, m_Music.GetPositionSeconds() );

	m_AutoKeysounds.Update(fDeltaTime);

	//
	// update GameState HealthState
	//
    FOREACH_EnabledPlayer(p)
	{
		if(
			(m_pLifeMeter[p] && m_pLifeMeter[p]->IsFailing()) || 
			(m_pCombinedLifeMeter && m_pCombinedLifeMeter->IsFailing(p)) )
		{
			GAMESTATE->m_pPlayerState[p]->m_HealthState = PlayerState::DEAD;
		}
		else if(
			(m_pLifeMeter[p] && m_pLifeMeter[p]->IsHot()) || 
			(m_pCombinedLifeMeter && m_pCombinedLifeMeter->IsHot(p)) )
		{
			GAMESTATE->m_pPlayerState[p]->m_HealthState = PlayerState::HOT;
		}
		else if( 
			(m_pLifeMeter[p] && m_pLifeMeter[p]->IsInDanger()) || 
			(m_pCombinedLifeMeter && m_pCombinedLifeMeter->IsInDanger(p)) )
		{
			GAMESTATE->m_pPlayerState[p]->m_HealthState = PlayerState::DANGER;
		}
		else
		{
			GAMESTATE->m_pPlayerState[p]->m_HealthState = PlayerState::ALIVE;
		}
	}


	switch( m_DancingState )
	{
	case STATE_DANCING:
		/* Set STATSMAN->m_CurStageStats.bFailed for failed players.  In, FAIL_IMMEDIATE, send
		 * SM_BeginFailed if all players failed, and kill dead Oni players. */
		FOREACH_EnabledPlayer( pn )
		{
			SongOptions::FailType ft = GAMESTATE->GetPlayerFailType(pn);
			SongOptions::LifeType lt = GAMESTATE->m_SongOptions.m_LifeType;

			if( ft == SongOptions::FAIL_OFF )
				continue;
			
			// check for individual fail
			if( (m_pLifeMeter[pn] && !m_pLifeMeter[pn]->IsFailing()) || 
				(m_pCombinedLifeMeter && !m_pCombinedLifeMeter->IsFailing(pn)) )
				continue; /* isn't failing */
			if( STATSMAN->m_CurStageStats.m_player[pn].bFailed )
				continue; /* failed and is already dead */
		
			/* If recovery is enabled, only set fail if both are failing.
			* There's no way to recover mid-song in battery mode. */
			if( lt != SongOptions::LIFE_BATTERY &&
				PREFSMAN->m_bTwoPlayerRecovery && !GAMESTATE->AllAreDead() )
				continue;

			LOG->Trace("Player %d failed", (int)pn);
			STATSMAN->m_CurStageStats.m_player[pn].bFailed = true;	// fail

			//
			// Check for and do Oni die.
			//
			bool bAllowOniDie = false;
			switch( lt )
			{
			case SongOptions::LIFE_BATTERY:
				bAllowOniDie = true;
				break;
			}
			if( bAllowOniDie && ft == SongOptions::FAIL_IMMEDIATE )
			{
				if( !STATSMAN->m_CurStageStats.AllFailedEarlier() )	// if not the last one to fail
				{
					// kill them!
					SOUND->PlayOnceFromDir( THEME->GetPathS(m_sName,"oni die") );
					ShowOniGameOver(pn);
					m_Player[pn].m_NoteData.Init();		// remove all notes and scoring
					m_Player[pn].FadeToFail();	// tell the NoteField to fade to white
				}
			}
		}

		bool bAllFailed = true;
		FOREACH_EnabledPlayer( pn )
		{
			SongOptions::FailType ft = GAMESTATE->GetPlayerFailType(pn);
			switch( ft )
			{
			case SongOptions::FAIL_IMMEDIATE:
				if( GAMESTATE->m_pPlayerState[pn]->m_HealthState < PlayerState::DEAD )
					bAllFailed = false;
				break;
			case SongOptions::FAIL_END_OF_SONG:
				bAllFailed = false;	// wait until the end of the song to fail.
				break;
			case SongOptions::FAIL_OFF:
				bAllFailed = false;	// never fail.
				break;
			default:
				ASSERT(0);
			}
		}
		
		if( bAllFailed )
			SCREENMAN->PostMessageToTopScreen( SM_BeginFailed, 0 );

		//
		// Update living players' alive time
		//
		FOREACH_EnabledPlayer(pn)
			if(!STATSMAN->m_CurStageStats.m_player[pn].bFailed)
				STATSMAN->m_CurStageStats.m_player[pn].fAliveSeconds += fDeltaTime * GAMESTATE->m_SongOptions.m_fMusicRate;

		// update fGameplaySeconds
		STATSMAN->m_CurStageStats.fGameplaySeconds += fDeltaTime;
		if( GAMESTATE->m_fSongBeat >= GAMESTATE->m_pCurSong->m_fFirstBeat && GAMESTATE->m_fSongBeat < GAMESTATE->m_pCurSong->m_fLastBeat )
			STATSMAN->m_CurStageStats.fStepsSeconds += fDeltaTime;

		//
		// Check for end of song
		//
		{
			float fSecondsToStop = GAMESTATE->m_pCurSong->GetElapsedTimeFromBeat( GAMESTATE->m_pCurSong->m_fLastBeat );

			/* Make sure we keep going long enough to register a miss for the last note. */
			fSecondsToStop += m_Player[GAMESTATE->m_MasterPlayerNumber].GetMaxStepDistanceSeconds();

			if( GAMESTATE->m_fMusicSeconds > fSecondsToStop && !m_SongFinished.IsTransitioning() && !m_NextSong.IsTransitioning() )
				m_SongFinished.StartTransitioning( SM_NotesEnded );
		}
	
		//
		// update 2d dancing characters
		//
        FOREACH_EnabledPlayer(p)
		{
			DancingCharacters *pCharacter = m_SongBackground.GetDancingCharacters();
			if( pCharacter != NULL )
			{
				TapNoteScore tns = m_Player[p].GetLastTapNoteScore();
				
				ANIM_STATES_2D state = AS2D_MISS;

				switch( tns )
				{
				case TNS_GOOD:
				case TNS_GREAT:
					state = AS2D_GOOD;
					break;
				case TNS_PERFECT:
				case TNS_MARVELOUS:
					state = AS2D_GREAT;
					break;
				default:
					state = AS2D_MISS;
					break;
				}

				if( state == AS2D_GREAT && m_pLifeMeter[p] && m_pLifeMeter[p]->GetLife() == 1.0f ) // full life
					state = AS2D_FEVER;

				pCharacter->Change2DAnimState( p, state );
			}
		}

		//
		// Check for enemy death in enemy battle
		//
		static float fLastSeenEnemyHealth = 1;
		if( fLastSeenEnemyHealth != GAMESTATE->m_fOpponentHealthPercent )
		{
			fLastSeenEnemyHealth = GAMESTATE->m_fOpponentHealthPercent;

			if( GAMESTATE->m_fOpponentHealthPercent == 0 )
			{
				// HACK:  Load incorrect directory on purpose for now.
				PlayAnnouncer( "gameplay battle damage level3", 0 );

				GAMESTATE->RemoveAllActiveAttacks();

                FOREACH_CpuPlayer(p)
				{
					SOUND->PlayOnceFromDir( THEME->GetPathS(m_sName,"oni die") );
                    ShowOniGameOver( p );
                    m_Player[p].m_NoteData.Init();		// remove all notes and scoring
                    m_Player[p].FadeToFail();	// tell the NoteField to fade to white
				}
			}
		}

		//
		// Check to see if it's time to play a ScreenGameplay comment
		//
		m_fTimeSinceLastDancingComment += fDeltaTime;

		switch( GAMESTATE->m_PlayMode )
		{
		case PLAY_MODE_REGULAR:
		case PLAY_MODE_BATTLE:
		case PLAY_MODE_RAVE:
			if( GAMESTATE->OneIsHot() )			
				PlayAnnouncer( "gameplay comment hot", SECONDS_BETWEEN_COMMENTS );
			else if( GAMESTATE->AllAreInDangerOrWorse() )	
				PlayAnnouncer( "gameplay comment danger", SECONDS_BETWEEN_COMMENTS );
			else
				PlayAnnouncer( "gameplay comment good", SECONDS_BETWEEN_COMMENTS );
			break;
		case PLAY_MODE_NONSTOP:
		case PLAY_MODE_ONI:
		case PLAY_MODE_ENDLESS:
			PlayAnnouncer( "gameplay comment oni", SECONDS_BETWEEN_COMMENTS );
			break;
		default:
			ASSERT(0);
		}
	}

	//
	// update give up
	//
	bool bGiveUpTimerFired = !m_GiveUpTimer.IsZero() && m_GiveUpTimer.Ago() > g_fGiveUpTime.Get();
	if( bGiveUpTimerFired || (FAIL_AFTER_30_MISSES && GAMESTATE->AllHumanHaveComboOf30OrMoreMisses()) )
	{
		// Give up

		FOREACH_PlayerNumber( p )
		{
			STATSMAN->m_CurStageStats.m_player[p].bGaveUp = true;
			STATSMAN->m_CurStageStats.m_player[p].bFailed |= GAMESTATE->AllHumanHaveComboOf30OrMoreMisses();
		}

		m_GiveUpTimer.SetZero();

		if( GIVING_UP_GOES_TO_PREV_SCREEN )
		{
			BackOutFromGameplay();

		}
		else if( GIVING_UP_GOES_TO_NEXT_SCREEN )
		{
			HandleScreenMessage( SM_LeaveGameplay );
			return;
		}
		else
		{
			this->PostScreenMessage( SM_NotesEnded, 0 );
		}
		return;
	}

	//
	// update bpm display
	//
	// XXX shouldn't this be BPMDisplay's job?
#if 0
	if( m_fLastBPS != GAMESTATE->m_fCurBPS && !m_BPMDisplay.GetHidden() )
	{
		m_fLastBPS = GAMESTATE->m_fCurBPS;

		/* change the BPM display to match the actual rate. */
		float fNewBPM = GAMESTATE->m_fCurBPS * 60.0f;
		fNewBPM *= GAMESTATE->m_SongOptions.m_fMusicRate;

		m_BPMDisplay.SetConstantBpm( fNewBPM );
	}
#endif
	m_BPMDisplay.SetConstantBpm( GAMESTATE->m_fCurBPS * 60.0f );

	CompareScores();

	PlayTicks();

	UpdateLights();

	SendCrossedMessages();

	if( NSMAN->useSMserver )
	{
		FOREACH_EnabledPlayer( pn2 )
			if( m_pLifeMeter[pn2] )
				NSMAN->m_playerLife[pn2] = int(m_pLifeMeter[pn2]->GetLife()*10000);

		FOREACH_NSScoreBoardColumn(cn)
			if( m_ShowScoreboard && NSMAN->ChangedScoreboard(cn) )
				m_Scoreboard[cn].SetText( NSMAN->m_Scoreboard[cn] );
	}
}

void ScreenGameplay::UpdateLights()
{
	if( !LIGHTSMAN->IsEnabled() )
		return;
	if( m_CabinetLightsNoteData.GetNumTracks() == 0 )	// light data wasn't loaded
		return;

	const Style* pStyle = GAMESTATE->GetCurrentStyle();
	bool bBlinkCabinetLight[NUM_CABINET_LIGHTS];
	bool bBlinkGameButton[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS];
	ZERO( bBlinkCabinetLight );
	ZERO( bBlinkGameButton );
	{
		const float fSongBeat = GAMESTATE->m_fLightSongBeat;
		const int iSongRow = BeatToNoteRowNotRounded( fSongBeat );

		static int iRowLastCrossed = 0;

		FOREACH_CabinetLight( cl )
		{	
			// for each index we crossed since the last update:
			FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( m_CabinetLightsNoteData, cl, r, iRowLastCrossed+1, iSongRow+1 )
			{
				if( m_CabinetLightsNoteData.GetTapNote( cl, r ).type != TapNote::empty )
					bBlinkCabinetLight[cl] = true;
			}

			if( m_CabinetLightsNoteData.IsHoldNoteAtBeat( cl, iSongRow ) )
				bBlinkCabinetLight[cl] = true;
		}

		FOREACH_EnabledPlayer( pn )
		{
			for( int t=0; t<m_Player[pn].m_NoteData.GetNumTracks(); t++ )
			{
				bool bBlink = false;

				// for each index we crossed since the last update:
				FOREACH_NONEMPTY_ROW_IN_TRACK_RANGE( m_Player[pn].m_NoteData, t, r, iRowLastCrossed+1, iSongRow+1 )
				{
					TapNote tn = m_Player[pn].m_NoteData.GetTapNote(t,r);
					if( tn.type != TapNote::empty && tn.type != TapNote::mine )
						bBlink = true;
				}

				// check if a hold should be active
				if( m_Player[pn].m_NoteData.IsHoldNoteAtBeat( t, iSongRow ) )
					bBlink = true;

				if( bBlink )
				{
					StyleInput si( pn, t );
					GameInput gi = pStyle->StyleInputToGameInput( si );
					bBlinkGameButton[gi.controller][gi.button] = true;
				}
			}
		}

		iRowLastCrossed = iSongRow;
	}

	// Before the first beat of the song, all cabinet lights solid on (except for menu buttons).
	bool bOverrideCabinetBlink = !m_bEasterEgg && (GAMESTATE->m_fSongBeat < GAMESTATE->m_pCurSong->m_fFirstBeat);
	FOREACH_CabinetLight( cl )
	{
		switch( cl )
		{
		case LIGHT_BUTTONS_LEFT:
		case LIGHT_BUTTONS_RIGHT:
			// don't blink
			break;
		default:
			bBlinkCabinetLight[cl] |= bOverrideCabinetBlink;
			break;
		}
	}

	// perform some voodoo
	float fLength = 0;
	if( m_bEasterEgg )
	{
		int iSeg = GAMESTATE->m_pCurSong->m_Timing.GetBPMSegmentIndexAtBeat( GAMESTATE->m_fSongBeat + 0.5 );
		fLength = (0.25 / GAMESTATE->m_pCurSong->m_Timing.m_BPMSegments[iSeg].m_fBPS) + 0.01;
		fLength = max( fLength, 0.0425f );
	}	

	// Send blink data.
	FOREACH_CabinetLight( cl )
	{
		if( bBlinkCabinetLight[cl] )
			LIGHTSMAN->BlinkCabinetLight( cl, fLength );
	}

	FOREACH_GameController( gc )
	{
		FOREACH_GameButton( gb )
		{
			if( bBlinkGameButton[gc][gb] )
				LIGHTSMAN->BlinkGameButton( GameInput(gc,gb), fLength );
		}
	}
}

void ScreenGameplay::SendCrossedMessages()
{
	{
		static int iRowLastCrossed = 0;
		float fPositionSeconds = GAMESTATE->m_fMusicSeconds;
		float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds );

		int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
		iRowNow = max( 0, iRowNow );

		for( int r=iRowLastCrossed+1; r<=iRowNow; r++ )
		{
			if( GetNoteType( r ) == NOTE_TYPE_4TH )
				MESSAGEMAN->Broadcast( MESSAGE_BEAT_CROSSED );
		}

		iRowLastCrossed = iRowNow;
	}

	{
		const int NUM_MESSAGES_TO_SEND = 4;
		const float MESSAGE_SPACING_SECONDS = 0.5f;

		PlayerNumber pn = PLAYER_INVALID;
		FOREACH_EnabledPlayer( p )
		{
			if( GAMESTATE->m_pCurSteps[p]->GetDifficulty() == DIFFICULTY_BEGINNER )
			{
				pn = p;
				break;
			}
		}
		if( pn == PLAYER_INVALID )
			return;

		NoteData &nd = m_Player[pn].m_NoteData;

		/* send ITG2's old Beginner messages */
		static int iRowLastCrossedAll[NUM_MESSAGES_TO_SEND] = { 0, 0, 0, 0 };

		for( int i=0; i<NUM_MESSAGES_TO_SEND; i++ )
		{
			float fOffsetFromCurrentSeconds = MESSAGE_SPACING_SECONDS * i;

			float fPositionSeconds = GAMESTATE->m_fMusicSeconds + fOffsetFromCurrentSeconds;
			float fSongBeat = GAMESTATE->m_pCurSong->GetBeatFromElapsedTime( fPositionSeconds );

			int iRowNow = BeatToNoteRowNotRounded( fSongBeat );
			iRowNow = max( 0, iRowNow );
			int &iRowLastCrossed = iRowLastCrossedAll[i];

			FOREACH_NONEMPTY_ROW_ALL_TRACKS_RANGE( nd, r, iRowLastCrossed+1, iRowNow+1 )
			{
				int iNumTracksWithTapOrHoldHead = 0;
				for( int t=0; t<nd.GetNumTracks(); t++ )
				{
					if( nd.GetTapNote(t,r).type == TapNote::empty )
						continue;

					iNumTracksWithTapOrHoldHead++;

					// Send col-specific crossed
					if( i == 0 )
					{
						StyleInput si( pn, t );
						const Game* pGame = GAMESTATE->GetCurrentGame();
						CString sButton = pGame->ColToButtonName( t );
						CString sMessageName = "NoteCrossed" + sButton;
						MESSAGEMAN->Broadcast( sMessageName );
					}
				}

				if( iNumTracksWithTapOrHoldHead > 0 )
					MESSAGEMAN->Broadcast( (Message)(MESSAGE_NOTE_CROSSED + i) );
				if( i == 0  &&  iNumTracksWithTapOrHoldHead >= 2 )
				{
					CString sMessageName = "NoteCrossedJump";
					MESSAGEMAN->Broadcast( sMessageName );
				}
			}

			iRowLastCrossed = iRowNow;
		}


		/* XXX: redundant. How can we use the last crossed ints above? */
		static int iRowLastBeatCrossed = 0;
		int iBeatRow = BeatToNoteRow( int(GAMESTATE->m_fSongBeat) );
		int iRowNow = BeatToNoteRowNotRounded( GAMESTATE->m_fSongBeat );
		iRowNow = max( 0, iRowNow );

		/* send our beat-based messages - the above takes care of NoteCrossed. */
		if( iBeatRow >= iRowLastBeatCrossed+1 && iBeatRow <= iRowNow+1 )
		{
			for( int i = 1; i < NUM_MESSAGES_TO_SEND; i++ )
			{
				int iRowToCheck = BeatToNoteRow( (int)(GAMESTATE->m_fSongBeat+i) );

				if( nd.IsThereATapOrHoldHeadAtRow(iRowToCheck) )
				{
					Message msg = (Message)(MESSAGE_NOTE_WILL_CROSS_IN_1_BEAT+(i-1));
					MESSAGEMAN->Broadcast( msg );
				}
			}
		}

		iRowLastBeatCrossed = iRowNow;
	}
}

void ScreenGameplay::BackOutFromGameplay()
{
	m_DancingState = STATE_OUTRO;
	AbortGiveUp( false );
	
	m_pSoundMusic->StopPlaying();
	m_soundAssistTick.StopPlaying(); /* Stop any queued assist ticks. */

	this->ClearMessageQueue();
	m_Cancel.StartTransitioning( SM_GoToScreenAfterBack );
}

void ScreenGameplay::AbortGiveUp( bool bShowText )
{
	if( m_GiveUpTimer.IsZero() )
		return;

	if ( !bShowText )
		GAMESTATE->SetSongInProgress("(none)");

	m_textDebug.StopTweening();
	if( bShowText )
		m_textDebug.SetText( GIVE_UP_ABORTED_TEXT );
	// otherwise tween out the text that's there

	m_textDebug.BeginTweening( 1/2.f );
	m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
	m_GiveUpTimer.SetZero();
}


void ScreenGameplay::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	//LOG->Trace( "ScreenGameplay::Input()" );

	if( type == IET_LEVEL_CHANGED )
		return;

	if( m_bPaused )
	{
		/* If we're paused, only accept MENU_BUTTON_START to unpause. */
		if( MenuI.IsValid() && GAMESTATE->IsHumanPlayer(MenuI.player) && MenuI.button == MENU_BUTTON_START && type == IET_FIRST_PRESS )
		{
			if( m_PauseController == GAME_CONTROLLER_INVALID || m_PauseController == GameI.controller )
				this->PauseGame( false );
		}
		return;
	}

	if( MenuI.IsValid()  &&  
		m_DancingState != STATE_OUTRO  &&
		GAMESTATE->IsHumanPlayer(MenuI.player) &&
		!m_Cancel.IsTransitioning() )
	{
		/* Allow bailing out by holding the START button of all active players.  This
		 * gives a way to "give up" when a back button isn't available.  Doing this is
		 * treated as failing the song, unlike BACK, since it's always available.
		 *
		 * However, if this is also a style button, don't do this. (pump center = start) */
		bool bHoldingGiveUp = false;
		bHoldingGiveUp |= ( START_GIVES_UP && MenuI.button == MENU_BUTTON_START && !StyleI.IsValid() );
		bHoldingGiveUp |= ( BACK_GIVES_UP && MenuI.button == MENU_BUTTON_BACK && !StyleI.IsValid() );
		
		if( bHoldingGiveUp )
		{
			/* No PREFSMAN->m_bDelayedEscape; always delayed. */
			if( type==IET_RELEASE )
				AbortGiveUp( true );
			else if( type==IET_FIRST_PRESS && m_GiveUpTimer.IsZero() )
			{
				m_textDebug.SetText( GIVE_UP_TEXT );
				m_textDebug.StopTweening();
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
				m_textDebug.BeginTweening( 1/8.f );
				m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
				m_GiveUpTimer.Touch(); /* start the timer */
			}

			return;
		}

		/* Only handle MENU_BUTTON_BACK as a regular BACK button if BACK_GIVES_UP is
		 * disabled. */
		if( MenuI.button == MENU_BUTTON_BACK && !BACK_GIVES_UP )
		{
			if( ((!PREFSMAN->m_bDelayedBack && type==IET_FIRST_PRESS) ||
				(DeviceI.device==DEVICE_KEYBOARD && (type==IET_SLOW_REPEAT||type==IET_FAST_REPEAT)) ||
				(DeviceI.device!=DEVICE_KEYBOARD && type==IET_FAST_REPEAT)) )
			{
				LOG->Trace("Player %i went back", MenuI.player+1);
				BackOutFromGameplay();
			}
			else if( PREFSMAN->m_bDelayedBack && type==IET_FIRST_PRESS )
			{
				m_textDebug.SetText( "Continue holding BACK to quit" );
				m_textDebug.StopTweening();
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
				m_textDebug.BeginTweening( 1/8.f );
				m_textDebug.SetDiffuse( RageColor(1,1,1,1) );
			}
			else if( PREFSMAN->m_bDelayedBack && type==IET_RELEASE )
			{
				m_textDebug.StopTweening();
				m_textDebug.BeginTweening( 1/8.f );
				m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
			}

			return;
		}
	}

	/* Nothing below cares about releases. */
	if(type == IET_RELEASE) return;


	//
	// handle a step or battle item activate
	//
	if( type==IET_FIRST_PRESS && 
		StyleI.IsValid() &&
		GAMESTATE->IsHumanPlayer( StyleI.player ) )
	{
		AbortGiveUp( true );
		
		if( PREFSMAN->m_AutoPlay == PC_HUMAN )
			m_Player[StyleI.player].Step( StyleI.col, DeviceI.ts ); 
	}
}



/*
 * Saving StageStats that are affected by the note pattern is a little tricky:
 *
 * Stats are cumulative for course play.
 *
 * For regular songs, it doesn't matter how we do it; the pattern doesn't change
 * during play.
 *
 * The pattern changes during play in battle and course mode.  We want to include these
 * changes, so run stats for a song after the song finishes.
 *
 * If we fail, be sure to include the current song in stats, with the current modifier set.
 *
 * So:
 * 
 * 1. At the end of a song in any mode, pass or fail, add stats for that song (from m_Player).
 * 2. At the end of gameplay in course mode, add stats for any songs that weren't played,
 *    applying the modifiers the song would have been played with.  This doesn't include songs
 *    that were played but failed; that was done in #1.
 */
void ScreenGameplay::SongFinished()
{
	// save any statistics
	FOREACH_EnabledPlayer(p)
	{
		/* Note that adding stats is only meaningful for the counters (eg. RADAR_NUM_JUMPS),
		 * not for the percentages (RADAR_AIR). */
		RadarValues v;
		
		NoteDataUtil::CalculateRadarValues( m_Player[p].m_NoteData, GAMESTATE->m_pCurSong->MusicLengthSeconds(), v );
		STATSMAN->m_CurStageStats.m_player[p].radarPossible += v;

		NoteDataWithScoring::GetActualRadarValues( m_Player[p].m_NoteData, p, GAMESTATE->m_pCurSong->MusicLengthSeconds(), v );
		STATSMAN->m_CurStageStats.m_player[p].radarActual += v;
	}

	/* Extremely important: if we don't remove attacks before moving on to the next
	 * screen, they'll still be turned on eventually. */
	GAMESTATE->SetSongInProgress("(none)");
	GAMESTATE->RemoveAllActiveAttacks();
	FOREACH_EnabledPlayer( p )
		m_ActiveAttackList[p].Refresh();
}

void ScreenGameplay::StageFinished( bool bBackedOut )
{
	if( GAMESTATE->IsCourseMode() && GAMESTATE->m_PlayMode != PLAY_MODE_ENDLESS )
	{
		LOG->Trace("Stage finished at index %i/%i", GAMESTATE->GetCourseSongIndex(), (int) m_apSongsQueue.size() );
		/* +1 to skip the current song; that's done already. */
		for( unsigned iPlaySongIndex = GAMESTATE->GetCourseSongIndex()+1;
			 iPlaySongIndex < m_apSongsQueue.size(); ++iPlaySongIndex )
		{
			LOG->Trace("Running stats for %i", iPlaySongIndex );
			SetupSong( iPlaySongIndex );
			FOREACH_EnabledPlayer(p)
				m_Player[p].ApplyWaitingTransforms();
			SongFinished();
		}
	}

	// save current stage stats
	if( !bBackedOut )
		STATSMAN->m_vPlayedStageStats.push_back( STATSMAN->m_CurStageStats );

	/* Reset options. */
	GAMESTATE->RestoreSelectedOptions();
}

void ScreenGameplay::HandleScreenMessage( const ScreenMessage SM )
{
	CHECKPOINT_M( ssprintf("HandleScreenMessage(%i)", SM) );
	if( SM == SM_DoneFadingIn )
	{
		SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay ready" ));
		m_Ready.StartTransitioning( SM_PlayGo );
	}
	else if( SM == SM_PlayGo )
	{
		if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay here we go extra" ));
		else if( GAMESTATE->IsFinalStage() )
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay here we go final" ));
		else
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay here we go normal" ));

		m_Go.StartTransitioning( SM_None );
		GAMESTATE->m_bPastHereWeGo = true;
		m_DancingState = STATE_DANCING;		// STATE CHANGE!  Now the user is allowed to press Back
	}
	else if( SM == SM_NotesEnded )	// received while STATE_DANCING
	{
		AbortGiveUp( false );	// don't allow giveup while the next song is loading

		/* Do this in LoadNextSong, so we don't tween off old attacks until
		 * m_NextSong finishes. */
		// GAMESTATE->RemoveAllActiveAttacks();

		FOREACH_EnabledPlayer(p)
		{
			/* If either player's passmark is enabled, check it. */
			if( GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.m_fPassmark > 0 &&
				m_pLifeMeter[p] &&
				m_pLifeMeter[p]->GetLife() < GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.m_fPassmark )
			{
				LOG->Trace("Player %i failed: life %f is under %f",
					p+1, m_pLifeMeter[p]->GetLife(), GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.m_fPassmark );
				STATSMAN->m_CurStageStats.m_player[p].bFailed = true;
			}

			/* Mark failure.  This hasn't been done yet if m_bTwoPlayerRecovery is set. */
			if( GAMESTATE->GetPlayerFailType(p) != SongOptions::FAIL_OFF &&
				((m_pLifeMeter[p] && m_pLifeMeter[p]->IsFailing()) || 
				(m_pCombinedLifeMeter && m_pCombinedLifeMeter->IsFailing(p))) )
				STATSMAN->m_CurStageStats.m_player[p].bFailed = true;

			if( !STATSMAN->m_CurStageStats.m_player[p].bFailed )
				STATSMAN->m_CurStageStats.m_player[p].iSongsPassed++;

			// set a life record at the point of failue
			if( STATSMAN->m_CurStageStats.m_player[p].bFailed )
				STATSMAN->m_CurStageStats.m_player[p].SetLifeRecordAt( 0, STATSMAN->m_CurStageStats.fGameplaySeconds );
		}

		/* If all players have *really* failed (bFailed, not the life meter or
		 * bFailedEarlier): */
		const bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();
		const bool bStopCourseEarly = STOP_COURSE_EARLY;
		const bool bIsLastSong = IsLastSong();

		LOG->Trace( "bAllReallyFailed = %d, bStopCourseEarly = %d, bIsLastSong = %d", bAllReallyFailed, bStopCourseEarly, bIsLastSong );

		if( bStopCourseEarly || bAllReallyFailed || bIsLastSong )
		{
			// Time to leave from ScreenGameplay
			HandleScreenMessage( SM_LeaveGameplay );
		}
		else
		{
			/* Load the next song in the course.  First, fade out and stop the music. */
			float fFadeLengthSeconds = MUSIC_FADE_OUT_SECONDS;
			RageSoundParams p = m_pSoundMusic->GetParams();
			p.m_FadeLength = fFadeLengthSeconds;
			p.m_LengthSeconds = GAMESTATE->m_fMusicSeconds + fFadeLengthSeconds;			
			m_pSoundMusic->SetParams(p);
			SCREENMAN->PostMessageToTopScreen( SM_StartLoadingNextSong, fFadeLengthSeconds );
			return;
		}
	}
	else if( SM == SM_LeaveGameplay )
	{
		// update dancing characters for win / lose
		DancingCharacters *Dancers = m_SongBackground.GetDancingCharacters();
		if( Dancers )
		{
			FOREACH_EnabledPlayer(p)
			{
				/* XXX: In battle modes, switch( GAMESTATE->GetStageResult(p) ). */
				if( STATSMAN->m_CurStageStats.m_player[p].bFailed )
					Dancers->Change2DAnimState( p, AS2D_FAIL ); // fail anim
				else if( m_pLifeMeter[p] && m_pLifeMeter[p]->GetLife() == 1.0f ) // full life
					Dancers->Change2DAnimState( p, AS2D_WINFEVER ); // full life pass anim
				else
					Dancers->Change2DAnimState( p, AS2D_WIN ); // pass anim
			}
		}

		/* End round. */
		if( m_DancingState == STATE_OUTRO )	// ScreenGameplay already ended
			return;		// ignore
		m_DancingState = STATE_OUTRO;
		AbortGiveUp( false );

		GAMESTATE->RemoveAllActiveAttacks();
		FOREACH_EnabledPlayer( p )
			m_ActiveAttackList[p].Refresh();

		LIGHTSMAN->SetLightsMode( LIGHTSMODE_ALL_CLEARED );

		bool bAllReallyFailed = STATSMAN->m_CurStageStats.AllFailed();

		if( bAllReallyFailed )
		{
			this->PostScreenMessage( SM_BeginFailed, 0 );
			return;
		}

		// do they deserve an extra stage?
		if( GAMESTATE->HasEarnedExtraStage() )
		{
			TweenOffScreen();
			m_Extra.StartTransitioning( SM_GoToNextScreen );
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay extra" ));
		}
		else
		{
			TweenOffScreen();
			
			switch( GAMESTATE->m_PlayMode )
			{
			case PLAY_MODE_BATTLE:
			case PLAY_MODE_RAVE:
				{
					PlayerNumber winner = GAMESTATE->GetBestPlayer();
					switch( winner )
					{
					case PLAYER_INVALID:
						m_Draw.StartTransitioning( SM_GoToNextScreen );
						break;
					default:
						m_Win[winner].StartTransitioning( SM_GoToNextScreen );
						break;
					}
				}
				break;
			default:
				m_Cleared.StartTransitioning( SM_GoToNextScreen );
				break;
			}
			
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay cleared" ));
		}

	}
	else if( SM == SM_StartLoadingNextSong )
	{	
		m_pSoundMusic->Stop();

		/* Next song. */

		// give a little life back between stages
		FOREACH_EnabledPlayer(p)
		{
			if( !STATSMAN->m_CurStageStats.m_player[p].bFailed )
			{
				if( m_pLifeMeter[p] )
					m_pLifeMeter[p]->OnSongEnded();	
				if( m_pPrimaryScoreDisplay[p] )
					m_pPrimaryScoreDisplay[p]->OnSongEnded();	
				if( m_pSecondaryScoreDisplay[p] )
					m_pSecondaryScoreDisplay[p]->OnSongEnded();	
			}
		}
		if( m_pCombinedLifeMeter )
			m_pCombinedLifeMeter->OnSongEnded();	

		int iPlaySongIndex = GAMESTATE->GetCourseSongIndex()+1;
		iPlaySongIndex %= m_apSongsQueue.size();
		Lua *L = LUA->Get();
		m_apSongsQueue[iPlaySongIndex]->PushSelf( L );
		GAMESTATE->m_Environment->Set( L, "NextSong" );
		MESSAGEMAN->Broadcast( "NextCourseSong" );
		GAMESTATE->m_Environment->Unset( L, "NextSong" );
		LUA->Release( L );
		m_NextSong.PlayCommand( "Start" );
		m_NextSong.Reset();
		m_NextSong.StartTransitioning( SM_LoadNextSong );
		LoadCourseSongNumber( GAMESTATE->GetCourseSongIndex()+1 );
		COMMAND( m_sprCourseSongNumber, "ChangeIn" );
	}
	else if( SM == SM_LoadNextSong )
	{
		SongFinished();

		COMMAND( m_sprCourseSongNumber, "ChangeOut" );

		LoadNextSong();
		GAMESTATE->m_bPastHereWeGo = true;

		m_NextSong.Reset();
		m_NextSong.PlayCommand( "Finish" );
		m_NextSong.StartTransitioning( SM_None );

		/* We're fading in, so don't hit any notes for a few seconds; they'll be
		 * obscured by the fade. */
		StartPlayingSong( m_NextSong.GetLengthSeconds()+2, 0 );
	}
	else if( SM == SM_PlayToasty )
	{
		if( PREFSMAN->m_bEasterEggs )
			if( !m_Toasty.IsTransitioning()  &&  !m_Toasty.IsFinished() )	// don't play if we've already played it once
				m_Toasty.StartTransitioning();
	}
	else if( SM >= SM_100Combo && SM <= SM_1000Combo )
	{
		int iCombo = (SM-SM_100Combo+1)*100;
		PlayAnnouncer( ssprintf("gameplay %d combo",iCombo), 2 );
	}
	else if( SM == SM_ComboStopped )
	{
		PlayAnnouncer( "gameplay combo stopped", 2 );
	}
	else if( SM == SM_ComboContinuing )
	{
		PlayAnnouncer( "gameplay combo overflow", 2 );
	}
	else if( SM >= SM_BattleTrickLevel1 && SM <= SM_BattleTrickLevel3 )
	{
		int iTrickLevel = SM-SM_BattleTrickLevel1+1;
		PlayAnnouncer( ssprintf("gameplay battle trick level%d",iTrickLevel), 3 );
		switch( SM )
		{
		case SM_BattleTrickLevel1: m_soundBattleTrickLevel1.Play();	break;
		case SM_BattleTrickLevel2: m_soundBattleTrickLevel2.Play();	break;
		case SM_BattleTrickLevel3: m_soundBattleTrickLevel3.Play();	break;
		default:	ASSERT(0);
		}
	}
	else if( SM >= SM_BattleDamageLevel1 && SM <= SM_BattleDamageLevel3 )
	{
		int iDamageLevel = SM-SM_BattleDamageLevel1+1;
		PlayAnnouncer( ssprintf("gameplay battle damage level%d",iDamageLevel), 3 );
	}
	else if( SM == SM_GoToScreenAfterBack )
	{
		SongFinished();
		StageFinished( true );

		GAMESTATE->CancelStage();

		HandleScreenMessage( SM_GoToPrevScreen );
	}
	else if( SM == SM_GoToNextScreen )
	{
		SongFinished();
		StageFinished( false );
	}
	else if( SM == SM_LoseFocus )
	{
		/* We might have turned the song timer off.  Be sure to turn it back on. */
		SOUND->HandleSongTimer( true );
	}
	else if( SM == SM_BeginFailed )
	{
		m_DancingState = STATE_OUTRO;
		AbortGiveUp( false );
		m_pSoundMusic->StopPlaying();
		m_soundAssistTick.StopPlaying(); /* Stop any queued assist ticks. */
		TweenOffScreen();
		m_Failed.StartTransitioning( SM_GoToNextScreen );

		// show the survive time if extra stage
		if( GAMESTATE->IsExtraStage()  ||  GAMESTATE->IsExtraStage2() )
		{
			float fMaxSurviveSeconds = 0;
            FOREACH_EnabledPlayer(p)
                fMaxSurviveSeconds = max( fMaxSurviveSeconds, STATSMAN->m_CurStageStats.m_player[p].fAliveSeconds );
			ASSERT( fMaxSurviveSeconds > 0 );
			m_textSurviveTime.SetText( "TIME: " + SecondsToMMSSMsMs(fMaxSurviveSeconds) );
			SET_XY_AND_ON_COMMAND( m_textSurviveTime );
		}
		
		if( GAMESTATE->IsCourseMode() )
			if( GAMESTATE->GetCourseSongIndex() >= int(m_apSongsQueue.size() / 2) )
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay oni failed halfway" ));
			else
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay oni failed" ));
		else
			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "gameplay failed" ));
	}
	else if( SM == SM_StopMusic )
	{
		m_pSoundMusic->Stop();
	}
	else if( SM == SM_Pause )
	{
		/* Ignore SM_Pause when in demonstration. */
		if( GAMESTATE->m_bDemonstrationOrJukebox )
			return;

		if( !m_bPaused )
			PauseGame( true );
	}

	ScreenWithMenuElements::HandleScreenMessage( SM );
}


void ScreenGameplay::TweenOnScreen()
{
	ON_COMMAND( m_sprLifeFrame );
	ON_COMMAND( m_sprCourseSongNumber );
	ON_COMMAND( m_sprStageFrame );
	ON_COMMAND( m_textSongOptions );
	ON_COMMAND( m_sprScoreFrame );
	ON_COMMAND( m_BPMDisplay );
	ON_COMMAND( m_MaxCombo );

	if( m_pCombinedLifeMeter )
		ON_COMMAND( *m_pCombinedLifeMeter );
    FOREACH_PlayerNumber(p)
	{
		if( m_pLifeMeter[p] )
			ON_COMMAND( *m_pLifeMeter[p] );
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;
		ON_COMMAND( m_textCourseSongNumber[p] );
		ON_COMMAND( m_textStepsDescription[p] );
		if( m_pPrimaryScoreDisplay[p] )
			ON_COMMAND( *m_pPrimaryScoreDisplay[p] );
		if( m_pSecondaryScoreDisplay[p] )
			ON_COMMAND( *m_pSecondaryScoreDisplay[p] );
		ON_COMMAND( m_textPlayerOptions[p] );
		ON_COMMAND( m_ActiveAttackList[p] );
		ON_COMMAND( m_DifficultyIcon[p] );
		ON_COMMAND( m_DifficultyMeter[p] );
		ON_COMMAND( m_Player[p] );
	}

	if (m_ShowScoreboard)
		FOREACH_NSScoreBoardColumn( sc )
			ON_COMMAND( m_Scoreboard[sc] );
}

void ScreenGameplay::TweenOffScreen()
{
	ScreenWithMenuElements::TweenOffScreen();

	// end gameplay music and stop updating scores
	m_pSoundMusic->StopPlaying();
	m_soundAssistTick.StopPlaying();

	OFF_COMMAND( m_sprLifeFrame );
	OFF_COMMAND( m_sprCourseSongNumber );
	OFF_COMMAND( m_sprStageFrame );
	OFF_COMMAND( m_textSongOptions );
	OFF_COMMAND( m_sprScoreFrame );
	OFF_COMMAND( m_BPMDisplay );
	OFF_COMMAND( m_MaxCombo );

	if( m_pCombinedLifeMeter )
		OFF_COMMAND( *m_pCombinedLifeMeter );
    FOREACH_PlayerNumber(p)
	{
		if( m_pLifeMeter[p] )
			OFF_COMMAND( *m_pLifeMeter[p] );
		if( !GAMESTATE->IsPlayerEnabled(p) )
			continue;
		OFF_COMMAND( m_textCourseSongNumber[p] );
		OFF_COMMAND( m_textStepsDescription[p] );
		if( m_pPrimaryScoreDisplay[p] )
			OFF_COMMAND( *m_pPrimaryScoreDisplay[p] );
		if( m_pSecondaryScoreDisplay[p] )
			OFF_COMMAND( *m_pSecondaryScoreDisplay[p] );
		OFF_COMMAND( m_textPlayerOptions[p] );
		OFF_COMMAND( m_ActiveAttackList[p] );
		OFF_COMMAND( m_DifficultyIcon[p] );
		OFF_COMMAND( m_DifficultyMeter[p] );
		OFF_COMMAND( m_Player[p] );
	}

	if (m_ShowScoreboard)
		FOREACH_NSScoreBoardColumn( sc )
			OFF_COMMAND( m_Scoreboard[sc] );

	m_textDebug.StopTweening();
	m_textDebug.BeginTweening( 1/8.f );
	m_textDebug.SetDiffuse( RageColor(1,1,1,0) );
}

void ScreenGameplay::ShowOniGameOver( PlayerNumber pn )
{
	m_sprOniGameOver[pn].PlayCommand( "Die" );
}


/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
 * (c) 2008 BoXoRRoXoRs
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
