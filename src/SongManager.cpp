#include "global.h"
#include "SongManager.h"
#include "IniFile.h"
#include "RageLog.h"
#include "MsdFile.h"
#include "NotesLoaderDWI.h"
#include "BannerCache.h"

#include "GameState.h"
#include "PrefsManager.h"
#include "RageException.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "Course.h"

#include "AnnouncerManager.h"
#include "ThemeManager.h"
#include "GameManager.h"
#include "RageFile.h"
#include "RageTextureManager.h"
#include "Sprite.h"
#include "ProfileManager.h"
#include "MemoryCardManager.h"
#include "NotesLoaderSM.h"
#include "SongUtil.h"
#include "StepsUtil.h"
#include "CourseUtil.h"
#include "RageFileManager.h"
#include "UnlockManager.h"
#include "Foreach.h"
#include "StatsManager.h"
#include "Style.h"
#include "BackgroundUtil.h"

SongManager*	SONGMAN = NULL;	// global and accessable from anywhere in our program

const CString SONGS_DIR		= "Songs/";
const CString COURSES_DIR	= "Courses/";

const int MAX_EDITS_PER_PROFILE	= 200;
const int MAX_EDIT_SIZE_BYTES	= 30*1024;	// 30KB

static const ThemeMetric<RageColor> BEGINNER_COLOR		("SongManager","BeginnerColor");
static const ThemeMetric<RageColor> EASY_COLOR			("SongManager","EasyColor");
static const ThemeMetric<RageColor> MEDIUM_COLOR		("SongManager","MediumColor");
static const ThemeMetric<RageColor> HARD_COLOR			("SongManager","HardColor");
static const ThemeMetric<RageColor> CHALLENGE_COLOR		("SongManager","ChallengeColor");
static const ThemeMetric<RageColor> EDIT_COLOR			("SongManager","EditColor");
static const ThemeMetric<RageColor> EXTRA_COLOR			("SongManager","ExtraColor");
static const ThemeMetric<int>		EXTRA_COLOR_METER	("SongManager","ExtraColorMeter");

CString SONG_GROUP_COLOR_NAME( size_t i )   { return ssprintf("SongGroupColor%i",(int) i+1); }
CString COURSE_GROUP_COLOR_NAME( size_t i ) { return ssprintf("CourseGroupColor%i",(int) i+1); }


SongManager::SongManager()
{
	LoadGroupColors();
}

SongManager::~SongManager()
{
	// Courses depend on Songs and Songs don't depend on Courses.
	// So, delete the Courses first.
	FreeCourses();
	FreeSongs();
}

void SongManager::LoadGroupColors()
{
	NUM_SONG_GROUP_COLORS	.Load("SongManager","NumSongGroupColors");
	SONG_GROUP_COLOR		.Load("SongManager",SONG_GROUP_COLOR_NAME,NUM_SONG_GROUP_COLORS);
	NUM_COURSE_GROUP_COLORS	.Load("SongManager","NumCourseGroupColors");
	COURSE_GROUP_COLOR		.Load("SongManager",COURSE_GROUP_COLOR_NAME,NUM_COURSE_GROUP_COLORS);
}

void SongManager::InitAll( LoadingWindow *ld )
{
	InitSongsFromDisk( ld );
	InitCoursesFromDisk( ld );
	InitAutogenCourses();
}

void SongManager::Reload( LoadingWindow *ld )
{
	FlushDirCache();

	if( ld )
		ld->SetText( "Reloading ..." );

	// save scores before unloading songs, or the scores will be lost
	PROFILEMAN->SaveMachineProfile();

	FreeSongs();
	FreeCourses();

	/* Always check songs for changes. */
	const bool OldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad.Set( false );

#if defined(LINUX) && defined(ITG_ARCADE)
	system("mount -o remount,rw /itgdata");
#endif
	InitAll( ld );
#if defined(LINUX) && defined(ITG_ARCADE)
	system("mount -o remount,ro /itgdata");
#endif

	// reload scores afterward
	PROFILEMAN->LoadMachineProfile();

	PREFSMAN->m_bFastLoad.Set( OldVal );
}

void SongManager::InitSongsFromDisk( LoadingWindow *ld )
{
	RageTimer tm;
	
	/* this loads the songs into m_pMachineSongs */
	LoadStepManiaSongDir( SONGS_DIR, ld );

	/* now, copy them into m_pSongs */
	m_pSongs.insert( m_pSongs.begin(), m_pMachineSongs.begin(), m_pMachineSongs.end() );

	LOG->Trace( "Found %d songs in %f seconds.", (int)m_pSongs.size(), tm.GetDeltaTime() );
}

void SongManager::SanityCheckGroupDir( CString sDir ) const
{
	// Check to see if they put a song directly inside the group folder.
	CStringArray arrayFiles;
	GetDirListing( sDir + "/*.mp3", arrayFiles );
	GetDirListing( sDir + "/*.ogg", arrayFiles );
	GetDirListing( sDir + "/*.wav", arrayFiles );
	if( !arrayFiles.empty() )
		RageException::Throw( 
			"The folder '%s' contains music files.\n\n"
			"This means that you have a music outside of a song folder.\n"
			"All song folders must reside in a group folder.  For example, 'Songs/DDR 4th Mix/B4U'.\n"
			"See the StepMania readme for more info.",
			sDir.c_str()
		);
	
}

void SongManager::AddGroup( CString sDir, CString sGroupDirName )
{
	unsigned j;
	for(j = 0; j < m_sSongGroupNames.size(); ++j)
		if( sGroupDirName == m_sSongGroupNames[j] ) break;

	if( j != m_sSongGroupNames.size() )
		return; /* the group is already added */

	// Look for a group banner in this group folder
	CStringArray arrayGroupBanners;
	GetDirListing( sDir+sGroupDirName+"/*.png", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.jpg", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.gif", arrayGroupBanners );
	GetDirListing( sDir+sGroupDirName+"/*.bmp", arrayGroupBanners );

	CString sBannerPath;
	if( !arrayGroupBanners.empty() )
		sBannerPath = sDir+sGroupDirName+"/"+arrayGroupBanners[0] ;
	else
		{
		// Look for a group banner in the parent folder
		GetDirListing( sDir+sGroupDirName+".png", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".jpg", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".gif", arrayGroupBanners );
		GetDirListing( sDir+sGroupDirName+".bmp", arrayGroupBanners );
		if( !arrayGroupBanners.empty() )
			sBannerPath = sDir+arrayGroupBanners[0];
	}

	LOG->Trace( "Group banner for '%s' is '%s'.", sGroupDirName.c_str(), 
				sBannerPath != ""? sBannerPath.c_str():"(none)" );
	m_sSongGroupNames.push_back( sGroupDirName );
	m_sSongGroupBannerPaths.push_back( sBannerPath );
}

