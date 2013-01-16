/* TODO: LoadFromCustomSongDir tries to save cache data. Where? */

#include "global.h"
#include "RageLog.h"
#include "RageSoundReader_FileReader.h"
#include "RageSurface_Load.h"
#include "SongCacheIndex.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "Style.h"
#include "TitleSubstitution.h"
#include "BannerCache.h"
#include "Sprite.h"
#include "RageFileManager.h"
#include "RageSurface.h"
#include "ProfileManager.h"
#include "Foreach.h"
#include "UnlockManager.h"
#include "BackgroundUtil.h"

#include "NotesLoaderSM.h"
#include "NotesLoaderDWI.h"
#include "NotesLoaderBMS.h"
#include "NotesLoaderKSF.h"
#include "NotesWriterDWI.h"
#include "NotesWriterSM.h"


#include <set>
#include <float.h>

#define CACHE_DIR "Cache/"

const int FILE_CACHE_VERSION = 144;	// increment this to invalidate cache

const float DEFAULT_MUSIC_SAMPLE_LENGTH = 12.f;


//////////////////////////////
// Song
//////////////////////////////
Song::Song()
{
	FOREACH_BackgroundLayer( i )
		m_BackgroundChanges[i] = AutoPtrCopyOnWrite<VBackgroundChange>(new VBackgroundChange);
	m_ForegroundChanges = AutoPtrCopyOnWrite<VBackgroundChange>(new VBackgroundChange);
	

	m_fMusicSampleStartSeconds = -1;
	m_fMusicSampleLengthSeconds = DEFAULT_MUSIC_SAMPLE_LENGTH;
	m_fMusicLengthSeconds = 0;
	m_fFirstBeat = -1;
	m_fLastBeat = -1;
	m_SelectionDisplay = SHOW_ALWAYS;
	m_DisplayBPMType = DISPLAY_ACTUAL;
	m_fSpecifiedBPMMin = 0;
	m_fSpecifiedBPMMax = 0;
	m_bIsSymLink = false;
	m_bHasMusic = false;
	m_bHasBanner = false;
	m_bIsCustomSong = false;
	m_SongOwner = PLAYER_INVALID;
	m_LoadedFromProfile = PROFILE_SLOT_INVALID;
}

Song::~Song()
{
	FOREACH( Steps*, m_vpSteps, s )
		SAFE_DELETE( *s );
	m_vpSteps.clear();
	
	// It's the responsibility of the owner of this Song to make sure
	// that all pointers to this Song and its Steps are invalidated.
}

void Song::DetachSteps()
{
	m_vpSteps.clear();
	FOREACH_StepsType( st )
		m_vpStepsByType[st].clear();
}

/* Reset to an empty song. */
void Song::Reset()
{
	FOREACH( Steps*, m_vpSteps, s )
		SAFE_DELETE( *s );
	m_vpSteps.clear();
	FOREACH_StepsType( st )
		m_vpStepsByType[st].clear();

	Song empty;
	*this = empty;

	// It's the responsibility of the owner of this Song to make sure
	// that all pointers to this Song and its Steps are invalidated.
}


void Song::AddBackgroundChange( BackgroundLayer iLayer, BackgroundChange seg )
{
	if (m_bIsCustomSong) return;
	// Delete old background change at this start beat, if any.
	FOREACH( BackgroundChange, GetBackgroundChanges(iLayer), bgc )
	{
		if( bgc->m_fStartBeat == seg.m_fStartBeat )
		{
			GetBackgroundChanges(iLayer).erase( bgc );
			break;
		}
	}

	ASSERT( iLayer >= 0 && iLayer < NUM_BackgroundLayer );
	GetBackgroundChanges(iLayer).push_back( seg );
	BackgroundUtil::SortBackgroundChangesArray( GetBackgroundChanges(iLayer) );
}

void Song::AddForegroundChange( BackgroundChange seg )
{
	if (m_bIsCustomSong) return;
	GetForegroundChanges().push_back( seg );
	BackgroundUtil::SortBackgroundChangesArray( GetForegroundChanges() );
}

void Song::AddLyricSegment( LyricSegment seg )
{
	if (m_bIsCustomSong) return;
	m_LyricSegments.push_back( seg );
}

void Song::GetDisplayBpms( DisplayBpms &AddTo ) const
{
	if( m_DisplayBPMType == DISPLAY_SPECIFIED )
	{
		AddTo.Add( m_fSpecifiedBPMMin );
		AddTo.Add( m_fSpecifiedBPMMax );
	}
	else
	{
		float fMinBPM, fMaxBPM;
		m_Timing.GetActualBPM( fMinBPM, fMaxBPM );
		AddTo.Add( fMinBPM );
		AddTo.Add( fMaxBPM );
	}
}

const BackgroundChange &Song::GetBackgroundAtBeat( BackgroundLayer iLayer, float fBeat ) const
{
	for( unsigned i=0; i<GetBackgroundChanges(iLayer).size()-1; i++ )
		if( GetBackgroundChanges(iLayer)[i+1].m_fStartBeat > fBeat )
			return GetBackgroundChanges(iLayer)[i];
	return GetBackgroundChanges(iLayer)[0];
}


CString Song::GetCacheFilePath() const
{
	return SongCacheIndex::GetCacheFilePath( "Songs", m_sSongDir );
}

/* Get a path to the SM containing data for this song.  It might
 * be a cache file. */
const CString &Song::GetSongFilePath() const
{
	ASSERT ( m_sSongFileName.length() != 0 );
	return m_sSongFileName;
}

NotesLoader *Song::MakeLoader( const CString &sDir ) const
{
	NotesLoader *ret;

	/* Actually, none of these have any persistant data, so we 
	 * could optimize this, but since they don't have any data,
	 * there's no real point ... */
	ret = new SMLoader;
	if(ret->Loadable( sDir )) return ret;
	delete ret;

	ret = new DWILoader;
	if(ret->Loadable( sDir )) return ret;
	delete ret;

	ret = new BMSLoader;
	if(ret->Loadable( sDir )) return ret;
	delete ret;

	ret = new KSFLoader;
	if(ret->Loadable( sDir )) return ret;
	delete ret;

	return NULL;
}

/* Hack: This should be a parameter to TidyUpData, but I don't want to
 * pull in <set> into Song.h, which is heavily used. */
static set<istring> BlacklistedImages;

/*
 * If PREFSMAN->m_bFastLoad is true, always load from cache if possible. Don't read
 * the contents of sDir if we can avoid it.  That means we can't call HasMusic(),
 * HasBanner() or GetHashForDirectory().
 *
 * If true, check the directory hash and reload the song from scratch if it's changed.
 */
