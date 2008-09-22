#include "global.h"
#include "HelpDisplay.h"
#include "ScreenAddSongs.h"
#include "Screen.h"
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

static RageMutex MountMutex("ITGDataMount");

REGISTER_SCREEN_CLASS( ScreenAddSongs );

ScreenAddSongs::ScreenAddSongs( CString sName ) : ScreenWithMenuElements( sName )
{
	m_bRefreshSongMan = false;
}

ScreenAddSongs::~ScreenAddSongs()
{
	LOG->Trace( "ScreenAddSongs::~ScreenAddSongs()" );
	m_bStopThread = true;
	m_PlayerSongLoadThread.Wait();

	FOREACH_PlayerNumber( pn )
		MEMCARDMAN->UnmountCard( pn );

	if ( m_bRefreshSongMan )
		SONGMAN->Reload();
}

void ScreenAddSongs::LoadAddedSongs()
{
	CStringArray asGroups;
	FILEMAN->GetDirListing( "/AdditionalSongs/*", asGroups, true, false );
	for (unsigned i = 0; i < asGroups.size(); i++)
	{
		LOG->Debug("Found group: %s", asGroups[i].c_str());
		CStringArray asCandidateSongs;
		FILEMAN->GetDirListing( "/AdditionalSongs/" + asGroups[i] + "/*", asCandidateSongs, true, false );
		for( unsigned j = 0; j < asCandidateSongs.size(); j++ )
		{
			LOG->Debug("Found song: %s", ("/AdditionalSongs/" + asGroups[i] + "/" + asCandidateSongs[j] + "/").c_str() );
			Song *pSong = new Song;
			if ( pSong->LoadFromCustomSongDir( "/AdditionalSongs/" + asGroups[i] + "/" + asCandidateSongs[j] + "/", asGroups[i], PLAYER_1 ) )
			{
				m_vLoadedSongs.push_back( pSong );
				m_vsLoadedSongs.push_back( pSong->m_sGroupName + " / " + pSong->GetTranslitFullTitle() );
			}
			else
				delete pSong;
		}
	}
}

int InitSASSongThread( void *pSAS )
{
	((ScreenAddSongs*)pSAS)->StartSongThread();
	return 1;
}

void ScreenAddSongs::Init()
{
	ScreenWithMenuElements::Init();

	LoadAddedSongs();

	m_AddedSongList.SetName( "LoadedSongList" );
	///////XXX TEST CODE//////
	m_AddedSongList.LoadFromFont( THEME->GetPathF("Common","normal") );
	m_AddedSongList.SetXY( SCREEN_CENTER_X + 140, SCREEN_CENTER_Y );
	m_AddedSongList.SetText( join("\n", m_vsLoadedSongs) );
	m_AddedSongList.SetZoom( 0.4f );
	//////////////////////////
	this->AddChild( &m_AddedSongList );

	m_AddableSongSelection.SetName( "AddableSongList" );
	///////XXX TEST CODE//////
	m_AddableSongSelection.LoadFromFont( THEME->GetPathF("Common","normal") );
	m_AddableSongSelection.SetXY( SCREEN_CENTER_X - 140, SCREEN_CENTER_Y );
	m_AddableSongSelection.SetText( "Please insert memory card with additional songs for transfer" );
	m_AddableSongSelection.SetZoom( 0.4f );
	//////////////////////////
	this->AddChild( &m_AddableSongSelection );
	this->SortByDrawOrder();

	m_bStopThread = false;
	m_PlayerSongLoadThread.SetName( "Song Add Thread" );
	m_PlayerSongLoadThread.Create( InitSASSongThread, this );
}

void ScreenAddSongs::StartSongThread()
{
	while ( !m_bStopThread )
	{
		FOREACH_PlayerNumber( pn )
		{
			MemoryCardState PCardState = MEMCARDMAN->GetCardState(pn);
			if (PCardState == MEMORY_CARD_STATE_READY && !m_bCardMounted[pn])
			{
				if( !PROFILEMAN->LoadProfileFromMemoryCard(pn) || !PROFILEMAN->IsPersistentProfile(pn) ) continue;
				LOG->Debug("Mounting memory card for player %d", pn+1);
				m_AddableSongSelection.SetText( ssprintf("Loading Player %d songs...", pn+1) );
				m_asAddableSongs[pn].clear();
				MEMCARDMAN->LockCards();
				MEMCARDMAN->MountCard(pn);
				
				CStringArray asGroups;
				CString sDir = PROFILEMAN->GetProfileDir(pn) + "/AdditionalSongs";
				LOG->Debug("Checking %s for additional songs...", sDir.c_str());
				GetDirListing( sDir+"/*", asGroups, true, false );
				for (unsigned i = 0; i < asGroups.size(); i++)
				{
					CStringArray asCandidateSongs;
					GetDirListing( sDir+"/"+asGroups[i]+"/*", asCandidateSongs, true, true );
					for (unsigned j = 0; j < asCandidateSongs.size(); j++)
					{
						Song *pSong = new Song;
						if ( pSong->LoadFromCustomSongDir( asCandidateSongs[j] + "/", asGroups[i], pn ) )
							m_asAddableSongs[pn].push_back(pSong->m_sGroupName + " / " + pSong->GetTranslitFullTitle());
						delete pSong;
					}
				}
				
				MEMCARDMAN->UnmountCard(pn);
				MEMCARDMAN->UnlockCards();
				m_bCardMounted[pn] = true;
			}
			if (PCardState == MEMORY_CARD_STATE_NO_CARD && m_bCardMounted[pn])
			{
				LOG->Debug("Unmounting memory card for player %d", pn+1);
				m_asAddableSongs[pn].clear();
				m_bCardMounted[pn] = false;
			}
		}
	
		SCREENMAN->SystemMessageNoAnimate( ssprintf("Player1: %d, Player2: %d", 
				m_bCardMounted[0], 
				m_bCardMounted[1]
		));
	
		if ( m_asAddableSongs[PLAYER_1].size() + m_asAddableSongs[PLAYER_2].size() > 0 )
			m_AddableSongSelection.SetText( join("\n", m_asAddableSongs[PLAYER_1]) + "\n" + join("\n", m_asAddableSongs[PLAYER_2]) );
		else
			m_AddableSongSelection.SetText( "Please insert memory card with additional songs for transfer" );
	
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


void ScreenAddSongs::HandleScreenMessage( const ScreenMessage SM )
{
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

