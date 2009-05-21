#include "global.h"
#include "RageLog.h"
#include "MemoryCardManager.h"		// for HasPatch
#include "ProfileManager.h"		// for HasPatch
#include "GameConstantsAndTypes.h"	// for memory card state
#include "ScreenArcadePatch.h"
#include "ScreenMessage.h"	// for AutoScreenMessage
#include "ActorUtil.h"		// for SET_XY_AND_ON_COMMAND
#include "DiagnosticsUtil.h"	// for GetRevision()
#include "RageUtil.h"		// for join
#include "HelpDisplay.h"	// so we can set the help text
#include "CryptHelpers.h"	// to verify signature
#include "XmlFile.h"		// for XML data handling

// file handling
#include "RageFileManager.h"
#include "RageFileDriverZip.h"
#include "RageFileDriverSlice.h"
#include "RageFile.h"

#include "arch/ArchHooks/ArchHooks.h"

#include <cerrno>

#define NEXT_SCREEN			THEME->GetMetric(m_sName, "NextScreen")

#define WAITING_HELP_TEXT		THEME->GetMetric(m_sName, "HelpTextWaiting")
#define ERROR_HELP_TEXT			THEME->GetMetric(m_sName, "HelpTextError")
#define FINISHED_HELP_TEXT		THEME->GetMetric(m_sName, "HelpTextFinished")

/* store the patch in memory, since the hard drive space is limited */
const CString ITG_TEMP_PATH	= "/@mem/patch.itg";

const CString TEMP_PATCH_DIR	= "Data/new-patch-unchecked/";

/* if on a Win32 system, then copy the data directly into Data/patch/. */
#ifdef WIN32
const CString FINAL_PATCH_DIR	= "Data/patch/";
#else
const CString FINAL_PATCH_DIR	= "Data/new-patch/";
#endif

AutoScreenMessage( SM_Reboot )

/* XXX: I really don't like doing this... */
CString g_StateText, g_PatchText;

REGISTER_SCREEN_CLASS( ScreenArcadePatch )

ScreenArcadePatch::ScreenArcadePatch( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadePatch::ScreenArcadePatch()" );
}

ScreenArcadePatch::~ScreenArcadePatch()
{
	LOG->Trace( "ScreenArcadePatch::~ScreenArcadePatch()" );
}

void ScreenArcadePatch::Init()
{
	ScreenWithMenuElements::Init();

	m_State = PATCH_NONE;
	m_LastUpdatedState = PATCH_NONE;

	/* initialize BitmapText actors */
	m_StateText.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );
	m_PatchText.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );

	m_StateText.SetName( "State" );
	m_PatchText.SetName( "List" );

	SET_XY_AND_ON_COMMAND( m_StateText );
	SET_XY_AND_ON_COMMAND( m_PatchText );

	/* this will be picked up on the first update */
	g_StateText = THEME->GetMetric("ScreenArcadePatch", "IntroText");
	g_PatchText = "Please insert a USB card containing an update.";
	m_textHelp->SetText( WAITING_HELP_TEXT );

	this->AddChild( &m_StateText );
	this->AddChild( &m_PatchText );
	this->SortByDrawOrder();

	/* initialize the patch-checking thread */
	m_Thread.SetName( "Patch thread" );
	m_Thread.Create( PatchThread_Start, this );
}

void ScreenArcadePatch::HandleScreenMessage( const ScreenMessage SM )
{
	if( SM == SM_GoToNextScreen )
	{
		SCREENMAN->SetNewScreen( NEXT_SCREEN );
	}
	else if( SM == SM_Reboot )
	{
		// force an unmount, just in case.
		FOREACH_PlayerNumber( pn )
			MEMCARDMAN->UnmountCard( pn );

		HOOKS->SystemReboot();
	}
}

void ScreenArcadePatch::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );

	m_StateText.SetText( g_StateText );
	m_PatchText.SetText( g_PatchText );

	if( m_State == m_LastUpdatedState )
		return;

	m_LastUpdatedState = m_State;

	// update the HelpText
	switch( m_State )
	{
	case PATCH_NONE:
	case PATCH_CHECKING:
		m_textHelp->SetText( WAITING_HELP_TEXT );
		break;
	case PATCH_ERROR:
		m_textHelp->SetText( ERROR_HELP_TEXT );
		break;
	case PATCH_INSTALLED:
		m_textHelp->SetText( FINISHED_HELP_TEXT );
		break;
	}
}

