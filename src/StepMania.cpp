#include "global.h"
#include "StepMania.h"

//
// Rage global classes
//
#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageSoundManager.h"
#include "GameSoundManager.h"
#include "RageInput.h"
#include "RageDisplay.h"

#include "arch/ArchHooks/ArchHooks.h"
#include "arch/LoadingWindow/LoadingWindow.h"
#include "arch/Dialog/Dialog.h"
#include <ctime>

#include "ProductInfo.h"

#if defined(HAVE_SDL)
#include "SDL_utils.h"
#endif

#include "Screen.h"
#include "CodeDetector.h"
#include "CommonMetrics.h"
#include "Game.h"
#include "RageSurface.h"
#include "RageSurface_Load.h"
#include "CatalogXml.h"

#if !defined(SUPPORT_OPENGL) && !defined(SUPPORT_D3D)
#define SUPPORT_OPENGL
#endif

//
// StepMania global classes
//
#include "NoteSkinManager.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "GameState.h"
#include "AnnouncerManager.h"
#include "ProfileManager.h"
#include "MemoryCardManager.h"
#include "GameManager.h"
#include "FontManager.h"
#include "InputMapper.h"
#include "InputQueue.h"
#include "SongCacheIndex.h"
#include "BannerCache.h"
#include "UnlockManager.h"
#include "Bookkeeper.h"
#include "LightsManager.h"
#include "ModelManager.h"
#include "CryptManager.h"
#include "NetworkSyncManager.h"
#include "StatsManager.h"
#include "UserPackManager.h"

// XXX: for I/O error reports
#if !defined(XBOX)
#include "io/ITGIO.h"
#endif

#if defined(XBOX)
#include "Archutils/Xbox/VirtualMemory.h"
#endif

#if defined(WIN32) && !defined(XBOX)
#include <windows.h>
#include "archutils/Win32/VideoDriverInfo.h"
#endif

#if defined(UNIX)
#include <cerrno>
#endif

/* TODO: this toggle probably isn't necessary, since these -should- resolve
 * to the same directory in the VFS. I just don't want to break patch data
 * for arcade cabinets without testing it first... -- vyhd */

#if defined(ITG_ARCADE) && defined(LINUX)
#define PATCH_FILE	"/rootfs/stats/patch/patch.zip"
#else
#define PATCH_FILE	"Data/patch/patch.zip"
#endif

/* If it exists, this dir is mounted as a patch instead of patch.zip */
#define PATCH_DATA_DIR	"Data/patch/patch"

#define ZIPS_DIR "Packages/"

int g_argc = 0;
char **g_argv = NULL;

static bool g_bHasFocus = true;
static bool g_bQuitting = false;

void ReadGamePrefsFromDisk( bool bSwitchToLastPlayedGame );

static RageDisplay::VideoModeParams GetCurVideoModeParams()
{
	return RageDisplay::VideoModeParams(
			PREFSMAN->m_bWindowed,
			PREFSMAN->m_iDisplayWidth,
			PREFSMAN->m_iDisplayHeight,
			PREFSMAN->m_iDisplayColorDepth,
			PREFSMAN->m_iRefreshRate,
			PREFSMAN->m_bVsync,
			PREFSMAN->m_bInterlaced,
			PREFSMAN->m_bSmoothLines,
			PREFSMAN->m_bTrilinearFiltering,
			PREFSMAN->m_bAnisotropicFiltering,
			WINDOW_TITLE,
			THEME->GetPathG("Common","window icon"),
			PREFSMAN->m_bPAL,
			PREFSMAN->m_fDisplayAspectRatio
	);
}

static void StoreActualGraphicOptions( bool initial )
{
	// find out what we actually have
	PREFSMAN->m_bWindowed.Set( DISPLAY->GetVideoModeParams().windowed );
	PREFSMAN->m_iDisplayWidth.Set( DISPLAY->GetVideoModeParams().width );
	PREFSMAN->m_iDisplayHeight.Set( DISPLAY->GetVideoModeParams().height );
	PREFSMAN->m_iDisplayColorDepth.Set( DISPLAY->GetVideoModeParams().bpp );
	PREFSMAN->m_iRefreshRate.Set( DISPLAY->GetVideoModeParams().rate );
	PREFSMAN->m_bVsync.Set( DISPLAY->GetVideoModeParams().vsync );

	CString log = ssprintf("%s %dx%d %d color %d texture %dHz %s %s",
		PREFSMAN->m_bWindowed ? "Windowed" : "Fullscreen",
		(int)PREFSMAN->m_iDisplayWidth, 
		(int)PREFSMAN->m_iDisplayHeight, 
		(int)PREFSMAN->m_iDisplayColorDepth, 
		(int)PREFSMAN->m_iTextureColorDepth, 
		(int)PREFSMAN->m_iRefreshRate,
		PREFSMAN->m_bVsync ? "Vsync" : "NoVsync",
		PREFSMAN->m_bSmoothLines? "AA" : "NoAA" );
	if( initial )
		LOG->Info( "%s", log.c_str() );
	else
		SCREENMAN->SystemMessage( log );

	Dialog::SetWindowed( DISPLAY->GetVideoModeParams().windowed );
}

RageDisplay *CreateDisplay();

static void StartDisplay()
{
	if( DISPLAY != NULL )
		return; // already started

	DISPLAY = CreateDisplay();

	DISPLAY->ChangeCentering(
		PREFSMAN->m_iCenterImageTranslateX, 
		PREFSMAN->m_iCenterImageTranslateY,
		PREFSMAN->m_fCenterImageAddWidth,
		PREFSMAN->m_fCenterImageAddHeight );

	TEXTUREMAN	= new RageTextureManager();
	TEXTUREMAN->SetPrefs( 
		RageTextureManagerPrefs( 
			PREFSMAN->m_iTextureColorDepth, 
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete, 
			PREFSMAN->m_iMaxTextureResolution,
			PREFSMAN->m_bForceMipMaps
			)
		);

	MODELMAN	= new ModelManager;
	MODELMAN->SetPrefs( 
		ModelManagerPrefs(
			PREFSMAN->m_bDelayedModelDelete 
			)
		);
}

void ApplyGraphicOptions()
{ 
	bool bNeedReload = false;

	CString sError = DISPLAY->SetVideoMode( GetCurVideoModeParams(), bNeedReload );
	if( sError != "" )
		RageException::Throw( sError );

	DISPLAY->ChangeCentering(
		PREFSMAN->m_iCenterImageTranslateX, 
		PREFSMAN->m_iCenterImageTranslateY,
		PREFSMAN->m_fCenterImageAddWidth,
		PREFSMAN->m_fCenterImageAddHeight );

	bNeedReload |= TEXTUREMAN->SetPrefs( 
		RageTextureManagerPrefs( 
			PREFSMAN->m_iTextureColorDepth, 
			PREFSMAN->m_iMovieColorDepth,
			PREFSMAN->m_bDelayedTextureDelete, 
			PREFSMAN->m_iMaxTextureResolution,
			PREFSMAN->m_bForceMipMaps
			)
		);

	bNeedReload |= MODELMAN->SetPrefs( 
		ModelManagerPrefs(
			PREFSMAN->m_bDelayedModelDelete 
			)
		);

	if( bNeedReload )
		TEXTUREMAN->ReloadAll();

	StoreActualGraphicOptions( false );

	/* Give the input handlers a chance to re-open devices as necessary. */
	INPUTMAN->WindowReset();
}

