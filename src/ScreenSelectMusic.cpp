#include "global.h"
#include "ScreenSelectMusic.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "GameManager.h"
#include "GameSoundManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "InputMapper.h"
#include "GameState.h"
#include "CodeDetector.h"
#include "ThemeManager.h"
#include "Steps.h"
#include "ActorUtil.h"
#include "RageDisplay.h"
#include "RageTextureManager.h"
#include "Course.h"
#include "ProfileManager.h"
#include "MenuTimer.h"
#include "LightsManager.h"
#include "StatsManager.h"
#include "StepsUtil.h"
#include "Foreach.h"
#include "Style.h"
#include "PlayerState.h"
#include "Command.h"
#include "CommonMetrics.h"
#include "BannerCache.h"
#include "ScreenDimensions.h"
#include "RageFile.h"
#include "MemoryCardManager.h" 
#include "InputQueue.h"
#include "OptionsList.h"
#include "AnnouncerManager.h"

// XXX: compatibility hackery for custom song text
#include "DiagnosticsUtil.h"
#include "FontCharAliases.h"

// XXX: custom song loading. remove these if we can.
#include "RageFileDriverTimeout.h"

// Draw every 1/6 second, approx. one per 10 minutes
static RageTimer DrawTimer;
const float DRAW_UPDATE_TIME = 0.1666667;

const int NUM_SCORE_DIGITS	=	9;

#define NEXT_SCREEN							THEME->GetMetric (m_sName,"NextScreen")
#define PREV_SCREEN							THEME->GetMetric (m_sName,"PrevScreen")
#define NEXT_OPTIONS_SCREEN					THEME->GetMetric (m_sName,"NextOptionsScreen")
#define SCORE_SORT_CHANGE_COMMAND(i) 		THEME->GetMetricA(m_sName,ssprintf("ScoreP%iSortChangeCommand", i+1))
#define SCORE_FRAME_SORT_CHANGE_COMMAND(i)	THEME->GetMetricA(m_sName,ssprintf("ScoreFrameP%iSortChangeCommand", i+1))
#define METER_TYPE							THEME->GetMetric (m_sName,"MeterType")
#define SHOW_OPTIONS_MESSAGE_SECONDS		THEME->GetMetricF(m_sName,"ShowOptionsMessageSeconds")
#define TWEEN_OFF_OPTIONS_MESSAGE_IMMEDIATELY	THEME->GetMetricB(m_sName,"TweenOptionsMessageOffImmediately")

// TODO: fix these up so they can use m_sName
ThemeMetric<CString>	CUSTOM_SONG_WAIT_TEXT	("ScreenSelectMusic", "CustomSongWaitText");
ThemeMetric<CString>	CUSTOM_SONG_CANCEL_TEXT	("ScreenSelectMusic", "CustomSongCancelText");

AutoScreenMessage( SM_AllowOptionsMenuRepeat )
AutoScreenMessage( SM_SongChanged )
AutoScreenMessage( SM_SortOrderChanging )
AutoScreenMessage( SM_SortOrderChanged )
AutoScreenMessage( SM_TweenOffOptionsMessage )

static CString g_sCDTitlePath;
static bool g_bWantFallbackCdTitle;
static bool g_bCDTitleWaiting = false;
static CString g_sBannerPath;
static bool g_bBannerWaiting = false;
static bool g_bSampleMusicWaiting = false;
static RageTimer g_StartedLoadingAt(RageZeroTimer);

REGISTER_SCREEN_CLASS( ScreenSelectMusic );
ScreenSelectMusic::ScreenSelectMusic( CString sClassName ) : ScreenWithMenuElements( sClassName ),
	FOV( m_sName, "FOV" ),
	FOV_CENTER_X( m_sName, "FOVCenterX" ),
	FOV_CENTER_Y( m_sName, "FOVCenterY" ),
	BANNER_WIDTH( m_sName, "BannerWidth" ),
	BANNER_HEIGHT( m_sName, "BannerHeight" ),
	SONG_OPTIONS_EXTRA_COMMAND( m_sName, "SongOptionsExtraCommand" ),
	SAMPLE_MUSIC_DELAY( m_sName, "SampleMusicDelay" ),
	SHOW_RADAR( m_sName, "ShowRadar" ),
	SHOW_GRAPH( m_sName, "ShowGraph" ),
	SHOW_PANES( m_sName, "ShowPanes" ),
	SHOW_DIFFICULTY_LIST( m_sName, "ShowDifficultyList" ),
	SHOW_COURSE_CONTENTS( m_sName, "ShowCourseContents" ),
	DO_ROULETTE_ON_MENU_TIMER( m_sName, "DoRouletteOnMenuTimer" ),
	ALIGN_MUSIC_BEATS( m_sName, "AlignMusicBeat" ),
	CODES( m_sName, "Codes" ),
	MUSIC_WHEEL_TYPE( m_sName, "MusicWheelType" ),
	OPTIONS_MENU_AVAILABLE( m_sName, "OptionsMenuAvailable" ),
	SELECT_MENU_AVAILABLE( m_sName, "SelectMenuAvailable" ),
	MODE_MENU_AVAILABLE( m_sName, "ModeMenuAvailable" ),
        USE_OPTIONS_LIST( m_sName, "UseOptionsList" )
{
	LOG->Trace( "ScreenSelectMusic::ScreenSelectMusic()" );

	LIGHTSMAN->SetLightsMode( LIGHTSMODE_MENU );

	if( PREFSMAN->m_bScreenTestMode )
	{
		GAMESTATE->m_PlayMode = PLAY_MODE_REGULAR;
		GAMESTATE->m_pCurStyle.Set( GAMEMAN->GameAndStringToStyle(GAMEMAN->GetDefaultGame(),"versus") );
		GAMESTATE->m_bSideIsJoined[PLAYER_1] = true;
		GAMESTATE->m_MasterPlayerNumber = PLAYER_1;
	}

	/* Finish any previous stage.  It's OK to call this when we havn't played a stage yet. 
	 * Do this before anything that might look at GAMESTATE->m_iCurrentStageIndex. */
	GAMESTATE->FinishStage();

	m_iSavedRoundIndex = GAMESTATE->m_iCurrentStageIndex;

	// COMPAT: set these if they haven't been set.
	if( CUSTOM_SONG_WAIT_TEXT.GetValue().empty() )
		CUSTOM_SONG_WAIT_TEXT.SetValue( "Please wait..." );

	if( CUSTOM_SONG_CANCEL_TEXT.GetValue().empty() )
	{
		CString sCancelMessage = ssprintf( "Pressing %s will cancel this selection.",
			DiagnosticsUtil::GetInputType() == "ITGIO" ? "&MENULEFT;+&MENURIGHT;" : "&SELECT;" );

		FontCharAliases::ReplaceMarkers( sCancelMessage );
		CUSTOM_SONG_CANCEL_TEXT.SetValue( sCancelMessage );
	}
}


