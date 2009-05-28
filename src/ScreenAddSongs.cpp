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
#include "CodeDetector.h"
#include "RageFileDriverZip.h"
#include "CryptManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "ProfileManager.h"
#include "RageUtil_FileDB.h" /* defines FileSet */
#include "RageFileDriverDirect.h" /* defines DirectFilenameDB */
#include "arch/ArchHooks/ArchHooks.h"
#include "arch/Dialog/DialogDriver.h"

#define NEXT_SCREEN					THEME->GetMetric (m_sName,"NextScreen")
#define PREV_SCREEN					THEME->GetMetric (m_sName,"PrevScreen")

static RageMutex MountMutex("ITGDataMount");

REGISTER_SCREEN_CLASS( ScreenAddSongs );

AutoScreenMessage( SM_ConfirmAddZip );
AutoScreenMessage( SM_AnswerConfirmAddZip );
AutoScreenMessage( SM_ConfirmDeleteZip );
AutoScreenMessage( SM_AnswerConfirmDeleteZip );
AutoScreenMessage( SM_LinkedMenuChange );

ScreenAddSongs::ScreenAddSongs( CString sName ) : ScreenWithMenuElements( sName )
{
	m_bRestart = false;
	m_bPrompt = false;
	m_CurPlayer = PLAYER_INVALID;
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
	ScreenWithMenuElements::Init();

	LoadAddedZips();

	m_AddedZips.SetName( "LinkedOptionsMenuAddedZips" );
	m_USBZips.SetName( "LinkedOptionsMenuUSBZips" );
	m_Exit.SetName( "LinkedOptionsMenuSASExit" );

	m_USBZips.Load( NULL, &m_AddedZips );
	m_AddedZips.Load( &m_USBZips, &m_Exit );
	m_Exit.Load( &m_AddedZips, NULL );

	m_USBZips.SetMenuChangeScreenMessage( SM_LinkedMenuChange );
	m_AddedZips.SetMenuChangeScreenMessage( SM_LinkedMenuChange );
	m_Exit.SetMenuChangeScreenMessage( SM_LinkedMenuChange ); // why not..

	this->AddChild( &m_AddedZips );
	this->AddChild( &m_USBZips );
	this->AddChild( &m_Exit );

	SET_XY_AND_ON_COMMAND( m_AddedZips );
	SET_XY_AND_ON_COMMAND( m_USBZips );
	SET_XY_AND_ON_COMMAND( m_Exit );

	m_Disclaimer.SetName( "Disclaimer" );
	m_Disclaimer.LoadFromFont( THEME->GetPathF( "ScreenAddSongs", "text" ) );
	m_Disclaimer.SetText( THEME->GetMetric(m_sName, "DisclaimerText") );
	SET_XY_AND_ON_COMMAND( m_Disclaimer );
	this->AddChild( &m_Disclaimer );

	this->SortByDrawOrder();

	m_AddedZips.SetChoices( m_asAddedZips );

	/*
	CStringArray sDummyChoices;
	sDummyChoices.push_back( "blah1" );
	sDummyChoices.push_back( "blah2" );
	sDummyChoices.push_back( "blah3" );
	sDummyChoices.push_back( "blah4" );
	sDummyChoices.push_back( "blah5" );
	sDummyChoices.push_back( "blah6" );
	sDummyChoices.push_back( "blah7" );
	m_USBZips.SetChoices( sDummyChoices );*/

	{
		CStringArray asExit;
		asExit.push_back( "Exit" );
		m_Exit.SetChoices( asExit );
	}

	m_Exit.Focus();
	m_pCurLOM = &m_Exit;
	
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

		if ( m_CurPlayer != PLAYER_INVALID && MEMCARDMAN->GetCardState(m_CurPlayer) != MEMORY_CARD_STATE_READY )
		{
			CStringArray asBlankChoices;
			m_USBZips.SetChoices( asBlankChoices );
			m_CurPlayer = PLAYER_INVALID;
			continue;
		}

		FOREACH_PlayerNumber( pn )
		{
			if ( m_CurPlayer != PLAYER_INVALID ) break;
			if ( MEMCARDMAN->GetCardState(pn) == MEMORY_CARD_STATE_READY )
			{
				bool bSuccessfulMount = MEMCARDMAN->MountCard(pn);
				if (!bSuccessfulMount) continue;
				CString sProfileDir = MEM_CARD_MOUNT_POINT[pn];
				if (sProfileDir.empty())
				{
					MEMCARDMAN->UnmountCard(pn);
					continue;
				}
				CString sPlayerAdditionalSongsDir = sProfileDir + "/AdditionalSongs";
				CStringArray sUSBZips;
				GetDirListing( sPlayerAdditionalSongsDir+"/*.zip", sUSBZips, false, false );
				MEMCARDMAN->UnmountCard(pn);
				m_USBZips.SetChoices( sUSBZips );
				m_CurPlayer = pn;
			}
		}

		usleep( 1000 );
	}
}