bool Song::LoadFromSongDir( const CString &sDir_ )
{
	//	LOG->Trace( "Song::LoadFromSongDir(%s)", sDir.c_str() );
	ASSERT( !sDir_.empty() );

	CString sDir = sDir_;

	// make sure there is a trailing slash at the end of sDir
	if( sDir.Right(1) != "/" )
		sDir += "/";

	// save song dir
	m_sSongDir = sDir;

	// save group name
	CStringArray sDirectoryParts;
	split( m_sSongDir, "/", sDirectoryParts, false );
	ASSERT( sDirectoryParts.size() >= 4 ); /* e.g. "/Songs/Slow/Taps/" */
	m_sGroupName = sDirectoryParts[sDirectoryParts.size()-3];	// second from last item
	ASSERT( m_sGroupName != "" );

	//
	// First look in the cache for this song (without loading NoteData)
	//
	unsigned uDirHash = SONGINDEX->GetCacheHash(m_sSongDir);
	bool bUseCache = true;
	if( !DoesFileExist(GetCacheFilePath()) )
		bUseCache = false;
	if( !PREFSMAN->m_bFastLoad && GetHashForDirectory(m_sSongDir) != uDirHash )
		bUseCache = false; // this cache is out of date 

	if( bUseCache )
	{
//		LOG->Trace( "Loading '%s' from cache file '%s'.", m_sSongDir.c_str(), GetCacheFilePath().c_str() );
		SMLoader ld;
		ld.LoadFromSMFile( GetCacheFilePath(), *this, true );
		ld.TidyUpData( *this, true );
		if ( !IsAFile(m_sSongDir + "/" + m_sMusicFile) )
		{
			LOG->Warn( "%s has a useless cache entry", m_sSongDir.c_str() );
			return false;
		}
	}
	else
	{
		//
		// There was no entry in the cache for this song, or it was out of date.
		// Let's load it from a file, then write a cache entry.
		//
		
		NotesLoader *ld = MakeLoader( sDir );
		if(!ld)
		{
			LOG->Warn( "Couldn't find any SM, DWI, BMS, or KSF files in '%s'.  This is not a valid song directory.", sDir.c_str() );
			return false;
		}

		bool success = ld->LoadFromDir( sDir, *this );

		BlacklistedImages = ld->GetBlacklistedImages();

		if(!success)
		{
			delete ld;
			return false;
		}

		TidyUpData();
		ld->TidyUpData( *this, false );

		delete ld;

		// save a cache file so we don't have to parse it all over again next time
		SaveToCacheFile();
	}
	
	FOREACH( Steps*, m_vpSteps, s )
	{
		(*s)->SetFile( GetCacheFilePath() );

		/* Compress all Steps.  During initial caching, this will remove cached NoteData;
		 * during cached loads, this will just remove cached SMData. */
		(*s)->Compress();
	}

	/* Load the cached banners, if it's not loaded already. */
	if( PREFSMAN->m_BannerCache == PrefsManager::BNCACHE_LOW_RES_PRELOAD && m_bHasBanner )
		BANNERCACHE->LoadBanner( GetBannerPath() );

	/* Add AutoGen pointers.  (These aren't cached.) */
	AddAutoGenNotes();

	/* Make sure we point to the correct place */
	m_sGameplayMusic = GetMusicPath();

	/* Get the length of the steps, so we have an accurate indicator of play length */
	m_fStepsLengthSeconds = m_Timing.GetElapsedTimeFromBeat( m_fLastBeat );

	if( !m_bHasMusic )
	{
		LOG->Trace( "Song \"%s\" ignored (no music)", sDir.c_str() );
		return false;	// don't load this song
	}

	return true;	// do load this song
}

bool Song::LoadFromCustomSongDir( const CString &sDir_, const CString &sGroupName, PlayerNumber pn )
{
	CString sDir = sDir_;

	ASSERT( !sDir.empty() );

	// Make sure there is a trailing slash at the end of sDir, then save it
	if( sDir.Right(1) != "/" )
		sDir += "/";
	m_sSongDir = sDir;

	// This is a song loaded by a player. mark it and set an owner.
	m_bIsCustomSong = true;
	m_SongOwner = pn;

	// Save group name
	m_sGroupName = sGroupName;
	ASSERT( m_sGroupName != "" );

	NotesLoader *ld = MakeLoader( sDir );

	if( !ld || !ld->LoadFromDir( sDir, *this ) )
	{
		delete ld;
		return false;
	}

	TidyUpData();
	ld->TidyUpData( *this, false );
	delete ld;

	FOREACH( Steps*, m_vpSteps, s )
	{
		(*s)->SetFile( "" ); // avoid loading from cache
		(*s)->Compress();
	}

	/* Get the length of the steps, so we have an accurate indicator of play length */
	m_fStepsLengthSeconds = m_Timing.GetElapsedTimeFromBeat( m_fLastBeat );

	// these won't load anyway -  don't bother
	m_sBannerFile = "";
	m_sBackgroundFile = "";

	m_sExtension = "." + m_sMusicFile.Right(3);
	m_sGameplayMusic = CUSTOM_SONG_PATH + "music" + m_sExtension;

	if( !m_bHasMusic )
	{
		LOG->Trace( "Song \"%s\" ignored (no music)", sDir.c_str() );
		return false;	// don't load this song
	}

	return true;
}

static void GetImageDirListing( const CString &sPath, CStringArray &AddTo, bool bReturnPathToo=false )
{
	GetDirListing( sPath + ".png", AddTo, false, bReturnPathToo ); 
	GetDirListing( sPath + ".jpg", AddTo, false, bReturnPathToo ); 
	GetDirListing( sPath + ".bmp", AddTo, false, bReturnPathToo ); 
	GetDirListing( sPath + ".gif", AddTo, false, bReturnPathToo ); 
}

static CString RemoveInitialWhitespace( CString s )
{
	size_t i = s.find_first_not_of(" \t\r\n");
	if( i != s.npos )
		s.erase( 0, i );
	return s;
}

/* This is called within TidyUpData, before autogen notes are added. */
void Song::DeleteDuplicateSteps( vector<Steps*> &vSteps )
{
	/* vSteps have the same StepsType and Difficulty.  Delete them if they have the
	 * same m_sDescription, m_iMeter and SMNoteData. */
	for( unsigned i=0; i<vSteps.size(); i++ )
	{
		const Steps *s1 = vSteps[i];

		for( unsigned j=i+1; j<vSteps.size(); j++ )
		{
			const Steps *s2 = vSteps[j];

			if( s1->GetDescription() != s2->GetDescription() )
				continue;
			if( s1->GetMeter() != s2->GetMeter() )
				continue;
			/* Compare, ignoring whitespace. */
			CString sSMNoteData1;
			s1->GetSMNoteData( sSMNoteData1 );
			CString sSMNoteData2;
			s2->GetSMNoteData( sSMNoteData2 );
			if( RemoveInitialWhitespace(sSMNoteData1) != RemoveInitialWhitespace(sSMNoteData2) )
				continue;

			LOG->Trace("Removed %p duplicate steps in song \"%s\" with description \"%s\" and meter \"%i\"",
				s2, this->GetSongDir().c_str(), s1->GetDescription().c_str(), s1->GetMeter() );
				
			/* Don't use RemoveSteps; autogen notes havn't yet been created and it'll
			 * create them. */
			FOREACH_StepsType( st )
			{
				for( int k=this->m_vpStepsByType[st].size()-1; k>=0; k-- )
				{
					if( this->m_vpStepsByType[st][k] == s2 )
					{
						// delete this->m_vpStepsByType[k]; // delete below
						this->m_vpStepsByType[st].erase( this->m_vpStepsByType[st].begin()+k );
						break;
					}
				}
			}

			for( int k=this->m_vpSteps.size()-1; k>=0; k-- )
			{
				if( this->m_vpSteps[k] == s2 )
				{
					delete this->m_vpSteps[k];
					this->m_vpSteps.erase( this->m_vpSteps.begin()+k );
					break;
				}
			}

			vSteps.erase(vSteps.begin()+j);
			--j;
		}
	}
}

/* Make any duplicate difficulties edits.  (Note that BMS files do a first pass
 * on this; see BMSLoader::SlideDuplicateDifficulties.) */