void ScreenSelectMusic::Init()
{
	m_bSelectIsDown = false; // used by LoadHelpText which is called by ScreenWithMenuElements::Init()

	ScreenWithMenuElements::Init();

	m_DisplayMode = GAMESTATE->IsCourseMode() ? DISPLAY_COURSES : DISPLAY_SONGS;

	/* Cache: */
	m_sSectionMusicPath = THEME->GetPathS(m_sName,"section music");
	m_sSortMusicPath = THEME->GetPathS(m_sName,"sort music");
	m_sRouletteMusicPath = THEME->GetPathS(m_sName,"roulette music");
	m_sRandomMusicPath = THEME->GetPathS(m_sName,"random music");
	m_sCourseMusicPath = THEME->GetPathS(m_sName,"course music");
	m_sFallbackCDTitlePath = THEME->GetPathG(m_sName,"fallback cdtitle");

	TEXTUREMAN->CacheTexture( m_sFallbackCDTitlePath );

	if( GAMESTATE->m_pCurStyle == NULL )
		RageException::Throw( "The Style has not been set.  A theme must set the Style before loading ScreenSelectMusic." );

	if( GAMESTATE->m_PlayMode == PLAY_MODE_INVALID )
		RageException::Throw( "The PlayMode has not been set.  A theme must set the PlayMode before loading ScreenSelectMusic." );

	/* Load low-res banners, if needed. */
	BANNERCACHE->Demand();

	// About 3/4 of the time, the user forgets this was set.  Let's cut him/her some slack.
	GAMESTATE->m_SongOptions.m_fMusicRate = 1.0f;

	m_MusicWheel.Load( MUSIC_WHEEL_TYPE );

	// pop'n music has this under the songwheel...
	FOREACH_PlayerNumber( p )
	{
		m_sprCharacterIcon[p].SetName( ssprintf("CharacterIconP%d",p+1) );

		Character* pChar = GAMESTATE->m_pCurCharacters[p];
		CString sPath = pChar->GetSongSelectIconPath();

		if( sPath.empty() )
			continue;

		m_sprCharacterIcon[p].Load( sPath );
		SET_XY( m_sprCharacterIcon[p] );
		this->AddChild( &m_sprCharacterIcon[p] );
	}



	m_MusicWheelUnder.Load( THEME->GetPathG(m_sName,"wheel under") );
	m_MusicWheelUnder->SetName( "WheelUnder" );
	SET_XY( m_MusicWheelUnder );
	this->AddChild( m_MusicWheelUnder );

	m_MusicWheel.SetName( "MusicWheel" );
	SET_XY( m_MusicWheel );
	this->AddChild( &m_MusicWheel );

	if( USE_OPTIONS_LIST )
	{
		FOREACH_PlayerNumber(p)
		{
			m_OptionsList[p].SetName( ssprintf("OptionsListP%d", p+1) );
			m_OptionsList[p].Load( ssprintf("OptionsListP%d", p+1), p );
			m_OptionsList[p].SetDrawOrder( 100 );
			ActorUtil::LoadAllCommands( m_OptionsList[p], m_sName );
			SET_XY( m_OptionsList[p] );
			this->AddChild( &m_OptionsList[p] );
		}
		m_OptionsList[PLAYER_1].Link( &m_OptionsList[PLAYER_2] );
		m_OptionsList[PLAYER_2].Link( &m_OptionsList[PLAYER_1] );
	}

	m_sprBannerMask.SetName( "Banner" );	// use the same metrics and animation as Banner
	m_sprBannerMask.Load( THEME->GetPathG(m_sName,"banner mask") );
	m_sprBannerMask.SetBlendMode( BLEND_NO_EFFECT );	// don't draw to color buffer
	m_sprBannerMask.SetZWrite( true );	// do draw to the zbuffer
	SET_XY( m_sprBannerMask );
	this->AddChild( &m_sprBannerMask );

	// this is loaded SetSong and TweenToSong
	m_Banner.SetName( "Banner" );
	m_Banner.SetZTestMode( ZTEST_WRITE_ON_PASS );	// do have to pass the z test
	m_Banner.ScaleToClipped( BANNER_WIDTH, BANNER_HEIGHT );
	SET_XY( m_Banner );
	this->AddChild( &m_Banner );

	m_sprBannerFrame.Load( THEME->GetPathG(m_sName,"banner frame") );
	m_sprBannerFrame->SetName( "BannerFrame" );
	SET_XY( m_sprBannerFrame );
	this->AddChild( m_sprBannerFrame );

	m_BPMDisplay.SetName( "BPMDisplay" );
	m_BPMDisplay.Load();
	SET_XY( m_BPMDisplay );
	this->AddChild( &m_BPMDisplay );

	m_DifficultyDisplay.SetName( "DifficultyDisplay" );
	m_DifficultyDisplay.SetShadowLength( 0 );
	SET_XY( m_DifficultyDisplay );
	this->AddChild( &m_DifficultyDisplay );

	m_sprCDTitleFront.SetName( "CDTitle" );
	m_sprCDTitleFront.Load( THEME->GetPathG(m_sName,"fallback cdtitle") );
	SET_XY( m_sprCDTitleFront );
	COMMAND( m_sprCDTitleFront, "Front" );
	this->AddChild( &m_sprCDTitleFront );

	m_sprCDTitleBack.SetName( "CDTitle" );
	m_sprCDTitleBack.Load( THEME->GetPathG(m_sName,"fallback cdtitle") );
	SET_XY( m_sprCDTitleBack );
	COMMAND( m_sprCDTitleBack, "Back" );
	this->AddChild( &m_sprCDTitleBack );

	m_GrooveRadar.SetName( "Radar" );
	SET_XY( m_GrooveRadar );
	if( SHOW_RADAR )
		this->AddChild( &m_GrooveRadar );

	m_GrooveGraph.SetName( "Graph" );
	SET_XY( m_GrooveGraph );
	if( SHOW_GRAPH )
		this->AddChild( &m_GrooveGraph );

	m_textSongOptions.SetName( "SongOptions" );
	m_textSongOptions.LoadFromFont( THEME->GetPathF("Common","normal") );
	SET_XY( m_textSongOptions );
	this->AddChild( &m_textSongOptions );

	m_textNumSongs.SetName( "NumSongs" );
	m_textNumSongs.LoadFromFont( THEME->GetPathF(m_sName,"num songs") );
	SET_XY( m_textNumSongs );
	this->AddChild( &m_textNumSongs );

	m_textTotalTime.SetName( "TotalTime" );
	m_textTotalTime.LoadFromFont( THEME->GetPathF(m_sName,"total time") );
	SET_XY( m_textTotalTime );
	this->AddChild( &m_textTotalTime );

	m_Artist.SetName( "ArtistDisplay" );
	m_Artist.Load( "ArtistDisplay" );
	SET_XY( m_Artist );
	this->AddChild( &m_Artist );
		
	m_MachineRank.SetName( "MachineRank" );
	m_MachineRank.LoadFromFont( THEME->GetPathF(m_sName,"rank") );
	SET_XY( m_MachineRank );
	this->AddChild( &m_MachineRank );

	if( SHOW_DIFFICULTY_LIST )
	{
		m_DifficultyList.SetName( "DifficultyList" );
		m_DifficultyList.Load();
		SET_XY_AND_ON_COMMAND( m_DifficultyList );
		this->AddChild( &m_DifficultyList );
	}

	if( SHOW_COURSE_CONTENTS )
	{
		m_CourseContents.SetName( "CourseContents" );
		m_CourseContents.Load();
		SET_XY( m_CourseContents );
		this->AddChild( &m_CourseContents );
	}

	FOREACH_HumanPlayer( p )
	{
		m_sprDifficultyFrame[p].SetName( ssprintf("DifficultyFrameP%d",p+1) );
		m_sprDifficultyFrame[p].Load( THEME->GetPathG(m_sName,ssprintf("difficulty frame p%d",p+1)) );
		m_sprDifficultyFrame[p].StopAnimating();
		SET_XY( m_sprDifficultyFrame[p] );
		this->AddChild( &m_sprDifficultyFrame[p] );

		m_DifficultyIcon[p].SetName( ssprintf("DifficultyIconP%d",p+1) );
		m_DifficultyIcon[p].Load( THEME->GetPathG(m_sName,ssprintf("difficulty icons 1x%d",NUM_DIFFICULTIES)) );
		SET_XY( m_DifficultyIcon[p] );
		this->AddChild( &m_DifficultyIcon[p] );

		m_AutoGenIcon[p].SetName( ssprintf("AutogenIconP%d",p+1) );
		m_AutoGenIcon[p].Load( THEME->GetPathG(m_sName,"autogen") );
		SET_XY( m_AutoGenIcon[p] );
		this->AddChild( &m_AutoGenIcon[p] );

		m_OptionIconRow[p].SetName( ssprintf("OptionIconsP%d",p+1) );
		m_OptionIconRow[p].Load( p );
		m_OptionIconRow[p].Refresh();
		SET_XY( m_OptionIconRow[p] );
		this->AddChild( &m_OptionIconRow[p] );

		m_sprMeterFrame[p].SetName( ssprintf("MeterFrameP%d",p+1) );
		m_sprMeterFrame[p].Load( THEME->GetPathG(m_sName,ssprintf("meter frame p%d",p+1)) );
		SET_XY( m_sprMeterFrame[p] );
		this->AddChild( &m_sprMeterFrame[p] );

		if( SHOW_PANES )
		{
			m_PaneDisplay[p].SetName( ssprintf("PaneDisplayP%d",p+1) );
			m_PaneDisplay[p].Load( "PaneDisplay", p );
			SET_XY( m_PaneDisplay[p] );
			this->AddChild( &m_PaneDisplay[p] );
		}

		m_DifficultyMeter[p].SetName( ssprintf("MeterP%d",p+1) );
		m_DifficultyMeter[p].Load( METER_TYPE );
		SET_XY_AND_ON_COMMAND( m_DifficultyMeter[p] );
		this->AddChild( &m_DifficultyMeter[p] );

		// add an icon onto the song select to show what
		// character they're using.

		m_sprHighScoreFrame[p].SetName( ssprintf("ScoreFrameP%d",p+1) );
		m_sprHighScoreFrame[p].Load( THEME->GetPathG(m_sName,ssprintf("score frame p%d",p+1)) );
		SET_XY( m_sprHighScoreFrame[p] );
		this->AddChild( &m_sprHighScoreFrame[p] );

		m_textHighScore[p].SetName( ssprintf("ScoreP%d",p+1) );
		m_textHighScore[p].LoadFromFont( THEME->GetPathF(m_sName,"score") );
		m_textHighScore[p].SetShadowLength( 0 );
		m_textHighScore[p].SetDiffuse( PLAYER_COLOR.GetValue(p) );
		SET_XY( m_textHighScore[p] );
		this->AddChild( &m_textHighScore[p] );
	}	

	m_MusicSortDisplay.SetName( "SortIcon" );
	m_MusicSortDisplay.Set( GAMESTATE->m_SortOrder );
	SET_XY( m_MusicSortDisplay );
	this->AddChild( &m_MusicSortDisplay );

	m_sprLongBalloon.Load( THEME->GetPathG(m_sName,"balloon long") );
	m_sprLongBalloon->SetName( "Balloon" );
	m_sprLongBalloon->SetHidden( 1 );
	this->AddChild( m_sprLongBalloon );

	m_sprMarathonBalloon.Load( THEME->GetPathG(m_sName,"balloon marathon") );
	m_sprMarathonBalloon->SetName( "Balloon" );
	m_sprMarathonBalloon->SetHidden( 1 );
	this->AddChild( m_sprMarathonBalloon );

	m_sprCourseHasMods.LoadAndSetName( m_sName, "CourseHasMods" );
	SET_XY( m_sprCourseHasMods );
	this->AddChild( m_sprCourseHasMods );

	m_sprOptionsMessage.SetName( "OptionsMessage" );
	m_sprOptionsMessage.Load( THEME->GetPathG(m_sName,"options message 1x2") );
	m_sprOptionsMessage.StopAnimating();
	m_sprOptionsMessage.SetHidden( true );
	SET_XY( m_sprOptionsMessage );
	ActorUtil::LoadAllCommands( m_sprOptionsMessage, m_sName );
	//this->AddChild( &m_sprOptionsMessage );	// we have to draw this manually over the top of transitions

	m_bgOptionsOut.Load( THEME->GetPathB(m_sName,"options out") );
//	this->AddChild( &m_bgOptionsOut ); // drawn on top
	m_bgNoOptionsOut.Load( THEME->GetPathB(m_sName,"no options out") );
//	this->AddChild( &m_bgNoOptionsOut ); // drawn on top

	m_soundDifficultyEasier.Load( THEME->GetPathS(m_sName,"difficulty easier") );
	m_soundDifficultyHarder.Load( THEME->GetPathS(m_sName,"difficulty harder") );
	m_soundOptionsChange.Load( THEME->GetPathS(m_sName,"options") );
	m_soundLocked.Load( THEME->GetPathS(m_sName,"locked") );
	m_soundSelectPressed.Load( THEME->GetPathS(m_sName,"select down"), true );

	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "select music intro" ));

	m_bMadeChoice = false;
	m_bGoToOptions = false;
	m_bAllowOptionsMenu = m_bAllowOptionsMenuRepeat = false;

	UpdateOptionsDisplays();

	AfterMusicChange();
	TweenOnScreen();

	this->SortByDrawOrder();

#ifndef WIN32
	/* At least for right now, we need the card mounted for previews */
	if( PREFSMAN->m_bCustomSongPreviews && PREFSMAN->m_bCustomSongs )
	{
		FOREACH_PlayerNumber( pn )
		{
			MEMCARDMAN->UnmountCard( pn );
			MEMCARDMAN->MountCard( pn, 600 );
		}
	}
#endif
}


ScreenSelectMusic::~ScreenSelectMusic()
{
	LOG->Trace( "ScreenSelectMusic::~ScreenSelectMusic()" );
	BANNERCACHE->Undemand();
	
#ifndef WIN32
	if( PREFSMAN->m_bCustomSongPreviews && PREFSMAN->m_bCustomSongs )
		FOREACH_EnabledPlayer( pn )
			MEMCARDMAN->UnmountCard( pn );
#endif

}

