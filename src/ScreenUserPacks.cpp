#include "global.h"
#include "HelpDisplay.h"
#include "ScreenUserPacks.h"
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
#include "UserPackManager.h"
#include "RageUtil_FileDB.h" /* defines FileSet */
#include "RageFileDriverDirect.h" /* defines DirectFilenameDB */
#include "arch/ArchHooks/ArchHooks.h"
#include "arch/Dialog/DialogDriver.h"

#define NEXT_SCREEN					THEME->GetMetric (m_sName,"NextScreen")
#define PREV_SCREEN					THEME->GetMetric (m_sName,"PrevScreen")

static RageMutex MountMutex("ITGDataMount");

REGISTER_SCREEN_CLASS( ScreenUserPacks );

AutoScreenMessage( SM_ConfirmAddZip );
AutoScreenMessage( SM_AnswerConfirmAddZip );
AutoScreenMessage( SM_ConfirmDeleteZip );
AutoScreenMessage( SM_AnswerConfirmDeleteZip );
AutoScreenMessage( SM_LinkedMenuChange );

ScreenUserPacks::ScreenUserPacks( CString sName ) : ScreenWithMenuElements( sName )
{
	m_bRestart = false;
	m_bPrompt = false;
	m_CurPlayer = PLAYER_INVALID;
	MEMCARDMAN->UnlockCards();
}

ScreenUserPacks::~ScreenUserPacks()
{
	LOG->Trace( "ScreenUserPacks::~ScreenUserPacks()" );
	m_bStopThread = true;
	if (m_PlayerSongLoadThread.IsCreated())
		m_PlayerSongLoadThread.Wait();

	FOREACH_PlayerNumber( pn )
		MEMCARDMAN->UnmountCard( pn );

	if ( m_bRestart )
		HOOKS->SystemReboot();
}

void ScreenUserPacks::LoadAddedZips()
{
	UPACKMAN->GetCurrentUserPacks( m_asAddedZips );
}

int InitSASSongThread( void *pSAS )
{
	((ScreenUserPacks*)pSAS)->StartSongThread();
	return 1;
}

void ScreenUserPacks::Init()
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
	m_Disclaimer.LoadFromFont( THEME->GetPathF( m_sName, "text" ) );
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

void ScreenUserPacks::StartSongThread()
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
				CString sDriveDir = MEM_CARD_MOUNT_POINT[pn];
				if (sDriveDir.empty())
				{
					MEMCARDMAN->UnmountCard(pn);
					continue;
				}
				CString sPlayerUserPacksDir = sDriveDir + "/" + UPACKMAN->GetUserTransferPath();
				CStringArray sUSBZips;
				GetDirListing( sPlayerUserPacksDir+"/*.zip", sUSBZips, false, false );
				MEMCARDMAN->UnmountCard(pn);
				m_USBZips.SetChoices( sUSBZips );
				m_CurPlayer = pn;
			}
		}

		usleep( 1000 );
	}
}

void ScreenUserPacks::Update( float fDeltaTime )
{
	ScreenWithMenuElements::Update( fDeltaTime );
}

void ScreenUserPacks::HandleMessage( const CString &sMessage )
{
	if ( sMessage == "LinkedMenuChange" )
	{
		m_pCurLOM = m_pCurLOM->SwitchToNextMenu();
		return;
	}
	ScreenWithMenuElements::HandleMessage( sMessage );
}

void ScreenUserPacks::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
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
	CString sMessage = ssprintf( "Please wait ...\n%u%%\n\n%s\n", (int)fPercent, g_CurXferFile.c_str() );
	SCREENMAN->OverlayMessage( sMessage );
	SCREENMAN->Draw();
}

void ScreenUserPacks::HandleScreenMessage( const ScreenMessage SM )
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
		bool bSuccess = UPACKMAN->UnlinkAndRemovePack( UPACKMAN->GetSavePath() + "/" + sSelection );
		if (bSuccess)
		{
			m_bRestart = true;
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

////////////////////////
#define XFER_CLEANUP MEMCARDMAN->UnmountCard(m_CurPlayer); \
MEMCARDMAN->UnlockCards(); \
MountMutex.Unlock(); \
m_bStopThread = false; \
m_PlayerSongLoadThread.Create( InitSASSongThread, this )
////////////////////////

			bBreakEarly = false;
			bSkip = false;

			g_CurXferFile = MEM_CARD_MOUNT_POINT[m_CurPlayer] + "/" + UPACKMAN->GetUserTransferPath() + sSelection;
			if ( !UPACKMAN->IsPackTransferable( sSelection, sError ) || !UPACKMAN->IsPackAddable( g_CurXferFile, sError ) )
			{
				SCREENMAN->SystemMessage( "Could not add pack to machine: " + sError );
				XFER_CLEANUP;
				return;
			}

			sError = ""; //  ??
			if (!UPACKMAN->TransferPack( g_CurXferFile, sSelection, UpdateXferProgress, sError ) )
			{
				SCREENMAN->SystemMessage( "Transfer error: " + sError );
				XFER_CLEANUP;
				return;
			}
		}
#if defined(LINUX) && defined(ITG_ARCADE)
		sync();
		system( "mount -o remount,ro /itgdata" );
#endif
		SCREENMAN->HideOverlayMessage();
		SCREENMAN->ZeroNextUpdate();
		FILEMAN->FlushDirCache(UPACKMAN->GetSavePath());

		m_bRestart = true;

		m_asAddedZips.clear();
		LoadAddedZips();
		m_AddedZips.SetChoices( m_asAddedZips );

		XFER_CLEANUP;
#undef XFER_CLEANUP
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

void ScreenUserPacks::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		this->PlayCommand( "Off" );
		this->StartTransitioning( SM_GoToPrevScreen );
	}
}

/*
void ScreenUserPacks::MenuStart( PlayerNumber pn )
{
	MenuBack(pn);
}
*/

void ScreenUserPacks::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

/*
 * (c) 2009 "infamouspat"
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