 /* Song - Holds all music metadata and steps for one song. */

#ifndef SONG_H
#define SONG_H

#include "TimingData.h"
#include "Difficulty.h"
#include "EnumHelper.h"
#include "RageUtil_AutoPtr.h"
#include "PlayerNumber.h" // needed for m_SongOwner

// copy custom songs into memory, so we don't need to worry about disk space
#define CUSTOM_SONG_PATH CString("/@mem/")

class Steps;
class Style;
class NotesLoader;
class LyricsLoader;
class Profile;
class StepsID;
struct lua_State;
struct BackgroundChange;

const int MAX_EDITS_PER_SONG_PER_PROFILE	= 5;
const int MAX_EDITS_PER_SONG				= 5*NUM_PROFILE_SLOTS;

extern const int FILE_CACHE_VERSION;

enum BackgroundLayer
{
	BACKGROUND_LAYER_1,
	BACKGROUND_LAYER_2,
	NUM_BackgroundLayer,
	BACKGROUND_LAYER_INVALID
};
#define FOREACH_BackgroundLayer( bl ) FOREACH_ENUM( BackgroundLayer, NUM_BackgroundLayer, bl )

struct LyricSegment
{
	float	m_fStartTime;
	CString m_sLyric;
	RageColor m_Color;
};

class Song
{
	CString m_sSongDir;

public:
	/* Set when this song should be displayed in the music wheel: */
	enum SelectionDisplay
	{ 
		SHOW_ALWAYS,	/* all the time */
		SHOW_ROULETTE,	/* only when rouletting */
		SHOW_NEVER		/* never (unless song hiding is turned off) */
	} m_SelectionDisplay;

	Song();
	~Song();
	void Reset();
	void DetachSteps();

	NotesLoader *MakeLoader( const CString &sDir ) const;

	bool LoadFromSongDir( const CString &sDir );
	bool LoadFromCustomSongDir( const CString &sDir, const CString &sGroupName, PlayerNumber pn );

	void TidyUpData();	// call after loading to clean up invalid data
	void ReCalculateRadarValuesAndLastBeat( float fSeconds = -1.0f );	// called by TidyUpData, and after saving
	void TranslateTitles();	// called by TidyUpData

	void SaveToSMFile( const CString &sPath, bool bSavingCache );
	void Save();	// saves SM and DWI
	void SaveToCacheFile();
	void SaveToDWIFile();

	const CString &GetSongFilePath() const;
	CString GetCacheFilePath() const;

	void AddAutoGenNotes();
	void AutoGen( StepsType ntTo, StepsType ntFrom );	// create Steps of type ntTo from Steps of type ntFrom
	void RemoveAutoGenNotes();

	/* Directory this song data came from: */
	const CString &GetSongDir() const { return m_sSongDir; }

	/* Filename associated with this file.  This will always have
	 * an .SM extension.  If we loaded an .SM, this will point to 
	 * it, but if we loaded any other type, this will point to a
	 * generated .SM filename. */
	CString m_sSongFileName;

	CString m_sGroupName;

	bool	m_bIsSymLink;

	CString	m_sMainTitle, m_sSubTitle, m_sArtist;
	CString m_sMainTitleTranslit, m_sSubTitleTranslit, m_sArtistTranslit;

	/* If PREFSMAN->m_bShowNative is off, these are the same as GetTranslit* below.
	 * Otherwise, they return the main titles. */
	CString GetDisplayMainTitle() const;
	CString GetDisplaySubTitle() const;
	CString GetDisplayArtist() const;

	/* Returns the transliterated titles, if any; otherwise returns the main titles. */
	CString GetTranslitMainTitle() const { return m_sMainTitleTranslit.size()? m_sMainTitleTranslit: m_sMainTitle; }
	CString GetTranslitSubTitle() const { return m_sSubTitleTranslit.size()? m_sSubTitleTranslit: m_sSubTitle; }
	CString GetTranslitArtist() const { return m_sArtistTranslit.size()? m_sArtistTranslit:m_sArtist; }

