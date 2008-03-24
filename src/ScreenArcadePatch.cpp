#include "global.h"
#include "HelpDisplay.h"
#include "ScreenArcadePatch.h"
#include "ScreenManager.h"
#include "RageLog.h"
#include "InputMapper.h"
#include "LuaManager.h"
#include "GameState.h"
#include "GameSoundManager.h"
#include "ThemeManager.h"
#include "Game.h"
#include "ScreenDimensions.h"
#include "GameManager.h"
#include "PrefsManager.h"
#include "RageInput.h"
#include "RageFileManager.h"
#include "RageFileDriverSlice.h"
#include "CryptHelpers.h"
#include "RageFileDriverZip.h"
#include "XmlFile.h"
#include "MiscITG.h"

#include "arch/ArchHooks/ArchHooks.h" // for HOOKS->SystemReboot()

#include "Foreach.h"		// Foreach loops without the command is hard.
#include "MemoryCardManager.h"	// Where else are we getting the patch from?
#include "GameState.h"		// Check which USB...
#include "ProfileManager.h"	// Check for USB...
#include "RageUtil.h"		// I need this for copying the patch
#include "RageFile.h"		// For the .itg extraction

#ifdef ITG_ARCADE
#define PATCH_DIR CString("/stats/")
#else
#define PATCH_DIR CString("Data/")
#endif

/* global so the screen and the thread can both access these */
CString g_sPatch;
CString g_sStatus;

REGISTER_SCREEN_CLASS( ScreenArcadePatch );

ScreenArcadePatch::ScreenArcadePatch( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadePatch::ScreenArcadePatch()" );
	m_bReboot = false;
}


ScreenArcadePatch::~ScreenArcadePatch()
{
	LOG->Trace( "ScreenArcadePatch::~ScreenArcadePatch() %i", (int)m_bReboot );

	FOREACH_PlayerNumber( pn )
		MEMCARDMAN->UnmountCard( pn );

	if( m_bReboot )
		HOOKS->SystemReboot();
}

void ScreenArcadePatch::Init()
{
	ScreenWithMenuElements::Init();

	// HACK: needed to prevent early exits
	m_bExit = false;
	g_sStatus = "";
	g_sPatch = THEME->GetMetric("ScreenArcadePatch", "IntroText");

	m_Status.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );
	m_Patch.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );
	
	m_Status.SetName( "State" );
	m_Patch.SetName( "List" );

	SET_XY_AND_ON_COMMAND( m_Status );
	SET_XY_AND_ON_COMMAND( m_Patch );

	m_Status.SetText( THEME->GetMetric("ScreenArcadePatch", "IntroText") );
	m_Patch.SetText( "Please insert a USB Card containing an update." );

	m_PatchThread.SetName( "Patch thread" );
	/* this does our initial patch checking for us */
	m_PatchThread.Create( PatchThread_Start, this );

	this->AddChild( &m_Status );
	this->AddChild( &m_Patch );

	this->SortByDrawOrder();
}

void ScreenArcadePatch::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore
	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenArcadePatch::Update( float fDeltaTime )
{
	m_Status.SetText( g_sStatus );
	m_Patch.SetText( g_sPatch );

	Screen::Update( fDeltaTime );
}

void ScreenArcadePatch::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenArcadePatch::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToNextScreen:
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		break;
	}
}

void ScreenArcadePatch::MenuStart( PlayerNumber pn )
{
	if( m_bExit )
		MenuBack(pn);
}

void ScreenArcadePatch::MenuBack( PlayerNumber pn )
{
	if( !m_bExit )
		return;
	if(!IsTransitioning())
	{
		this->PlayCommand( "Off" );
		this->StartTransitioning( SM_GoToPrevScreen );		
	}
}

// lol thanx Vyhd
void UpdatePatchCopyProgress( float fPercent )
{
	g_sPatch = ssprintf( "Copying patch (%u%%)\n\nPlease do not remove the USB Card.", (int)(fPercent) );
}