void ScreenSelectMusic::DrawPrimitives()
{
	DISPLAY->CameraPushMatrix();
	DISPLAY->LoadMenuPerspective( FOV, FOV_CENTER_X, FOV_CENTER_Y );

	Screen::DrawPrimitives();
	m_sprOptionsMessage.Draw();
	m_bgOptionsOut.Draw();
	m_bgNoOptionsOut.Draw();

	DISPLAY->CameraPopMatrix();
}

void ScreenSelectMusic::TweenSongPartsOnScreen( bool Initial )
{
	m_GrooveRadar.StopTweening();
	m_GrooveGraph.StopTweening();
	m_GrooveRadar.TweenOnScreen();
	m_GrooveGraph.TweenOnScreen();
	if( SHOW_DIFFICULTY_LIST )
	{
		if( Initial )
		{
			ON_COMMAND( m_DifficultyList );
			m_DifficultyList.TweenOnScreen();
		}
//		else // do this after SM_SortOrderChanged
//			m_DifficultyList.Show();
	}

	{
		FOREACH_HumanPlayer( p )
		{
			ON_COMMAND( m_sprDifficultyFrame[p] );
			ON_COMMAND( m_sprMeterFrame[p] );
			ON_COMMAND( m_DifficultyIcon[p] );
			ON_COMMAND( m_AutoGenIcon[p] );
		}
	}
}

void ScreenSelectMusic::TweenSongPartsOffScreen( bool Final )
{
	m_GrooveRadar.TweenOffScreen();
	m_GrooveGraph.TweenOffScreen();
	if( SHOW_DIFFICULTY_LIST )
	{
		if( Final )
		{
			OFF_COMMAND( m_DifficultyList );
			m_DifficultyList.TweenOffScreen();
		}
		else
			m_DifficultyList.Hide();
	}

	{
		FOREACH_HumanPlayer( p )
		{
			OFF_COMMAND( m_sprDifficultyFrame[p] );
			OFF_COMMAND( m_sprMeterFrame[p] );
			OFF_COMMAND( m_DifficultyIcon[p] );
			OFF_COMMAND( m_AutoGenIcon[p] );
		}
	}
}

void ScreenSelectMusic::TweenCoursePartsOnScreen( bool Initial )
{
	if( SHOW_COURSE_CONTENTS )
	{
		m_CourseContents.SetZoomY( 1 );
		if( Initial )
		{
			COMMAND( m_CourseContents, "On" );
		}
		else
		{
			m_CourseContents.SetFromGameState();
			COMMAND( m_CourseContents, "Show" );
		}
	}
}

void ScreenSelectMusic::TweenCoursePartsOffScreen( bool Final )
{
	if( SHOW_COURSE_CONTENTS )
	{
		if( Final )
			OFF_COMMAND( m_CourseContents );
		else
			COMMAND( m_CourseContents, "Hide" );
	}
}

void ScreenSelectMusic::SkipSongPartTweens()
{
	m_GrooveRadar.FinishTweening();
	m_GrooveGraph.FinishTweening();
	if( SHOW_DIFFICULTY_LIST )
		m_DifficultyList.FinishTweening();

	FOREACH_HumanPlayer( p )
	{		
		m_sprDifficultyFrame[p].FinishTweening();
		m_sprMeterFrame[p].FinishTweening();
		m_DifficultyIcon[p].FinishTweening();
		m_AutoGenIcon[p].FinishTweening();
	}
}

void ScreenSelectMusic::SkipCoursePartTweens()
{
	if( SHOW_COURSE_CONTENTS )
		m_CourseContents.FinishTweening();
}

void ScreenSelectMusic::TweenOnScreen()
{
	TweenSongPartsOnScreen( true );
	TweenCoursePartsOnScreen( true );

	switch( GAMESTATE->m_SortOrder )
	{
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
		TweenSongPartsOffScreen( false );
		SkipSongPartTweens();
		break;
	default:
		TweenCoursePartsOffScreen( false );
		SkipCoursePartTweens();
		break;
	}

	ON_COMMAND( m_sprBannerMask );
	ON_COMMAND( m_Banner );
	ON_COMMAND( m_sprBannerFrame );
	ON_COMMAND( m_BPMDisplay );
	ON_COMMAND( m_DifficultyDisplay );
	ON_COMMAND( m_sprCDTitleFront );
	ON_COMMAND( m_sprCDTitleBack );
	ON_COMMAND( m_GrooveRadar );
	ON_COMMAND( m_GrooveGraph );
	ON_COMMAND( m_textSongOptions );
	ON_COMMAND( m_textNumSongs );
	ON_COMMAND( m_textTotalTime );
	ON_COMMAND( m_MusicSortDisplay );
	ON_COMMAND( m_MusicWheelUnder );
	m_MusicWheel.TweenOnScreen();
	ON_COMMAND( m_sprLongBalloon );
	ON_COMMAND( m_sprMarathonBalloon );
	ON_COMMAND( m_sprCourseHasMods );
	ON_COMMAND( m_MusicWheel );
	ON_COMMAND( m_Artist );
	ON_COMMAND( m_MachineRank );

	FOREACH_HumanPlayer( p )
	{		
		ON_COMMAND( m_sprCharacterIcon[p] );
		ON_COMMAND( m_OptionIconRow[p] );
		ON_COMMAND( m_sprHighScoreFrame[p] );
		ON_COMMAND( m_textHighScore[p] );
		if( SHOW_PANES )
			ON_COMMAND( m_PaneDisplay[p] );
		ON_COMMAND( m_DifficultyMeter[p] );
	}

	if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
		m_textSongOptions.RunCommands( SONG_OPTIONS_EXTRA_COMMAND );
}

void ScreenSelectMusic::TweenOffScreen()
{	
	ScreenWithMenuElements::TweenOffScreen();

	switch( GAMESTATE->m_SortOrder )
	{
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
		TweenCoursePartsOffScreen( true );
		break;
	default:
		TweenSongPartsOffScreen( true );
		break;
	}

	OFF_COMMAND( m_sprBannerMask );
	OFF_COMMAND( m_Banner );
	OFF_COMMAND( m_sprBannerFrame );
	OFF_COMMAND( m_BPMDisplay );
	OFF_COMMAND( m_DifficultyDisplay );
	OFF_COMMAND( m_sprCDTitleFront );
	OFF_COMMAND( m_sprCDTitleBack );
	OFF_COMMAND( m_GrooveRadar );
	OFF_COMMAND( m_GrooveGraph );
	OFF_COMMAND( m_textSongOptions );
	OFF_COMMAND( m_textNumSongs );
	OFF_COMMAND( m_textTotalTime );
	OFF_COMMAND( m_MusicSortDisplay );
	m_MusicWheel.TweenOffScreen();
	OFF_COMMAND( m_MusicWheelUnder );
	OFF_COMMAND( m_MusicWheel );
	OFF_COMMAND( m_sprLongBalloon );
	OFF_COMMAND( m_sprMarathonBalloon );
	OFF_COMMAND( m_sprCourseHasMods );
	OFF_COMMAND( m_Artist );
	OFF_COMMAND( m_MachineRank );

	FOREACH_HumanPlayer( p )
	{		
		OFF_COMMAND(m_sprCharacterIcon[p]);
		OFF_COMMAND( m_OptionIconRow[p] );
		OFF_COMMAND( m_sprHighScoreFrame[p] );
		OFF_COMMAND( m_textHighScore[p] );
		if( SHOW_PANES )
			OFF_COMMAND( m_PaneDisplay[p] );
		OFF_COMMAND( m_DifficultyMeter[p] );
	}
}


/* This hides elements that are only relevant when displaying a single song,
 * and shows elements for course display.  XXX: Allow different tween commands. */
void ScreenSelectMusic::SwitchDisplayMode( DisplayMode dm )
{
	if( m_DisplayMode == dm )
		return;

	// tween off
	switch( m_DisplayMode )
	{
	case DISPLAY_SONGS:
		TweenSongPartsOffScreen( false );
		break;
	case DISPLAY_COURSES:
		TweenCoursePartsOffScreen( false );
		break;
	case DISPLAY_MODES:
		break;
	}

	// tween on
	m_DisplayMode = dm;
	switch( m_DisplayMode )
	{
	case DISPLAY_SONGS:
		TweenSongPartsOnScreen( false );
		break;
	case DISPLAY_COURSES:
		TweenCoursePartsOnScreen( false );
		break;
	case DISPLAY_MODES:
		break;
	}
}

void ScreenSelectMusic::TweenScoreOnAndOffAfterChangeSort()
{
	FOREACH_HumanPlayer( p )
	{
		m_textHighScore[p].RunCommands( SCORE_SORT_CHANGE_COMMAND(p) );
		m_sprHighScoreFrame[p].RunCommands( SCORE_FRAME_SORT_CHANGE_COMMAND(p) );
	}

	switch( GAMESTATE->m_SortOrder )
	{
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
		SwitchDisplayMode( DISPLAY_COURSES );
		break;
	case SORT_MODE_MENU:
		SwitchDisplayMode( DISPLAY_MODES );
		break;
	default:
		SwitchDisplayMode( DISPLAY_SONGS );
		break;
	}
}

void ScreenSelectMusic::CheckBackgroundRequests()
{
	if( g_bCDTitleWaiting )
	{
		/* The CDTitle is normally very small, so we don't bother waiting to display it. */
		CString sPath;
		if( m_BackgroundLoader.IsCacheFileFinished(g_sCDTitlePath, sPath) )
		{
			g_bCDTitleWaiting = false;

			CString sCDTitlePath = sPath;

			if( sCDTitlePath.empty() || !IsAFile(sCDTitlePath) )
				sCDTitlePath = g_bWantFallbackCdTitle? m_sFallbackCDTitlePath:CString("");

			if( !sCDTitlePath.empty() )
			{
				TEXTUREMAN->DisableOddDimensionWarning();
				m_sprCDTitleFront.Load( sCDTitlePath );
				m_sprCDTitleBack.Load( sCDTitlePath );
				TEXTUREMAN->EnableOddDimensionWarning();
			}

			m_BackgroundLoader.FinishedWithCachedFile( g_sCDTitlePath );
		}
		else
			return;
	}

	/* Loading the rest can cause small skips, so don't do it until the wheel settles. 
	 * Do load if we're transitioning out, though, so we don't miss starting the music
	 * for the options screen if a song is selected quickly.  Also, don't do this
	 * if the wheel is locked, since we're just bouncing around after selecting TYPE_RANDOM,
	 * and it'll take a while before the wheel will settle. */
	if( !m_MusicWheel.IsSettled() && !m_MusicWheel.WheelIsLocked() && !m_Out.IsTransitioning() )
		return;

	if( g_bBannerWaiting )
	{
		if( m_Banner.GetTweenTimeLeft() > 0 )
			return;

		CString sPath;
		bool bFreeCache = false;
		if( TEXTUREMAN->IsTextureRegistered( Sprite::SongBannerTexture(g_sBannerPath) ) )
		{
			/* If the file is already loaded into a texture, it's finished,
			 * and we only do this to honor the HighQualTime value. */
			sPath = g_sBannerPath;
		}
		else
		{
			if( !m_BackgroundLoader.IsCacheFileFinished( g_sBannerPath, sPath ) )
				return;
			bFreeCache = true;
		}

		g_bBannerWaiting = false;
		m_Banner.Load( sPath, true );

		if( bFreeCache )
			m_BackgroundLoader.FinishedWithCachedFile( g_sBannerPath );
	}

	/* Nothing else is going.  Start the music, if we havn't yet. */
	if( g_bSampleMusicWaiting )
	{
		/* Don't start the music sample when moving fast. */
		if( g_StartedLoadingAt.Ago() >= SAMPLE_MUSIC_DELAY )
		{
			g_bSampleMusicWaiting = false;

			SOUND->PlayMusic(
				m_sSampleMusicToPlay, m_pSampleMusicTimingData,
				true, m_fSampleStartSeconds, m_fSampleLengthSeconds,
				1.5f, /* fade out for 1.5 seconds */
				ALIGN_MUSIC_BEATS );
		}
		else
			return;
	}
}

