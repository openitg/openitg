#include "global.h"
#include "ScreenArcadePatch.h"
#include "ScreenManager.h"
#include "RageLog.h"
#include "InputMapper.h"
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

#include "Foreach.h"		// Foreach loops without the command is hard.
#include "MemoryCardManager.h"	// Where else are we getting the patch from?
#include "GameState.h"		// Check which USB...
#include "ProfileManager.h"	// Check for USB...
#include "RageUtil.h"		// I need this for copying the patch
#include "RageFile.h"		// For the .itg extraction

REGISTER_SCREEN_CLASS( ScreenArcadePatch );

ScreenArcadePatch::ScreenArcadePatch( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadePatch::ScreenArcadePatch()" );
}

void ScreenArcadePatch::Init()
{
	ScreenWithMenuElements::Init();

	this->SortByDrawOrder();
	
	m_Status.LoadFromFont( THEME->GetPathF("ScreenArcadePatch","text") );
	m_Patch.LoadFromFont( THEME->GetPathF("ScreenArcadePatch","text") );
	
	m_Status.SetName( "State" );
	m_Patch.SetName( "List" );
	
	m_Status.SetXY( SCREEN_CENTER_X , SCREEN_CENTER_Y );
	m_Patch.SetXY( SCREEN_CENTER_X , SCREEN_CENTER_Y + 200 );

	m_Status.SetZoom( 0.6 );

	m_Status.SetHidden( false );
	m_Patch.SetHidden( true );
	
	this->AddChild( &m_Status );
	this->AddChild( &m_Patch );
	
	bChecking = false;
}

void ScreenArcadePatch::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore
	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenArcadePatch::Update( float fDeltaTime )
{
	if( !bChecking )
		CommitPatch();
	Screen::Update(fDeltaTime);
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
		SCREENMAN->PlayStartSound();
		StartTransitioning( SM_GoToPrevScreen );		
	}
}

bool ScreenArcadePatch::CommitPatch()
{
	if( CheckCards() )
	{
		if( MountCards() )
		{
			if( ScanPatch() )
				CopyPatch();
			
			UnmountCards();
			
			if( bScanned )
				if( CheckSignature() )
					if( CheckXml() )
					{
						m_Status.SetText(m_sSuccessMsg);
						return true;
					}
		}
	} else
		bChecking = false;
	
	return false;
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
			pn = p;
			m_Status.SetText( ssprintf( "Checking Player %d's card for a patch..." , p + 1 ) );
			return true;
		}
	}
	
	// No cards found
	m_Status.SetText( "Please insert a USB Card containing an update." );

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
	if( MEMCARDMAN->MountCard( pn ) )
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
	m_Patch.SetText( aPatches[0].c_str() );
	m_Patch.SetHidden( false );
	
	bScanned = true;
	
	return true;
}

bool ScreenArcadePatch::CopyPatch()
{
	Root = ssprintf( "/rootfs/tmp/%s" , aPatches[0].c_str() );
	if( FileCopy( sFile , Root ) )
	{
		m_Status.SetText( "Patch copied! Checking..." );
		return true;
	} else {
		m_Status.SetText( "Patch copying failed!! Please re-insert your card and try again!" );
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
	GetFileContents("/Data/Patch.rsa", patchRSA);
	///////////////////////////////////////////////////////////

	fSig = new RageFileDriverSlice( rf, filesize - 128, 128 );
	if (fSig->Read(patchSig, 128) < 128) {
		m_Status.SetText( "Patch signature verification failed: unexpected end of file" );
		return false;
	}
	fZip = new RageFileDriverSlice( rf, 0, filesize - 128 );
	
	if (! CryptHelpers::VerifyFile( *fZip, patchSig, patchRSA, sErr ) ) {
		m_Status.SetText( ssprintf("Patch signature verification failed:\n%s", sErr.c_str()) );
		return false;
	}

	m_Status.SetText( "Patch signature verified :)" );
	return true;
}

bool ScreenArcadePatch::CheckXml()
{
	CString sErr, sResultMessage;
	int iErr;
	unsigned filesize;
	RageFileBasic *fRoot, *fScl, *fXml;
	RageFileDriverZip *rfdZip = new RageFileDriverZip;
	XNode *rNode = new XNode;

	fRoot = FILEMAN->Open( Root, RageFile::READ, iErr );
	filesize = GetFileSizeInBytes( Root ) - 128;
	fScl = new RageFileDriverSlice( fRoot, 0, filesize );

	
	if (! rfdZip->Load(fScl)) {
		m_Status.SetText( "Patch XML data check failed" );
		return false;
	}

	fXml = rfdZip->Open("patch.xml", RageFile::READ, iErr );

	if (fXml == NULL) {
		m_Status.SetText( "Could not open patch.xml" );
		return false;
	}

	rNode->m_sName = "Patch";
	rNode->LoadFromFile(*fXml);
	m_sSuccessMsg = rNode->GetChildValue("Message");
	
	return true;
}

