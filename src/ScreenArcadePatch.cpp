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

#include <sys/stat.h>
#include <sys/reboot.h>

#ifdef ITG_ARCADE
#define PATCH_DIR CString("/stats/")
#else
#define PATCH_DIR CString("Data/")
#endif

BitmapText *pStatus;

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

	m_Status.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );
	m_Patch.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );
	
	m_Status.SetName( "State" );
	m_Patch.SetName( "List" );

	SET_XY_AND_ON_COMMAND( m_Status );
	SET_XY_AND_ON_COMMAND( m_Patch );

	m_Status.SetText( THEME->GetMetric("ScreenArcadePatch", "IntroText") );
	m_Patch.SetText( "Please insert a USB Card containing an update." );

	this->AddChild( &m_Status );
	this->AddChild( &m_Patch );

	this->SortByDrawOrder();

	pStatus = &m_Patch;

	/* We only want to run this once per screen load */
	CheckForPatches();
}

void ScreenArcadePatch::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore
	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenArcadePatch::Update( float fDeltaTime )
{
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
	MenuBack(pn);
}

void ScreenArcadePatch::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		this->PlayCommand( "Off" );
		this->StartTransitioning( SM_GoToPrevScreen );		
	}
}

// lol thanx Vyhd
void UpdatePatchCopyProgress( float fPercent )
{
	LOG->Trace( "UpdatePatchCopyProgress( %f ), BitmapText.GetText() = %s", fPercent , pStatus->GetText().c_str());

	CString sText = ssprintf( "Copying patch (%u%%)\n\n"
		"Please do not remove the USB Card.", (int)(fPercent) );
	
	pStatus->SetText( sText );
	SCREENMAN->Draw();
}

void ScreenArcadePatch::CheckForPatches()
{
	FindCard();

	/* false if cards cannot be loaded */
	if( !LoadFromCard() )
		return;

	/* Call once for each directory path we want to find */
	AddPatches( "ITG 2 *.itg" );
	AddPatches( "OpenITG *.bxr" );

	/* Nothing found in any of the above */
	if( m_vsPatches.size() == 0 )
	{
		m_Status.SetText( ssprintf( "No patches found on Player %d's card." , m_Player+1 ) );
		return;
	}

	/* sort - it's ascending order by default */
	SortCStringArray( m_vsPatches );

	/* If there are any OpenITG patches, that'll be the one installed.
	 * Otherwise, it'll be the highest ITG2 revision. */
	LoadPatch( m_vsPatches[0] );

	/* We don't need the card to be mounted anymore */
	if( !FinalizePatch() )
		return;

	/* Everything's good. Set our message and be ready to restart. */
	m_Status.SetText( m_sSuccessMessage );
	m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextFinished" ) );
	
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
		m_Patch.SetText( "Please insert a USB Card containing an update." );
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
		m_Status.SetText( ssprintf( "Error mounting Player %d's card!" , m_Player+1 ) );

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
	CString sFile = ssprintf( "%s%s" , m_sCardDir.c_str() , sPath.c_str() );
	
	m_Status.SetText( ssprintf( "Patch file found on Player %d's card!" , m_Player+1 ) );
	m_textHelp->SetText( sPath.c_str() );

	SCREENMAN->Draw();

#ifdef ITG_ARCADE
	m_sPatchPath = "/rootfs/tmp/" + sPath;
#else
	m_sPatchPath = "Temp/" + sPath;
#endif
	if( CopyWithProgress(sFile, m_sPatchPath, &UpdatePatchCopyProgress) )
	{
		m_Status.SetText( "Patch copied! Checking..." );
	}
	else
	{
		m_Status.SetText( "Patch copying failed! Please re-insert your card and try again." );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	/* Check the signature of the newly copied file */

	int iErr;
	unsigned filesize;
	CString patchRSA, patchSig, sErr;
	RageFileBasic *fSig, *fZip;

	RageFileBasic *rf = FILEMAN->Open( m_sPatchPath, RageFile::READ, iErr );
	filesize = GetFileSizeInBytes( m_sPatchPath );

	/////////// LOLOLOLOLOLOLOL /////////////
	GetFileContents("Data/Patch.rsa", patchRSA);
	///////////////////////////////////////////////////////////

	fSig = new RageFileDriverSlice( rf, filesize - 128, 128 );

	if (fSig->Read(patchSig, 128) < 128)
	{
		m_Status.SetText( "Patch signature verification failed: unexpected end of file" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	fZip = new RageFileDriverSlice( rf, 0, filesize - 128 );
	
	if (! CryptHelpers::VerifyFile( *fZip, patchSig, patchRSA, sErr ) )
	{
		m_Status.SetText( ssprintf("Patch signature verification failed: %s", sErr.c_str()) );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	m_Status.SetText( "Patch signature verified :)" );

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

	fRoot = FILEMAN->Open( m_sPatchPath, RageFile::READ, iErr );
	filesize = GetFileSizeInBytes( m_sPatchPath ) - 128;
	fScl = new RageFileDriverSlice( fRoot, 0, filesize );

	
	if (! rfdZip->Load(fScl))
	{
		m_Status.SetText( "Patch XML data check failed, could not load .itg file" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	fXml = rfdZip->Open("patch.xml", RageFile::READ, iErr );

	if (fXml == NULL)
	{
		m_Status.SetText( "Patch XML data check failed, Could not open patch.xml" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	rNode->m_sName = "Patch";
	rNode->LoadFromFile(*fXml);

	//if (rNode->GetChild("Game")==NULL || rNode->GetChild("Revision")==NULL || rNode->GetChild("Message")==NULL)
	if ( !rNode->GetChild("Game") || !rNode->GetChild("Revision") || !rNode->GetChild("Message") )
	{
		m_Status.SetText( "Cannot proceed update, patch.xml corrupt" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	CString sGame = rNode->GetChildValue("Game");

	if ( sGame != "OpenITG" && sGame != "In The Groove 2" )
	{
			m_Status.SetText( ssprintf( "Cannot proceed update, revision is for another game (\"%s\").", rNode->GetChildValue("Game") ) );
			m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
			return false;
	}

	rNode->GetChild("Revision")->GetValue(iRevNum);

	if (GetRevision() == iRevNum)
	{
		m_Status.SetText( "Cannot proceed update, revision on USB card is the same as the machine revision" );
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
		m_Status.SetText( "Could not copy patch contents" );
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
			m_Status.SetText( ssprintf("Copying %s", sPath.c_str()) );
			SCREENMAN->Draw();
			
			RageFileBasic *fCopySrc = rfdZip->Open( sPath, RageFile::READ, iErr );

			RageFile fCopyDest;
			fCopyDest.Open( "Data/new-patch-unchecked/" + sPath, RageFile::WRITE );

			if (! FileCopy( *fCopySrc, fCopyDest, sErr, &bReadError ) )
			{
				LOG->Warn("Failed to copy file %s", sPath.c_str() );
				m_Status.SetText( ssprintf("Failed to copy file %s, cannot proceed", sPath.c_str()) );
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

	/* Phew! */
	return true;
}