void Song::AdjustDuplicateSteps()
{
	FOREACH_StepsType( st )
	{
		FOREACH_Difficulty( dc )
		{
			if( dc == DIFFICULTY_EDIT )
				continue;

			vector<Steps*> vSteps;
			this->GetSteps( vSteps, st, dc );

			/* Delete steps that are completely identical.  This happened due to a
			 * bug in an earlier version. */
			DeleteDuplicateSteps( vSteps );

			StepsUtil::SortNotesArrayByDifficulty( vSteps );
			for( unsigned k=1; k<vSteps.size(); k++ )
			{
				vSteps[k]->SetDifficulty( DIFFICULTY_EDIT );
				if( vSteps[k]->GetDescription() == "" )
				{
					/* "Hard Edit" */
					CString EditName = Capitalize( DifficultyToString(dc) ) + " Edit";
					vSteps[k]->SetDescription( EditName );
				}
			}
		}

		// delete duplicate edits, particularly ones that arise from Edit Mode's
		// Revert To Disk bug
		vector<Steps*> vSteps;
		this->GetSteps( vSteps, st, DIFFICULTY_EDIT );
		vector<Steps*> vProcessedSteps;
		for( int i = vSteps.size()-1; i >= 0; i-- )
		{
			if ( vSteps[i] == NULL )
				continue;
			if ( vSteps[i]->GetDescription() )
				LOG->Debug( "Edit %s, %08x", vSteps[i]->GetDescription().c_str(), vSteps[i]->GetHash() );
			bool bProcess = true;
			for( unsigned j = 0; j < vProcessedSteps.size(); j++ )
			{
				if ( vSteps[i]->GetHash() == vProcessedSteps[j]->GetHash() )
				{
					// found a duplicate edit, delete it
					for( int k=this->m_vpSteps.size()-1; k>=0; k-- )
					{
						if( this->m_vpSteps[k] == vSteps[i] )
						{
							delete this->m_vpSteps[k];
							this->m_vpSteps.erase( this->m_vpSteps.begin()+k );
							bProcess = false;
							break;
						}
					}
				}
			}
			if ( bProcess )
				vProcessedSteps.push_back(vSteps[i]);
		}

		/* XXX: Don't allow edits to have descriptions that look like regular difficulties.
		 * These are confusing, and they're ambiguous when passed to GetStepsByID. */
	}
}

/* Fix up song paths.  If there's a leading "./", be sure to keep it: it's
 * a signal that the path is from the root directory, not the song directory.
 * Other than a leading "./", song paths must never contain "." or "..". */
void FixupPath( CString &path, const CString &sSongPath )
{
	/* Replace backslashes with slashes in all paths. */	
	FixSlashesInPlace( path );

	/* Many imported files contain erroneous whitespace before or after
	 * filenames.  Paths usually don't actually start or end with spaces,
	 * so let's just remove it. */
	TrimLeft( path );
	TrimRight( path );
}

