/* vim: set noet */
#include "global.h"
#include "ScreenUserPacks.h"
#include "ScreenPrompt.h"
#include "MemoryCardManager.h"
#include "CodeDetector.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "UserPackManager.h"
#include "DiagnosticsUtil.h"
#include "arch/ArchHooks/ArchHooks.h"

#define NEXT_SCREEN					THEME->GetMetric (m_sName,"NextScreen")
#define PREV_SCREEN					THEME->GetMetric (m_sName,"PrevScreen")

#define USER_PACK_WAIT_TEXT			THEME->GetMetric ("ScreenUserPacks","TransferWaitText")
#define USER_PACK_CANCEL_TEXT		THEME->GetMetric ("ScreenUserPacks","TransferCancelText")

static RageMutex MountMutex("ITGDataMount");

REGISTER_SCREEN_CLASS( ScreenUserPacks );

AutoScreenMessage( SM_ConfirmAddZip );
AutoScreenMessage( SM_AnswerConfirmAddZip );
AutoScreenMessage( SM_ConfirmDeleteZip );
AutoScreenMessage( SM_AnswerConfirmDeleteZip );
AutoScreenMessage( SM_LinkedMenuChange );
AutoScreenMessage( SM_CancelTransfer );

/* draw transfer updates six frames a second */
static RageTimer DrawTimer;
const float DRAW_UPDATE_TIME = 0.166667f;

ScreenUserPacks::ScreenUserPacks( CString sName ) : ScreenWithMenuElements( sName )
{
	m_bRestart = false;
	m_bPrompt = false;
	m_CurPlayer = PLAYER_INVALID;
	MEMCARDMAN->UnlockCards();
	DrawTimer.SetZero();
}

ScreenUserPacks::~ScreenUserPacks()
{
	LOG->Trace( "ScreenUserPacks::~ScreenUserPacks()" );
	m_bStopThread = true;
	if (m_PlayerSongLoadThread.IsCreated())
		m_PlayerSongLoadThread.Wait();

	FOREACH_PlayerNumber( pn )
		MEMCARDMAN->UnmountCard( pn );

	// we sync on the end of each update.
	if ( m_bRestart )
		HOOKS->SystemReboot( false );
}

void ScreenUserPacks::ReloadZips()
{
	m_asAddedZips.clear();
	UPACKMAN->GetUserPacks( m_asAddedZips );
	m_AddedZips.SetChoices( m_asAddedZips );
}

int InitSASSongThread( void *pSAS )
{
	((ScreenUserPacks*)pSAS)->StartSongThread();
	return 1;
}

void ScreenUserPacks::Init()
{
	ScreenWithMenuElements::Init();

	m_SoundDelete.Load( THEME->GetPathS( m_sName, "delete" ) );
	m_SoundTransferDone.Load( THEME->GetPathS( m_sName, "transfer done" ) );

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

	ReloadZips();
}