/* make this all threaded, but only run it once per load.
 * if anything fails, end early by simply returning. */
void ScreenArcadePatch::CheckForPatches()
{
	FindCard();

	/* false if cards cannot be loaded */
	if( !LoadFromCard() )
	{
		m_bExit = true;
		return;
	}

	/* Call once for each directory path we want to find */
	AddPatches( "ITG 2 *.itg" );
	AddPatches( "OpenITG *.bxr" );

	/* Nothing found in any of the above */
	if( m_vsPatches.size() == 0 )
	{
		g_sStatus = ssprintf( "No patches found on Player %d's card." , m_Player+1 );
		m_bExit = true;
		return;
	}

	/* sort - it's ascending order by default */
	SortCStringArray( m_vsPatches );

	/* If there are any OpenITG patches, that'll be the one installed.
	 * Otherwise, it'll be the highest ITG2 revision. */
	if( !LoadPatch(m_vsPatches[0]) )
	{
		m_bExit = true;
		return;
	}
	CHECKPOINT;

	/* We don't need the card to be mounted anymore */
	if( !FinalizePatch() )
	{
		m_bExit = true;
		return;
	}
	CHECKPOINT;

	/* Everything's good. Set our message and be ready to restart. */
	g_sStatus = m_sSuccessMessage;
	g_sPatch = ""; // clear
	CHECKPOINT;

	m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextFinished" ) );
	
	m_bExit = true;
	m_bReboot = true;
}

void ScreenArcadePatch::FindCard()
{
	m_Player = PLAYER_INVALID;

	/* XXX: this doesn't respond to cards without a profile */
	FOREACH_PlayerNumber( p )
	{
		if( MEMCARDMAN->IsNameAvailable( p ) )
		{
			m_Status.SetText( ssprintf( "Checking Player %d's card for a patch..." , m_Player+1 ) );
			m_Player = p;
		}
	}
	
	// No cards found - LoadFromCard will catch this, so just set our warnings
	if( m_Player == PLAYER_INVALID )
	{
		g_sStatus = "Please insert a USB Card containing an update.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextWaiting" ) );
	}
}

bool ScreenArcadePatch::LoadFromCard()
{
	if( m_Player == PLAYER_INVALID )
		return false;

	// Prep variables, and Mount the cards
	LOG->Trace( "P1: %s, P2: %s, P%i: %s", 
		MEM_CARD_MOUNT_POINT[PLAYER_1].c_str(),
		MEM_CARD_MOUNT_POINT[PLAYER_2].c_str(),
		(int)m_Player, MEM_CARD_MOUNT_POINT[m_Player].c_str() );
	m_sCardDir = MEM_CARD_MOUNT_POINT[m_Player];

	LOG->Trace( "P%i dir: %s", (int)m_Player, m_sCardDir.c_str() );
	
	/* Fix up our path */
	if( m_sCardDir.Right(1) != "/" )
		m_sCardDir += "/";
		
	/* Mount for 1 hour - more than long enough to transfer */
	if( MEMCARDMAN->MountCard( m_Player, 3600 ) )
		return true;
	else
		g_sStatus = ssprintf( "Error mounting Player %d's card!" , m_Player+1 );

	/* fall through */
	return false;
}

// Check for .itg files
bool ScreenArcadePatch::AddPatches( CString sPattern )
{
	ASSERT( !sPattern.empty() );

	/* lol, iPatch */
	unsigned int iPatches = m_vsPatches.size();

	LOG->Trace( "Getting matches to %s%s", m_sCardDir.c_str(), sPattern.c_str() );
	GetDirListing( m_sCardDir + sPattern, m_vsPatches );

	/* Nothing new was loaded */
	if( iPatches == m_vsPatches.size() )
	{
		LOG->Warn( "No patches found matching \"%s\"", sPattern.c_str() );
		return false;
	}

	return true;
}