/* Songs in BlacklistImages will never be autodetected as song images. */
void Song::TidyUpData()
{
	/* We need to do this before calling any of HasMusic, HasHasCDTitle, etc. */
	ASSERT_M( m_sSongDir.Left(3) != "../", m_sSongDir ); /* meaningless */
	FixupPath( m_sSongDir, "" );
	FixupPath( m_sMusicFile, m_sSongDir );
	FixupPath( m_sBannerFile, m_sSongDir );
	FixupPath( m_sLyricsFile, m_sSongDir );
	FixupPath( m_sBackgroundFile, m_sSongDir );
	FixupPath( m_sCDTitleFile, m_sSongDir );

	if( !HasMusic() )
	{
		CStringArray arrayPossibleMusic;
		GetDirListing( m_sSongDir + CString("*.mp3"), arrayPossibleMusic );
		GetDirListing( m_sSongDir + CString("*.ogg"), arrayPossibleMusic );
		GetDirListing( m_sSongDir + CString("*.wav"), arrayPossibleMusic );

		if( !arrayPossibleMusic.empty() )
		{
			int idx = 0;
			/* If the first song is "intro", and we have more than one available,
			 * don't use it--it's probably a KSF intro music file, which we don't support. */
			if( arrayPossibleMusic.size() > 1 &&
				!arrayPossibleMusic[0].Left(5).CompareNoCase("intro") )
				++idx;

			// we found a match
			m_sMusicFile = arrayPossibleMusic[idx];
		}
	}

	/* This must be done before radar calculation. */
	if( HasMusic() )
	{
		CString error;
		SoundReader *Sample = SoundReader_FileReader::OpenFile( GetMusicPath(), error );
		/* XXX: Checking if the music file exists eliminates a warning originating from BMS files
		 * (which have no music file, per se) but it's something of a hack. */
		if( Sample == NULL && m_sMusicFile != "" )
		{
			LOG->Warn( "Error opening sound \"%s\": %s", GetMusicPath().c_str(), error.c_str() );

			/* Don't use this file. */
			m_sMusicFile = "";
		}
		else if ( Sample != NULL )
		{
			m_fMusicLengthSeconds = Sample->GetLength() / 1000.0f;
			delete Sample;

			if( m_fMusicLengthSeconds < 0 )
			{
				/* It failed; bad file or something.  It's already logged a warning. */
				m_fMusicLengthSeconds = 100; // guess
			}
			else if( m_fMusicLengthSeconds == 0 )
			{
				LOG->Warn( "File %s is empty?", GetMusicPath().c_str() );
			}
		}
	}
	else	// ! HasMusic()
	{
		m_fMusicLengthSeconds = 100;		// guess
		LOG->Warn("Song \"%s\" has no music file; guessing at %f seconds", this->GetSongDir().c_str(), m_fMusicLengthSeconds);
	}

	if(m_fMusicLengthSeconds < 0)
	{
		LOG->Warn( "File %s has negative length? (%f)", GetMusicPath().c_str(), m_fMusicLengthSeconds );
		m_fMusicLengthSeconds = 0;
	}

	/* Generate these before we autogen notes, so the new notes can inherit
	 * their source's values. */
	ReCalculateRadarValuesAndLastBeat( m_fStepsLengthSeconds );

	TrimLeft( m_sMainTitle );
	TrimRight( m_sMainTitle );
	TrimLeft( m_sSubTitle );
	TrimRight( m_sSubTitle );
	TrimLeft( m_sArtist );
	TrimRight( m_sArtist );

	/* Fall back on the song directory name. */
	if( m_sMainTitle == "" )
		NotesLoader::GetMainAndSubTitlesFromFullTitle( Basename(this->GetSongDir()), m_sMainTitle, m_sSubTitle );

	if( m_sArtist == "" )
		m_sArtist = "Unknown artist";
	TranslateTitles();

	if( m_Timing.m_BPMSegments.empty() )
	{
		/* XXX: Once we have a way to display warnings that the user actually
		 * cares about (unlike most warnings), this should be one of them. */
		LOG->Warn( "No BPM segments specified in '%s%s', default provided.",
			m_sSongDir.c_str(), m_sSongFileName.c_str() );

		m_Timing.AddBPMSegment( BPMSegment(0, 60) );
	}
	
	/* Make sure the first BPM segment starts at beat 0. */
	if( m_Timing.m_BPMSegments[0].m_iStartIndex != 0 )
		m_Timing.m_BPMSegments[0].m_iStartIndex = 0;


	if( m_fMusicSampleStartSeconds == -1 ||
		m_fMusicSampleStartSeconds == 0 ||
		m_fMusicSampleStartSeconds+m_fMusicSampleLengthSeconds > this->m_fMusicLengthSeconds )
	{
		m_fMusicSampleStartSeconds = this->GetElapsedTimeFromBeat( 100 );

		if( m_fMusicSampleStartSeconds+m_fMusicSampleLengthSeconds > this->m_fMusicLengthSeconds )
		{
			int iBeat = lrintf( m_fLastBeat/2 );
			iBeat -= iBeat%4;
			m_fMusicSampleStartSeconds = this->GetElapsedTimeFromBeat( (float)iBeat );
		}
	}
	

	/* Some DWIs have lengths in ms when they meant seconds, eg. #SAMPLELENGTH:10;.
	 * If the sample length is way too short, change it. */
	if( m_fMusicSampleLengthSeconds < 3 || m_fMusicSampleLengthSeconds > 30 )
		m_fMusicSampleLengthSeconds = DEFAULT_MUSIC_SAMPLE_LENGTH;

	//
	// Here's the problem:  We have a directory full of images.  We want to determine which 
	// image is the banner, which is the background, and which is the CDTitle.
	//

	//
	// First, check the file name for hints.
	//
	if( !HasBanner() )
	{
		/* If a nonexistant banner file is specified, and we can't find a replacement,
		 * don't wipe out the old value. */
//		m_sBannerFile = "";

		// find an image with "banner" in the file name
		CStringArray arrayPossibleBanners;
		GetImageDirListing( m_sSongDir + "*banner*", arrayPossibleBanners );

		/* Some people do things differently for the sake of being different.  Don't
		 * match eg. abnormal, numbness. */
		GetImageDirListing( m_sSongDir + "* BN", arrayPossibleBanners );

		if( !arrayPossibleBanners.empty() )
			m_sBannerFile = arrayPossibleBanners[0];
	}

	if( !HasBackground() )
	{
//		m_sBackgroundFile = "";

		// find an image with "bg" or "background" in the file name
		CStringArray arrayPossibleBGs;
		GetImageDirListing( m_sSongDir + "*bg*", arrayPossibleBGs );
		GetImageDirListing( m_sSongDir + "*background*", arrayPossibleBGs );
		if( !arrayPossibleBGs.empty() )
			m_sBackgroundFile = arrayPossibleBGs[0];
	}

	if( !HasCDTitle() )
	{
		// find an image with "cdtitle" in the file name
		CStringArray arrayPossibleCDTitles;
		GetImageDirListing( m_sSongDir + "*cdtitle*", arrayPossibleCDTitles );
		if( !arrayPossibleCDTitles.empty() )
			m_sCDTitleFile = arrayPossibleCDTitles[0];
	}

	if( !HasLyrics() )
	{
		// Check if there is a lyric file in here
		CStringArray arrayLyricFiles;
		GetDirListing(m_sSongDir + CString("*.lrc"), arrayLyricFiles );
		if(	!arrayLyricFiles.empty() )
			m_sLyricsFile = arrayLyricFiles[0];
	}

	//
	// Now, For the images we still haven't found, look at the image dimensions of the remaining unclassified images.
	//
	CStringArray arrayImages;
	GetImageDirListing( m_sSongDir + "*", arrayImages );

	for( unsigned i=0; i<arrayImages.size(); i++ )	// foreach image
	{
		if( HasBanner() && HasCDTitle() && HasBackground() )
			break; /* done */

		// ignore DWI "-char" graphics
		if( BlacklistedImages.find( arrayImages[i].c_str() ) != BlacklistedImages.end() )
			continue;	// skip
		
		// Skip any image that we've already classified

		if( HasBanner()  &&  stricmp(m_sBannerFile, arrayImages[i])==0 )
			continue;	// skip

		if( HasBackground()  &&  stricmp(m_sBackgroundFile, arrayImages[i])==0 )
			continue;	// skip

		if( HasCDTitle()  &&  stricmp(m_sCDTitleFile, arrayImages[i])==0 )
			continue;	// skip

		CString sPath = m_sSongDir + arrayImages[i];

		/* We only care about the dimensions. */
		CString error;
		RageSurface *img = RageSurfaceUtils::LoadFile( sPath, error, true );
		if( !img )
		{
			LOG->Trace( "Couldn't load '%s': %s", sPath.c_str(), error.c_str() );
			continue;
		}

		const int width = img->w;
		const int height = img->h;
		delete img;

		if( !HasBackground()  &&  width >= 320  &&  height >= 240 )
		{
			m_sBackgroundFile = arrayImages[i];
			continue;
		}

		if( !HasBanner() && Sprite::IsDiagonalBanner(width, height) )
		{
			m_sBannerFile = arrayImages[i];
			continue;
		}

		if( !HasBanner()  &&  100<=width  &&  width<=320  &&  50<=height  &&  height<=240 )
		{
			m_sBannerFile = arrayImages[i];
			continue;
		}

		/* Some songs have overlarge banners.  Check if the ratio is reasonable (over
		 * 2:1; usually over 3:1), and large (not a cdtitle). */
		if( !HasBanner() && width > 200 && float(width) / height > 2.0f )
		{
			m_sBannerFile = arrayImages[i];
			continue;
		}

		/* Agh.  DWI's inline title images are triggering this, resulting in kanji,
		 * etc., being used as a CDTitle for songs with none.  Some sample data
		 * from random incarnations:
		 *   42x50 35x50 50x50 144x49
		 * It looks like ~50 height is what people use to align to DWI's font.
		 *
		 * My tallest CDTitle is 44.  Let's cut off in the middle and hope for
		 * the best. */
		if( !HasCDTitle()  &&  width<=100  &&  height<=48 )
		{
			m_sCDTitleFile = arrayImages[i];
			continue;
		}
	}

	/* These will be written to cache, for Song::LoadFromSongDir to use later. */
	m_bHasMusic = HasMusic();
	m_bHasBanner = HasBanner();

	if( HasBanner() )
		BANNERCACHE->CacheBanner( GetBannerPath() );

	// If no BGChanges are specified and there are movies in the song directory, then assume
	// they are DWI style where the movie begins at beat 0.
	if( !HasBGChanges() )
	{
		CStringArray arrayPossibleMovies;
		GetDirListing( m_sSongDir + CString("*movie*.avi"), arrayPossibleMovies );
		GetDirListing( m_sSongDir + CString("*movie*.mpg"), arrayPossibleMovies );
		GetDirListing( m_sSongDir + CString("*movie*.mpeg"), arrayPossibleMovies );
		GetDirListing( m_sSongDir + CString("*.avi"), arrayPossibleMovies );
		GetDirListing( m_sSongDir + CString("*.mpg"), arrayPossibleMovies );
		GetDirListing( m_sSongDir + CString("*.mpeg"), arrayPossibleMovies );
		/* Use this->GetBeatFromElapsedTime(0) instead of 0 to start when the
		 * music starts. */
		if( arrayPossibleMovies.size() == 1 )
			this->AddBackgroundChange( BACKGROUND_LAYER_1, BackgroundChange(0,arrayPossibleMovies[0],"",1.f,SBE_StretchNoLoop) );
	}

	/* Don't allow multiple Steps of the same StepsType and Difficulty (except for edits).
	 * We should be able to use difficulty names as unique identifiers for steps. */
	AdjustDuplicateSteps();

	{
		/* Generated filename; this doesn't always point to a loadable file,
		 * but instead points to the file we should write changed files to,
		 * and will always be an .SM.
		 *
		 * This is a little tricky.  We can't always use the song title directly,
		 * since it might contain characters we can't store in filenames.  Two
		 * easy options: we could manually filter out invalid characters, or we
		 * could use the name of the directory, which is always a valid filename
		 * and should always be the same as the song.  The former might not catch
		 * everything--filename restrictions are platform-specific; we might even
		 * be on an 8.3 filesystem, so let's do the latter.
		 *
		 * We can't rely on searching for other data filenames; it works for DWIs,
		 * but not KSFs and BMSs.
		 *
		 * So, let's do this (by priority):
		 * 1. If there's an .SM file, use that filename.  No reason to use anything
		 *    else; it's the filename in use.
		 * 2. If there's a .DWI, use it with a changed extension.
		 * 3. Otherwise, use the name of the directory, since it's definitely a valid
		 *    filename, and should always be the title of the song (unlike KSFs).
		 */
		m_sSongFileName = m_sSongDir;
		CStringArray asFileNames;
		GetDirListing( m_sSongDir+"*.sm", asFileNames );
		if( !asFileNames.empty() )
			m_sSongFileName += asFileNames[0];
		else {
			GetDirListing( m_sSongDir+"*.dwi", asFileNames );
			if( !asFileNames.empty() ) {
				m_sSongFileName += SetExtension( asFileNames[0], "sm" );
			} else {
				m_sSongFileName += Basename(m_sSongDir);
				m_sSongFileName += ".sm";
			}
		}
	}
}

