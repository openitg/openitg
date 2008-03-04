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

REGISTER_SCREEN_CLASS( ScreenArcadePatch );

ScreenArcadePatch::ScreenArcadePatch( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadePatch::ScreenArcadePatch()" );
}

static BitmapText *m_PatchStatus;
static bool g_doReboot;

ScreenArcadePatch::~ScreenArcadePatch()
{
	LOG->Trace( "ScreenArcadePatch::~ScreenArcadePatch() %i", (int)g_doReboot );

	if( g_doReboot )
		HOOKS->SystemReboot();
}

void ScreenArcadePatch::Init()
{
	ScreenWithMenuElements::Init();

	m_Status.LoadFromFont( THEME->GetPathF("ScreenArcadePatch","text") );
	m_Patch.LoadFromFont( THEME->GetPathF("ScreenArcadePatch","text") );
	
	m_Status.SetName( "State" );
	m_Patch.SetName( "List" );

	SET_XY_AND_ON_COMMAND( m_Status );
	SET_XY_AND_ON_COMMAND( m_Patch );

	m_Status.SetText( THEME->GetMetric("ScreenArcadePatch","IntroText") );
	m_Patch.SetText( "Please insert a USB Card containing an update." );
	m_textHelp->SetText( "THIS IS A TEST LOL" );

	this->AddChild( &m_Status );
	this->AddChild( &m_Patch );

	this->SortByDrawOrder();
	
	bChecking = false;
	m_PatchStatus = &m_Status;
	g_doReboot = false;	
}

void ScreenArcadePatch::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore
	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

int ScreenArcadePatch::CommitPatch()
{
	if( CheckCards() )
	{
		if( MountCards() )
		{
			if( ScanPatch() )
			{
				if (CopyPatch())
				{
					UnmountCards();
					if( bScanned )
					{
						if( CheckSignature() )
						{
							if( CheckXml() )
							{
								if ( CopyPatchContents() )
								{
									m_Status.SetText(m_sSuccessMsg);
									m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextFinished" ) );
									g_doReboot = true;
									return 0;
								}
							}
						}
					}
				}
			}
		}
	} else
		bChecking = false;
	
	return -1;
}

void ScreenArcadePatch::Update( float fDeltaTime )
{
	Screen::Update(fDeltaTime);
	if( !bChecking )
	{
		CommitPatch();
	}
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
		COMMAND( m_Status, "Off" );
		COMMAND( m_Patch, "Off" );
		this->StartTransitioning( SM_GoToPrevScreen );		
	}
}

// Check the Cards Presence
bool ScreenArcadePatch::CheckCards()
{
	// Set first, so it doesn't check again.
	bChecking = true;

	FOREACH_PlayerNumber( p )
	{
		if( MEMCARDMAN->IsNameAvailable( p ) )
		{
			m_Patch.SetVisible(false);
			pn = p;
			m_Status.SetText( ssprintf( "Checking Player %d's card for a patch..." , p + 1 ) );
			return true;
		}
	}
	
	// No cards found
	m_Patch.SetText( "Please insert a USB Card containing an update." );
	m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextWaiting" ) );

	return false;
}

// Prep variables, and Mount the cards
bool ScreenArcadePatch::MountCards()
{
	sDir = MEM_CARD_MOUNT_POINT[pn];
	
	if( sDir.Right( 1 ) != "/" )
		sDir += "/";
	
	sFullDir = sDir + "ITG 2 *.itg";
	
	// Finally mount the card
	if( MEMCARDMAN->MountCard( pn, -1 ) )
		return true;
	else {
		m_Status.SetText( ssprintf( "Error mounting Player %d's card!" , pn + 1 ) );
		return false;
	}
}

// Check for .itg files
bool ScreenArcadePatch::ScanPatch()
{
	GetDirListing( sFullDir , aPatches );
	
	if( aPatches.size() == 0 )
	{
		m_Status.SetText( ssprintf( "No patches found on Player %d's card." , pn + 1 ) );
		bScanned = false;
		return false;
	}
	
	SortCStringArray( aPatches );
	
	sFile = ssprintf( "%s%s" , sDir.c_str() , aPatches[0].c_str() );
	
	m_Status.SetText( ssprintf( "Patchfile found on Player %d's Card!" , pn + 1 ) );
	m_textHelp->SetText( aPatches[0].c_str() );
	
	bScanned = true;
	
	return true;
}

// lol thanx Vyhd
void UpdatePatchCopyProgress( float fPercent )
{
	LOG->Trace( "UpdatePatchCopyProgress( %f ), BitmapText.GetText() = %s", fPercent , m_PatchStatus->GetText().c_str());

	CString sText = ssprintf( "Copying patch (%u%%)\n\n"
		"Please do not remove the USB Card.", 
		(int)(fPercent) );
	
	//SCREENMAN->OverlayMessage( sText );
	m_PatchStatus->SetText( sText );
	m_PatchStatus->Draw();
	SCREENMAN->Draw();
}