void ScreenSelectMusic::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
	m_bgOptionsOut.Update( fDeltaTime );
	m_bgNoOptionsOut.Update( fDeltaTime );
	m_sprOptionsMessage.Update( fDeltaTime );

	CheckBackgroundRequests();
}

void ScreenSelectMusic::Input( const DeviceInput& DeviceI, InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	// debugging?
	// I just like being able to see untransliterated titles occasionally.
	if( DeviceI.device == DEVICE_KEYBOARD && DeviceI.button == KEY_F9 )
	{
		if( type != IET_FIRST_PRESS ) return;
		PREFSMAN->m_bShowNativeLanguage.Set( !PREFSMAN->m_bShowNativeLanguage );
		m_MusicWheel.RebuildAllMusicWheelItems();
		if( SHOW_COURSE_CONTENTS )
			m_CourseContents.SetFromGameState();
		return;
	}

	if( !GameI.IsValid() )		return;		// don't care


	/* XXX: What's the difference between this and StyleI.player? */
	/* StyleI won't be valid if it's a menu button that's pressed.  
	 * There's got to be a better way of doing this.  -Chris */
	PlayerNumber pn = GAMESTATE->GetCurrentStyle()->ControllerToPlayerNumber( GameI.controller );
	if( !GAMESTATE->IsHumanPlayer(pn) )
		return;

	// Check for "Press START again for options" button press
	if( m_bMadeChoice  && (MenuI.IsValid() && MenuI.button == MENU_BUTTON_START
		&& type != IET_RELEASE  && type != IET_LEVEL_CHANGED &&
		OPTIONS_MENU_AVAILABLE.GetValue()) )
	{
		if(m_bGoToOptions) return; /* got it already */
		if(!m_bAllowOptionsMenu) return; /* not allowed */

		if( !m_bAllowOptionsMenuRepeat &&
			(type == IET_SLOW_REPEAT || type == IET_FAST_REPEAT ))
			return; /* not allowed yet */

		SCREENMAN->PlayStartSound();

		m_bGoToOptions = true;
		m_sprOptionsMessage.SetState( 1 );


		if( TWEEN_OFF_OPTIONS_MESSAGE_IMMEDIATELY )
		{
			// Send SM_TweenOffOptionsMessage faster.  Don't tween off the 
			// options message until the wheel has finished tweening off though.
			this->ClearMessageQueue( SM_TweenOffOptionsMessage );
			float fOffCommandLengthSeconds = Actor::GetCommandsLengthSeconds( m_sprOptionsMessage.GetCommand("Off") );
			float fSecondsToDelay = max( this->GetTweenTimeLeft() - fOffCommandLengthSeconds, 0 );
			this->PostScreenMessage( SM_TweenOffOptionsMessage, fSecondsToDelay );
		}
		return;
	}

	if( IsTransitioning() )
		return;		// ignore

	if( m_bMadeChoice )		return;		// ignore

	// handle options list input
	if( USE_OPTIONS_LIST )
	{
		PlayerNumber pn = MenuI.player;
		if( pn != PLAYER_INVALID )
		{
			if( m_OptionsList[pn].IsOpened() )
			{
				//m_OptionsList[pn].Input( input );
				m_OptionsList[pn].Input( DeviceI, type, GameI, MenuI, StyleI );

				if( !m_OptionsList[pn].IsOpened() )
					CloseOptionsList( pn );

				return;
			}
			else
			{
			if( type == IET_RELEASE 
				&& ( CodeDetector::EnteredCode( GameI.controller, CODE_OPTIONS_LIST_OPEN1 )
					|| CodeDetector::EnteredCode( GameI.controller, CODE_OPTIONS_LIST_OPEN2 ) ) )
						OpenOptionsList( pn );
			}
		}
	}

	LoadHelpText();

	bool bSelectIsPressed = SELECT_MENU_AVAILABLE && INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_SELECT) );

	if( bSelectIsPressed )
	{
		if( type == IET_FIRST_PRESS )
		{
			switch( MenuI.button )
			{
			case MENU_BUTTON_LEFT:
				ChangeDifficulty( pn, -1 );
				break;
			case MENU_BUTTON_RIGHT:
				ChangeDifficulty( pn, +1 );
				break;
			case MENU_BUTTON_START:
				if( MODE_MENU_AVAILABLE )
					m_MusicWheel.ChangeSort( SORT_MODE_MENU );
				else
					m_soundLocked.Play();
				break;
			case MENU_BUTTON_SELECT:
				return; break;
			}
		}
	}

	switch( MenuI.button )
	{
	case MENU_BUTTON_RIGHT:
	case MENU_BUTTON_LEFT:
		{
			/* If we're rouletting, hands off. */
			if( m_MusicWheel.IsRouletting() )
				return;

			// Reset the repeat timer when the button is released.
			// This fixes jumping when you release Left and Right after entering the sort 
			// code at the same if L & R aren't released at the exact same time.

			/* BUG: this causes the wheel to slow down, for some reason, when one player is
			 * is in OptionsList and the other is scrolling. However, ignoring the logic for
			 * players in the list causes the song wheel to ignore releases. How to handle? */
			if( type == IET_RELEASE )
			{
				FOREACH_HumanPlayer( p )
				{
					INPUTMAPPER->ResetKeyRepeat( MenuInput(p, MENU_BUTTON_LEFT) );
					INPUTMAPPER->ResetKeyRepeat( MenuInput(p, MENU_BUTTON_RIGHT) );
				}
			}

			bool bLeftIsDown = false;
			bool bRightIsDown = false;
			FOREACH_EnabledPlayer( p )
			{
				if( SELECT_MENU_AVAILABLE && INPUTMAPPER->IsButtonDown(MenuInput(p, MENU_BUTTON_SELECT)) )
					continue;

				bLeftIsDown |= INPUTMAPPER->IsButtonDown( MenuInput(p, MENU_BUTTON_LEFT) );
				bRightIsDown |= INPUTMAPPER->IsButtonDown( MenuInput(p, MENU_BUTTON_RIGHT) );
			}
			
			bool bBothDown = bLeftIsDown && bRightIsDown;
			bool bNeitherDown = !bLeftIsDown && !bRightIsDown;

			if( bBothDown || bNeitherDown )
			{
				m_MusicWheel.Move( 0 );
				if( type == IET_FIRST_PRESS && !bSelectIsPressed )
				{
					switch( MenuI.button )
					{
					case MENU_BUTTON_LEFT:
						m_MusicWheel.ChangeMusicUnlessLocked( -1 );
						break;
					case MENU_BUTTON_RIGHT:
						m_MusicWheel.ChangeMusicUnlessLocked( +1 );
						break;
					}
					/*if (! GAMESTATE->IsCourseMode() )
					{
						Song *pCurSong = m_MusicWheel.GetSelectedSong();
					}*/
				}
			}
			else if( bLeftIsDown )
			{
				if( type != IET_RELEASE )
					m_MusicWheel.Move( -1 );
			}
			else if( bRightIsDown )
			{
				if( type != IET_RELEASE )
					m_MusicWheel.Move( +1 );
			}
			else
			{
				ASSERT(0);
			}
		}
		break;
	}


	if( bSelectIsPressed )
		return;

	// TRICKY:  Do default processing of MenuLeft and MenuRight before detecting 
	// codes.  Do default processing of Start AFTER detecting codes.  This gives us a 
	// change to return if Start is part of a code because we don't want to process 
	// Start as "move to the next screen" if it was just part of a code.
	switch( MenuI.button )
	{
	case MENU_BUTTON_UP:	this->MenuUp( MenuI.player, type );		break;
	case MENU_BUTTON_DOWN:	this->MenuDown( MenuI.player, type );	break;
	case MENU_BUTTON_LEFT:	this->MenuLeft( MenuI.player, type );	break;
	case MENU_BUTTON_RIGHT:	this->MenuRight( MenuI.player, type );	break;
	case MENU_BUTTON_BACK:
		/* Don't make the user hold the back button if they're pressing escape and escape is the back button. */
		if( DeviceI.device == DEVICE_KEYBOARD  &&  DeviceI.button == KEY_ESC )
			this->MenuBack( MenuI.player );
		else
			Screen::MenuBack( MenuI.player, type );
		break;
	// Do the default handler for Start after detecting codes.
//	case MENU_BUTTON_START:	this->MenuStart( MenuI.player, type );	break;
	case MENU_BUTTON_COIN:	this->MenuCoin( MenuI.player, type );	break;
	}


	if( type == IET_FIRST_PRESS )
	{
		if( CodeDetector::EnteredEasierDifficulty(GameI.controller) )
		{
			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
				m_soundLocked.Play();
			else
				ChangeDifficulty( pn, -1 );
			return;
		}
		if( CodeDetector::EnteredHarderDifficulty(GameI.controller) )
		{
			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
				m_soundLocked.Play();
			else
				ChangeDifficulty( pn, +1 );
			return;
		}
		if( CodeDetector::EnteredModeMenu(GameI.controller) )
		{
			if( MODE_MENU_AVAILABLE )
				m_MusicWheel.ChangeSort( SORT_MODE_MENU );
			else
				m_soundLocked.Play();
			return;
		}
		if( CodeDetector::EnteredNextSort(GameI.controller) )
		{
			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
//			if( ( GAMESTATE->IsExtraStage() && !PREFSMAN->m_bPickExtraStage ) || GAMESTATE->IsExtraStage2() )
				m_soundLocked.Play();
			else
				m_MusicWheel.NextSort();
			return;
		}
		if( !GAMESTATE->IsExtraStage() && !GAMESTATE->IsExtraStage2() && CodeDetector::DetectAndAdjustMusicOptions(GameI.controller) )
		{
			m_soundOptionsChange.Play();
			UpdateOptionsDisplays();
			return;
		}
	}

	switch( MenuI.button )
	{
	case MENU_BUTTON_START:	Screen::MenuStart( MenuI.player, type );	break;
	}
}