	/* "title subtitle" */
	CString GetDisplayFullTitle() const;
	CString GetTranslitFullTitle() const;

	CString m_sGenre;

	/* This is read and saved, but never actually used. */
	CString	m_sCredit;

	/* This can be different from m_fMusicLengthSeconds, depending on step length. */
	float MusicLengthSeconds() const;

	CString	m_sMusicFile;
	float	m_fMusicLengthSeconds;
	float	m_fStepsLengthSeconds;
	float	m_fFirstBeat;	// beat of first note
	float	m_fLastBeat;	// beat of last note
	float	m_fMusicSampleStartSeconds;
	float	m_fMusicSampleLengthSeconds;
	enum { DISPLAY_ACTUAL, DISPLAY_SPECIFIED, DISPLAY_RANDOM } m_DisplayBPMType;
	float		m_fSpecifiedBPMMin;
	float		m_fSpecifiedBPMMax;	// if a range, then Min != Max

	ProfileSlot	m_LoadedFromProfile;

	CString	m_sBannerFile;
	CString m_sLyricsFile;
	CString	m_sBackgroundFile;
	CString	m_sCDTitleFile;

	CString GetMusicPath() const;
	CString GetBannerPath() const;
	CString	GetLyricsPath() const;
	CString GetBackgroundPath() const;
	CString GetCDTitlePath() const;

	/* For loading only: */
	bool m_bHasMusic, m_bHasBanner;
	
	// custom songs stuff
	CString m_sGameplayMusic;
	bool m_bIsCustomSong;
	PlayerNumber m_SongOwner;
	CString m_sExtension;
	
	bool CheckCustomSong( CString &sError );

	bool HasMusic() const;
	bool HasBanner() const;
	bool HasBackground() const;
	bool HasCDTitle() const;
	bool HasMovieBackground() const;
	bool HasBGChanges() const;
	bool HasLyrics() const;

	bool IsCustomSong() const;
	bool IsLong() const;
	bool IsMarathon() const;

	bool Matches( const CString &sGroup, const CString &sSong) const;

	TimingData m_Timing;

	typedef vector<BackgroundChange> VBackgroundChange;
private:
	// AutoPtr instead of raw pointer so that the auto gen'd copy constructor works correctly.
	AutoPtrCopyOnWrite<VBackgroundChange>	m_BackgroundChanges[NUM_BackgroundLayer];	// these must be sorted before gameplay
	AutoPtrCopyOnWrite<VBackgroundChange>	m_ForegroundChanges;	// this must be sorted before gameplay
public:
	const vector<BackgroundChange>	&GetBackgroundChanges( BackgroundLayer bl ) const;
	vector<BackgroundChange>		&GetBackgroundChanges( BackgroundLayer bl );
	const vector<BackgroundChange>	&GetForegroundChanges() const;
	vector<BackgroundChange>		&GetForegroundChanges();

	vector<LyricSegment>		m_LyricSegments;		// this must be sorted before gameplay

	void AddBPMSegment( const BPMSegment &seg ) { m_Timing.AddBPMSegment( seg ); }
	void AddStopSegment( const StopSegment &seg ) { m_Timing.AddStopSegment( seg ); }
	void AddBackgroundChange( BackgroundLayer blLayer, BackgroundChange seg );
	void AddForegroundChange( BackgroundChange seg );
	void AddLyricSegment( LyricSegment seg );

	void GetDisplayBpms( DisplayBpms &AddTo ) const;
	const BackgroundChange &GetBackgroundAtBeat( BackgroundLayer iLayer, float fBeat ) const;