// XXX: this cannot handle large patch files, plz fix this somehow kthx --infamouspat
bool ScreenArcadePatch::CopyPatch()
{
#ifdef ITG_ARCADE
	Root = "/rootfs/tmp/" + aPatches[0];
#else
	Root = "Temp/" + aPatches[0];
#endif
	if( CopyWithProgress( sFile , Root, &UpdatePatchCopyProgress ) )
	{
		m_Status.SetText( "Patch copied! Checking..." );
		return true;
	} else {
		m_Status.SetText( "Patch copying failed!! Please re-insert your card and try again!" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}
}

bool ScreenArcadePatch::UnmountCards()
{
	FOREACH_PlayerNumber( p )
		MEMCARDMAN->UnmountCard( pn );
	return true;
}

bool ScreenArcadePatch::CheckSignature()
{
	int iErr;
	unsigned filesize;
	CString patchRSA, patchSig, sErr;
	RageFileBasic *fSig, *fZip;

	RageFileBasic *rf = FILEMAN->Open( Root, RageFile::READ, iErr );
	filesize = GetFileSizeInBytes( Root );

	/////////// LOLOLOLOLOLOLOL /////////////
	GetFileContents("Data/Patch.rsa", patchRSA);
	///////////////////////////////////////////////////////////

	fSig = new RageFileDriverSlice( rf, filesize - 128, 128 );
	if (fSig->Read(patchSig, 128) < 128) {
		m_Status.SetText( "Patch signature verification failed: unexpected end of file" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}
	fZip = new RageFileDriverSlice( rf, 0, filesize - 128 );
	
	if (! CryptHelpers::VerifyFile( *fZip, patchSig, patchRSA, sErr ) ) {
		m_Status.SetText( ssprintf("Patch signature verification failed: %s", sErr.c_str()) );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	m_Status.SetText( "Patch signature verified :)" );
	return true;
}

bool ScreenArcadePatch::CheckXml()
{
	CString sErr, sResultMessage;
	int iErr, iRevNum;
	unsigned filesize;
	RageFileBasic *fRoot, *fScl, *fXml;
	RageFileDriverZip *rfdZip = new RageFileDriverZip;
	XNode *rNode = new XNode;

	fRoot = FILEMAN->Open( Root, RageFile::READ, iErr );
	filesize = GetFileSizeInBytes( Root ) - 128;
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

	if (rNode->GetChild("Game")==NULL || rNode->GetChild("Revision")==NULL || rNode->GetChild("Message")==NULL)
	{
		m_Status.SetText( "Cannot proceed update, patch.xml corrupt" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}
	CString sGame = rNode->GetChildValue("Game");
	if (sGame != "In The Groove 2" )
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

	m_sSuccessMsg = rNode->GetChildValue("Message");
	
	return true;
}

bool ScreenArcadePatch::CopyPatchContents()
{
	CString sErr;
	int iErr;
	RageFileDriverZip *rfdZip = new RageFileDriverZip;
	vector<CString> patchDirs;
	bool bReadError;

	if (! rfdZip->Load( Root ) )
	{
		m_Status.SetText( "Could not copy patch contents" );
		m_textHelp->SetText( THEME->GetMetric( "ScreenArcadePatch", "HelpTextError" ) );
		return false;
	}

	if (IsADirectory("Data/new-patch-unchecked")) {
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
			RageFileBasic *fCopySrc;
			RageFile fCopyDest;
			fCopySrc = rfdZip->Open( sPath, RageFile::READ, iErr );
			fCopyDest.Open( "Data/new-patch-unchecked/" + sPath, RageFile::WRITE );
			if (! FileCopy( *fCopySrc, fCopyDest, sErr, &bReadError ) )
			{
				LOG->Warn("Failed to copy file %s", sPath.c_str() );
				m_Status.SetText( ssprintf("Failed to copy file %s, cannot proceed", sPath.c_str()) );
				return false;
			}
			const RageFileDriverZip::FileInfo *fi = rfdZip->GetFileInfo( sPath );
			fCopyDest.Close();

#ifdef LINUX
#ifdef ITG_ARCADE
			sPath = "/stats/new-patch-unchecked/" + sPath;
#else
			sPath = "Data/new-patch-unchecked/" + sPath;
#endif
			chmod( sPath.c_str(), fi->m_iFilePermissions );
#endif

			
		}
		rfdZip->GetDirListing( sDirPath + "/*", patchDirs, true, true );

	}
#ifdef ITG_ARCADE
	rename("/stats/new-patch-unchecked", "/stats/new-patch" );
#else
	rename("Data/new-patch-unchecked", "Data/new-patch" );
#endif
	return true;		
}