/* Shutdown all global singletons.  Note that this may be called partway through
 * initialization, due to an object failing to initialize, in which case some of
 * these may still be NULL. */
void ShutdownGame()
{
	/* First, tell SOUND that we're shutting down.  This signals sound drivers to
	 * stop sounds, which we want to do before any threads that may have started sounds
	 * are closed; this prevents annoying DirectSound glitches and delays. */
	if( SOUNDMAN )
		SOUNDMAN->Shutdown();

	SAFE_DELETE( SCREENMAN );
	SAFE_DELETE( STATSMAN );
	SAFE_DELETE( MESSAGEMAN );
	SAFE_DELETE( NSMAN );
	/* Delete INPUTMAN before the other INPUTFILTER handlers, or an input
	 * driver may try to send a message to INPUTFILTER after we delete it. */
	SAFE_DELETE( INPUTMAN );
	SAFE_DELETE( INPUTQUEUE );
	SAFE_DELETE( INPUTMAPPER );
	SAFE_DELETE( INPUTFILTER );
	SAFE_DELETE( MODELMAN );
	SAFE_DELETE( PROFILEMAN );	// PROFILEMAN needs the songs still loaded
	SAFE_DELETE( UNLOCKMAN );
	SAFE_DELETE( CRYPTMAN );
	SAFE_DELETE( MEMCARDMAN );
	SAFE_DELETE( SONGMAN );
	SAFE_DELETE( BANNERCACHE );
	SAFE_DELETE( SONGINDEX );
	SAFE_DELETE( SOUND ); /* uses GAMESTATE, PREFSMAN */
	SAFE_DELETE( PREFSMAN );
	SAFE_DELETE( GAMESTATE );
	SAFE_DELETE( GAMEMAN );
	SAFE_DELETE( LUA );
	SAFE_DELETE( NOTESKIN );
	SAFE_DELETE( THEME );
	SAFE_DELETE( ANNOUNCER );
	SAFE_DELETE( BOOKKEEPER );
	SAFE_DELETE( LIGHTSMAN );
	SAFE_DELETE( SOUNDMAN );
	SAFE_DELETE( FONT );
	SAFE_DELETE( TEXTUREMAN );
	SAFE_DELETE( DISPLAY );
	Dialog::Shutdown();
	SAFE_DELETE( LOG );
	SAFE_DELETE( FILEMAN );
	SAFE_DELETE( UPACKMAN );
	SAFE_DELETE( HOOKS );
}

/* Cleanly shut down, show a dialog and exit the game.  We don't go back
 * up the call stack, to avoid having to use exceptions. */
void HandleException( CString error )
{
	if( g_bAutoRestart )
		HOOKS->RestartProgram();

	/* Shut down first, so we exit graphics mode before trying to open a dialog. */
	ShutdownGame();

	/* Throw up a pretty error dialog. */
	Dialog::Error( error );

	exit(1);
}
	
void ExitGame()
{
	g_bQuitting = true;
}

void ResetGame()
{
	GAMESTATE->Reset();
	
	if( !THEME->DoesThemeExist( THEME->GetCurThemeName() ) )
	{
		CString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
		if( THEME->DoesThemeExist( sGameName ) )
			THEME->SwitchThemeAndLanguage( sGameName, THEME->GetCurLanguage() );
		else
			THEME->SwitchThemeAndLanguage( PREFSMAN->m_sTheme.GetDefault(), THEME->GetCurLanguage() );
		TEXTUREMAN->DoDelayedDelete();
	}
	SaveGamePrefsToDisk();

	//
	// update last seen joysticks
	//
	vector<InputDevice> vDevices;
	vector<CString> vDescriptions;
	INPUTMAN->GetDevicesAndDescriptions(vDevices,vDescriptions);
	CString sInputDevices = join( ",", vDescriptions );

	if( PREFSMAN->m_sLastSeenInputDevices.Get() != sInputDevices )
	{
		LOG->Info( "Input devices changed from '%s' to '%s'.", PREFSMAN->m_sLastSeenInputDevices.Get().c_str(), sInputDevices.c_str() );

		if( PREFSMAN->m_bAutoMapOnJoyChange )
		{
			LOG->Info( "Remapping joysticks." );
			INPUTMAPPER->AutoMapJoysticksForCurrentGame();
			INPUTMAPPER->SaveMappingsToDisk();
		}

		PREFSMAN->m_sLastSeenInputDevices.Set( sInputDevices );
	}
}

static void GameLoop();

static bool ChangeAppPri()
{
	if( PREFSMAN->m_BoostAppPriority.Get() == PrefsManager::BOOST_NO )
		return false;

	// if using NTPAD don't boost or else input is laggy
#if defined(_WINDOWS)
	if( PREFSMAN->m_BoostAppPriority == PrefsManager::BOOST_AUTO )
	{
		vector<InputDevice> vDevices;
		vector<CString> vDescriptions;
		INPUTMAN->GetDevicesAndDescriptions(vDevices,vDescriptions);
		CString sInputDevices = join( ",", vDescriptions );
		if( sInputDevices.find("NTPAD") != -1 )
		{
			LOG->Trace( "Using NTPAD.  Don't boost priority." );
			return false;
		}
	}
#endif

	/* If -1 and this is a debug build, don't.  It makes the debugger sluggish. */
#ifdef DEBUG
	if( PREFSMAN->m_BoostAppPriority == PrefsManager::BOOST_AUTO )
		return false;
#endif

	return true;
}

static void CheckSettings()
{
#if defined(WIN32)
	/* Has the amount of memory changed? */
	MEMORYSTATUS mem;
	GlobalMemoryStatus(&mem);

	const int Memory = mem.dwTotalPhys / (1024*1024);

	if( PREFSMAN->m_iLastSeenMemory == Memory )
		return;
	
	LOG->Trace( "Memory changed from %i to %i; settings changed", PREFSMAN->m_iLastSeenMemory, Memory );
	PREFSMAN->m_iLastSeenMemory.Set( Memory );

	/* Let's consider 128-meg systems low-memory, and 256-meg systems high-memory.
	 * Cut off at 192.  This is somewhat conservative; many 128-meg systems can
	 * deal with higher memory profile settings, but some can't. 
	 *
	 * Actually, Windows lops off a meg or two; cut off a little lower to treat
	 * 192-meg systems as high-memory. */
	const bool HighMemory = (Memory >= 190);
	const bool LowMemory = (Memory < 100); /* 64 and 96-meg systems */

	/* Two memory-consuming features that we can disable are texture caching and
	 * preloaded banners.  Texture caching can use a lot of memory; disable it for
	 * low-memory systems. */
	PREFSMAN->m_bDelayedTextureDelete.Set( HighMemory );

	/* Preloaded banners takes about 9k per song. Although it's smaller than the
	 * actual song data, it still adds up with a lot of songs. Disable it for 64-meg
	 * systems. */
	PREFSMAN->m_BannerCache.Set( LowMemory ? PrefsManager::BNCACHE_OFF:PrefsManager::BNCACHE_LOW_RES_PRELOAD );

	PREFSMAN->SaveGlobalPrefsToDisk();
#endif
}