void ScreenAddSongs::Update( float fDeltaTime )
{
	ScreenWithMenuElements::Update( fDeltaTime );
}

void ScreenAddSongs::HandleMessage( const CString &sMessage )
{
	if ( sMessage == "LinkedMenuChange" )
	{
		m_pCurLOM = m_pCurLOM->SwitchToNextMenu();
		return;
	}
	ScreenWithMenuElements::HandleMessage( sMessage );
}

void ScreenAddSongs::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore
	switch( MenuI.button )
	{
	case MENU_BUTTON_LEFT:
	case MENU_BUTTON_RIGHT:
		m_pCurLOM->Input(DeviceI, type, GameI, MenuI, StyleI, m_pCurLOM);
	}

	bool bContinueWithStart = true;
	if ( CodeDetector::EnteredCode( GameI.controller, CODE_LINKED_MENU_SWITCH1 ) ||
		CodeDetector::EnteredCode( GameI.controller, CODE_LINKED_MENU_SWITCH2 ) )
	{
		const MenuInput nMenuI( MenuI.player, MENU_BUTTON_SELECT );
		m_pCurLOM->Input(DeviceI, type, GameI, nMenuI, StyleI, m_pCurLOM);
		bContinueWithStart = false;
	}

	if ( MenuI.button == MENU_BUTTON_START && bContinueWithStart )
	{
		if ( m_pCurLOM->GetName() == "LinkedOptionsMenuSASExit" )
		{
			this->PostScreenMessage( SM_GoToNextScreen, 0.0f);
		}
		else if ( m_pCurLOM->GetName() == "LinkedOptionsMenuUSBZips" )
		{
			this->PostScreenMessage( SM_ConfirmAddZip, 0.0f );
		}
		else if ( m_pCurLOM->GetName() == "LinkedOptionsMenuAddedZips" )
		{
			this->PostScreenMessage( SM_ConfirmDeleteZip, 0.0f );
		}
	}
	ScreenWithMenuElements::Input( DeviceI, type, GameI, MenuI, StyleI );
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
#define NUM_BLACKLIST_FOLDERS 4
static CString g_asBlacklistedFolders[] = { "Data", "Program", "Themes/default", "Themes/home" };