void SongManager::LoadStepManiaSongDir( CString sDir, LoadingWindow *ld )
{
	/* Make sure sDir has a trailing slash. */
	if( sDir.Right(1) != "/" )
		sDir += "/";

	// Find all group directories in "Songs" folder
	CStringArray arrayGroupDirs;
	GetDirListing( sDir+"*", arrayGroupDirs, true );
	SortCStringArray( arrayGroupDirs );

	for( unsigned i=0; i< arrayGroupDirs.size(); i++ )	// for each dir in /Songs/
	{
		CString sGroupDirName = arrayGroupDirs[i];

		SanityCheckGroupDir(sDir+sGroupDirName);

		// Find all Song folders in this group directory
		CStringArray arraySongDirs;
		GetDirListing( sDir+sGroupDirName + "/*", arraySongDirs, true, true );
		SortCStringArray( arraySongDirs );

		LOG->Trace("Attempting to load %i songs from \"%s\"", int(arraySongDirs.size()),
				   (sDir+sGroupDirName).c_str() );
		int loaded = 0;

		for( unsigned j=0; j< arraySongDirs.size(); ++j )	// for each song dir
		{
			CString sSongDirName = arraySongDirs[j];

			// this is a song directory.  Load a new song!
			if( ld )
			{
				ld->SetText( ssprintf("Loading songs...\n%s\n%s",
									  Basename(sGroupDirName).c_str(),
									  Basename(sSongDirName).c_str()));
				ld->Paint();
			}
			Song* pNewSong = new Song;
			if( !pNewSong->LoadFromSongDir( sSongDirName ) )
			{
				/* The song failed to load. */
				delete pNewSong;
				continue;
			}
			
			m_pMachineSongs.push_back( pNewSong );
			loaded++;
		}

		LOG->Trace("Loaded %i songs from \"%s\"", loaded, (sDir+sGroupDirName).c_str() );

		/* Don't add the group name if we didn't load any songs in this group. */
		if(!loaded) continue;

		/* Add this group to the group array. */
		AddGroup(sDir, sGroupDirName);

		/* Cache and load the group banner. */
		BANNERCACHE->CacheBanner( GetGroupBannerPath(sGroupDirName) );
		
		/* Load the group sym links (if any)*/
		LoadGroupSymLinks(sDir, sGroupDirName);
	}
}

void SongManager::LoadPlayerCourses( PlayerNumber pn )
{
	LOG->Trace( "SongManager::LoadPlayerCourses( %i )", pn );

	// if the memory card isn't here, don't bother
	if( !PROFILEMAN->IsPersistentProfile(pn) )
	{
		CHECKPOINT;
		return;
	}

	// sanity check (should always be true though)
	if( !PREFSMAN->m_bCustomCourses )
	{
		CHECKPOINT;
		return;
	}

	CString sDir = PROFILEMAN->GetProfileDir( (ProfileSlot)pn );
	if( sDir.Right(1) != "/" )
		sDir += "/";
	sDir = sDir + COURSES_DIR;

	CHECKPOINT_M( sDir.c_str() );

	CStringArray arrayProfileCourses;
	GetDirListing( sDir + "/*.crs", arrayProfileCourses, false, true ); /**/
	SortCStringArray( arrayProfileCourses );

	CHECKPOINT_M( ssprintf("%d",arrayProfileCourses.size()) );

	CString sDisplayName, sGroupName;

	Profile* p = PROFILEMAN->GetProfile( pn );

	if( p->GetDisplayName().empty() ) // no name
		sDisplayName = ssprintf( "Player %d", pn+1 );
	else
		sDisplayName = p->GetDisplayName();

	TrimLeft(sDisplayName);
	TrimRight(sDisplayName);

	sGroupName = sDisplayName + "\'s Courses";

	unsigned i = 0;
	for ( i; i < arrayProfileCourses.size(); i++ )
	{
		Course *crs = new Course;
		crs->LoadFromCRSFile( arrayProfileCourses[i], true );
		crs->m_sGroupName = sGroupName;
		CHECKPOINT_M( crs->GetDisplayFullTitle().c_str() );

		crs->m_bIsCustomCourse = true;
		crs->m_CourseOwner = pn;

		// is there already a course under the same name?
		if ( FindCourse( crs->GetDisplayFullTitle() ) != NULL )
		{
			CHECKPOINT;
			delete crs;
			continue;	
		}

		// for now, only accept Marathons
		if ( crs->GetCourseType() != COURSE_TYPE_NONSTOP )
		{
			CHECKPOINT;
			delete crs;
			continue;
		}
		
		// TODO: softcode this to a settable preference
		if ( crs->GetEstimatedNumStages() > 5 )
		{
			CHECKPOINT;
			delete crs;
			continue;
		}

		CHECKPOINT;
		// TODO: make sure songs exist and are not custom songs --infamouspat

		LOG->Trace( "SongManager::LoadPlayerCourses(): Adding custom course \"%s\"", crs->GetDisplayFullTitle().c_str() );
		m_pCourses.push_back( crs );
	}

	// if there are any courses, add the group
	if (i > 0)
		m_sCourseGroupNames.push_back( sGroupName );
}

void SongManager::LoadPlayerSongs( PlayerNumber pn )
{
	// if the memory card isn't here, don't bother
	if( !PROFILEMAN->IsPersistentProfile(pn) )
		return;

	// likewise if customs are disabled (sanity check as of now)
	if( !PREFSMAN->m_bCustomSongs )
		return;

	LOG->Trace( "SongManager::LoadPlayerSongs( %i )", pn );

	CString sDir = PROFILEMAN->GetProfileDir( (ProfileSlot)pn );
	if( sDir.Right(1) != "/" )
		sDir += "/";
	sDir = sDir + SONGS_DIR;

	CString sDisplayName, sGroupName;

	Profile* p = PROFILEMAN->GetProfile( pn );

	if( p->GetDisplayName().empty() ) // no name
		sDisplayName = ssprintf( "Player %d", pn+1 );
	else
		sDisplayName = p->GetDisplayName();

	sGroupName = sDisplayName + "\'s Songs";

	CStringArray arraySongRootDirs;
	GetDirListing( sDir + "/*", arraySongRootDirs, true, true );
	SortCStringArray( arraySongRootDirs );

	CStringArray arraySongSubDirs;

	for( unsigned i=0; i < arraySongRootDirs.size(); i++ )
	{
		CStringArray arraySubDirs;

		GetDirListing( arraySongRootDirs[i] + "/*", arraySubDirs, true, true );

		if( !arraySubDirs.size() ) // nothing here
			continue;

		for( unsigned j=0; j < arraySubDirs.size(); j++ )
			arraySongSubDirs.push_back( arraySubDirs[j] );
	}

	SortCStringArray( arraySongSubDirs ); // organise what we just loaded

	/* Load subdirectories before we load the base directory. */
	CStringArray arraySongDirs;
	arraySongDirs.insert( arraySongDirs.begin(), arraySongSubDirs.begin(), arraySongSubDirs.end() );
	arraySongDirs.insert( arraySongDirs.end(), arraySongRootDirs.begin(), arraySongRootDirs.end() );

	LOG->Trace("Attempting to load %i songs and %i subdirectory songs from \"%s\"",
		arraySongRootDirs.size(), arraySongSubDirs.size(), sDir.c_str() );
	int iSongsLoaded = 0;

	// keep time so we don't run much longer than expected.
	RageTimer LoadTimer;

	for( unsigned j=0; j < arraySongDirs.size(); j++ )
	{
		if( LoadTimer.Ago() > PREFSMAN->m_fCustomsLoadTimeout )

		{
			LOG->Warn( "Loading interrupted after %i songs. Timer reached %f, with a %f second limit.",
			iSongsLoaded, LoadTimer.Ago(), (float)PREFSMAN->m_fCustomsLoadTimeout );
			break;
		}

		// we want to stop right on the number, not after. "Greater than" is added as a safeguard.
		if( PREFSMAN->m_iCustomsLoadMax > 0 && iSongsLoaded >= PREFSMAN->m_iCustomsLoadMax )
		{
			LOG->Warn( "Loading interrupted. Limit of %i songs was reached, after %f seconds.",
			(int)PREFSMAN->m_iCustomsLoadMax, LoadTimer.Ago() );
			break;
		}

		CString sSongDirName = arraySongDirs[j];
		Song* pNewSong = new Song;
		
		// we need a custom loader for these songs.
		if( !pNewSong->LoadFromCustomSongDir( sSongDirName, sGroupName, pn ) )
		{
			// The song failed to load
			LOG->Warn("Loading %s from \"%s\" failed", sSongDirName.c_str(), arraySongDirs[j].c_str() );
			delete pNewSong;
			continue;
		}
		
		LOG->Trace( "Loading custom song '%s'...", pNewSong->m_sMainTitle.c_str() );
		
		// TODO: load everything into m_pCustomSongs, then insert() into m_pSongs,
		// instead of manually inserting the song into both arrays.
		m_pCustomSongs.push_back( pNewSong );
		m_pSongs.push_back( pNewSong );
		iSongsLoaded++;
	}

	LOG->Trace( "%i songs loaded from %s in %f seconds.", iSongsLoaded, sDir.c_str(), LoadTimer.Ago() );

	// If we have content, make sure to add the group and banner.
	if( iSongsLoaded )
		AddGroup( sDir, sGroupName );
}