#if defined(WIN32)
#include "RageDisplay_D3D.h"
#endif

#if defined(SUPPORT_OPENGL)
#include "RageDisplay_OGL.h"
#endif

#include "RageDisplay_Null.h"



struct VideoCardDefaults
{
	const char *szDriverRegex;
	const char *szVideoRenderers;
	int iWidth;
	int iHeight;
	int iDisplayColor;
	int iTextureColor;
	int iMovieColor;
	int iTextureSize;
	bool bSmoothLines;
} const g_VideoCardDefaults[] = 
{
	{
		"Xbox",
		"d3d,opengl",
		600,400,
		32,32,32,
		2048,
		true
	},
	{
		"Voodoo *5",
		"d3d,opengl",	// received 3 reports of opengl crashing. -Chris
		640,480,
		32,32,32,
		2048,
		true	// accelerated
	},
	{
		"Voodoo|3dfx", /* all other Voodoos: some drivers don't identify which one */
		"d3d,opengl",
		640,480,
		16,16,16,
		256,
		false	// broken, causes black screen
	},
	{
		"Radeon.* 7|Wonder 7500|ArcadeVGA",	// Radeon 7xxx, RADEON Mobility 7500
		"d3d,opengl",	// movie texture performance is terrible in OpenGL, but fine in D3D.
		640,480,
		16,16,16,
		2048,
		true	// accelerated
	},
	{
		"GeForce|Radeon|Wonder 9",
		"opengl,d3d",
		640,480,
		32,32,32,	// 32 bit textures are faster to load
		2048,
		true	// hardware accelerated
	},
	{
		"TNT|Vanta|M64",
		"opengl,d3d",
		640,480,
		16,16,16,	// Athlon 1.2+TNT demonstration w/ movies: 70fps w/ 32bit textures, 86fps w/ 16bit textures
		2048,
		true	// hardware accelerated
	},
	{
		"G200|G250|G400",
		"d3d,opengl",
		640,480,
		16,16,16,
		2048,
		false	// broken, causes black screen
	},
	{
		"Savage",
		"d3d",
			// OpenGL is unusable on my Savage IV with even the latest drivers.  
			// It draws 30 frames of gibberish then crashes.  This happens even with
			// simple NeHe demos.  -Chris
		640,480,
		16,16,16,
		2048,
		false
	},
	{
		"XPERT@PLAY|IIC|RAGE PRO|RAGE LT PRO",	// Rage Pro chip, Rage IIC chip
		"d3d",
			// OpenGL is not hardware accelerated, despite the fact that the 
			// drivers come with an ICD.  Also, the WinXP driver performance 
			// is terrible and supports only 640.  The ATI driver is usable.
			// -Chris
		320,240,	// lower resolution for 60fps.  In-box WinXP driver doesn't support 400x300.
		16,16,16,
		256,
		false
	},
	{
		"RAGE MOBILITY-M1",
		"d3d,opengl",	// Vertex alpha is broken in OpenGL, but not D3D. -Chris
		400,300,	// lower resolution for 60fps
		16,16,16,
		256,
		false
	},
	{
		"Mobility M3",	// ATI Rage Mobility 128 (AKA "M3")
		"d3d,opengl",	// bad movie texture performance in opengl
		640,480,
		16,16,16,
		1024,
		false
	},
#if 0
	{
		/* success report:
		 * Video driver: IntelR 82845G/GL/GE/PE/GV Graphics Controller [Intel Corporation]
		 * 6.14.10.3865, 7-1-2004 [pci\ven_8086&dev_2562] */
		/* 6.14.10.3889 failed (corrupted text); back to D3D */
		"Intel.*82845.*",
		"opengl,d3d",
		640,480,
		16,16,16,
		1024,
		true
	},
#endif
	{
		"Intel.*82810|Intel.*82815",
		"opengl,d3d",// OpenGL is 50%+ faster than D3D w/ latest Intel drivers.  -Chris
		512,384,	// lower resolution for 60fps
		16,16,16,
		512,
		false
	},
	{
		"Intel*Extreme Graphics",
		"d3d",	// OpenGL blue screens w/ XP drivers from 6-21-2002
		640,480,
		16,16,16,	// slow at 32bpp
		1024,
		false
	},
	{
		"Intel.*", /* fallback: all unknown Intel cards to D3D, since Intel is notoriously bad at OpenGL */
		"d3d,opengl",
		640,480,
		16,16,16,
		1024,
		false
	},
	{
		// Cards that have problems with OpenGL:
		// ASSERT fail somewhere in RageDisplay_OpenGL "Trident Video Accelerator CyberBlade"
		// bug 764499: ASSERT fail after glDeleteTextures for "SiS 650_651_740"
		// bug 764830: ASSERT fail after glDeleteTextures for "VIA Tech VT8361/VT8601 Graphics Controller"
		// bug 791950: AV in glsis630!DrvSwapBuffers for "SiS 630/730"
		"Trident Video Accelerator CyberBlade|VIA.*VT|SiS 6*",
		"d3d,opengl",
		640,480,
		16,16,16,
		2048,
		false
	},
	{
		/* Unconfirmed texture problems on this; let's try D3D, since it's a VIA/S3
		 * chipset. */
		"VIA/S3G KM400/KN400",
		"d3d,opengl",
		640,480,
		16,16,16,
		2048,
		false
	},
	{
		"OpenGL",	// This matches all drivers in Mac and Linux. -Chris
		"opengl",
		640,480,
		16,16,16,
		2048,
		true		// Right now, they've got to have NVidia or ATi Cards anyway..
	},
	{
		// Default graphics settings used for all cards that don't match above.
		// This must be the very last entry!
		"",
		"opengl,d3d",
		640,480,
		16,16,16,
		2048,
		false  // AA is slow on some cards, so let's selectively enable HW accelerated cards.
	},
};


static CString GetVideoDriverName()
{
#if defined(_WINDOWS)
	return GetPrimaryVideoDriverName();
#elif defined(_XBOX)
	return "Xbox";
#else
    return "OpenGL";
#endif
}