void Song::TranslateTitles()
{
	static TitleSubst tsub("songs");

	TitleFields title;
	title.LoadFromStrings( m_sMainTitle, m_sSubTitle, m_sArtist, m_sMainTitleTranslit, m_sSubTitleTranslit, m_sArtistTranslit );
	tsub.Subst( title );
	title.SaveToStrings( m_sMainTitle, m_sSubTitle, m_sArtist, m_sMainTitleTranslit, m_sSubTitleTranslit, m_sArtistTranslit );
}

void Song::ReCalculateRadarValuesAndLastBeat( float fSeconds )
{
	float fFirstBeat = FLT_MAX; /* inf */
	float fLastBeat = 0;

	// fix for rare case ogglengthpatch wankery
	if ( fSeconds <= 1.0f )
		fSeconds = m_fStepsLengthSeconds;
	if ( fSeconds <= 1.0f )
		fSeconds = m_fMusicLengthSeconds;

	for( unsigned i=0; i<m_vpSteps.size(); i++ )
	{
		Steps* pSteps = m_vpSteps[i];

		pSteps->CalculateRadarValues( fSeconds );

		//
		// calculate lastBeat
		//

		/* If it's autogen, then first/last beat will come from the parent. */
		if( pSteps->IsAutogen() )
			continue;

		/* Don't calculate based off edits from the machine profile. Use them
		 * only if they were in the original .SM file to begin with. */
		if( pSteps->IsAPlayerEdit() )
			continue;
		
		// Don't set first/last beat based on lights.  They often start very 
		// early and end very late.
		if( pSteps->m_StepsType == STEPS_TYPE_LIGHTS_CABINET )
			continue;

		NoteData tempNoteData;
		pSteps->GetNoteData( tempNoteData );

		/* Many songs have stray, empty song patterns.  Ignore them, so
		 * they don't force the first beat of the whole song to 0. */
		if( tempNoteData.GetLastRow() == 0 )
			continue;

		fFirstBeat = min( fFirstBeat, tempNoteData.GetFirstBeat() );
		fLastBeat = max( fLastBeat, tempNoteData.GetLastBeat() );
	}

	m_fFirstBeat = fFirstBeat;
	m_fLastBeat = fLastBeat;
}

void Song::GetSteps( 
	vector<Steps*>& arrayAddTo, 
	StepsType st, 
	Difficulty dc, 
	int iMeterLow, 
	int iMeterHigh, 
	const CString &sDescription, 
	bool bIncludeAutoGen, 
	unsigned uHash,
	int iMaxToGet 
	) const
{
	if( !iMaxToGet )
		return;

	const vector<Steps*> &vpSteps = st == STEPS_TYPE_INVALID ? GetAllSteps() : GetStepsByStepsType(st);
	for( unsigned i=0; i<vpSteps.size(); i++ )	// for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if( dc != DIFFICULTY_INVALID && dc != pSteps->GetDifficulty() )
			continue;
		if( iMeterLow != -1 && iMeterLow > pSteps->GetMeter() )
			continue;
		if( iMeterHigh != -1 && iMeterHigh < pSteps->GetMeter() )
			continue;
		if( sDescription.size() && sDescription != pSteps->GetDescription() )
			continue;
		if( uHash != 0 && uHash != pSteps->GetHash() )
			continue;
		if( !bIncludeAutoGen && pSteps->IsAutogen() )
			continue;

		arrayAddTo.push_back( pSteps );

		if( iMaxToGet != -1 )
		{
			--iMaxToGet;
			if( !iMaxToGet )
				break;
		}
	}
}

Steps* Song::GetOneSteps( 
	StepsType st, 
	Difficulty dc, 
	int iMeterLow, 
	int iMeterHigh, 
	const CString &sDescription, 
	unsigned uHash,
	bool bIncludeAutoGen
	) const
{
	vector<Steps*> vpSteps;
	GetSteps( vpSteps, st, dc, iMeterLow, iMeterHigh, sDescription, bIncludeAutoGen, uHash, 1 );	// get max 1
	if( vpSteps.empty() )
		return NULL;
	else
		return vpSteps[0];
}

Steps* Song::GetStepsByDifficulty( StepsType st, Difficulty dc, bool bIncludeAutoGen ) const
{
	const vector<Steps*>& vpSteps = (st == STEPS_TYPE_INVALID)? GetAllSteps():GetStepsByStepsType(st);
	for( unsigned i=0; i<vpSteps.size(); i++ )	// for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if( dc != DIFFICULTY_INVALID && dc != pSteps->GetDifficulty() )
			continue;
		if( !bIncludeAutoGen && pSteps->IsAutogen() )
			continue;

		return pSteps;
	}

	return NULL;
}

Steps* Song::GetStepsByMeter( StepsType st, int iMeterLow, int iMeterHigh ) const
{
	const vector<Steps*>& vpSteps = (st == STEPS_TYPE_INVALID)? GetAllSteps():GetStepsByStepsType(st);
	for( unsigned i=0; i<vpSteps.size(); i++ )	// for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if( iMeterLow > pSteps->GetMeter() )
			continue;
		if( iMeterHigh < pSteps->GetMeter() )
			continue;

		return pSteps;
	}

	return NULL;
}

Steps* Song::GetStepsByDescription( StepsType st, const CString &sDescription ) const
{
	vector<Steps*> vNotes;
	GetSteps( vNotes, st, DIFFICULTY_INVALID, -1, -1, sDescription );
	if( vNotes.size() == 0 )
		return NULL;
	else 
		return vNotes[0];
}


Steps* Song::GetClosestNotes( StepsType st, Difficulty dc, bool bIgnoreLocked ) const
{
	ASSERT( dc != DIFFICULTY_INVALID );

	const vector<Steps*>& vpSteps = (st == STEPS_TYPE_INVALID)? GetAllSteps():GetStepsByStepsType(st);
	Steps *pClosest = NULL;
	int iClosestDistance = 999;
	for( unsigned i=0; i<vpSteps.size(); i++ )	// for each of the Song's Steps
	{
		Steps* pSteps = vpSteps[i];

		if( pSteps->GetDifficulty() == DIFFICULTY_EDIT && dc != DIFFICULTY_EDIT )
			continue;
		if( bIgnoreLocked && UNLOCKMAN->StepsIsLocked(this, pSteps) )
			continue;

		int iDistance = abs(dc - pSteps->GetDifficulty());
		if( iDistance < iClosestDistance )
		{
			pClosest = pSteps;
			iClosestDistance = iDistance;
		}
	}

	return pClosest;
}

/* Return whether the song is playable in the given style. */
bool Song::SongCompleteForStyle( const Style *st ) const
{
	return HasStepsType( st->m_StepsType );
}

bool Song::HasStepsType( StepsType st ) const
{
	return GetOneSteps( st ) != NULL;
}

bool Song::HasStepsTypeAndDifficulty( StepsType st, Difficulty dc ) const
{
	return GetOneSteps( st, dc ) != NULL;
}