	float GetBPMAtBeat( float fBeat ) const { return m_Timing.GetBPMAtBeat( fBeat ); }
	void SetBPMAtBeat( float fBeat, float fBPM ) { m_Timing.SetBPMAtBeat( fBeat, fBPM ); }
	BPMSegment& GetBPMSegmentAtBeat( float fBeat ) { return m_Timing.GetBPMSegmentAtBeat( fBeat ); }
	void GetBeatAndBPSFromElapsedTime( float fElapsedTime, float &fBeatOut, float &fBPSOut, bool &bFreezeOut ) const { m_Timing.GetBeatAndBPSFromElapsedTime( fElapsedTime, fBeatOut, fBPSOut, bFreezeOut ); }
	float GetBeatFromElapsedTime( float fElapsedTime ) const { return m_Timing.GetBeatFromElapsedTime( fElapsedTime ); }
	float GetElapsedTimeFromBeat( float fBeat ) const { return m_Timing.GetElapsedTimeFromBeat( fBeat ); }
	bool HasSignificantBpmChangesOrStops() const;
	float GetStepsSeconds() const;

	bool SongCompleteForStyle( const Style *st ) const;
	bool HasStepsType( StepsType st ) const;
	bool HasStepsTypeAndDifficulty( StepsType st, Difficulty dc ) const;

	const vector<Steps*>& GetAllSteps() const { return m_vpSteps; }
	const vector<Steps*>& GetStepsByStepsType( StepsType st ) const { return m_vpStepsByType[st]; }
	void GetSteps( 
		vector<Steps*>& arrayAddTo, 
		StepsType st = STEPS_TYPE_INVALID, 
		Difficulty dc = DIFFICULTY_INVALID, 
		int iMeterLow = -1, 
		int iMeterHigh = -1, 
		const CString &sDescription = "", 
		bool bIncludeAutoGen = true, 
		unsigned uHash = 0,
		int iMaxToGet = -1 
		) const;
	Steps* GetOneSteps( 
		StepsType st = STEPS_TYPE_INVALID, 
		Difficulty dc = DIFFICULTY_INVALID, 
		int iMeterLow = -1, 
		int iMeterHigh = -1, 
		const CString &sDescription = "", 
		unsigned uHash = 0,
		bool bIncludeAutoGen = true
		) const;
	Steps* GetStepsByDifficulty( StepsType st, Difficulty dc, bool bIncludeAutoGen = true ) const;
	Steps* GetStepsByMeter( StepsType st, int iMeterLow, int iMeterHigh ) const;
	Steps* GetStepsByDescription( StepsType st, const CString &sDescription ) const;
	Steps* GetClosestNotes( StepsType st, Difficulty dc, bool bIgnoreLocked=false ) const;
	bool IsEasy( StepsType st ) const;
	bool IsTutorial() const;
	bool HasEdits( StepsType st ) const;
	SelectionDisplay GetDisplayed() const;
	bool NormallyDisplayed() const;
	bool NeverDisplayed() const;
	bool RouletteDisplayed() const;
	bool ShowInDemonstrationAndRanking() const;

	void AddSteps( Steps* pSteps );		// we are responsible for deleting the memory pointed to by pSteps!
	void RemoveSteps( const Steps* pSteps );

	void FreeAllLoadedFromProfile( ProfileSlot slot = PROFILE_SLOT_INVALID );
	bool WasLoadedFromProfile() const { return m_LoadedFromProfile != PROFILE_SLOT_INVALID; }
	int GetNumStepsLoadedFromProfile( ProfileSlot slot ) const;
	bool IsEditAlreadyLoaded( Steps* pSteps ) const;

	bool IsEditDescriptionUnique( StepsType st, const CString &sPreferredDescription, const Steps *pExclude ) const;
	void MakeUniqueEditDescription( StepsType st, CString &sPreferredDescriptionInOut ) const;

	// An array of keysound file names (e.g. "beep.wav").
	// The index in this array corresponds to the index in TapNote.  If you 
	// change the index in here, you must change all NoteData too.
	// Any note that doesn't have a value in the range of this array
	// means "this note doens't have a keysound".
	vector<CString> m_vsKeysoundFile;

	// Lua
	void PushSelf( lua_State *L );

private:
	void AdjustDuplicateSteps(); // part of TidyUpData
	void DeleteDuplicateSteps( vector<Steps*> &vSteps );

	vector<Steps*> m_vpSteps;
	vector<Steps*> m_vpStepsByType[NUM_STEPS_TYPES];
};

#endif

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