static void CheckVideoDefaultSettings()
{
	// Video card changed since last run
	CString sVideoDriver = GetVideoDriverName();
	
	LOG->Trace( "Last seen video driver: " + PREFSMAN->m_sLastSeenVideoDriver.Get() );

	const VideoCardDefaults* pDefaults = NULL;
	
	for( unsigned i=0; i<ARRAYLEN(g_VideoCardDefaults); i++ )
	{
		pDefaults = &g_VideoCardDefaults[i];

		CString sDriverRegex = pDefaults->szDriverRegex;
		Regex regex( sDriverRegex );
		if( regex.Compare(sVideoDriver) )
		{
			LOG->Trace( "Card matches '%s'.", sDriverRegex.size()? sDriverRegex.c_str():"(unknown card)" );
			break;
		}
	}

	ASSERT( pDefaults );	// we must have matched at least one

	CString sVideoRenderers = pDefaults->szVideoRenderers;

	bool SetDefaultVideoParams=false;
	if( PREFSMAN->m_sVideoRenderers.Get() == "" )
	{
		SetDefaultVideoParams = true;
		LOG->Trace( "Applying defaults for %s.", sVideoDriver.c_str() );
	}
	else if( PREFSMAN->m_sLastSeenVideoDriver.Get() != sVideoDriver ) 
	{
		SetDefaultVideoParams = true;
		LOG->Trace( "Video card has changed from %s to %s.  Applying new defaults.", PREFSMAN->m_sLastSeenVideoDriver.Get().c_str(), sVideoDriver.c_str() );
	}
		
	if( SetDefaultVideoParams )
	{
		PREFSMAN->m_sVideoRenderers.Set( pDefaults->szVideoRenderers );
		PREFSMAN->m_iDisplayWidth.Set( pDefaults->iWidth );
		PREFSMAN->m_iDisplayHeight.Set( pDefaults->iHeight );
		PREFSMAN->m_iDisplayColorDepth.Set( pDefaults->iDisplayColor );
		PREFSMAN->m_iTextureColorDepth.Set( pDefaults->iTextureColor );
		PREFSMAN->m_iMovieColorDepth.Set( pDefaults->iMovieColor );
		PREFSMAN->m_iMaxTextureResolution.Set( pDefaults->iTextureSize );
		PREFSMAN->m_bSmoothLines.Set( pDefaults->bSmoothLines );

		// Update last seen video card
		PREFSMAN->m_sLastSeenVideoDriver.Set( GetVideoDriverName() );
	}
	else if( PREFSMAN->m_sVideoRenderers.Get().CompareNoCase(sVideoRenderers) )
	{
		LOG->Warn("Video renderer list has been changed from '%s' to '%s'",
				sVideoRenderers.c_str(), PREFSMAN->m_sVideoRenderers.Get().c_str() );
		return;
	}

	LOG->Info( "Video renderers: '%s'", PREFSMAN->m_sVideoRenderers.Get().c_str() );
}

RageDisplay *CreateDisplay()
{
	/* We never want to bother users with having to decide which API to use.
	 *
	 * Some cards simply are too troublesome with OpenGL to ever use it, eg. Voodoos.
	 * If D3D8 isn't installed on those, complain and refuse to run (by default).
	 * For others, always use OpenGL.  Allow forcing to D3D as an advanced option.
	 *
	 * If we're missing acceleration when we load D3D8 due to a card being in the
	 * D3D list, it means we need drivers and that they do exist.
	 *
	 * If we try to load OpenGL and we're missing acceleration, it may mean:
	 *  1. We're missing drivers, and they just need upgrading.
	 *  2. The card doesn't have drivers, and it should be using D3D8.  In other words,
	 *     it needs an entry in this table.
	 *  3. The card doesn't have drivers for either.  (Sorry, no S3 868s.)  Can't play.
	 * 
	 * In this case, fail to load; don't silently fall back on D3D.  We don't want
	 * people unknowingly using D3D8 with old drivers (and reporting obscure bugs
	 * due to driver problems).  We'll probably get bug reports for all three types.
	 * #2 is the only case that's actually a bug.
	 *
	 * Actually, right now we're falling back.  I'm not sure which behavior is better.
	 */

	CheckVideoDefaultSettings();

	RageDisplay::VideoModeParams params(GetCurVideoModeParams());

	CString error = "There was an error while initializing your video card.\n\n"
		"   PLEASE DO NOT FILE THIS ERROR AS A BUG!\n\n"
		"Video Driver: "+GetVideoDriverName()+"\n\n";

	CStringArray asRenderers;
	split( PREFSMAN->m_sVideoRenderers, ",", asRenderers, true );

	if( asRenderers.empty() )
		RageException::Throw("No video renderers attempted.");

	for( unsigned i=0; i<asRenderers.size(); i++ )
	{
		CString sRenderer = asRenderers[i];

		if( sRenderer.CompareNoCase("opengl")==0 )
		{
#if defined(SUPPORT_OPENGL)
			RageDisplay_OGL *pRet = new RageDisplay_OGL;
			CString sError = pRet->Init( params, PREFSMAN->m_bAllowUnacceleratedRenderer );
			if( sError == "" )
				return pRet;
			error += "Initializing OpenGL...\n" + sError;
			delete pRet;
#endif
		}
		else if( sRenderer.CompareNoCase("d3d")==0 )
		{
#if defined(SUPPORT_D3D)
			RageDisplay_D3D *pRet = new RageDisplay_D3D;
			CString sError = pRet->Init( params );
			if( sError == "" )
				return pRet;
			error += "Initializing Direct3D...\n" + sError;
			delete pRet;
#endif
		}
		else if( sRenderer.CompareNoCase("null")==0 )
			return new RageDisplay_Null( params );
		else
			RageException::Throw("Unknown video renderer value: %s", sRenderer.c_str() );
	}

	RageException::Throw( error );
}

#define GAMEPREFS_INI_PATH "Data/GamePrefs.ini"
#define STATIC_INI_PATH "Data/Static.ini"

void ChangeCurrentGame( const Game* g )
{
	ASSERT( g );

	SaveGamePrefsToDisk();
	INPUTMAPPER->SaveMappingsToDisk();	// save mappings before switching the game

	GAMESTATE->m_pCurGame = g;

	/* Load this game's preferences.  If we just set an unavailable game type, this
	 * will change it back to the default. */
	ReadGamePrefsFromDisk( false );

	/* Save the newly-selected game. */
	SaveGamePrefsToDisk();

	/* Load keymaps for the new game. */
	INPUTMAPPER->ReadMappingsFromDisk();
}

void ReadGamePrefsFromDisk( bool bSwitchToLastPlayedGame )
{
	ASSERT( GAMESTATE );
	ASSERT( ANNOUNCER );
	ASSERT( THEME );
	ASSERT( GAMESTATE );

	IniFile ini;
	ini.ReadFile( GAMEPREFS_INI_PATH );	// it's OK if this fails
	ini.ReadFile( STATIC_INI_PATH );	// it's OK if this fails, too

	if( bSwitchToLastPlayedGame )
	{
		ASSERT( GAMEMAN != NULL );
		CString sGame;
		GAMESTATE->m_pCurGame = NULL;
		if( ini.GetValue("Options", "Game", sGame) )
			GAMESTATE->m_pCurGame = GAMEMAN->StringToGameType( sGame );
	}

	/* If the active game type isn't actually available, revert to the default. */
	if( GAMESTATE->m_pCurGame == NULL || !GAMEMAN->IsGameEnabled( GAMESTATE->m_pCurGame ) )
	{
		if( GAMESTATE->m_pCurGame != NULL )
			LOG->Warn( "Default note skin for \"%s\" missing, reverting to \"%s\"",
				GAMESTATE->m_pCurGame->m_szName, GAMEMAN->GetDefaultGame()->m_szName );
		GAMESTATE->m_pCurGame = GAMEMAN->GetDefaultGame();
	}

	/* Load keymaps for the new game. */
	if( INPUTMAPPER )
		INPUTMAPPER->ReadMappingsFromDisk();

	/* If the default isn't available, our default note skin is messed up. */
	if( !GAMEMAN->IsGameEnabled( GAMESTATE->m_pCurGame ) )
		RageException::Throw( "Default note skin for \"%s\" missing", GAMESTATE->m_pCurGame->m_szName );

	CString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
	CString sAnnouncer = sGameName;
	CString sTheme = sGameName;
	CString sNoteSkin = sGameName;
	CString sDefaultModifiers;

	// if these calls fail, the three strings will keep the initial values set above.
	ini.GetValue( sGameName, "Announcer",			sAnnouncer );
	ini.GetValue( sGameName, "Theme",				sTheme );
	ini.GetValue( sGameName, "DefaultModifiers",	sDefaultModifiers );
	PREFSMAN->m_sTheme.Set( sTheme );
	PREFSMAN->m_sDefaultModifiers.Set( sDefaultModifiers );

	// it's OK to call these functions with names that don't exist.
	ANNOUNCER->SwitchAnnouncer( sAnnouncer );
	THEME->SwitchThemeAndLanguage( sTheme, PREFSMAN->m_sLanguage );

//	NOTESKIN->SwitchNoteSkin( sNoteSkin );
}