void Song::Save()
{
	LOG->Trace( "Song::SaveToSongFile()" );

	ReCalculateRadarValuesAndLastBeat();
	TranslateTitles();

	/* Save the new files.  These calls make backups on their own. */
	SaveToSMFile( GetSongFilePath(), false );
	SaveToDWIFile();
	SaveToCacheFile();

	/* We've safely written our files and created backups.  Rename non-SM and non-DWI
	 * files to avoid confusion. */
	CStringArray arrayOldFileNames;
	GetDirListing( m_sSongDir + "*.bms", arrayOldFileNames );
	GetDirListing( m_sSongDir + "*.ksf", arrayOldFileNames );
	
	for( unsigned i=0; i<arrayOldFileNames.size(); i++ )
	{
		const CString sOldPath = m_sSongDir + arrayOldFileNames[i];
		const CString sNewPath = sOldPath + ".old";

		if( !FileCopy( sOldPath, sNewPath ) )
		{
			LOG->Warn( "Backup of \"%s\" failed", sOldPath.c_str() );
			/* Don't remove. */
		} else
			FILEMAN->Remove( sOldPath );
	}
}


void Song::SaveToSMFile( const CString &sPath, bool bSavingCache )
{
	LOG->Trace( "Song::SaveToSMFile('%s')", sPath.c_str() );

	/* If the file exists, make a backup. */
	if( !bSavingCache && IsAFile(sPath) )
		FileCopy( sPath, sPath + ".old" );

	NotesWriterSM wr;
	wr.Write(sPath, *this, bSavingCache);
}

void Song::SaveToCacheFile()
{
	SONGINDEX->AddCacheIndex(m_sSongDir, GetHashForDirectory(m_sSongDir));
	SaveToSMFile( GetCacheFilePath(), true );
}

void Song::SaveToDWIFile()
{
	const CString sPath = SetExtension( GetSongFilePath(), "dwi" );
	LOG->Trace( "Song::SaveToDWIFile(%s)", sPath.c_str() );

	/* If the file exists, make a backup. */
	if( IsAFile(sPath) )
		FileCopy( sPath, sPath + ".old" );

	NotesWriterDWI wr;
	wr.Write(sPath, *this);
}


void Song::AddAutoGenNotes()
{
	bool HasNotes[NUM_STEPS_TYPES];
	memset( HasNotes, 0, sizeof(HasNotes) );
	for( unsigned i=0; i < m_vpSteps.size(); i++ ) // foreach Steps
	{
		if( m_vpSteps[i]->IsAutogen() )
			continue;

		StepsType st = m_vpSteps[i]->m_StepsType;
		HasNotes[st] = true;
	}
		
	FOREACH_StepsType( stMissing )
	{
		if( HasNotes[stMissing] )
			continue;

		/* If m_bAutogenSteps is disabled, only autogen lights. */
		if( !PREFSMAN->m_bAutogenSteps && stMissing != STEPS_TYPE_LIGHTS_CABINET )
			continue;
		/* XXX: disable lights autogen for now */
		if( stMissing  == STEPS_TYPE_LIGHTS_CABINET )
			continue;

		// missing Steps of this type
		int iNumTracksOfMissing = GAMEMAN->StepsTypeToNumTracks(stMissing);

		// look for closest match
		StepsType	stBestMatch = (StepsType)-1;
		int			iBestTrackDifference = INT_MAX;

		FOREACH_StepsType( st )
		{
			if( !HasNotes[st] )
				continue;

			/* has (non-autogen) Steps of this type */
			const int iNumTracks = GAMEMAN->StepsTypeToNumTracks(st);
			const int iTrackDifference = abs(iNumTracks-iNumTracksOfMissing);
			if( iTrackDifference < iBestTrackDifference )
			{
				stBestMatch = st;
				iBestTrackDifference = iTrackDifference;
			}
		}

		if( stBestMatch != -1 )
			AutoGen( stMissing, stBestMatch );
	}
}

void Song::AutoGen( StepsType ntTo, StepsType ntFrom )
{
//	int iNumTracksOfTo = GAMEMAN->StepsTypeToNumTracks(ntTo);

	for( unsigned int j=0; j<m_vpSteps.size(); j++ )
	{
		const Steps* pOriginalNotes = m_vpSteps[j];
		if( pOriginalNotes->m_StepsType == ntFrom )
		{
			Steps* pNewNotes = new Steps;
			pNewNotes->AutogenFrom(pOriginalNotes, ntTo);
			this->AddSteps( pNewNotes );
		}
	}
}

void Song::RemoveAutoGenNotes()
{
	FOREACH_StepsType(st)
	{
		for( int j=m_vpStepsByType[st].size()-1; j>=0; j-- )
		{
			if( m_vpStepsByType[st][j]->IsAutogen() )
			{
//				delete m_vpSteps[j]; // delete below
				m_vpStepsByType[st].erase( m_vpStepsByType[st].begin()+j );
			}
		}
	}

	for( int j=m_vpSteps.size()-1; j>=0; j-- )
	{
		if( m_vpSteps[j]->IsAutogen() )
		{
			delete m_vpSteps[j];
			m_vpSteps.erase( m_vpSteps.begin()+j );
		}
	}
}

bool Song::IsEasy( StepsType st ) const
{
	/* Very fast songs and songs with wide tempo changes are hard for new players,
	 * even if they have beginner steps. */
	DisplayBpms bpms;
	this->GetDisplayBpms(bpms);
	if( bpms.GetMax() >= 250 || bpms.GetMax() - bpms.GetMin() >= 75 )
		return false;

	/* The easy marker indicates which songs a beginner, having selected "beginner",
	 * can play and actually get a very easy song: if there are actual beginner
	 * steps, or if the light steps are 1- or 2-foot. */
	const Steps* pBeginnerNotes = GetStepsByDifficulty( st, DIFFICULTY_BEGINNER );
	if( pBeginnerNotes )
		return true;
	
	const Steps* pEasyNotes = GetStepsByDifficulty( st, DIFFICULTY_EASY );
	if( pEasyNotes && pEasyNotes->GetMeter() == 1 )
		return true;

	return false;
}

bool Song::IsTutorial() const
{
	// A Song is a tutorial song is it has only Beginner steps.
	FOREACH_CONST( Steps*, m_vpSteps, s )
	{
		if( (*s)->m_StepsType == STEPS_TYPE_LIGHTS_CABINET )
			continue;	// ignore
		if( (*s)->GetDifficulty() != DIFFICULTY_BEGINNER )
			return false;
	}

	return true;
}

bool Song::HasEdits( StepsType st ) const
{
	for( unsigned i=0; i<m_vpSteps.size(); i++ )
	{
		Steps* pSteps = m_vpSteps[i];
		if( pSteps->m_StepsType == st &&
			pSteps->GetDifficulty() == DIFFICULTY_EDIT )
		{
			return true;
		}
	}
	
	return false;
}

Song::SelectionDisplay Song::GetDisplayed() const
{
	if( !PREFSMAN->m_bHiddenSongs )
		return SHOW_ALWAYS;
	return m_SelectionDisplay;
}
bool Song::NormallyDisplayed() const { return GetDisplayed() == SHOW_ALWAYS; }
bool Song::NeverDisplayed() const { return GetDisplayed() == SHOW_NEVER; }
bool Song::RouletteDisplayed() const { if(IsTutorial()) return false; return GetDisplayed() != SHOW_NEVER; }
bool Song::ShowInDemonstrationAndRanking() const { return !IsTutorial() && NormallyDisplayed(); }