// Instead of "symlinks", songs should have membership in multiple groups.
// -Chris
void SongManager::LoadGroupSymLinks(CString sDir, CString sGroupFolder)
{
	// Find all symlink files in this folder
	CStringArray arraySymLinks;
	GetDirListing( sDir+sGroupFolder+"/*.include", arraySymLinks, false );
	SortCStringArray( arraySymLinks );
	for( unsigned s=0; s< arraySymLinks.size(); s++ )	// for each symlink in this dir, add it in as a song.
	{
		MsdFile		msdF;
		msdF.ReadFile( sDir+sGroupFolder+"/"+arraySymLinks[s].c_str() );
		CString	sSymDestination = msdF.GetParam(0,1);	// Should only be 1 vale&param...period.
		
		Song* pNewSong = new Song;
		if( !pNewSong->LoadFromSongDir( sSymDestination ) )
		{
			delete pNewSong; // The song failed to load.
		}
		else
		{
			const vector<Steps*>& vpSteps = pNewSong->GetAllSteps();
			while( vpSteps.size() )
				pNewSong->RemoveSteps( vpSteps[0] );

			FOREACH_BackgroundLayer( i )
				pNewSong->GetBackgroundChanges(i).clear();

			pNewSong->m_bIsSymLink = true;	// Very important so we don't double-parse later
			pNewSong->m_sGroupName = sGroupFolder;
			m_pMachineSongs.push_back( pNewSong );
		}
	}
}

void SongManager::PreloadSongImages()
{
	ASSERT( TEXTUREMAN );
	if( PREFSMAN->m_BannerCache != PrefsManager::BNCACHE_FULL )
		return;

	const vector<Song*> &songs = SONGMAN->GetAllSongs();
	for( unsigned i = 0; i < songs.size(); ++i )
	{
		if( !songs[i]->HasBanner() )
			continue;

		const RageTextureID ID = Sprite::SongBannerTexture( songs[i]->GetBannerPath() );
		TEXTUREMAN->PermanentTexture( ID );
	}

	vector<Course*> courses;
	SONGMAN->GetAllCourses( courses, false );
	for( unsigned i = 0; i < courses.size(); ++i )
	{
		if( !courses[i]->HasBanner() )
			continue;

		const RageTextureID ID = Sprite::SongBannerTexture( courses[i]->m_sBannerPath );
		TEXTUREMAN->PermanentTexture( ID );
	}
}

void SongManager::FreeSongs()
{
	LOG->Trace( "SongManager::FreeSongs" );

	m_sSongGroupNames.clear();
	m_sSongGroupBannerPaths.clear();

	for( unsigned i=0; i<m_pCustomSongs.size(); i++ )
		SAFE_DELETE( m_pCustomSongs[i] );
	m_pCustomSongs.clear();

	for( unsigned i=0; i<m_pMachineSongs.size(); i++ )
		SAFE_DELETE( m_pMachineSongs[i] );
	m_pMachineSongs.clear();

	// m_pSongs is simply cleared because all its pointers
	// were already freed in the two previous vectors
	m_pSongs.clear();

	m_sSongGroupBannerPaths.clear();

	for( int i = 0; i < NUM_PROFILE_SLOTS; ++i )
		m_pBestSongs[i].clear();
	m_pShuffledSongs.clear();
}

CString SongManager::GetGroupBannerPath( CString sGroupName )
{
	unsigned i;
	for(i = 0; i < m_sSongGroupNames.size(); ++i)
		if( sGroupName == m_sSongGroupNames[i] ) break;

	if( i == m_sSongGroupNames.size() )
		return "";

	return m_sSongGroupBannerPaths[i];
}

void SongManager::GetGroupNames( CStringArray &AddTo )
{
	AddTo.insert(AddTo.end(), m_sSongGroupNames.begin(), m_sSongGroupNames.end() );
}

bool SongManager::DoesGroupExist( CString sGroupName )
{
	for( unsigned i = 0; i < m_sSongGroupNames.size(); ++i )
		if( !m_sSongGroupNames[i].CompareNoCase(sGroupName) )
			return true;

	return false;
}

RageColor SongManager::GetSongGroupColor( const CString &sSongGroupName )
{
	CHECKPOINT;
	// search for the group index
	unsigned i;
	for( i=0; i<m_sSongGroupNames.size(); i++ )
	{
		if( m_sSongGroupNames[i] == sSongGroupName )
		{
			CHECKPOINT_M( "Found group name." );
			break;
		}
	}
	ASSERT_M( i != m_sSongGroupNames.size(), sSongGroupName );	// this is not a valid group
	CHECKPOINT_M( "Assert not met." );

	return SONG_GROUP_COLOR.GetValue( i%NUM_SONG_GROUP_COLORS );
}

RageColor SongManager::GetSongColor( const Song* pSong )
{
	ASSERT( pSong );

	/* XXX:
	 * Previously, this matched all notes, which set a song to "extra" if it
	 * had any 10-foot steps at all, even edits or doubles.
	 *
	 * For now, only look at notes for the current note type.  This means that
	 * if a song has 10-foot steps on Doubles, it'll only show up red in Doubles.
	 * That's not too bad, I think.  This will also change it in the song scroll,
	 * which is a little odd but harmless. 
	 *
	 * XXX: Ack.  This means this function can only be called when we have a style
	 * set up, which is too restrictive.  How to handle this?
	 */
//	const StepsType st = GAMESTATE->GetCurrentStyle()->m_StepsType;
	const vector<Steps*>& vpSteps = pSong->GetAllSteps();
	for( unsigned i=0; i<vpSteps.size(); i++ )
	{
		const Steps* pSteps = vpSteps[i];
		switch( pSteps->GetDifficulty() )
		{
		case DIFFICULTY_CHALLENGE:
		case DIFFICULTY_EDIT:
			continue;
		}

//		if(pSteps->m_StepsType != st)
//			continue;

		if( pSteps->GetMeter() >= EXTRA_COLOR_METER )
			return (RageColor)EXTRA_COLOR;
	}

	return GetSongGroupColor( pSong->m_sGroupName );
}

RageColor SongManager::GetCourseGroupColor( const CString &sCourseGroupName )
{
	// search for the group index
	unsigned i;
	for( i=0; i<m_sCourseGroupNames.size(); i++ )
	{
		if( m_sCourseGroupNames[i] == sCourseGroupName )
			break;
	}
	ASSERT_M( i != m_sCourseGroupNames.size(), "Course: '" + sCourseGroupName + "' is not a valid group." );	// this is not a valid group
	return COURSE_GROUP_COLOR.GetValue( i%NUM_COURSE_GROUP_COLORS.GetValue() );
}

RageColor SongManager::GetCourseColor( const Course* pCourse )
{
	return GetCourseGroupColor( pCourse->m_sGroupName );
}