void SaveGamePrefsToDisk()
{
	if( !GAMESTATE )
		return;

	CString sGameName = GAMESTATE->GetCurrentGame()->m_szName;
	IniFile ini;
	ini.ReadFile( GAMEPREFS_INI_PATH );	// it's OK if this fails

	ini.SetValue( sGameName, "Announcer",			ANNOUNCER->GetCurAnnouncerName() );
	ini.SetValue( sGameName, "Theme",				PREFSMAN->m_sTheme );
	ini.SetValue( sGameName, "DefaultModifiers",	PREFSMAN->m_sDefaultModifiers );
	ini.SetValue( "Options", "Game",				(CString)GAMESTATE->GetCurrentGame()->m_szName );

	ini.WriteFile( GAMEPREFS_INI_PATH );
}

static void MountTreeOfZips( const CString &dir, bool recurse = true )
{
	vector<CString> dirs;
	dirs.push_back( dir );

	while( dirs.size() )
	{
		CString path = dirs.back();
		dirs.pop_back();

#if !defined(XBOX)
		// Xbox doesn't detect directories properly, so we'll ignore this
		if( !IsADirectory(path) )
			continue;
#endif

		vector<CString> zips;
		GetDirListing( path + "/*.zip", zips, false, true );
		GetDirListing( path + "/*.smzip", zips, false, true );

		for( unsigned i = 0; i < zips.size(); ++i ) /**/
		{
			if( !IsAFile(zips[i]) )
				continue;

			LOG->Trace( "VFS: found %s", zips[i].c_str() );
			FILEMAN->Mount( "zip", zips[i], "/" );
		}

		if (recurse) GetDirListing( path + "/*", dirs, true, true ); /**/
	}
}

static void WriteLogHeader()
{
	LOG->Info( ProductInfo::GetFullVersion() );

	LOG->Info( "Compiled %s (build %s)",
		ProductInfo::GetBuildDate().c_str(),
		ProductInfo::GetBuildRevision().c_str()
	);

	time_t cur_time;
	time(&cur_time);
	struct tm now;
	localtime_r( &cur_time, &now );

	LOG->Info( "Log starting %.4d-%.2d-%.2d %.2d:%.2d:%.2d", 
		1900+now.tm_year, now.tm_mon+1, now.tm_mday, now.tm_hour, now.tm_min, now.tm_sec );
	LOG->Trace( " " );

	if( g_argc > 1 )
	{
		CString args;
		for( int i = 1; i < g_argc; ++i )
		{
			if( i>1 )
				args += " ";

			// surround all params with some marker, as they might have whitespace.
			// using [[ and ]], as they are not likely to be in the params.
			args += ssprintf( "[[%s]]", g_argv[i] );
		}
		LOG->Info( "Command line args (count=%d): %s", (g_argc - 1), args.c_str());
	}
}

static void ApplyLogPreferences()
{
	LOG->SetShowLogOutput( PREFSMAN->m_bShowLogOutput );
	LOG->SetLogToDisk( PREFSMAN->m_bLogToDisk );
	LOG->SetInfoToDisk( true );
	LOG->SetFlushing( PREFSMAN->m_bForceLogFlush );
	Checkpoints::LogCheckpoints( PREFSMAN->m_bLogCheckpoints );
}

/* Search for the commandline argument given; eg. "test" searches for
 * the option "--test".  All commandline arguments are getopt_long style:
 * --foo; short arguments (-x) are not supported.  (As commandline arguments
 * are not intended for common, general use, having short options isn't
 * needed.)  If argument is non-NULL, accept an argument. */
bool GetCommandlineArgument( const CString &option, CString *argument, int iIndex )
{
	const CString optstr = "--" + option;
	
	for( int arg = 1; arg < g_argc; ++arg )
	{
		const CString CurArgument = g_argv[arg];

		const size_t i = CurArgument.find( "=" );
		CString CurOption = CurArgument.substr(0,i);
		if( CurOption.CompareNoCase(optstr) )
			continue; /* no match */

		/* Found it. */
		if( iIndex )
		{
			--iIndex;
			continue;
		}

		if( argument )
		{
			if( i != CString::npos )
				*argument = CurArgument.substr( i+1 );
			else
				*argument = "";
		}
		
		return true;
	}

	return false;
}

