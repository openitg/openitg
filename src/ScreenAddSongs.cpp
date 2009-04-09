#include "global.h"
#include "HelpDisplay.h"
#include "ScreenAddSongs.h"
#include "Screen.h"
#include "ScreenPrompt.h"
#include "ScreenWithMenuElements.h"
#include "PlayerNumber.h"
#include "SongManager.h"
#include "RageThreads.h"
#include "MemoryCardManager.h"
#include "ScreenDimensions.h"
#include "RageFileManager.h"
#include "RageFileDriverZip.h"
#include "CryptManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "ProfileManager.h"
#include "RageUtil_FileDB.h" /* defines FileSet */
#include "RageFileDriverDirect.h" /* defines DirectFilenameDB */
#include "arch/ArchHooks/ArchHooks.h"

static RageMutex MountMutex("ITGDataMount");

REGISTER_SCREEN_CLASS( ScreenAddSongs );

AutoScreenMessage( SM_ConfirmAddZips );
AutoScreenMessage( SM_AnswerConfirmAddZips );

PlayerNumber g_CurrentPlayer;

ScreenAddSongs::ScreenAddSongs( CString sName ) : ScreenWithMenuElements( sName )
{
	m_bRestart = false;
	m_bPrompt = false;
	MEMCARDMAN->UnlockCards();
}

ScreenAddSongs::~ScreenAddSongs()
{
	LOG->Trace( "ScreenAddSongs::~ScreenAddSongs()" );
	m_bStopThread = true;
	if (m_PlayerSongLoadThread.IsCreated())
		m_PlayerSongLoadThread.Wait();

	FOREACH_PlayerNumber( pn )
		MEMCARDMAN->UnmountCard( pn );

	OFF_COMMAND( m_AddedZipList );
	OFF_COMMAND( m_AddableZipSelection );

	if ( m_bRestart )
		HOOKS->SystemReboot();
}

void ScreenAddSongs::LoadAddedZips()
{
	GetDirListing( "/AdditionalSongs/*.zip", m_asAddedZips ); /**/
}

int InitSASSongThread( void *pSAS )
{
	((ScreenAddSongs*)pSAS)->StartSongThread();
	return 1;
}

void ScreenAddSongs::Init()
{
	map<CString,CString>::iterator iter;

	/////// XXX Temporarily disabled until I code in an OptionsList for each package list /////
	SCREENMAN->SystemMessage("Adding songs screen is still in its alpha stage, so it is disabled for now.");
	SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
	return;
	///////////////////////////////////////////////////////////////////////////////////

	ScreenWithMenuElements::Init();


	LoadAddedZips();

	m_AddedZipList.SetName( "LoadedSongList" );
	m_AddedZipList.LoadFromFont( THEME->GetPathF( "ScreenAddSongs", "text" ) );
	SET_XY_AND_ON_COMMAND( m_AddedZipList );
	m_AddedZipList.SetText( join("\n", m_asAddedZips) );
	this->AddChild( &m_AddedZipList );

	m_AddableZipSelection.SetName( "AddableSongList" );
	m_AddableZipSelection.LoadFromFont( THEME->GetPathF( "ScreenAddSongs", "text" ) );
	m_AddableZipSelection.SetText( THEME->GetMetric(m_sName,"AddableSongListHelpText") );
	SET_XY_AND_ON_COMMAND( m_AddableZipSelection );
	this->AddChild( &m_AddableZipSelection );

	m_Disclaimer.SetName( "Disclaimer" );
	m_Disclaimer.LoadFromFont( THEME->GetPathF( "ScreenAddSongs", "text" ) );
	m_Disclaimer.SetText( THEME->GetMetric(m_sName, "DisclaimerText") );
	SET_XY_AND_ON_COMMAND( m_Disclaimer );
	this->AddChild( &m_Disclaimer );

	this->SortByDrawOrder();

	m_bStopThread = false;
	m_PlayerSongLoadThread.SetName( "Song Add Thread" );
	m_PlayerSongLoadThread.Create( InitSASSongThread, this );
}