void ScreenUserPacks::StartSongThread()
{
	while ( !m_bStopThread )
	{
		if (m_bPrompt)
		{
			usleep( 10000 );
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
				CString sPlayerUserPacksDir = sDriveDir + "/" + USER_PACK_TRANSFER_PATH;
				CStringArray sUSBZips;
				GetDirListing( sPlayerUserPacksDir+"/*.zip", sUSBZips, false, false ); /**/
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
			MenuBack( MenuI.player );
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
CString g_CurSelection;
uint64_t g_iLastCurrentBytes;
RageTimer g_UpdateDuration;

bool UpdateXferProgress( uint64_t iBytesCurrent, uint64_t iBytesTotal )
{
	bool bInterrupt = false;

	FOREACH_EnabledPlayer(pn)
	{
		bInterrupt |= INPUTMAPPER->IsButtonDown( MenuInput(pn, MENU_BUTTON_SELECT) );

		bInterrupt |= INPUTMAPPER->IsButtonDown(MenuInput(pn, MENU_BUTTON_LEFT)) &&
			INPUTMAPPER->IsButtonDown(MenuInput(pn, MENU_BUTTON_RIGHT));
	}

	if ( bInterrupt )
	{
		InputEventArray throwaway;
		INPUTFILTER->GetInputEvents( throwaway );
		return false;
	}

	// Draw() is very expensive: only do it on occasion.
	if( DrawTimer.Ago() < DRAW_UPDATE_TIME )
		return true;

	/* this truncates to int, but that's okay for our purposes */
	float iTransferRate = iBytesCurrent / g_UpdateDuration.Ago();

	float fPercent = iBytesCurrent / (iBytesTotal/100);

	const CString sRate = FormatByteValue( iTransferRate ) + "/sec";

	CString sMessage = ssprintf( "\n\n%s\n%.2f%% %s\n\n%s",
		USER_PACK_WAIT_TEXT.c_str(),
		fPercent,
		sRate.c_str(),
		USER_PACK_CANCEL_TEXT.c_str()
	);
	SCREENMAN->OverlayMessage( sMessage );


	SCREENMAN->Draw();
	DrawTimer.Touch();
	return true;
}

void ScreenUserPacks::HandleScreenMessage( const ScreenMessage SM )
{
#if 0
	/* Not compatible with FileCopy callback system; was this ever used? -- vyhd */
	if ( SM == SM_CancelTransfer )
	{
		Dialog::OK("SM_CancelTransfer");

		InterruptCopy();

		InputEventArray throwaway;
		INPUTFILTER->GetInputEvents( throwaway );

		LOG->Warn("Cancelled Transfer of user pack.");
	}
#endif
	if ( SM == SM_LinkedMenuChange )
	{
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
		g_CurSelection = sSelection;
		bool bSuccess = UPACKMAN->Remove( USER_PACK_SAVE_PATH + sSelection );
		if (bSuccess)
		{
			m_SoundDelete.Play();
			ReloadZips();
			m_bRestart = true;
		}
		else
		{
			SCREENMAN->SystemMessage( "Failed to delete zip file from machine. Check your file permissions." );
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
		CString sError;

		m_bPrompt = false;
		if (ScreenPrompt::s_LastAnswer == ANSWER_NO)
			return;

		m_bStopThread = true;
		m_PlayerSongLoadThread.Wait();

		MountMutex.Lock();

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

			g_CurXferFile = MEM_CARD_MOUNT_POINT[m_CurPlayer] + "/" + USER_PACK_TRANSFER_PATH + sSelection;
			if ( !UPACKMAN->IsPackTransferable( sSelection, g_CurXferFile, sError ) || !UPACKMAN->IsPackMountable( g_CurXferFile, sError ) )
			{
				SCREENMAN->SystemMessage( "Could not add pack to machine:\n" + sError );
				XFER_CLEANUP;
				return;
			}

			sError = ""; //  ??
			RageTimer start;
			DrawTimer.Touch();
			g_iLastCurrentBytes = 0;
			g_UpdateDuration.Touch();
			if (!UPACKMAN->TransferPack( g_CurXferFile, sSelection, UpdateXferProgress, sError ) )
			{
				SCREENMAN->SystemMessage( "Transfer error:\n" + sError );
				XFER_CLEANUP;
				SCREENMAN->HideOverlayMessage();
				SCREENMAN->ZeroNextUpdate();
				return;
			}
			LOG->Debug( "Transferred %s in %f seconds.", g_CurXferFile.c_str(), start.Ago() );
		}
#if defined(LINUX) && defined(ITG_ARCADE)
		sync();
#endif
		SCREENMAN->HideOverlayMessage();
		SCREENMAN->ZeroNextUpdate();
		FILEMAN->FlushDirCache(USER_PACK_SAVE_PATH);

		m_bRestart = true;

		m_SoundTransferDone.Play();
		ReloadZips();

		XFER_CLEANUP;
#undef XFER_CLEANUP
	}
	switch( SM )
	{
	case SM_GoToNextScreen:
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