// ignore input while transitions occur and we're checking for patches
void ScreenArcadePatch::MenuStart( PlayerNumber pn )
{
	if( m_State == PATCH_CHECKING )
		return;

	if( this->IsTransitioning() )
		return;

	this->PlayCommand( "Off" );
	
	/* if the patch is installed, we need to restart the game. */
	if( m_State != PATCH_INSTALLED )
		this->StartTransitioning( SM_GoToNextScreen );
	else
		this->StartTransitioning( SM_Reboot );
}

// check to see if this player's card exists and if it has any matches
// to the given pattern. if so, then add matches into the patches list.
bool ScreenArcadePatch::HasPatch( PlayerNumber pn, const CStringArray &vsPatterns )
{
	// no card for this player
	if( MEMCARDMAN->GetCardState(pn) != MEMORY_CARD_STATE_READY )
		return false;

	// no reason to check, since we already have a patch
	if( m_vsPatches.size() )
		return false;

	g_StateText = ssprintf( "Checking Player %d's card for a patch...", pn+1 );

	// attempt to read from the card directly
	m_sProfileDir = MEM_CARD_MOUNT_POINT[pn];

	// fix up the path so we can append to it
	if( m_sProfileDir.Right(1) != "/" )
		m_sProfileDir += "/";

	LOG->Debug( "Memory card mount point: %s", m_sProfileDir.c_str() );

	// mount the card for a minute so we can check for files on it
	if( !MEMCARDMAN->MountCard(pn, 60) )
	{
		g_StateText = ssprintf( "Error mounting Player %d's card!", pn+1 );
		return false;
	}

	for( unsigned i = 0; i < vsPatterns.size(); i++ )
	{
		LOG->Trace( "Finding matches for %s%s", m_sProfileDir.c_str(), vsPatterns[i].c_str() );
		GetDirListing( m_sProfileDir + vsPatterns[i], m_vsPatches );

		// if we found something, stop early.
		if( m_vsPatches.size() != 0 )
			break;
	}

	MEMCARDMAN->UnmountCard( pn );

	// no patches were found on the drive
	if( m_vsPatches.size() == 0 )
		return false;

	CString sDebugMsg = ssprintf( "%i match%s found: ", m_vsPatches.size(), (m_vsPatches.size() != 1) ? "es" : "" );
	sDebugMsg += join( ", ", m_vsPatches );
	LOG->Debug( sDebugMsg.c_str() );

	return true;
}

bool ScreenArcadePatch::VerifyPatch( const CString &sPath, const CStringArray &vsKeyPaths )
{
	if( !IsAFile(sPath) )
	{
		LOG->Warn( "VerifyPatch(): sPath doesn't exist! (%s)", sPath.c_str() );
		g_StateText = ssprintf( "Error: could not access patch file!\n" "(%s)", sPath.c_str() );
		return false;
	}

	/* for that special someone */
	g_PatchText = "Please do not remove the USB card.";

	/* get the 128-byte patch signature from the end of the file */
	int iError = 0;
	RageFileBasic *fRoot = FILEMAN->Open( sPath, RageFile::READ, iError );

	if( fRoot == NULL )
	{
		g_StateText = ssprintf( "Error verifying patch file:\n%s (%d)", strerror(iError), iError);
		return false;
	}

	unsigned iFileSize = GetFileSizeInBytes( sPath ) - 128;

	/* fZip contains the ZIP data, fSig contains the RSA signature */
	RageFileBasic *fZip = new RageFileDriverSlice( fRoot, 0, iFileSize );
	RageFileBasic *fSig = new RageFileDriverSlice( fRoot, iFileSize, 128 );

	CString sPatchSig;
	if( fSig->Read(sPatchSig, 128) < 128 )
	{
		g_StateText = "Patch signature verification failed:\nunexpected end of file.";
		SAFE_DELETE( fSig );
		SAFE_DELETE( fZip );
		SAFE_DELETE( fRoot );
		return false;
	}

	// attempt to check the signature against all given RSA keys
	bool bVerified = false;
	CString sError = "no signature keys available";

	for( unsigned i = 0; i < vsKeyPaths.size(); i++ )
	{
		if( !IsAFile(vsKeyPaths[i]) )
		{
			LOG->Warn( "RSA key \"%s\" missing.", vsKeyPaths[i].c_str() );
			continue;
		}

		CString sRSAData;
		GetFileContents( vsKeyPaths[i], sRSAData );
		LOG->Debug( "Attempting to verify %s with %s.", sPath.c_str(), vsKeyPaths[i].c_str() );

		CString s;
		for( unsigned i=0; i<sRSAData.size(); i++ )
		{
			unsigned val = sRSAData[i];
			s += ssprintf( "%x", val );
		}
		LOG->Debug( "%s", s.c_str() );

		// attempt to verify, early abort if we have a match.
		if( CryptHelpers::VerifyFile(*fZip, sPatchSig, sRSAData, sError) )
			bVerified = true;

		// ignore signature mismatches
		else if( !sError.CompareNoCase("Signature mismatch") )
			continue;

		// if we encountered anything more than a mismatch, abort.
		break;
	}

	CString sMessage;
	if( bVerified )
		sMessage = "Patch signature verified. ☺";
	else
		sMessage = ssprintf( "Patch signature verification failed:\n"
			"%s\n\n" "The patch file may be corrupt.", sError.c_str() );

	g_StateText = sMessage;

	SAFE_DELETE( fSig );
	SAFE_DELETE( fZip );
	SAFE_DELETE( fRoot );
	
	return bVerified;
}