#ifdef _XBOX
void __cdecl main()
#else
int main(int argc, char* argv[])
#endif
{
#if defined(XBOX)
	int argc = 1;
	char *argv[] = {"default.xbe"};
#endif

	g_argc = argc;
	g_argv = argv;

	/* Set up arch hooks first.  This may set up crash handling. */
	HOOKS = MakeArchHooks();

#if !defined(DEBUG)
	/* Tricky: for other exceptions, we want a backtrace.  To do this in Windows,
	 * we need to catch the exception and force a crash.  The call stack is still
	 * there, and gets picked up by the crash handler.  (If we don't catch it, we'll
	 * get a generic, useless "abnormal termination" dialog.)  In Linux, if we do this
	 * we'll only get main() on the stack, but if we don't catch the exception, it'll
	 * just work.  So, only catch generic exceptions in Windows. */
#if defined(_WINDOWS)
	try { /* exception */
#endif

#endif

	/* Almost everything uses this to read and write files.  Load this early. */
	FILEMAN = new RageFileManager( argv[0] );
	FILEMAN->MountInitialFilesystems();

	/* Set this up next.  Do this early, since it's needed for RageException::Throw. */
	LOG			= new RageLog();

	/* Whew--we should be able to crash safely now! */

	//
	// load preferences and mount any alternative trees.
	//
	PREFSMAN	= new PrefsManager;

	ApplyLogPreferences();

#if defined(XBOX)
	if(PREFSMAN->m_bEnableVirtualMemory)
	{
		if(!vmem_Manager.Init(1024 * 1024 * PREFSMAN->m_iPageFileSize, 1024 * PREFSMAN->m_iPageSize, 1024 * PREFSMAN->m_iPageThreshold))
			return;
	}

	/* Logging the virtual memory manager seems to crash on exit, so it should be enabled only
	 * for debugging. */
	vmem_Manager.SetLogging(PREFSMAN->m_bLogVirtualMemory);
#endif

	WriteLogHeader();

	/* Set up alternative filesystem trees. */
	if( PREFSMAN->m_sAdditionalFolders.Get() != "" )
	{
		CStringArray dirs;
		split( PREFSMAN->m_sAdditionalFolders, ",", dirs, true );
		for( unsigned i=0; i < dirs.size(); i++)
			FILEMAN->Mount( "dir", dirs[i], "/" );
	}
	if( PREFSMAN->m_sAdditionalSongFolders.Get() != "" )
	{
		CStringArray dirs;
		split( PREFSMAN->m_sAdditionalSongFolders, ",", dirs, true );
		for( unsigned i=0; i < dirs.size(); i++)
			FILEMAN->Mount( "dir", dirs[i], "/Songs" );
	}

	MountTreeOfZips( "Packages/", false );

	/* Mount patch data. If PATCH_DATA_DIR exists, use it instead 
	 * of PATCH_FILE (easier testing and mucking about, etc.) */
	if( IsADirectory(PATCH_DATA_DIR) )
	{
		LOG->Info( "VFS: mounting Data/patch/patch/." );

		// IsADirectory checks against the VFS, but we need to mount against a physical path
		CString physicalPath = FILEMAN->ResolvePath( PATCH_DATA_DIR );
		FILEMAN->Mount( "dirro", physicalPath, "/", false );
	}
	else if( IsAFile(PATCH_FILE) )
	{
		LOG->Info( "VFS: mounting patch.zip." );

		CString patchFileVirtualDir = Dirname(PATCH_FILE);
		CString patchDirPhysicalPath = FILEMAN->ResolvePath( patchFileVirtualDir );

		FILEMAN->Mount( "patch", patchDirPhysicalPath, "/Patch" );
		FILEMAN->Mount( "zip", "/Patch/patch.zip", "/", false );
	}
	else
	{
		LOG->Info("VFS: No patch data found");
	}

	LOG->Info("======= MOUNTPOINTS =========");
	vector<RageFileManager::DriverLocation> mymounts;
	FILEMAN->GetLoadedDrivers(mymounts);
	for (unsigned i = 0; i < mymounts.size(); i++)
		LOG->Info("%s ..... %s ..... %s", mymounts[i].Type.c_str(), mymounts[i].Root.c_str(), mymounts[i].MountPoint.c_str() );
	LOG->Info("=============================");

	/* One of the above filesystems might contain files that affect preferences, eg Data/Static.ini.
	 * Re-read preferences. */
	PREFSMAN->ReadGlobalPrefsFromDisk();
	ApplyLogPreferences();
	
#if defined(HAVE_SDL)
	SetupSDL();
#endif

	/* initialize the pack manager and mount all data after prefs loading.
	 * this helps to make sure that packages can't mess with core data. */
	UPACKMAN = new UserPackManager();
	UPACKMAN->MountAll();

	/* This needs PREFSMAN. */
	Dialog::Init();

	//
	// Create game objects
	//

	LUA			= new LuaManager;
	GAMESTATE	= new GameState;

	/* This requires PREFSMAN, for g_bShowLoadingWindow. */
	LoadingWindow *loading_window = LoadingWindow::Create();

	srand( time(NULL) );	// seed number generator

	/* Do this early, so we have debugging output if anything else fails.  LOG and
	 * Dialog must be set up first.  It shouldn't take long, but it might take a
	 * little time; do this after the LoadingWindow is shown, since we don't want
	 * that to appear delayed. */
	HOOKS->DumpDebugInfo();

#if defined(HAVE_TLS)
	LOG->Info( "TLS is %savailable", RageThread::GetSupportsTLS()? "":"not " );
#endif

	CheckSettings();

	GAMEMAN		= new GameManager;
	THEME		= new ThemeManager;
	ANNOUNCER	= new AnnouncerManager;
	NOTESKIN	= new NoteSkinManager;


	/* Set up the theme and announcer, and switch to the last game type. */
	ReadGamePrefsFromDisk( true );

	{
		CString sSection = "Preferences";
		IniFile staticIni;
		GetCommandlineArgument( "Type", &sSection );
		THEME->LoadPreferencesFromSection( sSection );


		// think of it as a bootstrap
		staticIni.ReadFile( STATIC_INI_PATH );
		PREFSMAN->ReadGlobalPrefsFromIni( staticIni );
	}

	if( PREFSMAN->m_iSoundWriteAhead )
		LOG->Info( "Sound writeahead has been overridden to %i", PREFSMAN->m_iSoundWriteAhead.Get() );
	SOUNDMAN	= new RageSoundManager;
	SOUNDMAN->Init();
	SOUNDMAN->SetPrefs( PREFSMAN->GetSoundVolume() );
	SOUND		= new GameSoundManager;
	BOOKKEEPER	= new Bookkeeper;
	LIGHTSMAN	= new LightsManager;
	INPUTFILTER	= new InputFilter;
	INPUTMAPPER	= new InputMapper;
	INPUTQUEUE	= new InputQueue;
	SONGINDEX	= new SongCacheIndex;
	BANNERCACHE = new BannerCache;
	
	/* depends on SONGINDEX: */
	SONGMAN		= new SongManager();

	SONGMAN->InitAll( loading_window );		// this takes a long time

	CRYPTMAN	= new CryptManager;	// need to do this before ProfileMan
	MEMCARDMAN	= new MemoryCardManager;
	PROFILEMAN	= new ProfileManager;
	PROFILEMAN->Init();				// must load after SONGMAN
	UNLOCKMAN	= new UnlockManager;


	{
		/* Now that THEME is loaded, load the icon for the current theme into the
		 * loading window. */
		CString sError;
		RageSurface *pIcon = RageSurfaceUtils::LoadFile( THEME->GetPathG( "Common", "window icon" ), sError );
		if( pIcon )
			loading_window->SetIcon( pIcon );
		delete pIcon;
	}

	/* This shouldn't need to be here; if it's taking long enough that this is
	 * even visible, we should be fixing it, not showing a progress display. */
	SaveCatalogXml( loading_window );
	
	NSMAN 		= new NetworkSyncManager( loading_window ); 
	MESSAGEMAN	= new MessageManager;
	STATSMAN	= new StatsManager;

	SAFE_DELETE( loading_window );		// destroy this before init'ing Display

	StartDisplay();

	StoreActualGraphicOptions( true );

	SONGMAN->PreloadSongImages();

	/* This initializes objects that change the SDL event mask, and has other
	 * dependencies on the SDL video subsystem, so it must be initialized after DISPLAY. */
	INPUTMAN	= new RageInput;

	// These things depend on the TextureManager, so do them after!
	FONT		= new FontManager;
	SCREENMAN	= new ScreenManager;

	// UGLY: Now that all global singletons are constructed so that they, let them
	// all register with Lua.
	//
	// ResetState wipes out method tables.   We need to call UpdateLuaGlobals, so
	// we re-run scripts that may add to them.
	THEME->UpdateLuaGlobals();

	/* People may want to do something else while songs are loading, so do
	 * this after loading songs. */
	if( ChangeAppPri() )
		HOOKS->BoostPriority();

	ResetGame();
	SCREENMAN->SetNewScreen( INITIAL_SCREEN );

	CodeDetector::RefreshCacheItems();

	/* Initialize which courses are ranking courses here. */
	SONGMAN->UpdateRankingCourses();

	if( GetCommandlineArgument("netip") )
		NSMAN->DisplayStartupStatus();	// If we're using networking show what happened

	PREFSMAN->SaveGlobalPrefsToDisk();
	SaveGamePrefsToDisk();
    
	/* Run the main loop. */
	GameLoop();

	PREFSMAN->SaveGlobalPrefsToDisk();
	SaveGamePrefsToDisk();

	ShutdownGame();

#if !defined(DEBUG) && defined(_WINDOWS)
	}
	catch( const exception &e )
	{
		/* This can be things like calling std::string::reserve(-1), or out of memory. */
		FAIL_M( e.what() );
	}
#endif
	
#ifndef _XBOX
	return 0;
#endif
}