RageColor SongManager::GetDifficultyColor( Difficulty dc ) const
{
	switch( dc )
	{
	case DIFFICULTY_BEGINNER:	return (RageColor)BEGINNER_COLOR;
	case DIFFICULTY_EASY:		return (RageColor)EASY_COLOR;
	case DIFFICULTY_MEDIUM:		return (RageColor)MEDIUM_COLOR;
	case DIFFICULTY_HARD:		return (RageColor)HARD_COLOR;
	case DIFFICULTY_CHALLENGE:	return (RageColor)CHALLENGE_COLOR;
	case DIFFICULTY_EDIT:		return (RageColor)EDIT_COLOR;
	default:	ASSERT(0);		return (RageColor)EDIT_COLOR;
	}
}

static void GetSongsFromVector( const vector<Song*> &Songs, vector<Song*> &AddTo, CString sGroupName, int iMaxStages )
{
	CHECKPOINT;
	AddTo.clear();

	for( unsigned i=0; i<Songs.size(); i++ )
		if( sGroupName==GROUP_ALL_MUSIC || sGroupName==Songs[i]->m_sGroupName )
				if( SongManager::GetNumStagesForSong(Songs[i]) <= iMaxStages )
					AddTo.push_back( Songs[i] );
	CHECKPOINT;
}

void SongManager::GetSongs( vector<Song*> &AddTo, CString sGroupName, int iMaxStages ) const
{
	GetSongsFromVector( m_pSongs, AddTo, sGroupName, iMaxStages );
}

void SongManager::GetBestSongs( vector<Song*> &AddTo, CString sGroupName, int iMaxStages, ProfileSlot slot ) const
{
	GetSongsFromVector( m_pBestSongs[slot], AddTo, sGroupName, iMaxStages );
}

int SongManager::GetNumSongs() const
{
	return m_pSongs.size();
}

int SongManager::GetNumGroups() const
{
	return m_sSongGroupNames.size();
}

int SongManager::GetNumCourses() const
{
	return m_pCourses.size();
}

CString SongManager::ShortenGroupName( CString sLongGroupName )
{
	sLongGroupName.Replace( "Dance Dance Revolution", "DDR" );
	sLongGroupName.Replace( "dance dance revolution", "DDR" );
	sLongGroupName.Replace( "DANCE DANCE REVOLUTION", "DDR" );
	sLongGroupName.Replace( "Pump It Up", "PIU" );
	sLongGroupName.Replace( "pump it up", "PIU" );
	sLongGroupName.Replace( "PUMP IT UP", "PIU" );
	sLongGroupName.Replace( "ParaParaParadise", "PPP" );
	sLongGroupName.Replace( "paraparaparadise", "PPP" );
	sLongGroupName.Replace( "PARAPARAPARADISE", "PPP" );
	sLongGroupName.Replace( "Para Para Paradise", "PPP" );
	sLongGroupName.Replace( "para para paradise", "PPP" );
	sLongGroupName.Replace( "PARA PARA PARADISE", "PPP" );
	sLongGroupName.Replace( "Dancing Stage", "DS" );
	sLongGroupName.Replace( "dancing stage", "DS" );
	sLongGroupName.Replace( "DANCING STAGE", "DS" );
	sLongGroupName.Replace( "Ez2dancer", "EZ2" );
	sLongGroupName.Replace( "Ez 2 Dancer", "EZ2");
	sLongGroupName.Replace( "Technomotion", "TM");
	sLongGroupName.Replace( "Techno Motion", "TM");
	sLongGroupName.Replace( "Dance Station 3DDX", "3DDX");
	sLongGroupName.Replace( "DS3DDX", "3DDX");
	sLongGroupName.Replace( "BeatMania", "BM");
	sLongGroupName.Replace( "Beatmania", "BM");
	sLongGroupName.Replace( "BEATMANIA", "BM");
	sLongGroupName.Replace( "beatmania", "BM");
	return sLongGroupName;
}

int SongManager::GetNumStagesForSong( const Song* pSong )
{
	ASSERT( pSong );
	if( pSong->IsMarathon() )
		return 3;
	if( pSong->IsLong() )
		return 2;
	else
		return 1;
}

void SongManager::InitCoursesFromDisk( LoadingWindow *ld )
{
	LOG->Info( "Loading courses." );

	m_sCourseGroupNames.clear();

	//
	// Load courses from in Courses dir
	//
	{
		CStringArray saCourseFiles;
		GetDirListing( COURSES_DIR+"*.crs", saCourseFiles, false, true );
		for( unsigned i=0; i<saCourseFiles.size(); i++ )
		{
			Course* pCourse = new Course;
			pCourse->LoadFromCRSFile( saCourseFiles[i] );
			m_pCourses.push_back( pCourse );

			if( ld )
			{
				ld->SetText( ssprintf("Loading courses...\n%s\n%s",
					"Courses",
					Basename(saCourseFiles[i]).c_str()));
				ld->Paint();
			}

		}
		if( !saCourseFiles.empty() )
			m_sCourseGroupNames.push_back( "" );
	}


	// Find all group directories in Courses dir
	{
		GetDirListing( COURSES_DIR+"*", m_sCourseGroupNames, true );
		SortCStringArray( m_sCourseGroupNames );
		
		FOREACH( CString, m_sCourseGroupNames, sCourseGroup )	// for each dir in /Courses/
		{
			// Find all CRS files in this group directory
			CStringArray vsCoursePaths;
			GetDirListing( COURSES_DIR + *sCourseGroup + "/*.crs", vsCoursePaths, false, true );
			SortCStringArray( vsCoursePaths );

			FOREACH_CONST( CString, vsCoursePaths, sCoursePath )
			{
				if( ld )
				{
					ld->SetText( ssprintf("Loading courses...\n%s\n%s",
						Basename(*sCourseGroup).c_str(),
						Basename(*sCoursePath).c_str()));
					ld->Paint();
				}

				Course* pCourse = new Course;
				pCourse->LoadFromCRSFile( *sCoursePath );
				m_pCourses.push_back( pCourse );
			}
		}
	}
}
	
void SongManager::InitAutogenCourses()
{
	//
	// Create group courses for Endless and Nonstop
	//
	CStringArray saGroupNames;
	this->GetGroupNames( saGroupNames );
	Course* pCourse;
	for( unsigned g=0; g<saGroupNames.size(); g++ )	// foreach Group
	{
		CString sGroupName = saGroupNames[g];
		vector<Song*> apGroupSongs;
		GetSongs( apGroupSongs, sGroupName );

		// Generate random courses from each group.
		pCourse = new Course;
		pCourse->AutogenEndlessFromGroup( sGroupName, DIFFICULTY_MEDIUM );
		m_pCourses.push_back( pCourse );

		pCourse = new Course;
		pCourse->AutogenNonstopFromGroup( sGroupName, DIFFICULTY_MEDIUM );
		m_pCourses.push_back( pCourse );
	}
	
	vector<Song*> apCourseSongs = GetAllSongs();

	// Generate "All Songs" endless course.
	pCourse = new Course;
	pCourse->AutogenEndlessFromGroup( "", DIFFICULTY_MEDIUM );
	m_pCourses.push_back( pCourse );

	/* Generate Oni courses from artists.  Only create courses if we have at least
	 * four songs from an artist; create 3- and 4-song courses. */
	{
		/* We normally sort by translit artist.  However, display artist is more
		 * consistent.  For example, transliterated Japanese names are alternately
		 * spelled given- and family-name first, but display titles are more consistent. */
		vector<Song*> apSongs = this->GetAllSongs();
		SongUtil::SortSongPointerArrayByDisplayArtist( apSongs );

		CString sCurArtist = "";
		CString sCurArtistTranslit = "";
		int iCurArtistCount = 0;

		vector<Song *> aSongs;
		unsigned i = 0;
		do {
			CString sArtist = i >= apSongs.size()? CString(""): apSongs[i]->GetDisplayArtist();
			CString sTranslitArtist = i >= apSongs.size()? CString(""): apSongs[i]->GetTranslitArtist();
			if( i < apSongs.size() && !sCurArtist.CompareNoCase(sArtist) )
			{
				aSongs.push_back( apSongs[i] );
				++iCurArtistCount;
				continue;
			}

			/* Different artist, or we're at the end.  If we have enough entries for
			 * the last artist, add it.  Skip blanks and "Unknown artist". */
			if( iCurArtistCount >= 3 && sCurArtistTranslit != "" &&
				sCurArtistTranslit.CompareNoCase("Unknown artist") &&
				sCurArtist.CompareNoCase("Unknown artist") )
			{
				pCourse = new Course;
				pCourse->AutogenOniFromArtist( sCurArtist, sCurArtistTranslit, aSongs, DIFFICULTY_HARD );
				m_pCourses.push_back( pCourse );
			}

			aSongs.clear();
			
			if( i < apSongs.size() )
			{
				sCurArtist = sArtist;
				sCurArtistTranslit = sTranslitArtist;
				iCurArtistCount = 1;
				aSongs.push_back( apSongs[i] );
			}
		} while( i++ < apSongs.size() );
	}
}