void ScreenSelectMusic::OpenOptionsList( PlayerNumber pn )
{
        m_OptionsList[pn].Open();
        CString msg(ssprintf("OptionsListOpenedP%d",pn+1));
        //msg.SetParam( "Player", pn );
        MESSAGEMAN->Broadcast( msg );
}

void ScreenSelectMusic::CloseOptionsList( PlayerNumber pn )
{
        m_OptionsList[pn].Close();
        CString msg(ssprintf("OptionsListClosedP%d",pn+1));
        //msg.SetParam( "Player", pn );
        MESSAGEMAN->Broadcast( msg );
}

void ScreenSelectMusic::LoadHelpText()
{
	ScreenWithMenuElements::LoadHelpText();

	bool bSelectIsDown = false;
	FOREACH_EnabledPlayer( p )
		bSelectIsDown |= INPUTMAPPER->IsButtonDown( MenuInput(p, MENU_BUTTON_SELECT) );
	if( !SELECT_MENU_AVAILABLE )
		bSelectIsDown = false;

	/* If m_soundSelectPressed isn't loaded yet, wait until it is before we do this. */
	if( m_bSelectIsDown != bSelectIsDown && m_soundSelectPressed.IsLoaded() )
	{
		if( bSelectIsDown )
			m_soundSelectPressed.Play();

		m_bSelectIsDown = bSelectIsDown;
		if( bSelectIsDown )
			MESSAGEMAN->Broadcast( "SelectMenuOn" );
		else
			MESSAGEMAN->Broadcast( "SelectMenuOff" );
	}
}

void ScreenSelectMusic::ChangeDifficulty( PlayerNumber pn, int dir )
{
	LOG->Trace( "ScreenSelectMusic::ChangeDifficulty( %d, %d )", pn, dir );

	ASSERT( GAMESTATE->IsHumanPlayer(pn) );

	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_SONG:
	case TYPE_PORTAL:
		{
			m_iSelection[pn] += dir;
			if( CLAMP(m_iSelection[pn],0,m_vpSteps.size()-1) )
				return;
			
			// the user explicity switched difficulties.  Update the preferred difficulty
			GAMESTATE->ChangePreferredDifficulty( pn, m_vpSteps[ m_iSelection[pn] ]->GetDifficulty() );

			if( dir < 0 )
				m_soundDifficultyEasier.Play();
			else
				m_soundDifficultyHarder.Play();

			vector<PlayerNumber> vpns;
			FOREACH_HumanPlayer( p )
			{
				if( pn == p || GAMESTATE->DifficultiesLocked() )
				{
					m_iSelection[p] = m_iSelection[pn];
					vpns.push_back( p );
				}
			}
			AfterStepsChange( vpns );
		}
		break;

	case TYPE_COURSE:
		{
			m_iSelection[pn] += dir;
			if( CLAMP(m_iSelection[pn],0,m_vpTrails.size()-1) )
				return;

			// the user explicity switched difficulties.  Update the preferred difficulty
			GAMESTATE->ChangePreferredCourseDifficulty( pn, m_vpTrails[ m_iSelection[pn] ]->m_CourseDifficulty );

			if( dir < 0 )
				m_soundDifficultyEasier.Play();
			else
				m_soundDifficultyHarder.Play();

			vector<PlayerNumber> vpns;
			FOREACH_HumanPlayer( p )
			{
				if( pn == p || GAMESTATE->DifficultiesLocked() )
				{
					m_iSelection[p] = m_iSelection[pn];
					vpns.push_back( p );
				}
			}
			AfterTrailChange( vpns );
		}
		break;

	case TYPE_RANDOM:
	case TYPE_ROULETTE:
	case TYPE_SECTION:
		/* XXX: We could be on a music or course sort, or even one with both; we don't
		 * really know which difficulty to change.  Maybe the two difficulties should be
		 * linked ... */
		if( GAMESTATE->ChangePreferredDifficulty( pn, dir ) )
		{
			if( dir < 0 )
				m_soundDifficultyEasier.Play();
			else
				m_soundDifficultyHarder.Play();

			vector<PlayerNumber> vpns;
			FOREACH_HumanPlayer( p )
			{
				if( pn == p || GAMESTATE->DifficultiesLocked() )
				{
					m_iSelection[p] = m_iSelection[pn];
					vpns.push_back( p );
				}
			}
			AfterStepsChange( vpns );
		}
		break;
	case TYPE_SORT:
		break;
	default:
		WARN( ssprintf("%i", m_MusicWheel.GetSelectedType()) );
		break;
	}
}


void ScreenSelectMusic::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_AllowOptionsMenuRepeat )
	{
		m_bAllowOptionsMenuRepeat = true;
	}
	else if( SM == SM_TweenOffOptionsMessage )
	{
		OFF_COMMAND( m_sprOptionsMessage );
		this->HandleScreenMessage( SM_BeginFadingOut );
	}
	else if( SM == SM_MenuTimer )
	{
		if( m_MusicWheel.IsRouletting() )
		{
			MenuStart(PLAYER_INVALID);
			m_MenuTimer->SetSeconds( 15 );
			m_MenuTimer->Start();
		}
		else if( DO_ROULETTE_ON_MENU_TIMER )
		{
			if( m_MusicWheel.GetSelectedType() != TYPE_SONG )
			{
				m_MusicWheel.StartRoulette();
				m_MenuTimer->SetSeconds( 15 );
				m_MenuTimer->Start();
			}
			else
			{
				MenuStart(PLAYER_INVALID);
			}
		}
		else
		{
			// Finish sort changing so that the wheel can respond immediately to our
			// request to choose random.
			m_MusicWheel.FinishChangingSorts();
			switch( m_MusicWheel.GetSelectedType() )
			{
			case TYPE_SONG:
			case TYPE_COURSE:
			case TYPE_RANDOM:
			case TYPE_PORTAL:
				break;
			default:
				m_MusicWheel.StartRandom();
				break;
			}
			MenuStart(PLAYER_INVALID);
			m_MenuTimer->SetSeconds( 15 );
			m_MenuTimer->Start();
		}
		return;
	}
	else if( SM == SM_GoToPrevScreen )
	{
		SCREENMAN->DeletePreparedScreens();
		SCREENMAN->SetNewScreen( PREV_SCREEN );

		/* We may have stray SM_SongChanged messages from the music wheel.  We can't
		 * handle them anymore, since the title menu (and attract screens) reset
		 * the game state, so just discard them. */
		ClearMessageQueue();
		return;
	}
	else if( SM == SM_BeginFadingOut )
	{
		/* XXX: yuck.  Later on, maybe this can be done in one BGA with lua ... */
		if( m_bGoToOptions )
			m_bgOptionsOut.StartTransitioning( SM_GoToNextScreen );
		else
			m_bgNoOptionsOut.StartTransitioning( SM_GoToNextScreen );
	}
	else if( SM == SM_GoToNextScreen )
	{
		if( m_bGoToOptions )
		{
			SCREENMAN->SetNewScreen( NEXT_OPTIONS_SCREEN );
		}
		else
		{
			SOUND->StopMusic();
			SCREENMAN->SetNewScreen( NEXT_SCREEN );
		}
		return;
	}
	else if( SM == SM_SongChanged )
	{
		AfterMusicChange();
	}
	else if( SM == SM_SortOrderChanging ) /* happens immediately */
	{
//		m_MusicSortDisplay.FadeOff( 0, "fade", TWEEN_TIME );
		TweenScoreOnAndOffAfterChangeSort();
	}
	else if( SM == SM_SortOrderChanged ) /* happens after the wheel is off and the new song is selected */
	{
		SortOrderChanged();
	}
	else if( SM == SM_GainFocus )
	{
		CodeDetector::RefreshCacheItems( CODES );
	}
	else if( SM == SM_LoseFocus )
	{
		CodeDetector::RefreshCacheItems(); /* reset for other screens */
	}

	Screen::HandleScreenMessage( SM );
}

// XXX: lots of ctors/dtors and redundant calls. How can we best fix this?
void UpdateLoadProgress( unsigned long iCurrent, unsigned long iTotal )
{
	// UGLY: send a manual update to INPUTFILTER to force input buffering
	INPUTFILTER->Update( 0 );

	bool bInterrupt = false;

	// if a player presses Select or ML+MR, stop loading the song.
	FOREACH_EnabledPlayer( pn )
	{
		bInterrupt |= INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_SELECT) );
		
		bInterrupt |= INPUTMAPPER->IsButtonDown(MenuInput(pn, MENU_BUTTON_LEFT)) &&
			INPUTMAPPER->IsButtonDown(MenuInput(pn, MENU_BUTTON_RIGHT));
	}

	if( bInterrupt )
	{
		InterruptCopy();
		LOG->Warn( "Custom song load interrupted." );

		// TRICKY DISCO: discard all input events recorded since update.
		// otherwise, we'll get all the pressed buttons at once.
		InputEventArray throwaway;
		INPUTFILTER->GetInputEvents( throwaway );
	}

	/* only Draw() occasionally, since this is expensive. */
	if( DrawTimer.Ago() < DRAW_UPDATE_TIME )
		return;

	unsigned long iPercent = iCurrent / (iTotal/100);

	// XXX: kind of voodoo
	CString sMessage = ssprintf( "\n\n%s\n%lu%%\n%s",
		CUSTOM_SONG_WAIT_TEXT.GetValue().c_str(), 
		iPercent,
		CUSTOM_SONG_CANCEL_TEXT.GetValue().c_str() );

	SCREENMAN->OverlayMessage( sMessage );
	SCREENMAN->Draw();

	DrawTimer.Touch();
}