CString SaveScreenshot( CString sDir, bool bSaveCompressed, bool bMakeSignature, int iIndex )
{
	//
	// Find a file name for the screenshot
	//
	FlushDirCache();

	vector<CString> files;
	GetDirListing( sDir + "screen*", files, false, false );
	sort( files.begin(), files.end() );

	/* Files should be of the form "screen######.xxx".  Ignore the extension; find
	 * the last file of this form, and use the next number.  This way, we don't
	 * write the same screenshot number for different formats (screen00011.bmp,
	 * screen00011.jpg), and we always increase from the end, so if screen00003.jpg
	 * is deleted, we won't fill in the hole (which makes screenshots hard to find). */
	if( iIndex == -1 ) 
	{
		iIndex = 0;

		for( int i = files.size()-1; i >= 0; --i )
		{
			static Regex re( "^screen([0-9]{5})\\....$" );
			vector<CString> matches;
			if( !re.Compare( files[i], matches ) )
				continue;

			ASSERT( matches.size() == 1 );
			iIndex = atoi( matches[0] )+1;
			break;
		}
	}

	//
	// Save the screenshot.  If writing lossy to a memcard, use SAVE_LOSSY_LOW_QUAL, so we
	// don't eat up lots of space.
	//
	RageDisplay::GraphicsFileFormat fmt;
	if( bSaveCompressed && MEMCARDMAN->PathIsMemCard(sDir) )
		fmt = RageDisplay::SAVE_LOSSY_LOW_QUAL;
	else if( bSaveCompressed )
		fmt = RageDisplay::SAVE_LOSSY_HIGH_QUAL;
	else
		fmt = RageDisplay::SAVE_LOSSLESS;

	CString sFileName = ssprintf( "screen%05d.%s",iIndex,bSaveCompressed ? "jpg" : "bmp" );
	CString sPath = sDir+sFileName;
	bool bResult = DISPLAY->SaveScreenshot( sPath, fmt );
	if( !bResult )
	{
		SCREENMAN->PlayInvalidSound();
		return "";
	}

	SCREENMAN->PlayScreenshotSound();

	// We wrote a new file, and SignFile won't pick it up unless we invalidate
	// the Dir cache.  There's got to be a better way of doing this than 
	// thowing out all the cache. -Chris
	FlushDirCache();

	if( PREFSMAN->m_bSignProfileData && bMakeSignature )
		CryptManager::SignFileToFile( sPath );

	return sFileName;
}

void InsertCoin( int iNum, bool bRecord )
{
	GAMESTATE->m_iCoins += iNum;
	LOG->Trace("%i coins inserted, %i needed to play", GAMESTATE->m_iCoins, PREFSMAN->m_iCoinsPerCredit.Get() );

	// record software insertions separately
	if( bRecord )
	{
		BOOKKEEPER->CoinInserted();
		LIGHTSMAN->PulseCoinCounter();
	}
	else
	{
		BOOKKEEPER->ServiceCoinInserted();
	}

	SCREENMAN->RefreshCreditsMessages();
	SCREENMAN->PlayCoinSound();
	MESSAGEMAN->Broadcast( MESSAGE_COIN_INSERTED );
}

void InsertCredit( bool bRecord )
{
	InsertCoin( PREFSMAN->m_iCoinsPerCredit, bRecord );
}

// Clears all credits currently in the machine.
// Useful for techs testing the coin mechs.
// -- Matt1360
void ClearCredits()
{
	int iLost = GAMESTATE->m_iCoins;
	GAMESTATE->m_iCoins = 0;
	SCREENMAN->RefreshCreditsMessages();
	LOG->Trace( "%i Coins discarded" , iLost );
}

/* Returns true if the key has been handled and should be discarded, false if
 * the key should be sent on to screens. */
bool HandleGlobalInputs( DeviceInput DeviceI, InputEventType type, GameInput GameI, MenuInput MenuI, StyleInput StyleI )
{
	/* None of the globals keys act on types other than FIRST_PRESS */
	if( type != IET_FIRST_PRESS && MenuI.button != MENU_BUTTON_OPERATOR) 
		return false;

	switch( MenuI.button )
	{
	case MENU_BUTTON_OPERATOR:

		/* Global operator key, to get quick access to the options menu. Don't
		 * do this if we're on a "system menu", which includes the editor
		 * (to prevent quitting without storing changes). */
		if( SCREENMAN->GetTopScreen()->GetScreenType() != system_menu && (type==IET_SLOW_REPEAT||type==IET_FAST_REPEAT))
		{
			SCREENMAN->DeletePreparedScreens();
			SCREENMAN->SystemMessage( "Service switch pressed" );
			GAMESTATE->Reset();
			SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		}
		return true;

	case MENU_BUTTON_COIN:
		/* Handle a coin insertion. */
		if( GAMESTATE->m_bEditing )	// no coins while editing
		{
			LOG->Trace( "Ignored coin insertion (editing)" );
			break;
		}
		InsertCoin();
		return false;	// Attract need to know because they go to TitleMenu on > 1 credit
	}

#ifndef DARWIN
	if(DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F4))
	{
		if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RALT)) ||
			INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LALT)) )
		{
			// pressed Alt+F4
			ExitGame();
			return true;
		}
	}
#else
	if(DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_Cq))
	{
		if(INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RMETA)) ||
			INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LMETA)))
		{
			// pressed CMD-Q
			ExitGame();
			return true;
		}
	}