void ScreenAddSongs::HandleScreenMessage( const ScreenMessage SM )
{
	if ( SM == SM_LinkedMenuChange )
	{
		Dialog::OK( "SM_LinkedMenuChange" );
		m_pCurLOM = m_pCurLOM->SwitchToNextMenu();
		return;
	}
	if ( SM == SM_ConfirmDeleteZip )
	{
		SCREENMAN->Prompt( SM_AnswerConfirmDeleteZip, 
			"Proceed to delete pack from machine?",
			PROMPT_YES_NO, ANSWER_NO );
	}
	if ( SM == SM_AnswerConfirmDeleteZip )
	{
		if (ScreenPrompt::s_LastAnswer == ANSWER_NO)
			return;
		CString sSelection = m_AddedZips.GetCurrentSelection();
		CString sToDelete = "/AdditionalSongs/" + sSelection;
		FILEMAN->Unmount("zip", sToDelete, "/Songs");
		FILEMAN->Unmount("zip", sToDelete, "/");
		bool bSuccess = FILEMAN->Remove( sToDelete );
		if (bSuccess)
		{
			m_bRestart = true;
			FILEMAN->FlushDirCache( "/AdditionalSongs" );
			m_asAddedZips.clear();
			LoadAddedZips();
			m_AddedZips.SetChoices( m_asAddedZips );
		}
		else
		{
			SCREENMAN->SystemMessage( "Failed to delete zip file from machine.\n"
								"The harddrive may be corrupt or have wrong permissions set for the destination folder.");
		}
	}
	if ( SM == SM_ConfirmAddZip )
	{
		SCREENMAN->Prompt( SM_AnswerConfirmAddZip, 
			"Proceed to add pack to machine?",
			PROMPT_YES_NO, ANSWER_NO );
	}
	if ( SM == SM_AnswerConfirmAddZip )
	{
		bool bSuccess = false, bBreakEarly = false, bSkip = false;
		CString sError;

		m_bPrompt = false;
		if (ScreenPrompt::s_LastAnswer == ANSWER_NO)
			return;

		m_bStopThread = true;
		m_PlayerSongLoadThread.Wait();

		MountMutex.Lock();
#if defined(LINUX) && defined(ITG_ARCADE)
		system( "mount -o remount,rw /itgdata" );
#endif
		MEMCARDMAN->LockCards();
		MEMCARDMAN->MountCard(m_CurPlayer, 99999);
		CString sSelection = m_USBZips.GetCurrentSelection();
		{
			CStringArray asRootFiles;
			RageFileDriverZip *pZip = new RageFileDriverZip;

			bBreakEarly = false;
			bSkip = false;

			g_CurXferFile = MEM_CARD_MOUNT_POINT[m_CurPlayer] + "/AdditionalSongs/" + sSelection;
			if ( !pZip->Load( g_CurXferFile ) )
			{
				SCREENMAN->SystemMessage( ssprintf("Skipping %s (corrupt zip file)", sSelection.c_str()) );
				SAFE_DELETE(pZip);
				MEMCARDMAN->UnmountCard(m_CurPlayer);
				MEMCARDMAN->UnlockCards();
				MountMutex.Unlock();
				m_bStopThread = false;
				m_PlayerSongLoadThread.Create( InitSASSongThread, this );
				return;
			}

			/* Sanity checks */
			for( unsigned m = 0; m < m_asAddedZips.size(); m++ )
			{
				if ( sSelection == m_asAddedZips[m] ) // zip is already on hard drive
				{
					SCREENMAN->SystemMessage(sSelection + " is already on the machine.");
					MEMCARDMAN->UnmountCard(m_CurPlayer);
					MEMCARDMAN->UnlockCards();
					MountMutex.Unlock();
					m_bStopThread = false;
					m_PlayerSongLoadThread.Create( InitSASSongThread, this );
					return;
				}
			}

			pZip->GetDirListing( "*", asRootFiles, false, false );

			LOG->Trace("%s: root entries: %s", g_CurXferFile.c_str(), join( ", ", asRootFiles ).c_str()); // XXX

			FOREACH( CString, asRootFiles, sRootFile )
			{
				bool bpBreak = false;
				// do not allow actual files in the root of the zip, too sploity
				if ( pZip->GetFileType(*sRootFile) == RageFileManager::TYPE_FILE )
				{
					SCREENMAN->SystemMessage( ssprintf("Skipping %s (loose file %s not allowed in root of zip)", sSelection.c_str(), sRootFile->c_str()) );
					SAFE_DELETE(pZip);
					bSkip = true;
					break;
				}
			}

			// do not allow zips that have protected folders to be added --infamouspat
			for( unsigned f = 0; f < NUM_BLACKLIST_FOLDERS; f++ )
			{
				if ( pZip->GetFileInfo(g_asBlacklistedFolders[f]) != NULL )
				{
					SCREENMAN->SystemMessage( ssprintf("Skipping %s (directory %s is not allowed in root of zip)", sSelection.c_str(), g_asBlacklistedFolders[f].c_str()) );
					SAFE_DELETE(pZip);
					bSkip = true;
					break;
				}
			}
			if (bSkip) 
			{
				MEMCARDMAN->UnmountCard(m_CurPlayer);
				MEMCARDMAN->UnlockCards();
				MountMutex.Unlock();
				m_bStopThread = false;
				m_PlayerSongLoadThread.Create( InitSASSongThread, this );
				return;
			}

			// sanity checks completed, proceeding...
			SAFE_DELETE(pZip);

			if (!CopyWithProgress(g_CurXferFile, "/AdditionalSongs/" + sSelection, UpdateXferProgress, sError) )
			{
				SCREENMAN->SystemMessage( "Transfer error: " + sError );
				bBreakEarly = true;
			}
		}
		MEMCARDMAN->UnmountCard(m_CurPlayer);
		MEMCARDMAN->UnlockCards();
#if defined(LINUX) && defined(ITG_ARCADE)
		sync();
		system( "mount -o remount,ro /itgdata" );
#endif
		MountMutex.Unlock();

		if (!bBreakEarly) bSuccess = true;
		SCREENMAN->HideOverlayMessage();
		SCREENMAN->ZeroNextUpdate();
		FILEMAN->FlushDirCache("/AdditionalSongs");
		// hmm...
		if (bSuccess)
		{
			m_bRestart = true;

			m_asAddedZips.clear();
			LoadAddedZips();
			m_AddedZips.SetChoices( m_asAddedZips );
		}

		// recreate the song accept thread if xfer failed or user declined to add songs
		m_bStopThread = false;
		m_PlayerSongLoadThread.Create( InitSASSongThread, this );
		
	}
	switch( SM )
	{
	case SM_GoToNextScreen:
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
		break;
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( PREV_SCREEN );
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

/*
void ScreenAddSongs::MenuStart( PlayerNumber pn )
{
	MenuBack(pn);
}
*/

void ScreenAddSongs::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