bool ScreenArcadePatch::LoadPatch( CString sPath )
{
	CHECKPOINT;
	CString sFile = ssprintf( "%s%s" , m_sCardDir.c_str() , sPath.c_str() );
	
	g_sStatus = ssprintf( "Patch file found on Player %d's card!" , m_Player+1 );
	m_textHelp->SetText( sPath.c_str() );

#ifdef ITG_ARCADE
	m_sPatchPath = "/rootfs/tmp/" + sPath;
#else
	m_sPatchPath = "Temp/" + sPath;
#endif
	CHECKPOINT;
	if( CopyWithProgress(sFile, m_sPatchPath, &UpdatePatchCopyProgress) )
	{
		CHECKPOINT;
		g_sStatus = "Patch copied! Checking...";
		g_sPatch = "";
	}
	else
	{
		CHECKPOINT;
		/* This generally fails because OpenITG doesn't have access to Data. -- Vyhd */
		g_sStatus = "Patch copying failed!\nPlease check your permissions and try again.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}
	CHECKPOINT;

	/* Check the signature of the newly copied file */

	int iErr;
	unsigned filesize;
	CString patchRSA, patchSig, sErr;
	RageFileBasic *fSig, *fZip;

	CHECKPOINT;
	RageFileBasic *rf = FILEMAN->Open( m_sPatchPath, RageFile::READ, iErr );
	filesize = GetFileSizeInBytes( m_sPatchPath );
	CHECKPOINT;

	/////////// LOLOLOLOLOLOLOL /////////////
	if( GetExtension( m_sPatchPath ) == "bxr" ) /* OpenITG patch */
		GetFileContents("Data/Patch-OpenITG.rsa", patchRSA);
	else if( GetExtension( m_sPatchPath ) == "itg" ) /* regular ITG patch */
		GetFileContents("Data/Patch.rsa", patchRSA);		
	///////////////////////////////////////////////////////////

	CHECKPOINT;
	fSig = new RageFileDriverSlice( rf, filesize - 128, 128 );

	if (fSig->Read(patchSig, 128) < 128)
	{
		CHECKPOINT;
		g_sStatus = "Patch signature verification failed:\nunexpected end of file.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		CHECKPOINT;
		return false;
	}
	CHECKPOINT;

	fZip = new RageFileDriverSlice( rf, 0, filesize - 128 );
	
	CHECKPOINT;
	if (! CryptHelpers::VerifyFile( *fZip, patchSig, patchRSA, sErr ) )
	{
		g_sStatus = ssprintf("Patch signature verification failed:\n%s", sErr.c_str() );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}
	CHECKPOINT;

	g_sStatus = "Patch signature verified :)";
	CHECKPOINT;

	return true;
}

/* XXX: memory leaks :/ there's no nice way to deal with them, though */
bool ScreenArcadePatch::FinalizePatch()
{
	/* Make sure the XML in this patch is valid */
	CString sErr, sResultMessage;
	int iErr, iRevNum;
	unsigned filesize;
	RageFileBasic *fRoot, *fScl, *fXml;
	RageFileDriverZip *rfdZip = new RageFileDriverZip;
	XNode *rNode = new XNode;
	CHECKPOINT;

	fRoot = FILEMAN->Open( m_sPatchPath, RageFile::READ, iErr );
	filesize = GetFileSizeInBytes( m_sPatchPath ) - 128;
	fScl = new RageFileDriverSlice( fRoot, 0, filesize );
	CHECKPOINT;

	
	if (! rfdZip->Load(fScl))
	{
	CHECKPOINT;
		g_sStatus = "Patch XML data check failed, could not load patch file.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
	CHECKPOINT;
		return false;
	}
	CHECKPOINT;

	fXml = rfdZip->Open("patch.xml", RageFile::READ, iErr );

	CHECKPOINT;
	if (fXml == NULL)
	{
	CHECKPOINT;
		g_sStatus = "Patch XML data check failed, Could not open patch.xml.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
	CHECKPOINT;
		return false;
	}

	CHECKPOINT;
	rNode->m_sName = "Patch";
	rNode->LoadFromFile(*fXml);
	CHECKPOINT;

	//if (rNode->GetChild("Game")==NULL || rNode->GetChild("Revision")==NULL || rNode->GetChild("Message")==NULL)
	if ( !rNode->GetChild("Game") || !rNode->GetChild("Revision") || !rNode->GetChild("Message") )
	{
		g_sStatus = "Cannot proceed update, patch.xml corrupt.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	CString sGame = rNode->GetChildValue("Game");

	/* accept patches for either */
	if ( sGame != "OpenITG" && sGame != "In The Groove 2" )
	{
			g_sStatus = ssprintf( "Cannot proceed update, revision is for another game\n(\"%s\").", rNode->GetChildValue("Game") );
			m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
			return false;
	}

	rNode->GetChild("Revision")->GetValue(iRevNum);

	if (GetRevision() == iRevNum)
	{
		g_sStatus = "Cannot proceed update, revision on USB card\nis the same as the machine revision.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	m_sSuccessMessage = rNode->GetChildValue( "Message" );

	
	/* Copy the contents to the patch directory to finish up */
	delete rfdZip;

	rfdZip = new RageFileDriverZip;
	vector<CString> patchDirs;
	bool bReadError;

	if (! rfdZip->Load( m_sPatchPath ) )
	{
		g_sStatus = "Could not copy patch contents.";
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	if( IsADirectory("Data/new-patch-unchecked") )
	{
		FILEMAN->Remove("Data/new-patch-unchecked/*");
		FILEMAN->Remove("Data/new-patch-unchecked");
	}
	
	FILEMAN->CreateDir( "Data/new-patch-unchecked" );

	// MountTreeOfZips copycat ;)
	patchDirs.push_back("/");

	while ( patchDirs.size() )
	{
		CString sDirPath = patchDirs.back();
		patchDirs.pop_back();

		vector<CString> patchFiles;

		rfdZip->GetDirListing( sDirPath + "/*", patchFiles, false, true );
		
		for (unsigned i = 0; i < patchFiles.size(); i++)
		{
			CString sPath = patchFiles[i];
			if ( rfdZip->GetFileType( sPath ) != RageFileManager::TYPE_FILE )
				continue;

			LOG->Trace("ScreenArcadePatch::CopyPatchContents(): copying %s", sPath.c_str());

			//dunno if this would affect Rage or not
			CString sCleanPath = sPath;
			TrimLeft( sCleanPath, "/" );

			g_sPatch = ssprintf("Copying %s", sCleanPath.c_str() );
			
			RageFileBasic *fCopySrc = rfdZip->Open( sPath, RageFile::READ, iErr );

			RageFile fCopyDest;
			fCopyDest.Open( "Data/new-patch-unchecked/" + sPath, RageFile::WRITE );

			if (! FileCopy( *fCopySrc, fCopyDest, sErr, &bReadError ) )
			{
				LOG->Warn("Failed to copy file %s", sPath.c_str() );
				g_sPatch = ssprintf("Failed to copy file %s, cannot proceed", sPath.c_str());
				return false;
			}

			const RageFileDriverZip::FileInfo *fi = rfdZip->GetFileInfo( sPath );
			fCopyDest.Close();

			sPath = PATCH_DIR + "new-patch-unchecked/" + sPath;
#ifdef LINUX
			chmod( sPath.c_str(), fi->m_iFilePermissions );
#endif
		}
		rfdZip->GetDirListing( sDirPath + "/*", patchDirs, true, true );

	}
	rename( PATCH_DIR + "new-patch-unchecked", PATCH_DIR + "/new-patch" );

	/* It's been verified, copied, and installed. We're good. */
	return true;
}

/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