#endif

	/* The default Windows message handler will capture the desktop window upon
	 * pressing PrntScrn, or will capture the foregroud with focus upon pressing
	 * Alt+PrntScrn.  Windows will do this whether or not we save a screenshot 
	 * ourself by dumping the frame buffer.  */
	/* Pressing F13 on an Apple keyboard sends KEY_PRINT.
	 * However, notebooks don't have F13. Use cmd-F12 then*/
	// "if pressing PrintScreen and not pressing Alt"
	if( (DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_PRTSC) &&
		 !INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RALT)) &&
		 !INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LALT))) ||
	    (DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_F12) &&
		 (INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RMETA)) ||
             INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LMETA)))))
	{
		// If holding LShift save uncompressed, else save compressed
		bool bSaveCompressed = !INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT) );
		SaveScreenshot( "Screenshots/", bSaveCompressed, false );
		return true;	// handled
	}

	if(DeviceI == DeviceInput(DEVICE_KEYBOARD, KEY_ENTER))
	{
		if( INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_RALT)) ||
			INPUTFILTER->IsBeingPressed(DeviceInput(DEVICE_KEYBOARD, KEY_LALT)) )
		{
			/* alt-enter */
			PREFSMAN->m_bWindowed.Set( !PREFSMAN->m_bWindowed );
			ApplyGraphicOptions();
			return true;
		}
	}

	return false;
}

static void HandleInputEvents(float fDeltaTime)
{
	INPUTFILTER->Update( fDeltaTime );
	
	/* Hack: If the topmost screen hasn't been updated yet, don't process input, since
	 * we must not send inputs to a screen that hasn't at least had one update yet. (The
	 * first Update should be the very first thing a screen gets.)  We'll process it next
	 * time.  Do call Update above, so the inputs are read and timestamped. */
	if( SCREENMAN->GetTopScreen()->IsFirstUpdate() )
		return;

	static InputEventArray ieArray;
	ieArray.clear();	// empty the array
	INPUTFILTER->GetInputEvents( ieArray );

	/* If we don't have focus, discard input. */
	if( !g_bHasFocus )
		return;

	for( unsigned i=0; i<ieArray.size(); i++ )
	{
		DeviceInput DeviceI = (DeviceInput)ieArray[i];
		InputEventType type = ieArray[i].type;
		GameInput GameI;
		MenuInput MenuI;
		StyleInput StyleI;

		INPUTMAPPER->DeviceToGame( DeviceI, GameI );
		
		if( GameI.IsValid()  &&  type == IET_FIRST_PRESS )
			INPUTQUEUE->RememberInput( GameI );
		if( GameI.IsValid() )
		{
			INPUTMAPPER->GameToMenu( GameI, MenuI );
			INPUTMAPPER->GameToStyle( GameI, StyleI );
		}

		// HACK:  Numlock is read is being pressed if the NumLock light is on.
		// Filter out all NumLock repeat messages
		/* XXX: Is this still needed?  If so, it should probably be done in the
		 * affected input driver. */
//		if( DeviceI.device == DEVICE_KEYBOARD && DeviceI.button == KEY_NUMLOCK && type != IET_FIRST_PRESS )
//			continue;	// skip

		if( HandleGlobalInputs(DeviceI, type, GameI, MenuI, StyleI ) )
			continue;	// skip
		
		// check back in event mode
		if( GAMESTATE->IsEventMode() &&
			CodeDetector::EnteredCode(GameI.controller,CODE_BACK_IN_EVENT_MODE) )
		{
			MenuI.player = PLAYER_1;
			MenuI.button = MENU_BUTTON_BACK;
		}

		SCREENMAN->Input( DeviceI, type, GameI, MenuI, StyleI );
	}
}

void FocusChanged( bool bHasFocus )
{
	if( g_bHasFocus == bHasFocus )
		return;

	g_bHasFocus = bHasFocus;

	LOG->Trace( "App %s focus", g_bHasFocus? "has":"doesn't have" );

	/* If we lose focus, we may lose input events, especially key releases. */
	INPUTFILTER->Reset();

	if( ChangeAppPri() )
	{
		if( g_bHasFocus )
			HOOKS->BoostPriority();
		else
			HOOKS->UnBoostPriority();
	}
}

static void CheckSkips( float fDeltaTime )
{
	if( !PREFSMAN->m_bLogSkips )
		return;

	static int iLastFPS = 0;
	int iThisFPS = DISPLAY->GetFPS();

	/* If vsync is on, and we have a solid framerate (vsync == refresh and we've sustained this
	 * for at least one frame), we expect the amount of time for the last frame to be 1/FPS. */

	if( iThisFPS != DISPLAY->GetVideoModeParams().rate || iThisFPS != iLastFPS )
	{
		iLastFPS = iThisFPS;
		return;
	}

	const float fExpectedTime = 1.0f / iThisFPS;
	const float fDifference = fDeltaTime - fExpectedTime;
	if( fabsf(fDifference) > 0.002f && fabsf(fDifference) < 0.100f )
		LOG->Trace("GameLoop timer skip: %i FPS, expected %.3f, got %.3f (%.3f difference)",
			iThisFPS, fExpectedTime, fDeltaTime, fDifference );
}

static void GameLoop()
{
	RageTimer timer;

	while(!g_bQuitting)
	{

		/*
		 * Update
		 */

#ifndef XBOX
		// XXX: the Iow InputHandler acts as a singleton removed from
		// the game loop. It needs an external error monitor because
		// it waits until reconnecting to continue...can we improve this?
		if( !ITGIO::m_sInputError.empty() )
		{
			SCREENMAN->SystemMessage( ITGIO::m_sInputError );
			ITGIO::m_sInputError.clear();
		}
#endif

		float fDeltaTime = timer.GetDeltaTime();

		if( PREFSMAN->m_fConstantUpdateDeltaSeconds > 0 )
			fDeltaTime = PREFSMAN->m_fConstantUpdateDeltaSeconds;
		
		CheckSkips( fDeltaTime );

		if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_TAB) ) ) {
			if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_ACCENT) ) )
				fDeltaTime = 0; /* both; stop time */
			else
				fDeltaTime *= 4;
		}
		else if( INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_ACCENT) ) )
		{
			fDeltaTime /= 4;
		}

		DISPLAY->Update( fDeltaTime );

		/* Update SOUNDMAN early (before any RageSound::GetPosition calls), to flush position data. */
		SOUNDMAN->Update( fDeltaTime );

		/* Update song beat information -before- calling update on all the classes that
		 * depend on it.  If you don't do this first, the classes are all acting on old 
		 * information and will lag.  (but no longer fatally, due to timestamping -glenn) */
		SOUND->Update( fDeltaTime );
		TEXTUREMAN->Update( fDeltaTime );
		GAMESTATE->Update( fDeltaTime );
		SCREENMAN->Update( fDeltaTime );
		MEMCARDMAN->Update();
		NSMAN->Update( fDeltaTime );

		/* Important:  Process input AFTER updating game logic, or input will be acting on song beat from last frame */
		HandleInputEvents( fDeltaTime );

		LIGHTSMAN->Update( fDeltaTime );
		HOOKS->Update( fDeltaTime );

		/*
		 * Render
		 */
		SCREENMAN->Draw();

		/* If we don't have focus, give up lots of CPU. */
		if( !g_bHasFocus )
			usleep( 10000 );// give some time to other processes and threads
#if defined(_WINDOWS)
		/* In Windows, we want to give up some CPU for other threads.  Most OS's do
		 * this more intelligently. */
		else
			usleep( 1000 );	// give some time to other processes and threads
#endif
	}

	GAMESTATE->EndGame();
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

