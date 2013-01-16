/* ScreenSelectMusic - Choose a Song and Steps. */

#ifndef SCREEN_SELECT_MUSIC_H
#define SCREEN_SELECT_MUSIC_H

#include "ScreenWithMenuElements.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "GameConstantsAndTypes.h"
#include "MusicWheel.h"
#include "Banner.h"
#include "FadingBanner.h"
#include "BPMDisplay.h"
#include "GrooveRadar.h"
#include "GrooveGraph.h"
#include "DifficultyIcon.h"
#include "DifficultyMeter.h"
#include "OptionIconRow.h"
#include "DifficultyDisplay.h"
#include "DifficultyList.h"
#include "CourseContentsList.h"
#include "HelpDisplay.h"
#include "PaneDisplay.h"
#include "Character.h"
#include "RageUtil_BackgroundLoader.h"
#include "ThemeMetric.h"
#include "ActorCommands.h"
#include "song.h"
#include "OptionsList.h"

class ScreenSelectMusic : public ScreenWithMenuElements
{
public:
	ScreenSelectMusic( CString sName );
	virtual ~ScreenSelectMusic();
	virtual void Init();

	virtual void DrawPrimitives();
	virtual void Update( float fDeltaTime );
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
protected:
	void TweenOnScreen();
	void TweenOffScreen();
	void TweenScoreOnAndOffAfterChangeSort();
	enum DisplayMode { DISPLAY_SONGS, DISPLAY_COURSES, DISPLAY_MODES } m_DisplayMode;
	void SwitchDisplayMode( DisplayMode dm );
	void TweenSongPartsOnScreen( bool Initial );
	void TweenSongPartsOffScreen( bool Final );
	void TweenCoursePartsOnScreen( bool Initial );
	void TweenCoursePartsOffScreen( bool Final );
	void SkipSongPartTweens();
	void SkipCoursePartTweens();
	virtual void LoadHelpText();


        void OpenOptionsList( PlayerNumber pn );
        void CloseOptionsList( PlayerNumber pn );

	bool ValidateCustomSong( Song* pSong );

	void ChangeDifficulty( PlayerNumber pn, int dir );

	void AfterStepsChange( const vector<PlayerNumber> &vpns );
	void AfterTrailChange( const vector<PlayerNumber> &vpns );
	void SwitchToPreferredDifficulty();
	void AfterMusicChange();
	void SortOrderChanged();

	void UpdateOptionsDisplays();
	void CheckBackgroundRequests();

	vector<Steps*>		m_vpSteps;
	vector<Trail*>		m_vpTrails;
	int					m_iSelection[NUM_PLAYERS];

	ThemeMetric<float> FOV;
	ThemeMetric<float> FOV_CENTER_X;
	ThemeMetric<float> FOV_CENTER_Y;
	ThemeMetric<float> BANNER_WIDTH;
	ThemeMetric<float> BANNER_HEIGHT;
	ThemeMetric<apActorCommands> SONG_OPTIONS_EXTRA_COMMAND;
	ThemeMetric<float> SAMPLE_MUSIC_DELAY;
	ThemeMetric<bool> SHOW_RADAR;
	ThemeMetric<bool> SHOW_GRAPH;
	ThemeMetric<bool> SHOW_PANES;
	ThemeMetric<bool> SHOW_DIFFICULTY_LIST;
	ThemeMetric<bool> SHOW_COURSE_CONTENTS;
	ThemeMetric<bool> DO_ROULETTE_ON_MENU_TIMER;
	ThemeMetric<bool> ALIGN_MUSIC_BEATS;
	ThemeMetric<CString> CODES;
	ThemeMetric<CString> MUSIC_WHEEL_TYPE;
	ThemeMetric<bool> OPTIONS_MENU_AVAILABLE;
	DynamicThemeMetric<bool> SELECT_MENU_AVAILABLE;
	DynamicThemeMetric<bool> MODE_MENU_AVAILABLE;
        ThemeMetric<bool> USE_OPTIONS_LIST;

	CString m_sSectionMusicPath;
	CString m_sSortMusicPath;
	CString m_sRouletteMusicPath;
	CString m_sRandomMusicPath;
	CString m_sCourseMusicPath;
	CString m_sFallbackCDTitlePath;

	// overlay has been moved to SCREENMAN->OverlayMessage

	Sprite				m_sprCharacterIcon[NUM_PLAYERS];
	Sprite				m_sprBannerMask;
	FadingBanner		m_Banner;
	AutoActor			m_sprBannerFrame;
	BPMDisplay			m_BPMDisplay;
	Sprite				m_sprCDTitleFront, m_sprCDTitleBack;
	Sprite				m_sprDifficultyFrame[NUM_PLAYERS];
	DifficultyIcon		m_DifficultyIcon[NUM_PLAYERS];
	Sprite				m_AutoGenIcon[NUM_PLAYERS];
    GrooveRadar			m_GrooveRadar;
    GrooveGraph			m_GrooveGraph;
	BitmapText			m_textSongOptions;
	BitmapText			m_textNumSongs;
	BitmapText			m_textTotalTime;
	OptionIconRow		m_OptionIconRow[NUM_PLAYERS];
	Sprite				m_sprMeterFrame[NUM_PLAYERS];
	DifficultyMeter			m_DifficultyMeter[NUM_PLAYERS];
	MusicSortDisplay	m_MusicSortDisplay;
	Sprite				m_sprHighScoreFrame[NUM_PLAYERS];
	BitmapText			m_textHighScore[NUM_PLAYERS];
	MusicWheel			m_MusicWheel;
	OptionsList			m_OptionsList[NUM_PLAYERS];
	AutoActor			m_MusicWheelUnder;
	AutoActor			m_sprLongBalloon;
	AutoActor			m_sprMarathonBalloon;
	AutoActor			m_sprCourseHasMods;
	DifficultyDisplay   m_DifficultyDisplay;
	DifficultyList		m_DifficultyList;
	CourseContentsList	m_CourseContents;
	HelpDisplay			m_Artist;
	BitmapText			m_MachineRank;
	PaneDisplay			m_PaneDisplay[NUM_PLAYERS];

	bool				m_bMadeChoice;
	bool				m_bGoToOptions;
	Sprite				m_sprOptionsMessage;
	CString				m_sSampleMusicToPlay;
	TimingData			*m_pSampleMusicTimingData;
	float				m_fSampleStartSeconds, m_fSampleLengthSeconds;
	bool				m_bAllowOptionsMenu, m_bAllowOptionsMenuRepeat;
	bool				m_bSelectIsDown;

	int				m_iSavedRoundIndex;

	Transition			m_bgOptionsOut;
	Transition			m_bgNoOptionsOut;

	RageSound			m_soundDifficultyEasier;
	RageSound			m_soundDifficultyHarder;
	RageSound			m_soundOptionsChange;
	RageSound			m_soundLocked;
	RageSound			m_soundSelectPressed;

	BackgroundLoader	m_BackgroundLoader;
};

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