void ScreenAddSongs::StartSongThread()
{
	while ( !m_bStopThread )
	{
		bool bLaunchPrompt = false;
		if (m_bPrompt)
		{
			usleep( 1000 );
			continue;
		}

		FOREACH_PlayerNumber( pn )
		{
			MemoryCardState PCardState = MEMCARDMAN->GetCardState(pn);
			map<CString,CString>::iterator iter;
			if (PCardState == MEMORY_CARD_STATE_READY && !m_bCardMounted[pn])
			{
				if( !PROFILEMAN->LoadProfileFromMemoryCard(pn) || !PROFILEMAN->IsPersistentProfile(pn) ) continue;
				m_AddableZipSelection.SetText( ssprintf("Loading Player %d songs...\n", pn+1) );
				m_asAddableZips[pn].clear();
				MEMCARDMAN->LockCards();
				MEMCARDMAN->MountCard(pn, 999);
				
				CStringArray asZips;
				CString sDir = PROFILEMAN->GetProfileDir(pn) + "/AdditionalSongs";
				GetDirListing( sDir+"/*.zip", asZips ); /**/
				for (unsigned i = 0; i < asZips.size(); i++)
				{
					bool bAdd = true;

					// don't add duplicate zips
					FOREACH_CONST( CString, m_asAddedZips, iter )
					{
#if defined(WIN32)
						// Windows: no case sensitivity, don't take chances
						if ( iter->CompareNoCase(asZips[i]) == 0 ) bAdd = false;
#else
						if ( *iter == asZips[i] ) bAdd = false; // same file names?
#endif
					}
					
					if (!bAdd) continue;
					m_asAddableZips[pn].push_back( asZips[i] );
				}
				
				MEMCARDMAN->UnmountCard(pn);
				MEMCARDMAN->UnlockCards();
				m_bCardMounted[pn] = true;
				g_CurrentPlayer = pn;
				bLaunchPrompt = true;
			}
			if (PCardState == MEMORY_CARD_STATE_NO_CARD && m_bCardMounted[pn])
			{
				LOG->Debug("Unmounting memory card for player %d", pn+1);
				m_asAddableZips[pn].clear();
				m_bCardMounted[pn] = false;
			}
		}
	
		if ( m_asAddableZips[PLAYER_1].size() + m_asAddableZips[PLAYER_2].size() > 0 )
		{
			CStringArray asAddableZips;

			FOREACH( CString, m_asAddableZips[PLAYER_1], iter )
			{
				asAddableZips.push_back( *iter );
			}
			FOREACH( CString, m_asAddableZips[PLAYER_2], iter )
			{
				asAddableZips.push_back( *iter );
			}

			m_AddableZipSelection.SetText( join("\n", asAddableZips ) );

			if (bLaunchPrompt)
			{
				this->PostScreenMessage( SM_ConfirmAddZips, 0.0f );
				m_bPrompt = true;
			}
		}
		else
			m_AddableZipSelection.SetText( "Please insert memory card with additional songs for transfer" );
		usleep( 1000 );
	}
}

void ScreenAddSongs::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
}

void ScreenAddSongs::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore
	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

CString g_CurXferFile;

// shamelessly copied from vyhd's function in ScreenSelectMusic
void UpdateXferProgress( float fPercent )
{
	LOG->Trace( "UpdateXferProgress( %f )", fPercent );
	CString sMessage = ssprintf( "Please wait ...\n%u%%\n\n%s\n", (int)fPercent, g_CurXferFile.c_str() );
	SCREENMAN->OverlayMessage( sMessage );
	SCREENMAN->Draw();
}

/* Folders not allowed in zip file root */
#define NUM_BLACKLIST_FOLDERS 2
static CString g_asBlacklistedFolders[] = { "Data", "Program" };