// run a few basic tests to be sure we aren't breaking any limits...
// TODO: hardcore cleanup.
bool ScreenSelectMusic::ValidateCustomSong( Song* pSong )
{
	LOG->Trace( "ScreenSelectMusic::ValidateCustomSong()" );

	// playing the preview while attempting to copy the song from USB = bad
	SOUND->StopMusic();

	SCREENMAN->OverlayMessage( CUSTOM_SONG_WAIT_TEXT );
	SCREENMAN->Draw();

	// set a timeout for USB access, so we don't get flooded with messages.
	RageFileDriverTimeout::SetTimeout( 6000 );

	// whoever owns the song we're reading, mount their card.
#ifndef WIN32
	if( !PREFSMAN->m_bCustomSongPreviews )
		MEMCARDMAN->MountCard( pSong->m_SongOwner, 6000 );
#endif
	
	// verify the song internally since we can read the data now
	CString sError;
	bool bVerified = pSong->CheckCustomSong( sError );

	// catch any errors here. if no sError and bVerified
	// is still false, a Bad Thing(tm) happened.
	if( !sError.empty() )
		SCREENMAN->SystemMessage( sError );
	else if( !bVerified )
		SCREENMAN->SystemMessage( "Error accessing USB drive." );

	bool bCopied = false;

	// if bVerified is false, we still want to do our cleanup after
	// this code block, so it's wrapped in this conditional.
	if( bVerified )
	{
		RageTimer timer;

		// we can copy the music. destination is determined with
		// "m_sGameplayMusic" so we can change that from one place
		bCopied = FileCopy( GAMESTATE->m_pCurSong->GetMusicPath(), 
		GAMESTATE->m_pCurSong->m_sGameplayMusic, sError, &UpdateLoadProgress );

		// failed, most likely a permissions error
		if( !bCopied && !sError.empty() )
			SCREENMAN->SystemMessage( ssprintf("Copying error: %s.", sError.c_str()) );

		LOG->Trace( "Copied %s -> %s in %f seconds.", GAMESTATE->m_pCurSong->GetMusicPath().c_str(),
			GAMESTATE->m_pCurSong->m_sGameplayMusic.c_str(), timer.Ago() );
	}

	// ...and unmount, now that we're done.
#ifndef WIN32
	if( !PREFSMAN->m_bCustomSongPreviews )
		MEMCARDMAN->UnmountCard( pSong->m_SongOwner );
#endif
	SCREENMAN->HideOverlayMessage();

	// force a zero on next update so the animation doesn't
	// skip - we spent a lot of time loading.
	SCREENMAN->ZeroNextUpdate();

	// reset the USB timeout
	RageFileDriverTimeout::ResetTimeout();

	return bVerified && bCopied;
}

void ScreenSelectMusic::MenuStart( PlayerNumber pn )
{
	/* If the timer is still going and an options list is open, ignore.
	 * If the timer's run out, force-close all lists and continue. */
	FOREACH_EnabledPlayer( pn )
	{
		if( this->m_OptionsList[pn].IsOpened() )
		{
			if( (int)m_MenuTimer->GetSeconds() != 0 )
				return;
			else
				CloseOptionsList( pn );
		}
	}

	// this needs to check whether valid Steps are selected!
	bool bResult = m_MusicWheel.Select();

	/* If false, we don't have a selection just yet. */
	if( !bResult )
		return;

	// a song was selected
	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_SONG:
	case TYPE_PORTAL:
		{
			SCREENMAN->PlayStartSound();

			if( m_MusicWheel.GetSelectedSong()->IsCustomSong() ) // a song we need to test
			{
				// if false and no time, set a random song that isn't a custom - those could fail.
				if( (int)m_MenuTimer->GetSeconds() == 0 )
				{
					m_MusicWheel.StartRandom();
					m_bMadeChoice = m_MusicWheel.Select();
				}
				else
					m_bMadeChoice = ValidateCustomSong( m_MusicWheel.GetSelectedSong() );

				if(m_bMadeChoice) m_bGoToOptions = false;
			}
			else
				m_bMadeChoice = true;


			const bool bIsNew = PROFILEMAN->IsSongNew( m_MusicWheel.GetSelectedSong() );
			bool bIsHard = false;
			FOREACH_HumanPlayer( p )
			{
				if( GAMESTATE->m_pCurSteps[p]  &&  GAMESTATE->m_pCurSteps[p]->GetMeter() >= 10 )
					bIsHard = true;
			}

			/* See if this song is a repeat.  If we're in event mode, only check the last five songs. */
			bool bIsRepeat = false;
			int i = 0;
			if( GAMESTATE->IsEventMode() )
				i = max( 0, int(STATSMAN->m_vPlayedStageStats.size())-5 );
			for( ; i < (int)STATSMAN->m_vPlayedStageStats.size(); ++i )
				if( STATSMAN->m_vPlayedStageStats[i].vpPlayedSongs.back() == m_MusicWheel.GetSelectedSong() )
					bIsRepeat = true;

			/* Don't complain about repeats if the user didn't get to pick. */
//			if( GAMESTATE->IsExtraStage() && !PREFSMAN->m_bPickExtraStage )
//				bIsRepeat = false;

			if( bIsRepeat )
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "select music comment repeat" ));
			else if( bIsNew )
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "select music comment new" ));
			else if( bIsHard )
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "select music comment hard" ));
			else
				SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "select music comment general" ));

			/* If we're in event mode, we may have just played a course (putting us
			 * in course mode).  Make sure we're in a single song mode. */
			if( GAMESTATE->IsCourseMode() )
				GAMESTATE->m_PlayMode = PLAY_MODE_REGULAR;
		}
		break;

	case TYPE_COURSE:
		{
			SCREENMAN->PlayStartSound();

			SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo( "select course comment general" ));

			Course *pCourse = m_MusicWheel.GetSelectedCourse();
			ASSERT( pCourse );

			bool bError = false;
			CString sError;
			if ( pCourse->m_bIsCustomCourse )
			{
				for (unsigned i = 0; i < pCourse->m_entries.size(); i++)
				{
					Song *pSong = pCourse->m_entries[i].pSong;
					if (pSong->IsCustomSong())
					{
						CString sNewPath = ssprintf(CUSTOM_SONG_PATH+"crsmusic%d%s",i+1,pSong->m_sExtension.c_str());
#ifndef WIN32
						MEMCARDMAN->MountCard( pSong->m_SongOwner, 20 );
#endif

						bError = !pSong->CheckCustomSong( sError );
						// TODO: give custom song for course xfer its own progress function
						if (!bError) bError = !FileCopy( pSong->GetMusicPath(), sNewPath, &UpdateLoadProgress );
#ifndef WIN32
						MEMCARDMAN->UnmountCard( pSong->m_SongOwner );
#endif
						if ( bError ) break;
						pSong->m_sGameplayMusic = sNewPath;
						pSong->m_sBackgroundFile = THEME->GetPathG( "Common", "fallback background" );
					}
				}
				SCREENMAN->HideOverlayMessage();
				SCREENMAN->ZeroNextUpdate();
			}
			if ( bError )
			{
				SCREENMAN->SystemMessage( sError );
				m_bMadeChoice = false;
				break;
			}

			GAMESTATE->m_PlayMode = pCourse->GetPlayMode();

			// apply #LIVES
			if( pCourse->m_iLives != -1 )
			{
				GAMESTATE->m_SongOptions.m_LifeType = SongOptions::LIFE_BATTERY;
				GAMESTATE->m_SongOptions.m_iBatteryLives = pCourse->m_iLives;
			}

			m_bMadeChoice = true;
		}
		break;
	case TYPE_SECTION:
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
	case TYPE_SORT:
		break;
	default:
		ASSERT(0);
	}

	if( m_bMadeChoice )
	{
		MESSAGEMAN->Broadcast("SongChosen");

		TweenOffScreen();

		if( OPTIONS_MENU_AVAILABLE )
		{
//			float fShowSeconds = m_Out.GetLengthSeconds();

			// show "hold START for options"
			m_sprOptionsMessage.SetHidden( false );
			ON_COMMAND( m_sprOptionsMessage );

			m_bAllowOptionsMenu = true;
			/* Don't accept a held START for a little while, so it's not
			 * hit accidentally.  Accept an initial START right away, though,
			 * so we don't ignore deliberate fast presses (which would be
			 * annoying). */
			this->PostScreenMessage( SM_AllowOptionsMenuRepeat, 0.5f );
		}

		/* If we're currently waiting on song assets, abort all except the music and
		 * start the music, so if we make a choice quickly before background requests
		 * come through, the music will still start. */
		g_bCDTitleWaiting = g_bBannerWaiting = false;
		m_BackgroundLoader.Abort();
		CheckBackgroundRequests();

		if( OPTIONS_MENU_AVAILABLE )
		{
			StartTransitioning( SM_None );
			this->PostScreenMessage( SM_TweenOffOptionsMessage, SHOW_OPTIONS_MESSAGE_SECONDS );
		}
		else
		{
			StartTransitioning( SM_BeginFadingOut );
		}
	}

//	if( GAMESTATE->IsExtraStage() && PREFSMAN->m_bPickExtraStage )
	if( GAMESTATE->IsExtraStage() )
	{
		/* Check if user selected the real extra stage. */
		Song* pSong;
		Steps* pSteps;
		PlayerOptions po;
		SongOptions so;
		SONGMAN->GetExtraStageInfo( false, GAMESTATE->GetCurrentStyle(), pSong, pSteps, po, so );
		ASSERT(pSong);
		
		/* Enable 2nd extra stage if user chose the correct song */
		if( m_MusicWheel.GetSelectedSong() == pSong )
			GAMESTATE->m_bAllow2ndExtraStage = true;
		else
			GAMESTATE->m_bAllow2ndExtraStage = false;
	}
}


void ScreenSelectMusic::MenuBack( PlayerNumber pn )
{
	m_BackgroundLoader.Abort();

	Cancel( SM_GoToPrevScreen );
}

void ScreenSelectMusic::AfterStepsChange( const vector<PlayerNumber> &vpns )
{
	FOREACH_CONST( PlayerNumber, vpns, p )
	{
		PlayerNumber pn = *p;
		ASSERT( GAMESTATE->IsHumanPlayer(pn) );
		
		CLAMP( m_iSelection[pn], 0, m_vpSteps.size()-1 );

		Song* pSong = GAMESTATE->m_pCurSong;
		Steps* pSteps = m_vpSteps.empty()? NULL: m_vpSteps[m_iSelection[pn]];

		GAMESTATE->m_pCurSteps[pn].Set( pSteps );

		int iScore = 0;
		if( pSteps )
		{
			Profile* pProfile = PROFILEMAN->IsPersistentProfile(pn) ? PROFILEMAN->GetProfile(pn) : PROFILEMAN->GetMachineProfile();
			iScore = pProfile->GetStepsHighScoreList(pSong,pSteps).GetTopScore().iScore;
		}

		m_textHighScore[pn].SetText( ssprintf("%*i", NUM_SCORE_DIGITS, iScore) );
		
		m_DifficultyIcon[pn].SetFromSteps( pn, pSteps );
		if( pSteps && pSteps->IsAutogen() )
		{
			m_AutoGenIcon[pn].SetEffectNone();
			m_AutoGenIcon[pn].SetDiffuse( RageColor(1,1,1,1) );
		}
		else
		{
			m_AutoGenIcon[pn].SetEffectNone();
			m_AutoGenIcon[pn].SetDiffuse( RageColor(1,1,1,0) );
		}
		m_DifficultyMeter[pn].SetFromGameState( pn );
		m_GrooveRadar.SetFromSteps( pn, pSteps );
		m_MusicWheel.NotesOrTrailChanged( pn );
		if( SHOW_PANES )
			m_PaneDisplay[pn].SetFromGameState( GAMESTATE->m_SortOrder );
	}

	if( SHOW_DIFFICULTY_LIST )
		m_DifficultyList.SetFromGameState();
}