bool ScreenArcadePatch::GetXMLData( RageFileDriverZip *fZip, CString &sGame, CString &sMessage, int &iRevision )
{
	int iError;
	RageFileBasic *fXML = fZip->Open( "patch.xml", RageFile::READ, iError );

	if( fXML == NULL )
	{
		g_StateText = "Patch information check failed: could not open patch.xml.";
		return false;
	}

	/* check the actual XML data now */
	XNode *pNode = new XNode;
	pNode->m_sName = "Patch";
	pNode->LoadFromFile( *fXML );

	if( !pNode->GetChild("Game") || !pNode->GetChild("Revision") || !pNode->GetChild("Message") )
	{
		g_StateText = "Patch information check failed: patch.xml is corrupt.";
		SAFE_DELETE( pNode );
		SAFE_DELETE( fXML );
		return false;
	}

	/* save the patch data */
	pNode->GetChild("Revision")->GetValue(iRevision);
	sGame = pNode->GetChildValue("Game");
	sMessage = pNode->GetChildValue("Message");

	SAFE_DELETE( pNode );
	SAFE_DELETE( fXML );

	return true;
}

void UpdateProgress( float fPercent )
{
	CString sProgress = ssprintf( "Copying patch (%.0f%%)\n\n"
		"Please do not remove the USB Card.", fPercent );

	g_PatchText = sProgress;	
}