void SongManager::FreeCourses()
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		delete m_pCourses[i];
	m_pCourses.clear();

	for( int i = 0; i < NUM_PROFILE_SLOTS; ++i )
		FOREACH_CourseType( ct )
			m_pBestCourses[i][ct].clear();
	m_pShuffledCourses.clear();

	m_sCourseGroupNames.clear();
}

/* Called periodically to wipe out cached NoteData.  This is called when we change
 * screens. */
void SongManager::Cleanup()
{
	for( unsigned i=0; i<m_pSongs.size(); i++ )
	{
		Song* pSong = m_pSongs[i];
		const vector<Steps*>& vpSteps = pSong->GetAllSteps();
		for( unsigned n=0; n<vpSteps.size(); n++ )
		{
			Steps* pSteps = vpSteps[n];
			pSteps->Compress();
		}
	}
}

/* Flush all Song*, Steps* and Course* caches.  This is called on reload, and when
 * any of those are removed or changed.  This doesn't touch GAMESTATE and StageStats
 * pointers, which are updated explicitly in Song::RevertFromDisk. */
void SongManager::Invalidate( Song *pStaleSong )
{
	//
	// Save list of all old Course and Trail pointers
	//
	map<Course*,CourseID> mapOldCourseToCourseID;
	typedef pair<TrailID,Course*> TrailIDAndCourse;
	map<Trail*,TrailIDAndCourse> mapOldTrailToTrailIDAndCourse;
	FOREACH_CONST( Course*, this->m_pCourses, pCourse )
	{
		CourseID id;
		id.FromCourse( *pCourse );
		mapOldCourseToCourseID[*pCourse] = id;
		vector<Trail *> Trails;
		(*pCourse)->GetAllCachedTrails( Trails );
		FOREACH_CONST( Trail*, Trails, pTrail )
		{
			TrailID id;
			id.FromTrail( *pTrail );
			mapOldTrailToTrailIDAndCourse[*pTrail] = TrailIDAndCourse(id, *pCourse);
		}
	}

	// It's a real pain to selectively invalidate only those Courses with 
	// dependencies on the stale Song.  So, instead, just reload all Courses.
	// It doesn't take very long.
	FreeCourses();
	InitCoursesFromDisk( NULL );
	InitAutogenCourses();

	// invalidate cache
	StepsID::ClearCache();

#define CONVERT_COURSE_POINTER( pCourse ) do { \
	CourseID id = mapOldCourseToCourseID[pCourse]; /* this will always succeed */ \
	pCourse = id.ToCourse(); \
} while(false)

	/* Ugly: We need the course pointer to restore a trail pointer, and both have
	 * been invalidated.  We need to go through our mapping, and update the course
	 * pointers, so we can use that to update trail pointers.  */
	{
		map<Trail*,TrailIDAndCourse>::iterator it;
		for( it = mapOldTrailToTrailIDAndCourse.begin(); it != mapOldTrailToTrailIDAndCourse.end(); ++it )
		{
			TrailIDAndCourse &tidc = it->second;
			CONVERT_COURSE_POINTER( tidc.second );
		}
	}

	{
		CourseID id = mapOldCourseToCourseID[GAMESTATE->m_pCurCourse]; /* this will always succeed */
		GAMESTATE->m_pCurCourse.Set( id.ToCourse() );
	}
	CONVERT_COURSE_POINTER( GAMESTATE->m_pPreferredCourse );

#define CONVERT_TRAIL_POINTER( pTrail ) do { \
	if( pTrail != NULL ) { \
		map<Trail*,TrailIDAndCourse>::iterator it; \
		it = mapOldTrailToTrailIDAndCourse.find(pTrail); \
		ASSERT_M( it != mapOldTrailToTrailIDAndCourse.end(), ssprintf("%p", pTrail.Get()) ); \
		const TrailIDAndCourse &tidc = it->second; \
		const TrailID &id = tidc.first; \
		const Course *pCourse = tidc.second; \
		pTrail.Set( id.ToTrail( pCourse, true ) ); \
	} \
} while(false)

	FOREACH_PlayerNumber( pn )
	{
		CONVERT_TRAIL_POINTER( GAMESTATE->m_pCurTrail[pn] );
	}
}

/* If bAllowNotesLoss is true, any global notes pointers which no longer exist
 * (or exist but couldn't be matched) will be set to NULL.  This is used when
 * reverting out of the editor.  If false, this is unexpected and will assert.
 * This is used when reverting out of gameplay, in which case we may have StageStats,
 * etc. which may cause hard-to-trace crashes down the line if we set them to NULL. */