void ScreenSelectMusic::AfterTrailChange( const vector<PlayerNumber> &vpns )
{
	FOREACH_CONST( PlayerNumber, vpns, p )
	{
		PlayerNumber pn = *p;
		ASSERT( GAMESTATE->IsHumanPlayer(pn) );
		
		CLAMP( m_iSelection[pn], 0, m_vpTrails.size()-1 );

		Course* pCourse = GAMESTATE->m_pCurCourse;
		Trail* pTrail = m_vpTrails.empty()? NULL: m_vpTrails[m_iSelection[pn]];

		GAMESTATE->m_pCurTrail[pn].Set( pTrail );

		int iScore = 0;
		if( pTrail )
		{
			Profile* pProfile = PROFILEMAN->IsPersistentProfile(pn) ? PROFILEMAN->GetProfile(pn) : PROFILEMAN->GetMachineProfile();
			iScore = pProfile->GetCourseHighScoreList(pCourse,pTrail).GetTopScore().iScore;
		}

		m_textHighScore[pn].SetText( ssprintf("%*i", NUM_SCORE_DIGITS, iScore) );
		
		m_DifficultyIcon[pn].SetFromTrail( pn, pTrail );
		//if( pTrail && pTrail->IsAutogen() )
		//{
		//	m_AutoGenIcon[pn].SetEffectDiffuseShift();
		//}
		//else
		//{
		//	m_AutoGenIcon[pn].SetEffectNone();
		//	m_AutoGenIcon[pn].SetDiffuse( RageColor(1,1,1,0) );
		//}

		/* Update the trail list, but don't actually start the tween; only do that when
		* the actual course changes (AfterMusicChange). */
		if( SHOW_COURSE_CONTENTS )
		{
			m_CourseContents.SetFromGameState();
			// m_CourseContents.TweenInAfterChangedCourse();
		}

		m_DifficultyMeter[pn].SetFromGameState( pn );
		m_GrooveRadar.SetEmpty( pn );
		m_MusicWheel.NotesOrTrailChanged( pn );
		if( SHOW_PANES )
			m_PaneDisplay[pn].SetFromGameState( GAMESTATE->m_SortOrder );
	}

	if( SHOW_DIFFICULTY_LIST )
		m_DifficultyList.SetFromGameState();
}

void ScreenSelectMusic::SwitchToPreferredDifficulty()
{
	if( !GAMESTATE->m_pCurCourse )
	{
		FOREACH_HumanPlayer( pn )
		{
			/* Find the closest match to the user's preferred difficulty. */
			int CurDifference = -1;
			for( unsigned i=0; i<m_vpSteps.size(); i++ )
			{
				int Diff = abs(m_vpSteps[i]->GetDifficulty() - GAMESTATE->m_PreferredDifficulty[pn]);

				if( CurDifference == -1 || Diff < CurDifference )
				{
					m_iSelection[pn] = i;
					CurDifference = Diff;
				}
			}

			CLAMP( m_iSelection[pn],0,m_vpSteps.size()-1 );
		}
	}
	else
	{
		FOREACH_HumanPlayer( pn )
		{
			/* Find the closest match to the user's preferred difficulty. */
			int CurDifference = -1;
			for( unsigned i=0; i<m_vpTrails.size(); i++ )
			{
				int Diff = abs(m_vpTrails[i]->m_CourseDifficulty - GAMESTATE->m_PreferredCourseDifficulty[pn]);

				if( CurDifference == -1 || Diff < CurDifference )
				{
					m_iSelection[pn] = i;
					CurDifference = Diff;
				}
			}

			CLAMP( m_iSelection[pn],0,m_vpTrails.size()-1 );
		}
	}
}

template<class T>
int FindCourseIndexOfSameMode( T begin, T end, const Course *p )
{
	const PlayMode pm = p->GetPlayMode();
	
	int n = 0;
	for( T it = begin; it != end; ++it )
	{
		if( *it == p )
			return n;

		/* If it's not playable in this mode, don't increment.  It might result in 
		 * different output in different modes, but that's better than having holes. */
		if( !(*it)->IsPlayableIn( GAMESTATE->GetCurrentStyle()->m_StepsType ) )
			continue;
		if( (*it)->GetPlayMode() != pm )
			continue;
		++n;
	}

	return -1;
}