void ScreenArcadePatch::PatchMain()
{
	m_State = PATCH_CHECKING;

	/* set up patterns to match, in order of priority. */
	CStringArray vsPatterns;
	vsPatterns.push_back( "OpenITG *.itg" );
	vsPatterns.push_back( "ITG 2 *.itg" );

	/* check to see if either player has patches */
	PlayerNumber pn = PLAYER_INVALID;

	/* HasPatch() will early abort for us, due to the size() check,
	 * if P1 and P2 both have patches. We just want the first one. */
	FOREACH_PlayerNumber( p )
		if( this->HasPatch(p, vsPatterns) )
			pn = p;

	/* no matches on either card. */
	if( pn == PLAYER_INVALID )
	{
		m_State = PATCH_NONE;
		return;
	}

	/* set the help text to the patch name, blank the patch text */
	m_textHelp->SetText( m_vsPatches[0] );
	g_PatchText = "";

	/* set up the key paths we want to verify against */
	CStringArray vsRSAPaths;
	vsRSAPaths.push_back( "Data/Patch-OpenITG.rsa" );
	vsRSAPaths.push_back( "Data/Patch.rsa" );

	/* create the path for the patch file */
	CString sPatchFile = m_sProfileDir + m_vsPatches[0];

	/* give up to an hour for the data to verify and copy */
	MEMCARDMAN->MountCard( pn, 3600 );

	if( !VerifyPatch(sPatchFile, vsRSAPaths) )
	{
		m_State = PATCH_ERROR;
		g_PatchText.clear();
		MEMCARDMAN->UnmountCard( pn );
		return;
	}

	/* now that it's verified, copy it into memory */
	if( IsAFile(ITG_TEMP_PATH) )
		FILEMAN->Remove( ITG_TEMP_PATH );

	CString sError;
	if( CopyWithProgress( sPatchFile, ITG_TEMP_PATH, &UpdateProgress, sError) )
	{
		g_StateText = "Patch copied! Checking...";
		g_PatchText.clear();
	}
	else
	{
		g_StateText = ssprintf( "Patch copying failed:\n" "%s", sError.c_str() );
		m_State = PATCH_ERROR;
	}

	MEMCARDMAN->UnmountCard( pn );

	if( m_State == PATCH_ERROR )
		return;

	int iError = 0;

	/* this should never fail, so we won't worry about error handling... */
	unsigned int filesize = GetFileSizeInBytes( ITG_TEMP_PATH ) - 128;

	/* open our new copy and read from it */
	RageFileBasic *fRoot = FILEMAN->Open( ITG_TEMP_PATH, RageFile::READ, iError );
	RageFileDriverSlice *fPatch = new RageFileDriverSlice( fRoot, 0, filesize );
	RageFileDriverZip *fZip = new RageFileDriverZip( fPatch );

	/* check the XML data */
	CString sGame, sMessage;
	int iPatchRevision;

	// we'll catch this in a bit, after we've freed our memory
	if( !this->GetXMLData(fZip, sGame, sMessage, iPatchRevision) )
		m_State = PATCH_ERROR;

	SAFE_DELETE( fZip );
	SAFE_DELETE( fPatch );
	SAFE_DELETE( fRoot );

	// if the XML get earlier failed, return now.
	if( m_State == PATCH_ERROR )
		return;

	// accept patches for oITG or ITG2
	if( sGame != "OpenITG" && sGame != "In The Groove 2" )
	{
		sError = ssprintf( "revision is for another game\n" "(\"%s\")", sGame.c_str() );
		g_StateText = ssprintf( "Cannot proceed: %s", sError.c_str() );
		m_State = PATCH_ERROR;
		return;
	}

	int iCurrentRevision = DiagnosticsUtil::GetRevision();

	// HACK: allow any patch at all if it's revision 1.
	if( iCurrentRevision != 1 && iCurrentRevision == iPatchRevision )
	{
		sError = ssprintf( "patch revision (%d) matches the machine revision.", iPatchRevision );
		g_StateText = ssprintf( "Cannot proceed: %s", sError.c_str() );
		m_State = PATCH_ERROR;
		return;
	}

	/* wipe any data from a previous, unsuccessful patch */
	if( IsADirectory(TEMP_PATCH_DIR) )
	{
		FILEMAN->Remove( TEMP_PATCH_DIR + "*" );
		FILEMAN->Remove( TEMP_PATCH_DIR );
	}

	FILEMAN->CreateDir( TEMP_PATCH_DIR );

	CStringArray vsDirs, vsFiles;
	vsFiles.push_back( "" );	// this makes more sense than you think.

	/* re-open the ZIP file now */
	fZip = new RageFileDriverZip;
	fZip->Load( ITG_TEMP_PATH );

	/* find all the files we're going to write with a recursive check */
	while( vsDirs.size() )
	{
		CString sDir = vsDirs.back();
		vsDirs.pop_back();

		fZip->GetDirListing( sDir + "/*", vsFiles, false, true );
		fZip->GetDirListing( sDir + "/*", vsDirs, true, true );
	}

	/* write them now */
	for( unsigned i = 0; i < vsFiles.size(); i++ )
	{
		CString sPath = vsFiles[i];

		if( fZip->GetFileType(sPath) != RageFileManager::TYPE_FILE )
			continue;

		CString sCleanPath = sPath;
		TrimLeft( sCleanPath, "/" );

		LOG->Trace( "ScreenArcadePatch: copying file \"%s\"", sCleanPath.c_str() );
		g_PatchText = ssprintf( "Copying files:\n" "%s", sCleanPath.c_str() );

		RageFileBasic *fCopyFrom = fZip->Open( sPath, RageFile::READ, iError );

		RageFile fCopyTo;
		fCopyTo.Open( TEMP_PATCH_DIR + sCleanPath, RageFile::WRITE );

		if( !FileCopy(*fCopyFrom, fCopyTo, sError) )
		{
			g_PatchText = ssprintf("Could not copy \"%s\":\n" "%s",
				sCleanPath.c_str(), sError.c_str() );

			m_State = PATCH_ERROR;
			SAFE_DELETE( fCopyFrom );
			SAFE_DELETE( fZip );
			return;
		}

		fCopyTo.Close();

/* set CHMOD info if applicable */
#ifdef LINUX
		const RageFileDriverZip::FileInfo *fi = fZip->GetFileInfo( sPath );
		sPath = TEMP_PATCH_DIR + sPath;
		chmod( sPath.c_str(), fi->m_iFilePermissions );
#endif
	}

	/* we've successfully copied everything. now, move the directory and we're done. */
	rename( TEMP_PATCH_DIR.c_str(), FINAL_PATCH_DIR.c_str() );
}

/*
 * (c) 2008-2009 BoXoRRoXoRs, Matt Vandermeulen
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