/* Hack: see Song::TidyUpData comments. */
bool Song::HasMusic() const
{
	/* If we have keys, we always have music. */
	if( m_vsKeysoundFile.size() != 0 )
		return true;

	return m_sMusicFile != "" && IsAFile(GetMusicPath());
}
bool Song::HasBanner() const 		{return m_sBannerFile != ""			&&  IsAFile(GetBannerPath()); }
bool Song::HasLyrics() const		{return m_sLyricsFile != ""			&&	IsAFile(GetLyricsPath()); }
bool Song::HasBackground() const 	{return m_sBackgroundFile != ""		&&  IsAFile(GetBackgroundPath()); }
bool Song::HasCDTitle() const 		{return m_sCDTitleFile != ""		&&  IsAFile(GetCDTitlePath()); }
bool Song::IsCustomSong() const		{ return m_bIsCustomSong; }

bool Song::IsLong() const
{
	return !IsMarathon() && m_fStepsLengthSeconds > PREFSMAN->m_fLongVerSongSeconds;
}

bool Song::IsMarathon() const
{
	return m_fStepsLengthSeconds >= PREFSMAN->m_fMarathonVerSongSeconds;
}

float Song::MusicLengthSeconds() const
{
	return m_fMusicLengthSeconds;
}

bool Song::HasBGChanges() const
{
	FOREACH_BackgroundLayer( i )
	{
		if( !GetBackgroundChanges(i).empty() )
			return true;
	}
	return false;
}


const vector<BackgroundChange> &Song::GetBackgroundChanges( BackgroundLayer bl ) const
{
	return *(m_BackgroundChanges[bl]);
}
vector<BackgroundChange> &Song::GetBackgroundChanges( BackgroundLayer bl )
{
	return *(m_BackgroundChanges[bl].Get());
}

const vector<BackgroundChange> &Song::GetForegroundChanges() const
{
	return *m_ForegroundChanges;
}
vector<BackgroundChange> &Song::GetForegroundChanges()
{
	return *m_ForegroundChanges.Get();
}


CString GetSongAssetPath( const CString &sPath_, const CString &sSongPath )
{
	if( sPath_.empty() )
		return "";

	/* If there's no path in the file, the file is in the same directory
	 * as the song.  (This is the preferred configuration.) */
	if( sPath_.find('/') == CString::npos )
		return sSongPath+sPath_;

	CString sPath = sPath_;
	CollapsePath( sPath );

	/* The song contains a path; treat it as relative to the top SM directory. */
	if( sPath.Left(3) == "../" )
	{
		/* The path begins with "../".  Resolve it wrt. the song directory. */
		sPath = sSongPath + sPath;
	}

	/* If the path still begins with "../", then there were an unreasonable number
	 * of them at the beginning of the path.  Ignore the path entirely. */
	if( sPath.Left(3) == "../" )
		return "";

	return sPath;
}


/* Note that supplying a path relative to the top-level directory is only for compatibility
 * with DWI.  We prefer paths relative to the song directory. */
CString Song::GetMusicPath() const
{
	return GetSongAssetPath( m_sMusicFile, m_sSongDir );
}

CString Song::GetBannerPath() const
{
	return GetSongAssetPath( m_sBannerFile, m_sSongDir );
}

CString Song::GetLyricsPath() const
{
	return GetSongAssetPath( m_sLyricsFile, m_sSongDir );
}

CString Song::GetCDTitlePath() const
{
	return GetSongAssetPath( m_sCDTitleFile, m_sSongDir );
}

CString Song::GetBackgroundPath() const
{
	return GetSongAssetPath( m_sBackgroundFile, m_sSongDir );
}

CString Song::GetDisplayMainTitle() const
{
	if(!PREFSMAN->m_bShowNativeLanguage) return GetTranslitMainTitle();
	return m_sMainTitle;
}

CString Song::GetDisplaySubTitle() const
{
	if(!PREFSMAN->m_bShowNativeLanguage) return GetTranslitSubTitle();
	return m_sSubTitle;
}

CString Song::GetDisplayArtist() const
{
	if(!PREFSMAN->m_bShowNativeLanguage) return GetTranslitArtist();
	return m_sArtist;
}


CString Song::GetDisplayFullTitle() const
{
	CString Title = GetDisplayMainTitle();
	CString SubTitle = GetDisplaySubTitle();

	if(!SubTitle.empty()) Title += " " + SubTitle;
	return Title;
}

CString Song::GetTranslitFullTitle() const
{
	CString Title = GetTranslitMainTitle();
	CString SubTitle = GetTranslitSubTitle();

	if(!SubTitle.empty()) Title += " " + SubTitle;
	return Title;
}

void Song::AddSteps( Steps* pSteps )
{
	m_vpSteps.push_back( pSteps );
	ASSERT_M( pSteps->m_StepsType < NUM_STEPS_TYPES, ssprintf("%i", pSteps->m_StepsType) );
	m_vpStepsByType[pSteps->m_StepsType].push_back( pSteps );
}

void Song::RemoveSteps( const Steps* pSteps )
{
	// Avoid any stale Note::parent pointers by removing all AutoGen'd Steps,
	// then adding them again.

	RemoveAutoGenNotes();


	vector<Steps*> &vpSteps = m_vpStepsByType[pSteps->m_StepsType];
	for( int j=vpSteps.size()-1; j>=0; j-- )
	{
		if( vpSteps[j] == pSteps )
		{
//			delete vpSteps[j]; // delete below
			vpSteps.erase( vpSteps.begin()+j );
			break;
		}
	}

	for( int j=m_vpSteps.size()-1; j>=0; j-- )
	{
		if( m_vpSteps[j] == pSteps )
		{
			delete m_vpSteps[j];
			m_vpSteps.erase( m_vpSteps.begin()+j );
			break;
		}
	}

	AddAutoGenNotes();
}

bool Song::Matches(const CString &sGroup, const CString &sSong) const
{
	if( sGroup.size() && sGroup.CompareNoCase(this->m_sGroupName) != 0)
		return false;

	CString sDir = this->GetSongDir();
	sDir.Replace("\\","/");
	CStringArray bits;
	split( sDir, "/", bits );
	ASSERT(bits.size() >= 2); /* should always have at least two parts */
	const CString &sLastBit = bits[bits.size()-1];

	// match on song dir or title (ala DWI)
	if( !sSong.CompareNoCase(sLastBit) )
		return true;
	if( !sSong.CompareNoCase(this->GetTranslitFullTitle()) )
		return true;

	return false;
}

void Song::FreeAllLoadedFromProfile( ProfileSlot slot )
{
	/* RemoveSteps will remove and recreate autogen notes, which may reorder
	 * m_vpSteps, so be careful not to skip over entries. */
	vector<Steps*> apToRemove;
	for( int s=m_vpSteps.size()-1; s>=0; s-- )
	{
		Steps* pSteps = m_vpSteps[s];
		if( !pSteps->WasLoadedFromProfile() )
			continue;
		if( slot == PROFILE_SLOT_INVALID || pSteps->GetLoadedFromProfileSlot() == slot )
			apToRemove.push_back( pSteps );
	}

	for( unsigned i = 0; i < apToRemove.size(); ++i )
		this->RemoveSteps( apToRemove[i] );
}

int Song::GetNumStepsLoadedFromProfile( ProfileSlot slot ) const
{
	int iCount = 0;
	for( unsigned s=0; s<m_vpSteps.size(); s++ )
	{
		Steps* pSteps = m_vpSteps[s];
		if( pSteps->GetLoadedFromProfileSlot() == slot )
			iCount++;
	}
	return iCount;
}

