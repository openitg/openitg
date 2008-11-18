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
#include "RageUtil.h"
#include "RageLog.h"
#include "ProfileManager.h"
#include "RageUtil_FileDB.h" /* defines FileSet */
#include "RageFileDriverDirect.h" /* defines DirectFilenameDB */
#include "arch/ArchHooks/ArchHooks.h"

static RageMutex MountMutex("ITGDataMount");

REGISTER_SCREEN_CLASS( ScreenAddSongs );

AutoScreenMessage( SM_ConfirmAddGroups );
AutoScreenMessage( SM_AnswerConfirmAddGroups );

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

	if ( m_bRestart )
		HOOKS->SystemReboot();
}

/**
	Ok I know what you're thinking Mark, and yes, this will be expanded upon --infamouspat
*/
void ScreenAddSongs::LoadAddedGroups()
{
	FILEMAN->GetDirListing( "/AdditionalSongs/*", m_asAddedGroups, true, false );
}

int InitSASSongThread( void *pSAS )
{
	((ScreenAddSongs*)pSAS)->StartSongThread();
	return 1;
}

void ScreenAddSongs::Init()
{
	ScreenWithMenuElements::Init();

	LoadAddedGroups();

	m_AddedGroupList.SetName( "LoadedSongList" );
	///////XXX TEST CODE//////
	m_AddedGroupList.LoadFromFont( THEME->GetPathF("Common","normal") );
	m_AddedGroupList.SetXY( SCREEN_CENTER_X + 140, SCREEN_CENTER_Y );
	m_AddedGroupList.SetText( join("\n", m_asAddedGroups) );
	m_AddedGroupList.SetZoom( 0.4f );
	//////////////////////////
	this->AddChild( &m_AddedGroupList );

	m_AddableGroupSelection.SetName( "AddableSongList" );
	///////XXX TEST CODE//////
	m_AddableGroupSelection.LoadFromFont( THEME->GetPathF("Common","normal") );
	m_AddableGroupSelection.SetXY( SCREEN_CENTER_X - 140, SCREEN_CENTER_Y );
	m_AddableGroupSelection.SetText( "Please insert memory card with additional songs for transfer" );
	m_AddableGroupSelection.SetZoom( 0.4f );
	//////////////////////////
	this->AddChild( &m_AddableGroupSelection );
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
			if (PCardState == MEMORY_CARD_STATE_READY && !m_bCardMounted[pn])
			{
				if( !PROFILEMAN->LoadProfileFromMemoryCard(pn) || !PROFILEMAN->IsPersistentProfile(pn) ) continue;
				m_AddableGroupSelection.SetText( ssprintf("Loading Player %d songs...\n", pn+1) );
				m_asAddableGroups[pn].clear();
				MEMCARDMAN->LockCards();
				MEMCARDMAN->MountCard(pn);
				
				CStringArray asGroups;
				CString sDir = PROFILEMAN->GetProfileDir(pn) + "/AdditionalSongs";
				GetDirListing( sDir+"/*", asGroups, true, false );
				for (unsigned i = 0; i < asGroups.size(); i++)
				{
					bool bAdd = true;

					// don't add duplicate groups
					// TODO: support for overlaying songs into already existing groups
					for (unsigned x = 0; x < m_asAddedGroups.size(); x++)
						if ( m_asAddedGroups[x] == asGroups[i] ) bAdd = false;
					if (!bAdd) continue;

					CStringArray asCandidateSongs;
					GetDirListing( sDir+"/"+asGroups[i]+"/*", asCandidateSongs, true, true );
					if ( asCandidateSongs.size() == 0 ) continue;
					for (unsigned j = 0; j < asCandidateSongs.size(); j++)
					{
						Song *pSong = new Song;
						if ( ! pSong->LoadFromCustomSongDir( asCandidateSongs[j] + "/", asGroups[i], pn ) )
							bAdd = false;
						delete pSong;
					}
					if (bAdd) m_asAddableGroups[pn].push_back( asGroups[i] );
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
				m_asAddableGroups[pn].clear();
				m_bCardMounted[pn] = false;
			}
		}
	
#if 0
		SCREENMAN->SystemMessageNoAnimate( ssprintf("Player1: %d, Player2: %d", 
				m_bCardMounted[0], 
				m_bCardMounted[1]
		));
#endif
	
		if ( m_asAddableGroups[PLAYER_1].size() + m_asAddableGroups[PLAYER_2].size() > 0 )
		{
			m_AddableGroupSelection.SetText( join("\n", m_asAddableGroups[PLAYER_1]) + "\n" + join("\n", m_asAddableGroups[PLAYER_2]) );

			if (bLaunchPrompt)
			{
				this->PostScreenMessage( SM_ConfirmAddGroups, 0.0f );
				m_bPrompt = true;
			}
		}
		else
			m_AddableGroupSelection.SetText( "Please insert memory card with additional songs for transfer" );
	
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

void ScreenAddSongs::HandleScreenMessage( const ScreenMessage SM )
{
	if ( SM == SM_ConfirmAddGroups )
	{
		SCREENMAN->Prompt( SM_AnswerConfirmAddGroups, 
			m_AddableGroupSelection.GetText() + "\n\nProceed to add song packs to machine?",
		PROMPT_YES_NO, ANSWER_NO );
	}
	if ( SM == SM_AnswerConfirmAddGroups )
	{
		bool bSuccess = false, bBreakEarly = false;
		CString sError;

		m_bPrompt = false;
		if (ScreenPrompt::s_LastAnswer == ANSWER_NO)
		{
			LOG->Debug("SM_AnswerConfirmAddGroups: returning...");
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
		for( unsigned i = 0; i < m_asAddableGroups[g_CurrentPlayer].size(); i++ )
		{
			bBreakEarly = false;
			CString sAddGroup = m_asAddableGroups[g_CurrentPlayer][i];
			CString sAddDb = PROFILEMAN->GetProfileDir(g_CurrentPlayer) + "/AdditionalSongs/" + sAddGroup + "/";
			CStringArray sFiles;
			GetDirListingRecursive( sAddDb, "/*", sFiles );
			for( unsigned j = 0; j < sFiles.size(); j++)
			{
				CString sFile = sFiles[j];
				CStringArray parts;
				split( sFile, "/", parts );
				CString sDestFile = "/" + join( "/", parts.begin()+2, parts.end() );

				g_CurXferFile = sDestFile;
				if (!CopyWithProgress(sFile, sDestFile, UpdateXferProgress, sError) )
				{
					SCREENMAN->SystemMessage( "Transfer error: " + sError );
					bBreakEarly = true;
					// TODO: delete remnant transfer contents
					break;
				}
			}
			if (bBreakEarly) break;
		}
		MEMCARDMAN->UnmountCard(g_CurrentPlayer);
		MEMCARDMAN->UnlockCards();
#if defined(LINUX) && defined(ITG_ARCADE)
		system( "mount -o remount,ro /itgdata" );
#endif
		MountMutex.Unlock();

		if (!bBreakEarly) bSuccess = true;
		SCREENMAN->HideOverlayMessage();
		SCREENMAN->ZeroNextUpdate();
		// hmm...
		if (bSuccess)
		{
			m_AddableGroupSelection.SetText(
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