void ScreenAddSongs::HandleScreenMessage( const ScreenMessage SM )
{
	if ( SM == SM_ConfirmAddZips )
	{
		SCREENMAN->Prompt( SM_AnswerConfirmAddZips, 
			m_AddableZipSelection.GetText() + "\n\nProceed to add song packs to machine?",
		PROMPT_YES_NO, ANSWER_NO );
	}
	if ( SM == SM_AnswerConfirmAddZips )
	{
		bool bSuccess = false, bBreakEarly = false, bSkip = false;
		CString sError;

		m_bPrompt = false;
		if (ScreenPrompt::s_LastAnswer == ANSWER_NO)
		{
			LOG->Debug("SM_AnswerConfirmAddZips: returning...");
			return;
		}
		m_bStopThread = true;
		m_PlayerSongLoadThread.Wait();

		MountMutex.Lock();
#if defined(LINUX) && defined(ITG_ARCADE)
		system( "mount -o remount,rw /itgdata" );
#endif
		MEMCARDMAN->LockCards();
		MEMCARDMAN->MountCard(g_CurrentPlayer, 99999);
		for( unsigned i = 0; i < m_asAddableZips[g_CurrentPlayer].size(); i++ )
		{
			CStringArray asRootFiles;
			RageFileDriverZip *pZip = new RageFileDriverZip;

			bBreakEarly = false;
			bSkip = false;

			g_CurXferFile = PROFILEMAN->GetProfileDir(g_CurrentPlayer) + "/AdditionalSongs/" + m_asAddableZips[g_CurrentPlayer][i];
			if ( !pZip->Load( g_CurXferFile ) )
			{
				SCREENMAN->SystemMessage( ssprintf("Skipping %s (corrupt zip file)", m_asAddableZips[g_CurrentPlayer][i].c_str()) );
				SAFE_DELETE(pZip);
				continue;
			}

			/* Sanity checks */
			pZip->GetDirListing( "*", asRootFiles, false, false );

			LOG->Debug("%s: root entries: %s", g_CurXferFile.c_str(), join( ", ", asRootFiles ).c_str()); // XXX

			FOREACH( CString, asRootFiles, sRootFile )
			{
				bool bpBreak = false;
				// do not allow actual files in the root of the zip, too sploity
				if ( pZip->GetFileType(*sRootFile) == RageFileManager::TYPE_FILE )
				{
					SCREENMAN->SystemMessage( ssprintf("Skipping %s (loose file %s not allowed in root of zip)", m_asAddableZips[g_CurrentPlayer][i].c_str(), sRootFile->c_str()) );
					SAFE_DELETE(pZip);
					bSkip = true;
					break;
				}

				// do not allow zips that have protected folders to be added --infamouspat
				//
				// XXX: should Themes/ really be in the blacklist?
				for( unsigned f = 0; f < NUM_BLACKLIST_FOLDERS; f++ )
				{
					if ( !sRootFile->CompareNoCase(g_asBlacklistedFolders[f]) )
					{
						SCREENMAN->SystemMessage( ssprintf("Skipping %s (directory %s is not allowed in root of zip)", m_asAddableZips[g_CurrentPlayer][i].c_str(), sRootFile->c_str()) );
						SAFE_DELETE(pZip);
						bSkip = true;
						bpBreak = true;
						break;
					}
					if (bpBreak) break;
				}
			}
			if (bSkip) continue;

			// sanity checks completed, proceeding...
			SAFE_DELETE(pZip);

			if (!CopyWithProgress(g_CurXferFile, "/AdditionalSongs/" + m_asAddableZips[g_CurrentPlayer][i], UpdateXferProgress, sError) )
			{
				SCREENMAN->SystemMessage( "Transfer error: " + sError );
				bBreakEarly = true;
				// TODO: delete remnant transfer contents
				break;
			}
			if (bBreakEarly) break;
		}
		MEMCARDMAN->UnmountCard(g_CurrentPlayer);
		MEMCARDMAN->UnlockCards();
#if defined(LINUX) && defined(ITG_ARCADE)
		sync();
		system( "mount -o remount,ro /itgdata" );
#endif
		MountMutex.Unlock();

		if (!bBreakEarly) bSuccess = true;
		SCREENMAN->HideOverlayMessage();
		SCREENMAN->ZeroNextUpdate();
		// hmm...
		if (bSuccess)
		{
			m_AddableZipSelection.SetText(
				"The song folders have been successfully added to the machine\n"
				"Press enter to restart..." );
			m_bRestart = true;
			return;
		}

		// recreate the song accept thread if xfer failed or user declined to add songs
		m_bStopThread = false;
		m_PlayerSongLoadThread.Create( InitSASSongThread, this );
		
	}
	switch( SM )
	{
	case SM_GoToNextScreen:
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		break;
	}
}

void ScreenAddSongs::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		this->PlayCommand( "Off" );
		this->StartTransitioning( SM_GoToPrevScreen );
	}
}

void ScreenAddSongs::MenuStart( PlayerNumber pn )
{
	MenuBack(pn);
}

void ScreenAddSongs::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