void CONVERT_STEPS_POINTER( Steps *&pSteps, const map<Steps*,StepsID> &mapOldStepsToStepsID, const Song *pSong, bool bAllowNotesLoss )
{
	if( pSteps == NULL )
		return;

	map<Steps*,StepsID>::const_iterator it = mapOldStepsToStepsID.find(pSteps);
	if( it != mapOldStepsToStepsID.end() )
		pSteps = it->second.ToSteps(pSong, bAllowNotesLoss);
}
void CONVERT_STEPS_POINTER( BroadcastOnChangePtr<Steps> &pSteps, const map<Steps*,StepsID> &mapOldStepsToStepsID, const Song *pSong, bool bAllowNotesLoss )
{
	if( pSteps == NULL )
		return;

	map<Steps*,StepsID>::const_iterator it = mapOldStepsToStepsID.find(pSteps);
	if( it != mapOldStepsToStepsID.end() )
		pSteps.Set( it->second.ToSteps(pSong, bAllowNotesLoss) );
}
void SongManager::RevertFromDisk( Song *pSong, bool bAllowNotesLoss )
{
	/* Reverting from disk is brittle, and touches a lot of tricky and rarely-
	 * used code paths.  If it's ever used during a game, log it. */
	LOG->MapLog( "RevertFromDisk", "Reverted \"%s\" from disk", pSong->GetTranslitMainTitle().c_str() );

	// Ugly:  When we re-load the song, the Steps* will change.
	// Fix GAMESTATE->m_CurSteps, STATSMAN->m_CurStageStats, STATSMAN->m_vPlayedStageStats[] after reloading.
	/* XXX: This is very brittle.  However, we must know about all globals uses of Steps*,
	 * so we can check to make sure we didn't lose any steps which are referenced ... */


	//
	// Save list of all old Steps pointers for the song
	//
	map<Steps*,StepsID> mapOldStepsToStepsID;
	FOREACH_CONST( Steps*, pSong->GetAllSteps(), pSteps )
	{
		StepsID id;
		id.FromSteps( *pSteps );
		mapOldStepsToStepsID[*pSteps] = id;
	}


	//
	// Reload the song
	//
	const CString dir = pSong->GetSongDir();
	FILEMAN->FlushDirCache( dir );

	/* Erase existing data and reload. */
	pSong->Reset();
	const bool OldVal = PREFSMAN->m_bFastLoad;
	PREFSMAN->m_bFastLoad.Set( false );
	pSong->LoadFromSongDir( dir );	
	/* XXX: reload edits? */
	PREFSMAN->m_bFastLoad.Set( OldVal );


	/* Courses cache Steps pointers.  On the off chance that this isn't the last
	 * thing this screen does, clear that cache. */
	/* TODO: Don't make Song depend on SongManager.  This is breaking 
	 * encapsulation and placing confusing limitation on what can be done in 
	 * SONGMAN->Invalidate(). -Chris */
	this->Invalidate( pSong );
	StepsID::ClearCache();


	FOREACH_PlayerNumber( p )
	{
		CONVERT_STEPS_POINTER( GAMESTATE->m_pCurSteps[p], mapOldStepsToStepsID, pSong, bAllowNotesLoss );

		FOREACH( Steps*, STATSMAN->m_CurStageStats.m_player[p].vpPlayedSteps, pSteps )
			CONVERT_STEPS_POINTER( *pSteps, mapOldStepsToStepsID, pSong, bAllowNotesLoss );

		FOREACH( StageStats, STATSMAN->m_vPlayedStageStats, ss )
			FOREACH( Steps*, ss->m_player[p].vpPlayedSteps, pSteps )
				CONVERT_STEPS_POINTER( *pSteps, mapOldStepsToStepsID, pSong, bAllowNotesLoss );
	}

	CONVERT_STEPS_POINTER( GAMESTATE->m_pEditSourceSteps, mapOldStepsToStepsID, pSong, bAllowNotesLoss );
}

void SongManager::RegenerateNonFixedCourses()
{
	for( unsigned i=0; i < m_pCourses.size(); i++ )
		m_pCourses[i]->RegenerateNonFixedTrails();
}

void SongManager::SetPreferences()
{
	for( unsigned int i=0; i<m_pSongs.size(); i++ )
	{
		/* PREFSMAN->m_bAutogenSteps may have changed. */
		m_pSongs[i]->RemoveAutoGenNotes();
		m_pSongs[i]->AddAutoGenNotes();
	}
}

void SongManager::GetAllCourses( vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
			AddTo.push_back( m_pCourses[i] );
}

void SongManager::GetCourses( CourseType ct, vector<Course*> &AddTo, bool bIncludeAutogen )
{
	for( unsigned i=0; i<m_pCourses.size(); i++ )
		if( m_pCourses[i]->GetCourseType() == ct )
			if( bIncludeAutogen || !m_pCourses[i]->m_bIsAutogen )
				AddTo.push_back( m_pCourses[i] );
}

bool SongManager::GetExtraStageInfoFromCourse( bool bExtra2, CString sPreferredGroup,
								   Song*& pSongOut, Steps*& pStepsOut, PlayerOptions& po_out, SongOptions& so_out )
{
	const CString sCourseSuffix = sPreferredGroup + "/" + (bExtra2 ? "extra2" : "extra1") + ".crs";
	CString sCoursePath = SONGS_DIR + sCourseSuffix;

	/* Couldn't find course in DWI path or alternative song folders */
	if( !DoesFileExist(sCoursePath) )
		return false;

	Course course;
	course.LoadFromCRSFile( sCoursePath );
	if( course.GetEstimatedNumStages() <= 0 ) return false;

	Trail *pTrail = course.GetTrail( GAMESTATE->GetCurrentStyle()->m_StepsType );
	if( pTrail->m_vEntries.empty() )
		return false;

	po_out.Init();
	po_out.FromString( pTrail->m_vEntries[0].Modifiers );
	so_out.Init();
	so_out.FromString( pTrail->m_vEntries[0].Modifiers );

	pSongOut = pTrail->m_vEntries[0].pSong;
	pStepsOut = pTrail->m_vEntries[0].pSteps;
	return true;
}

/* Return true if n1 < n2. */
bool CompareNotesPointersForExtra(const Steps *n1, const Steps *n2)
{
	/* Equate CHALLENGE to HARD. */
	Difficulty d1 = min(n1->GetDifficulty(), DIFFICULTY_HARD);
	Difficulty d2 = min(n2->GetDifficulty(), DIFFICULTY_HARD);

	if(d1 < d2) return true;
	if(d1 > d2) return false;
	/* n1 difficulty == n2 difficulty */

	if(StepsUtil::CompareNotesPointersByMeter(n1,n2)) return true;
	if(StepsUtil::CompareNotesPointersByMeter(n2,n1)) return false;
	/* n1 meter == n2 meter */

	return StepsUtil::CompareNotesPointersByRadarValues(n1,n2);
}