bool Song::IsEditAlreadyLoaded( Steps* pSteps ) const
{
	ASSERT( pSteps->GetDifficulty() == DIFFICULTY_EDIT );

	for( unsigned i=0; i<m_vpSteps.size(); i++ )
	{
		Steps* pOther = m_vpSteps[i];
		if( pOther->GetDifficulty() == DIFFICULTY_EDIT &&
			pOther->m_StepsType == pSteps->m_StepsType &&
			pOther->GetHash() == pSteps->GetHash() )
		{
			return true;
		}
	}

	return false;
}

bool Song::HasSignificantBpmChangesOrStops() const
{
	if( m_Timing.HasStops() )
		return true;

	// Don't consider BPM changes that only are only for maintaining sync as 
	// a real BpmChange.
	if( m_DisplayBPMType == DISPLAY_SPECIFIED )
	{
		if( m_fSpecifiedBPMMin != m_fSpecifiedBPMMax )
			return true;
	}
	else if( m_Timing.HasBpmChanges() )
	{
		return true;
	}

	return false;
}

float Song::GetStepsSeconds() const
{
	return GetElapsedTimeFromBeat( m_fLastBeat ) - GetElapsedTimeFromBeat( m_fFirstBeat );
}

bool Song::IsEditDescriptionUnique( StepsType st, const CString &sPreferredDescription, const Steps *pExclude ) const
{
	FOREACH_CONST( Steps*, m_vpSteps, s )
	{
		Steps *pSteps = *s;

		if( pSteps->GetDifficulty() != DIFFICULTY_EDIT )
			continue;
		if( pSteps->m_StepsType != st )
			continue;
		if( pSteps == pExclude )
			continue;
		if( pSteps->GetDescription() == sPreferredDescription )
			return false;
	}
	return true;
}

void Song::MakeUniqueEditDescription( StepsType st, CString &sPreferredDescriptionInOut ) const
{
	if( IsEditDescriptionUnique( st, sPreferredDescriptionInOut, NULL ) )
		return;

	CString sTemp;

	for( int i=0; i<1000; i++ )
	{
		// make name "My Edit" -> "My Edit"
		CString sNum = ssprintf("%d", i+1);
		sTemp = sPreferredDescriptionInOut.Left( MAX_EDIT_DESCRIPTION_LENGTH - sNum.size() ) + sNum;

		if( IsEditDescriptionUnique(st, sTemp, NULL) )
		{
			sPreferredDescriptionInOut = sTemp;
			return;
		}
	}
	
	// Edit limit guards should keep us from ever having more than 1000 edits per song.
}

// this is uglier than it could be...
bool Song::CheckCustomSong( CString &sError )
{
	// why bother? not sure this could happen, but it's good practice.
	if( !PREFSMAN->m_bCustomSongs )
	{
		sError = "Custom songs have been disabled.";
		return false;
	}

	/* we used to use this to test music length, but we only care about steps length.
	 * do this anyway, as a simple check to make sure the song can actually load. */
	CString sResult;
	SoundReader *Sample = SoundReader_FileReader::OpenFile( GetMusicPath(), sResult );

	if( Sample == NULL )
	{
		sError = ssprintf( "Error loading song: %s", sResult.c_str() );
		LOG->Warn( sError );

		delete Sample;
		return false;
	}

	SAFE_DELETE( Sample );

	// steps too long?
	if( PREFSMAN->m_iCustomMaxSeconds > 0 && m_fStepsLengthSeconds > (float)PREFSMAN->m_iCustomMaxSeconds )
	{
		sError = ssprintf( "This file is %.0f seconds long.\nThe maximum play length is %.0f seconds.", m_fStepsLengthSeconds, (float)PREFSMAN->m_iCustomMaxSeconds );
		return false;
	}

	//the file's fine. let's head on back.
	return true;
}


// lua start

template<class T>
class LunaSong : public Luna<T>
{
public:
	LunaSong() { LUA->Register( Register ); }

	static int GetDisplayFullTitle( T* p, lua_State *L )	{ lua_pushstring(L, p->GetDisplayFullTitle() ); return 1; }
	static int GetTranslitFullTitle( T* p, lua_State *L )	{ lua_pushstring(L, p->GetTranslitFullTitle() ); return 1; }
	static int GetDisplayMainTitle( T* p, lua_State *L )	{ lua_pushstring(L, p->GetDisplayMainTitle() ); return 1; }
	static int GetTranslitMainTitle( T* p, lua_State *L )	{ lua_pushstring(L, p->GetTranslitMainTitle() ); return 1; }
	static int GetDisplayArtist( T* p, lua_State *L )		{ lua_pushstring(L, p->GetDisplayArtist() ); return 1; }
	static int GetTranslitArtist( T* p, lua_State *L )		{ lua_pushstring(L, p->GetTranslitArtist() ); return 1; }
	static int GetGenre( T* p, lua_State *L )				{ lua_pushstring(L, p->m_sGenre ); return 1; }
	static int GetAllSteps( T* p, lua_State *L )
	{
		const vector<Steps*> &v = p->GetAllSteps();
		LuaHelpers::CreateTableFromArray<Steps*>( v, L );
		return 1;
	}
	static int GetStepsByStepsType( T* p, lua_State *L )
	{
		StepsType st = (StepsType)IArg(1);
		const vector<Steps*> &v = p->GetStepsByStepsType( st );
		LuaHelpers::CreateTableFromArray<Steps*>( v, L );
		return 1;
	}
	static int GetSongDir( T* p, lua_State *L )	{ lua_pushstring(L, p->GetSongDir() ); return 1; }
	static int GetBannerPath( T* p, lua_State *L )		{ if( !p->HasBanner() ) lua_pushnil(L); else lua_pushstring(L, p->GetBannerPath()); return 1; }
	static int GetBackgroundPath( T* p, lua_State *L )	{ if( !p->HasBackground() ) lua_pushnil(L); else lua_pushstring(L, p->GetBackgroundPath()); return 1; }
	static int GetGroupName( T* p, lua_State *L )		{ lua_pushstring(L, p->m_sGroupName); return 1; }
	static int IsLong( T* p, lua_State *L )				{ lua_pushboolean(L, p->IsLong()); return 1; }
	static int IsMarathon( T* p, lua_State *L )			{ lua_pushboolean(L, p->IsMarathon()); return 1; }
	static int IsCustomSong( T* p, lua_State *L )		{ lua_pushboolean(L, p->IsCustomSong()); return 1; }
	static int MusicLengthSeconds( T* p, lua_State *L )	{ lua_pushnumber(L, p->MusicLengthSeconds()); return 1; }
	static int StepsLengthSeconds( T* p, lua_State *L )	{ lua_pushnumber(L, p->m_fStepsLengthSeconds); return 1; }
	static void Register(lua_State *L)
	{
		ADD_METHOD( GetDisplayFullTitle )
		ADD_METHOD( GetTranslitFullTitle )
		ADD_METHOD( GetDisplayMainTitle )
		ADD_METHOD( GetTranslitMainTitle )
		ADD_METHOD( GetDisplayArtist )
		ADD_METHOD( GetTranslitArtist )
		ADD_METHOD( GetGenre )
		ADD_METHOD( GetAllSteps )
		ADD_METHOD( GetStepsByStepsType )
		ADD_METHOD( GetSongDir )
		ADD_METHOD( GetBannerPath )
		ADD_METHOD( GetBackgroundPath )
		ADD_METHOD( GetGroupName )
		ADD_METHOD( IsLong )
		ADD_METHOD( IsMarathon )
		ADD_METHOD( IsCustomSong )
		ADD_METHOD( MusicLengthSeconds )
		Luna<T>::Register( L );
	}
};

LUA_REGISTER_CLASS( Song )
// lua end


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