void ScreenSelectMusic::AfterMusicChange()
{
	if( !m_MusicWheel.IsRouletting() )
		m_MenuTimer->Stall();

	// lock difficulties.  When switching from arcade to rave, we need to 
	// enforce that all players are at the same difficulty.
	if( GAMESTATE->DifficultiesLocked() )
	{
		FOREACH_HumanPlayer( p )
		{
			m_iSelection[p] = m_iSelection[0];
			GAMESTATE->m_PreferredDifficulty[p] = GAMESTATE->m_PreferredDifficulty[0];
		}
	}

	Song* pSong = m_MusicWheel.GetSelectedSong();
	GAMESTATE->m_pCurSong.Set( pSong );
	if( pSong )
		GAMESTATE->m_pPreferredSong = pSong;

	m_GrooveGraph.SetFromSong( pSong );

	Course* pCourse = m_MusicWheel.GetSelectedCourse();
	GAMESTATE->m_pCurCourse.Set( pCourse );
	if( pCourse )
		GAMESTATE->m_pPreferredCourse = pCourse;

	FOREACH_PlayerNumber( p )
	{
		GAMESTATE->m_pCurSteps[p].Set( NULL );
		GAMESTATE->m_pCurTrail[p].Set( NULL );
		m_vpSteps.clear();
		m_vpTrails.clear();
	}

	m_Banner.SetMovingFast( !!m_MusicWheel.IsMoving() );

	CString SampleMusicToPlay, SampleMusicTimingData;
	vector<CString> m_Artists, m_AltArtists;

	m_MachineRank.SetText( "" );

	m_sSampleMusicToPlay = "";
	m_pSampleMusicTimingData = NULL;
	g_sCDTitlePath = "";
	g_sBannerPath = "";
	g_bWantFallbackCdTitle = false;
	bool bWantBanner = true;

	static SortOrder s_lastSortOrder = SORT_INVALID;
	if( GAMESTATE->m_SortOrder != s_lastSortOrder )
	{
		// Reload to let Lua metrics have a chance to change the help text.
		LoadHelpText();
		s_lastSortOrder = GAMESTATE->m_SortOrder;
	}

	// do this here, so it'll properly update if there's no song
	UpdateStage();

	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_SECTION:
	case TYPE_SORT:
		{	
			CString sGroup = m_MusicWheel.GetSelectedSection();
			FOREACH_PlayerNumber( p )
				m_iSelection[p] = -1;

			m_BPMDisplay.NoBPM();
			g_sCDTitlePath = ""; // none
			m_DifficultyDisplay.UnsetDifficulties();

			m_fSampleStartSeconds = 0;
			m_fSampleLengthSeconds = -1;

			m_textNumSongs.SetText( "" );
			m_textTotalTime.SetText( "" );
			
			switch( m_MusicWheel.GetSelectedType() )
			{
			case TYPE_SECTION:
				g_sBannerPath = SONGMAN->GetGroupBannerPath( sGroup );
				m_sSampleMusicToPlay = m_sSectionMusicPath;
				break;
			case TYPE_SORT:
				bWantBanner = false; /* we load it ourself */
				switch( GAMESTATE->m_SortOrder )
				{
				case SORT_MODE_MENU:
					m_Banner.LoadMode();
					break;
				}
				m_sSampleMusicToPlay = m_sSortMusicPath;
				break;
			default:
				ASSERT(0);
			}

			m_sprLongBalloon->StopTweening();
			COMMAND( m_sprLongBalloon, "Hide" );
			m_sprMarathonBalloon->StopTweening();
			COMMAND( m_sprMarathonBalloon, "Hide" );
			
			COMMAND( m_sprCourseHasMods, "Hide" );
		}
		break;
	case TYPE_SONG:
	case TYPE_PORTAL:
		{
			// this can occur if the song wheel only has custom songs on it.
			if( !pSong )
				break;

			// UGLY: grab the path to _silent.ogg so timing
			// data still runs - it'll only run if we play audio
			if( pSong->IsCustomSong() && !PREFSMAN->m_bCustomSongPreviews )
				m_sSampleMusicToPlay = THEME->GetPathS( "", "_silent" );
			else
				m_sSampleMusicToPlay = pSong->GetMusicPath();

			m_pSampleMusicTimingData = &pSong->m_Timing;
			m_fSampleStartSeconds = pSong->m_fMusicSampleStartSeconds;
			m_fSampleLengthSeconds = pSong->m_fMusicSampleLengthSeconds;

			m_textNumSongs.SetText( ssprintf("%d", SongManager::GetNumStagesForSong(pSong) ) );
			m_textTotalTime.SetText( SecondsToMMSSMsMs(pSong->MusicLengthSeconds()) );

			pSong->GetSteps( m_vpSteps, GAMESTATE->GetCurrentStyle()->m_StepsType );

			StepsUtil::RemoveLockedSteps( pSong, m_vpSteps );
			StepsUtil::SortNotesArrayByDifficulty( m_vpSteps );

			// if it's a custom, load the 'edit' banner.
			/* Please note: displaying banners if CustomSongPreviews
			 * is for testing only and is not intended behaviour. */
			if ( PREFSMAN->m_bShowBanners )
			{
				if( pSong->IsCustomSong() && !PREFSMAN->m_bCustomSongPreviews )
					g_sBannerPath = THEME->GetPathG("Banner","custom");
				else
					g_sBannerPath = pSong->GetBannerPath();
			}

			if( GAMESTATE->IsExtraStage() || GAMESTATE->IsExtraStage2() )
			{
				m_BPMDisplay.CycleRandomly();
			}
			else
			{
				m_BPMDisplay.SetBpmFromSong( pSong );
			}

			g_sCDTitlePath = pSong->GetCDTitlePath();
			g_bWantFallbackCdTitle = true;

			const vector<Song*> best = SONGMAN->GetBestSongs( PROFILE_SLOT_MACHINE );
			const int index = FindIndex( best.begin(), best.end(), pSong );
			if( index != -1 )
				m_MachineRank.SetText( FormatNumberAndSuffix( index+1 ) );

			m_DifficultyDisplay.SetDifficulties( pSong, GAMESTATE->GetCurrentStyle()->m_StepsType );

			SwitchToPreferredDifficulty();

			/* Short delay before actually showing these, so they don't show
			 * up when scrolling fast.  It'll still show up in "slow" scrolling,
			 * but it doesn't look at weird as it does in "fast", and I don't
			 * like the effect with a lot of delay. */
			if( pSong->IsMarathon() )
			{
				m_sprMarathonBalloon->StopTweening();
				SET_XY( m_sprMarathonBalloon );
				COMMAND( m_sprMarathonBalloon, "Show" );

				m_sprLongBalloon->StopTweening();
				COMMAND( m_sprLongBalloon, "Hide" );

			}
			else if( pSong->IsLong() )
			{
				m_sprLongBalloon->StopTweening();
				SET_XY( m_sprLongBalloon );
				COMMAND( m_sprLongBalloon, "Show" );
				
				m_sprMarathonBalloon->StopTweening();
				COMMAND( m_sprMarathonBalloon, "Hide" );
			}
			else
			{
				m_sprLongBalloon->StopTweening();
				COMMAND( m_sprLongBalloon, "Hide" );

				m_sprMarathonBalloon->StopTweening();
				COMMAND( m_sprMarathonBalloon, "Hide" );
			}

			if ( !GAMESTATE->IsFinalStage() )
				GAMESTATE->m_iCurrentStageIndex = m_iSavedRoundIndex;

			COMMAND( m_sprCourseHasMods, "Hide" );

			m_Artists.push_back( pSong->GetDisplayArtist() );
			m_AltArtists.push_back( pSong->GetTranslitArtist() );
		}
		break;
	case TYPE_ROULETTE:
	case TYPE_RANDOM:
		bWantBanner = false; /* we load it ourself */
		switch(m_MusicWheel.GetSelectedType())
		{
		case TYPE_ROULETTE:	m_Banner.LoadRoulette();	break;
		case TYPE_RANDOM: 	m_Banner.LoadRandom();		break;
		default: ASSERT(0);
		}
		m_BPMDisplay.NoBPM();
		g_sCDTitlePath = ""; // none
		m_DifficultyDisplay.UnsetDifficulties();

		m_fSampleStartSeconds = 0;
		m_fSampleLengthSeconds = -1;

		m_textNumSongs.SetText( "" );
		m_textTotalTime.SetText( "" );

		switch( m_MusicWheel.GetSelectedType() )
		{
		case TYPE_ROULETTE:
			m_sSampleMusicToPlay = m_sRouletteMusicPath;
			break;
		case TYPE_RANDOM:
			m_sSampleMusicToPlay = m_sRandomMusicPath;
			break;
		default:
			ASSERT(0);
		}

		m_sprLongBalloon->StopTweening();
		COMMAND( m_sprLongBalloon, "Hide" );
		m_sprMarathonBalloon->StopTweening();
		COMMAND( m_sprMarathonBalloon, "Hide" );
		
		COMMAND( m_sprCourseHasMods, "Hide" );
		
		break;
	case TYPE_COURSE:
	{
		Course* pCourse = m_MusicWheel.GetSelectedCourse();
		StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
		Trail *pTrail = pCourse->GetTrail( st );
		ASSERT( pTrail );
		pCourse->GetTrails( m_vpTrails, GAMESTATE->GetCurrentStyle()->m_StepsType );

		m_sSampleMusicToPlay = m_sCourseMusicPath;
		m_fSampleStartSeconds = 0;
		m_fSampleLengthSeconds = -1;

		m_textNumSongs.SetText( ssprintf("%d", pCourse->GetEstimatedNumStages()) );
		float fTotalSeconds;
		if( pCourse->GetTotalSeconds(st,fTotalSeconds) )
			m_textTotalTime.SetText( SecondsToMMSSMsMs(fTotalSeconds) );
		else
			m_textTotalTime.SetText( "xx:xx.xx" );	// The numbers format doesn't have a '?'.  Is there a better solution?

		g_sBannerPath = pCourse->m_sBannerPath;
		if( g_sBannerPath.empty() )
			m_Banner.LoadFallback();

		if( (int)pTrail->m_vEntries.size() > MAX_COURSE_ENTRIES_BEFORE_VARIOUS )
			m_BPMDisplay.SetVarious();
		else
			m_BPMDisplay.SetBpmFromCourse( pCourse );

		m_DifficultyDisplay.UnsetDifficulties();

		SwitchToPreferredDifficulty();

		FOREACH_CONST( TrailEntry, pTrail->m_vEntries, e )
		{
			if( e->bSecret )
			{
				m_Artists.push_back( "???" );
				m_AltArtists.push_back( "???" );
			}
			else
			{
				m_Artists.push_back( e->pSong->GetDisplayArtist() );
				m_AltArtists.push_back( e->pSong->GetTranslitArtist() );
			}
		}

		CourseType ct = PlayModeToCourseType( GAMESTATE->m_PlayMode );
		const vector<Course*> best = SONGMAN->GetBestCourses( ct, PROFILE_SLOT_MACHINE );
		const int index = FindCourseIndexOfSameMode( best.begin(), best.end(), pCourse );
		if( index != -1 )
			m_MachineRank.SetText( FormatNumberAndSuffix( index+1 ) );



		m_sprLongBalloon->StopTweening();
		COMMAND( m_sprLongBalloon, "Hide" );
		m_sprMarathonBalloon->StopTweening();
		COMMAND( m_sprMarathonBalloon, "Hide" );

		if( pCourse->HasMods() )
		{
			COMMAND( m_sprCourseHasMods, "Show" );
		}
		else
		{
			COMMAND( m_sprCourseHasMods, "Hide" );
		}

		break;
	}
	default:
		ASSERT(0);
	}

	m_sprCDTitleFront.UnloadTexture();
	m_sprCDTitleBack.UnloadTexture();

	/* Cancel any previous, incomplete requests for song assets, since we need new ones. */
	m_BackgroundLoader.Abort();

	g_bCDTitleWaiting = false;
	if( !g_sCDTitlePath.empty() || g_bWantFallbackCdTitle )
	{
		LOG->Trace( "cache \"%s\"", g_sCDTitlePath.c_str());
		m_BackgroundLoader.CacheFile( g_sCDTitlePath ); // empty OK
		g_bCDTitleWaiting = true;
	}

	g_bBannerWaiting = false;
	if( bWantBanner )
	{
		LOG->Trace("LoadFromCachedBanner(%s)",g_sBannerPath .c_str());
		if( m_Banner.LoadFromCachedBanner( g_sBannerPath ) )
		{
			/* If the high-res banner is already loaded, just
			 * delay before loading it, so the low-res one has
			 * time to fade in. */
			if( !TEXTUREMAN->IsTextureRegistered( Sprite::SongBannerTexture(g_sBannerPath) ) )
				m_BackgroundLoader.CacheFile( g_sBannerPath );

			g_bBannerWaiting = true;
		}
	}

	// Don't stop music if it's already playing the right file.
	g_bSampleMusicWaiting = false;

	// However, if it is a custom song, force a reload so we get new timing data.
	if( m_MusicWheel.GetSelectedType() == TYPE_SONG && !PREFSMAN->m_bCustomSongPreviews && pSong->IsCustomSong() )
	{
		SOUND->StopMusic();
		if( !m_sSampleMusicToPlay.empty() )
			g_bSampleMusicWaiting = true;
	}

	if( !m_MusicWheel.IsRouletting() && SOUND->GetMusicPath() != m_sSampleMusicToPlay )
	{
		SOUND->StopMusic();
		if( !m_sSampleMusicToPlay.empty() )
			g_bSampleMusicWaiting = true;
	}

	g_StartedLoadingAt.Touch();

	if( (int) m_Artists.size() > MAX_COURSE_ENTRIES_BEFORE_VARIOUS )
	{
		m_Artists.clear();
		m_AltArtists.clear();
		m_Artists.push_back( "Various Artists" );
		m_AltArtists.push_back( "Various Artists" );
	}

	m_Artist.SetTips( m_Artists, m_AltArtists );

	vector<PlayerNumber> vpns;
	FOREACH_HumanPlayer( p )
		vpns.push_back( p );

	if( GAMESTATE->m_pCurCourse )
		AfterTrailChange( vpns );
	else
		AfterStepsChange( vpns );

	switch( m_MusicWheel.GetSelectedType() )
	{
	case TYPE_COURSE:
		if( SHOW_COURSE_CONTENTS )
			m_CourseContents.TweenInAfterChangedCourse();
		break;
	}
}


void ScreenSelectMusic::UpdateOptionsDisplays()
{
//	m_OptionIcons.Load( GAMESTATE->m_PlayerOptions, &GAMESTATE->m_SongOptions );

//	m_PlayerOptionIcons.Refresh();

	FOREACH_HumanPlayer( p )
	{
		m_OptionIconRow[p].Refresh();

		CString s = GAMESTATE->m_pPlayerState[p]->m_PlayerOptions.GetString();
			s.Replace( ", ", "\n" );
//		m_textPlayerOptions[p].SetText( s );
	}

	CString s = GAMESTATE->m_SongOptions.GetString();
	s.Replace( ", ", "\n" );
	m_textSongOptions.SetText( s );
}

void ScreenSelectMusic::SortOrderChanged()
{
	m_MusicSortDisplay.Set( GAMESTATE->m_SortOrder );
	if( SHOW_PANES )
		FOREACH_HumanPlayer(pn)
			m_PaneDisplay[pn].SetFromGameState( GAMESTATE->m_SortOrder );

	switch( GAMESTATE->m_SortOrder )
	{
	case SORT_ALL_COURSES:
	case SORT_NONSTOP_COURSES:
	case SORT_ONI_COURSES:
	case SORT_ENDLESS_COURSES:
	case SORT_MODE_MENU:
		// do nothing
		break;
	default:
		if( SHOW_DIFFICULTY_LIST )
			m_DifficultyList.Show();
		break;
	}

	// tween music sort on screen
//	m_MusicSortDisplay.FadeOn( 0, "fade", TWEEN_TIME );
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