void SongManager::GetExtraStageInfo( bool bExtra2, const Style *sd, 
								   Song*& pSongOut, Steps*& pStepsOut, PlayerOptions& po_out, SongOptions& so_out )
{
	CString sGroup = GAMESTATE->m_sPreferredSongGroup;
	if( sGroup == GROUP_ALL_MUSIC )
	{
		if( GAMESTATE->m_pCurSong == NULL )
		{
			/* This normally shouldn't happen, but it's helpful to permit it for testing. */
			LOG->Warn( "GetExtraStageInfo() called in GROUP_ALL_MUSIC, but GAMESTATE->m_pCurSong == NULL" );
			GAMESTATE->m_pCurSong.Set( SONGMAN->GetRandomSong() );
		}
		sGroup = GAMESTATE->m_pCurSong->m_sGroupName;
	}

	ASSERT_M( sGroup != "", ssprintf("%p '%s' '%s'",
		GAMESTATE->m_pCurSong.Get(),
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->GetSongDir().c_str():"",
		GAMESTATE->m_pCurSong? GAMESTATE->m_pCurSong->m_sGroupName.c_str():"") );

	if(GetExtraStageInfoFromCourse(bExtra2, sGroup, pSongOut, pStepsOut, po_out, so_out))
		return;
	
	// Choose a hard song for the extra stage
	Song*	pExtra1Song = NULL;		// the absolute hardest Song and Steps.  Use this for extra stage 1.
	Steps*	pExtra1Notes = NULL;
	Song*	pExtra2Song = NULL;		// a medium-hard Song and Steps.  Use this for extra stage 2.
	Steps*	pExtra2Notes = NULL;
	
	vector<Song*> apSongs;
	SONGMAN->GetSongs( apSongs, sGroup );
	for( unsigned s=0; s<apSongs.size(); s++ )	// foreach song
	{
		Song* pSong = apSongs[s];

		vector<Steps*> apSteps;
		pSong->GetSteps( apSteps, sd->m_StepsType );
		for( unsigned n=0; n<apSteps.size(); n++ )	// foreach Steps
		{
			Steps* pSteps = apSteps[n];

			if( pExtra1Notes == NULL || CompareNotesPointersForExtra(pExtra1Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				pExtra1Song = pSong;
				pExtra1Notes = pSteps;
			}

			// for extra 2, we don't want to choose the hardest notes possible.  So, we'll disgard Steps with meter > 8
			if(	bExtra2  &&  pSteps->GetMeter() > 8 )	
				continue;	// skip
			if( pExtra2Notes == NULL  ||  CompareNotesPointersForExtra(pExtra2Notes,pSteps) )	// pSteps is harder than pHardestNotes
			{
				pExtra2Song = pSong;
				pExtra2Notes = pSteps;
			}
		}
	}

	if( pExtra2Song == NULL  &&  pExtra1Song != NULL )
	{
		pExtra2Song = pExtra1Song;
		pExtra2Notes = pExtra1Notes;
	}

	// If there are any notes at all that match this StepsType, everything should be filled out.
	// Also, it's guaranteed that there is at least one Steps that matches the StepsType because the player
	// had to play something before reaching the extra stage!
	ASSERT( pExtra2Song && pExtra1Song && pExtra2Notes && pExtra1Notes );

	pSongOut = (bExtra2 ? pExtra2Song : pExtra1Song);
	pStepsOut = (bExtra2 ? pExtra2Notes : pExtra1Notes);


	po_out.Init();
	so_out.Init();
	po_out.m_fScrolls[PlayerOptions::SCROLL_REVERSE] = 1;
	po_out.m_fScrollSpeed = 1.5f;
	so_out.m_DrainType = (bExtra2 ? SongOptions::DRAIN_SUDDEN_DEATH : SongOptions::DRAIN_NO_RECOVER);
	po_out.m_fDark = 1;
}

Song* SongManager::GetRandomSong()
{
	if( m_pShuffledSongs.empty() )
		return NULL;

	static int i = 0;

	for( int iThrowAway=0; iThrowAway<100; iThrowAway++ )
	{
		i++;
		wrap( i, m_pShuffledSongs.size() );
		Song *pSong = m_pShuffledSongs[ i ];
		if( pSong->IsTutorial() )
			continue;
		if( UNLOCKMAN->SongIsLocked(pSong) )
			continue;
		return pSong;
	}

	return NULL;
}

Course* SongManager::GetRandomCourse()
{
	if( m_pShuffledCourses.empty() )
		return NULL;

	static int i = 0;

	for( int iThrowAway=0; iThrowAway<100; iThrowAway++ )
	{
		i++;
		wrap( i, m_pShuffledCourses.size() );
		Course *pCourse = m_pShuffledCourses[ i ];
		if( pCourse->m_bIsAutogen && !PREFSMAN->m_bAutogenGroupCourses )
			continue;
		if( pCourse->GetCourseType() == COURSE_TYPE_ENDLESS )
			continue;
		if( UNLOCKMAN->CourseIsLocked(pCourse) )
			continue;
		return pCourse;
	}

	return NULL;
}

Song* SongManager::GetSongFromDir( CString sDir )
{
	if( sDir.Right(1) != "/" )
		sDir += "/";

	sDir.Replace( '\\', '/' );

	for( unsigned int i=0; i<m_pSongs.size(); i++ )
		if( sDir.CompareNoCase(m_pSongs[i]->GetSongDir()) == 0 )
			return m_pSongs[i];

	return NULL;
}

Course* SongManager::GetCourseFromPath( CString sPath )
{
	if( sPath == "" )
		return NULL;

	FOREACH_CONST( Course*, m_pCourses, c )
	{
		if( sPath.CompareNoCase((*c)->m_sPath) == 0 )
			return *c;
	}

	return NULL;
}

Course* SongManager::GetCourseFromName( CString sName )
{
	if( sName == "" )
		return NULL;

	for( unsigned int i=0; i<m_pCourses.size(); i++ )
		if( sName.CompareNoCase(m_pCourses[i]->GetDisplayFullTitle()) == 0 )
			return m_pCourses[i];

	return NULL;
}


/*
 * GetSongDir() contains a path to the song, possibly a full path, eg:
 * Songs\Group\SongName                   or 
 * My Other Song Folder\Group\SongName    or
 * c:\Corny J-pop\Group\SongName
 *
 * Most course group names are "Group\SongName", so we want to
 * match against the last two elements. Let's also support
 * "SongName" alone, since the group is only important when it's
 * potentially ambiguous.
 *
 * Let's *not* support "Songs\Group\SongName" in course files.
 * That's probably a common error, but that would result in
 * course files floating around that only work for people who put
 * songs in "Songs"; we don't want that.
 */

Song *SongManager::FindSong( CString sPath )
{
	sPath.Replace( '\\', '/' );
	CStringArray bits;
	split( sPath, "/", bits );

	if( bits.size() == 1 )
		return FindSong( "", bits[0] );
	else if( bits.size() == 2 )
		return FindSong( bits[0], bits[1] );

	return NULL;	
}

Song *SongManager::FindSong( CString sGroup, CString sSong )
{
	// foreach song
	for( unsigned i = 0; i < m_pSongs.size(); i++ )
	{
		if( m_pSongs[i]->Matches(sGroup, sSong) )
			return m_pSongs[i];
	}

	return NULL;	
}

Course *SongManager::FindCourse( CString sName )
{
	for( unsigned i = 0; i < m_pCourses.size(); i++ )
	{
		if( !sName.CompareNoCase(m_pCourses[i]->GetDisplayFullTitle()) )
			return m_pCourses[i];
	}

	return NULL;
}

void SongManager::UpdateBest()
{
	// update players best
	FOREACH_ProfileSlot( i )
	{
		vector<Song*> &Best = m_pBestSongs[i];
		Best = m_pSongs;

		for ( unsigned j=0; j < Best.size() ; ++j )
		{
			bool bFiltered = false;
			/* Filter out hidden songs. */
			if( Best[j]->GetDisplayed() != Song::SHOW_ALWAYS )
				bFiltered = true;
			/* Filter out locked songs. */
			if( UNLOCKMAN->SongIsLocked(Best[j]) )
				bFiltered = true;
			if( !bFiltered )
				continue;

			/* Remove it. */
			swap( Best[j], Best.back() );
			Best.erase( Best.end()-1 );
			--j;
		}

		SongUtil::SortSongPointerArrayByTitle( m_pBestSongs[i] );
		SongUtil::SortSongPointerArrayByNumPlays( m_pBestSongs[i], i, true );

		FOREACH_CourseType( ct )
		{
			vector<Course*> &vpCourses = m_pBestCourses[i][ct];
			vpCourses.clear();
			GetCourses( ct, vpCourses, PREFSMAN->m_bAutogenGroupCourses );
			CourseUtil::SortCoursePointerArrayByTitle( vpCourses );
			CourseUtil::SortCoursePointerArrayByNumPlays( vpCourses, i, true );
		}
	}
}

void SongManager::UpdateShuffled()
{
	// update shuffled
	m_pShuffledSongs = m_pSongs;
	random_shuffle( m_pShuffledSongs.begin(), m_pShuffledSongs.end() );

	m_pShuffledCourses = m_pCourses;
	random_shuffle( m_pShuffledCourses.begin(), m_pShuffledCourses.end() );
}

void SongManager::SortSongs()
{
	SongUtil::SortSongPointerArrayByTitle( m_pSongs );
}

void SongManager::UpdateRankingCourses()
{
	/* Updating the ranking courses data is fairly expensive
	 * since it involves comparing strings.  Do so sparingly. */
	CStringArray RankingCourses;
	split( THEME->GetMetric("ScreenRanking","CoursesToShow"),",", RankingCourses);

	for( unsigned i=0; i < m_pCourses.size(); i++ )
	{
		if( m_pCourses[i]->GetEstimatedNumStages() > 7 )
			m_pCourses[i]->m_SortOrder_Ranking = 3;
		else
			m_pCourses[i]->m_SortOrder_Ranking = 2;
		
		for( unsigned j = 0; j < RankingCourses.size(); j++ )
			if( !RankingCourses[j].CompareNoCase(m_pCourses[i]->m_sPath) )
				m_pCourses[i]->m_SortOrder_Ranking = 1;
	}
}

void SongManager::LoadAllFromProfileDir( const CString &sProfileDir, ProfileSlot slot )
{
	//
	// Load all .edit files.
	//
	CString sEditsDir = sProfileDir + EDIT_SUBDIR;

	CStringArray asEditsFilesWithPath;
	GetDirListing( sEditsDir+"*.edit", asEditsFilesWithPath, false, true );

	int iNumEditsLoaded = GetNumEditsLoadedFromProfile( slot );
	int size = min( (int) asEditsFilesWithPath.size(), MAX_EDITS_PER_PROFILE - iNumEditsLoaded );

	for( int i=0; i<size; i++ )
	{
		CString fn = asEditsFilesWithPath[i];

		int iBytes = FILEMAN->GetFileSizeInBytes( fn );
		if( iBytes > MAX_EDIT_SIZE_BYTES )
		{
			LOG->Warn( "The file '%s' is unreasonably large.  It won't be loaded.", fn.c_str() );
			continue;
		}

		SMLoader::LoadEdit( fn, slot );
	}
}

int SongManager::GetNumEditsLoadedFromProfile( ProfileSlot slot ) const
{
	int iCount = 0;
	for( unsigned s=0; s<m_pSongs.size(); s++ )
	{
		const Song *pSong = m_pSongs[s];
		vector<Steps*> apSteps;
		pSong->GetSteps( apSteps );

		for( unsigned i = 0; i < apSteps.size(); ++i )
		{
			const Steps *pSteps = apSteps[i];
			if( pSteps->WasLoadedFromProfile() && pSteps->GetLoadedFromProfileSlot() == slot )
				++iCount;
		}
	}
	return iCount;
}

void SongManager::FreeAllLoadedPlayerCourses()
{
	LOG->Trace( "SongManager::FreeAllLoadedPlayerCourses()" );
	vector<Course *> m_pSavedCourses;

	for ( unsigned i = 0; i < m_pCourses.size(); i++ )
		if (! m_pCourses[i]->m_bIsCustomCourse )
			m_pSavedCourses.push_back( m_pCourses[i] );

	m_pCourses.clear();

	for ( unsigned i = 0; i < m_pSavedCourses.size(); i++ )
		m_pCourses.push_back( m_pSavedCourses[i] );
}

void SongManager::FreeAllLoadedPlayerSongs()
{
	// if we don't have any songs to free, don't bother
	if( m_pCustomSongs.empty() )
		return;

	LOG->Trace( "SongManager::FreeAllLoadedPlayerSongs()" );

	/* free the previously loaded song data */
	for( unsigned i = 0; i < m_pCustomSongs.size(); i++ )
		SAFE_DELETE( m_pCustomSongs[i] );
	m_pCustomSongs.clear();

	/* Now, we just need to rebuild m_pSongs from m_pMachineSongs. */
	m_pSongs.clear();
	m_pSongs.insert( m_pSongs.begin(), m_pMachineSongs.begin(), m_pMachineSongs.end() );
}

void SongManager::FreeAllLoadedFromProfile( ProfileSlot slot )
{
	for( unsigned s=0; s<m_pSongs.size(); s++ )
	{
		Song* pSong = m_pSongs[s];
		pSong->FreeAllLoadedFromProfile( slot );
	}

	// After freeing some Steps pointers, the cache will be invalid.
	StepsID::ClearCache();
}

int SongManager::GetNumStepsLoadedFromProfile()
{
	int iCount = 0;
	FOREACH( Song*, m_pSongs, s )
	{
		vector<Steps*> vpAllSteps = (*s)->GetAllSteps();

		FOREACH( Steps*, vpAllSteps, ss )
		{
			if( (*ss)->GetLoadedFromProfile() != PROFILE_SLOT_INVALID )
				iCount++;
		}
	}	

	return iCount;
}


// lua start
#include "LuaBinding.h"

template<class T>
class LunaSongManager : public Luna<T>
{
public:
	LunaSongManager() { LUA->Register( Register ); }

	static int GetAllSongs( T* p, lua_State *L )
	{
		const vector<Song*> &v = p->GetAllSongs();
		LuaHelpers::CreateTableFromArray<Song*>( v, L );
		return 1;
	}
	static int GetAllCourses( T* p, lua_State *L )
	{
		vector<Course*> v;
		p->GetAllCourses( v, BArg(1) );
		LuaHelpers::CreateTableFromArray<Course*>( v, L );
		return 1;
	}
	static int FindSong( T* p, lua_State *L )		{ Song *pS = p->FindSong(SArg(1)); if(pS) pS->PushSelf(L); else lua_pushnil(L); return 1; }
	static int FindCourse( T* p, lua_State *L )		{ Course *pC = p->FindCourse(SArg(1)); if(pC) pC->PushSelf(L); else lua_pushnil(L); return 1; }
	static int GetRandomSong( T* p, lua_State *L )	{ Song *pS = p->GetRandomSong(); if(pS) pS->PushSelf(L); else lua_pushnil(L); return 1; }
	static int GetRandomCourse( T* p, lua_State *L ){ Course *pC = p->GetRandomCourse(); if(pC) pC->PushSelf(L); else lua_pushnil(L); return 1; }

	static void Register(lua_State *L)
	{
		ADD_METHOD( GetAllSongs )
		ADD_METHOD( GetAllCourses )
		ADD_METHOD( FindSong )
		ADD_METHOD( FindCourse )
		ADD_METHOD( GetRandomSong )
		ADD_METHOD( GetRandomCourse )
		Luna<T>::Register( L );

		// Add global singleton if constructed already.  If it's not constructed yet,
		// then we'll register it later when we reinit Lua just before 
		// initializing the display.
		if( SONGMAN )
		{
			lua_pushstring(L, "SONGMAN");
			SONGMAN->PushSelf( L );
			lua_settable(L, LUA_GLOBALSINDEX);
		}
	}
};

LUA_REGISTER_CLASS( SongManager )
// lua end





#include "LuaFunctions.h"

CString GetCurrentSongDisplayTitle()
{
	const Song* pSong = GAMESTATE->m_pCurSong;
	if( pSong == NULL )
		return "";
	return pSong->GetDisplayFullTitle();
}

CString GetCurrentSongDisplayArtist()
{
	const Song* pSong = GAMESTATE->m_pCurSong;
	if( pSong == NULL )
		return "";
	return pSong->GetDisplayArtist();
}

CString GetCurrentSongCredit()
{
	const Song* pSong = GAMESTATE->m_pCurSong;
	if( pSong == NULL )
		return "";
	return pSong->m_sCredit;
}

CString GetCurrentStepsCredits()
{
	const Song* pSong = GAMESTATE->m_pCurSong;
	if( pSong == NULL )
		return "";

	CString s;

	// use a vector and not a set so that ordering is maintained
	vector<Steps*> vpStepsToShow;
	FOREACH_PlayerNumber( p )
	{
		if( !GAMESTATE->IsHumanPlayer(p) )
			continue;	// skip
		
		Steps* pSteps = GAMESTATE->m_pCurSteps[p];
		bool bAlreadyAdded = find( vpStepsToShow.begin(), vpStepsToShow.end(), pSteps ) != vpStepsToShow.end();
		if( !bAlreadyAdded )
			vpStepsToShow.push_back( pSteps );
	}
	for( unsigned i=0; i<vpStepsToShow.size(); i++ )
	{
		Steps* pSteps = vpStepsToShow[i];
		CString sDifficulty = DifficultyToThemedString( pSteps->GetDifficulty() );
		
		// HACK: reset capitalization
		sDifficulty.MakeLower();
		sDifficulty = Capitalize( sDifficulty );
		
		s += sDifficulty + " steps: " + pSteps->GetDescription();
		s += "\n";
	}

	// erase the last newline
	s.erase( s.end()-1 );
	return s;
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
