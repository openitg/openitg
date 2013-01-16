#include "global.h"
#include "RageLog.h"
#include "MemoryCardManager.h"		// for HasPatch
#include "ScreenArcadePatch.h"
#include "DiagnosticsUtil.h"	// for GetRevision()
#include "HelpDisplay.h"	// so we can set the help text
#include "CryptHelpers.h"	// to verify signature
#include "XmlFile.h"		// for XML data handling

// manual file handling
#include "RageFileDriverTimeout.h"
#include "RageFileDriverZip.h"
#include "RageFileDriverSlice.h"
#include "RageFileDriverMemory.h"

#include "arch/ArchHooks/ArchHooks.h"

#include <cerrno>

// for chmod (which mysteriously never needed a header before...)
#ifdef LINUX
#include <sys/stat.h>
#endif

AutoScreenMessage( SM_Reboot )

#define NEXT_SCREEN			THEME->GetMetric(m_sName, "NextScreen")

#define WAITING_HELP_TEXT		THEME->GetMetric(m_sName, "HelpTextWaiting")
#define ERROR_HELP_TEXT			THEME->GetMetric(m_sName, "HelpTextError")
#define FINISHED_HELP_TEXT		THEME->GetMetric(m_sName, "HelpTextFinished")

/* XXX: I want to refactor this and make it unnecessary. 
 * Using defines so we can easily change this later. */
CString g_StateText, g_PatchText;
#define STATE_TEXT(msg)			g_StateText = msg
#define PATCH_TEXT(msg)			g_PatchText = msg

/* name of the patch file in memory - hard drive space is limited. */
const CString ITG_TEMP_FILE = "/tmp-patch.itg";

/* directory where patch data is decompressed and saved */
const CString TEMP_PATCH_DIR	= "Data/new-patch-unchecked/";

/* if PC build, move to Data/patch/ on completion. They're
 * not likely to have boot scripts or modify patch data. */
#ifndef ITG_ARCADE
const CString FINAL_PATCH_DIR	= "Data/patch/";
#else
const CString FINAL_PATCH_DIR	= "Data/new-patch/";
#endif

REGISTER_SCREEN_CLASS( ScreenArcadePatch );

ScreenArcadePatch::ScreenArcadePatch( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenArcadePatch::ScreenArcadePatch()" );
	RageFileDriverTimeout::SetTimeout( 3600 );

	m_MemDriver = NULL;
	m_PatchFile = NULL;
}

ScreenArcadePatch::~ScreenArcadePatch()
{
	LOG->Trace( "ScreenArcadePatch::~ScreenArcadePatch()" );
	RageFileDriverTimeout::ResetTimeout();

	if( m_MemDriver && m_PatchFile )
		m_MemDriver->Remove( ITG_TEMP_FILE );

	SAFE_DELETE( m_PatchFile );
	SAFE_DELETE( m_MemDriver );
}

void ScreenArcadePatch::Init()
{
	ScreenWithMenuElements::Init();

	// HACK: set last updated differently to force an update
	m_State = PATCH_NONE;
	m_LastUpdatedState = PATCH_ERROR;

	/* initialize BitmapText actors */
	m_StateText.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );
	m_PatchText.LoadFromFont( THEME->GetPathF("ScreenArcadePatch", "text") );

	m_StateText.SetName( "State" );
	m_PatchText.SetName( "List" );

	SET_XY_AND_ON_COMMAND( m_StateText );
	SET_XY_AND_ON_COMMAND( m_PatchText );

	/* this will be picked up on the first update */
	STATE_TEXT( "Please wait..." );
	m_textHelp->SetText( WAITING_HELP_TEXT );

	this->AddChild( &m_StateText );
	this->AddChild( &m_PatchText );
	this->SortByDrawOrder();

	/* init the patch thread, but don't start it yet */
	m_Thread.SetName( "Patch thread" );
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

		// we explicitly synced. don't bother now.
		HOOKS->SystemReboot( false );
	}
}

void ScreenArcadePatch::Update( float fDeltaTime )
{
	/* start the thread once the opening transition has finished. */
	if( !m_Thread.IsCreated() && !this->IsTransitioning() )
			m_Thread.Create( PatchThread_Start, this );

	Screen::Update( fDeltaTime );

	m_StateText.SetText( g_StateText );
	m_PatchText.SetText( g_PatchText );

	// nothing to do.
	if( m_State == m_LastUpdatedState )
		return;

	m_LastUpdatedState = m_State;

	// update the HelpText
	switch( m_State )
	{
	case PATCH_NONE:
		STATE_TEXT( "Please insert a USB card containing an update." );
		PATCH_TEXT( THEME->GetMetric("ScreenArcadePatch","IntroText") );
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

void ScreenArcadePatch::MenuStart( PlayerNumber pn )
{
	// ignore presses while we're checking for patches
	if( m_State == PATCH_CHECKING )
		return;

	// ignore presses while we're transitioning
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

	STATE_TEXT( ssprintf("Checking Player %d's card for a patch...", pn+1) );

	// attempt to read from the card directly
	m_sProfileDir = MEM_CARD_MOUNT_POINT[pn];

	// fix up the path so we can append to it
	if( m_sProfileDir.Right(1) != "/" )
		m_sProfileDir += "/";

	// mount the card for a minute so we can check for files on it
	if( !MEMCARDMAN->MountCard(pn, 60) )
	{
		STATE_TEXT( ssprintf("Error mounting Player %d's card!", pn+1) );
		return false;
	}

	for( unsigned i = 0;i < vsPatterns.size(); i++ )
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
	LOG->Trace( sDebugMsg.c_str() );

	return true;
}

bool ScreenArcadePatch::VerifyPatch( RageFileBasic *pFile, const CStringArray &vsKeyPaths )
{
	/* get the 128-byte patch signature from the end of the file */
	unsigned iFileSize = pFile->GetFileSize() - 128;

	/* fZip contains the ZIP data, fSig contains the RSA signature */
	RageFileBasic *fZip = new RageFileDriverSlice( pFile, 0, iFileSize );
	RageFileBasic *fSig = new RageFileDriverSlice( pFile, iFileSize, 128 );

	CString sPatchSig;
	if( fSig->Read(sPatchSig, 128) < 128 )
	{
		STATE_TEXT( "Patch signature verification failed:\nunexpected end of file." );
		SAFE_DELETE( fSig );
		SAFE_DELETE( fZip );
		return false;
	}

	// attempt to check the signature against all given RSA keys
	bool bVerified = false;

	// if no key paths worked, use this error message
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
		LOG->Trace( "Attempting to verify with %s.", vsKeyPaths[i].c_str() );

		// attempt to verify, early abort if we have a match.
		if( CryptHelpers::VerifyFile(*fZip, sPatchSig, sRSAData, sError) )
			bVerified = true;

		// ignore signature mismatches
		else if( !sError.CompareNoCase("Signature mismatch") )
			continue;

		// if we encountered anything besides a mismatch, continue processing
		break;
	}

	CString sMessage;
	if( bVerified )
		sMessage = "Patch signature verified.";
	else
		sMessage = ssprintf( "Patch signature verification failed:\n"
			"%s\n\n" "The patch file may be corrupt.", sError.c_str() );

	LOG->Trace( sMessage );
	STATE_TEXT( sMessage );

	SAFE_DELETE( fSig );
	SAFE_DELETE( fZip );
	
	return bVerified;
}

bool ScreenArcadePatch::GetXMLData( RageFileDriverZip *fZip, CString &sGame, CString &sMessage, int &iRevision )
{
	int iError;
	RageFileBasic *fXML = fZip->Open( "patch.xml", RageFile::READ, iError );

	if( fXML == NULL )
	{
		STATE_TEXT( "Patch information check failed: could not open patch.xml." );
		return false;
	}

	/* check the actual XML data now */
	XNode *pNode = new XNode;
	pNode->m_sName = "Patch";
	pNode->LoadFromFile( *fXML );

	if( !pNode->GetChild("Game") || !pNode->GetChild("Revision") || !pNode->GetChild("Message") )
	{
		STATE_TEXT( "Patch information check failed: patch.xml is corrupt." );
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

bool UpdateProgress( uint64_t iCurrent, uint64_t iTotal )
{
	float fPercent = iCurrent / (iTotal/100);
	CString sProgress = ssprintf( "Copying patch (%.0f%%)\n\n"
		"Please do not remove the USB Card.", fPercent );

	PATCH_TEXT( sProgress );
	return true;
}

/* helper functions to get the actual file paths for CHMODing. */

#ifdef LINUX
static CString GetDataMountPath()
{
#ifdef ITG_ARCADE
// XXX: not currently used
	return "/stats/";
#endif // ITG_ARCADE

	static CString sMountPath;

	// optimization
	if( !sMountPath.empty() )
		return sMountPath;

	/* We need to get the fully qualified path. This is not pleasant... */
	vector<RageFileManager::DriverLocation> mountpoints;
	FILEMAN->GetLoadedDrivers( mountpoints );

	// the first direct driver mounted to / should be our root.
	for( unsigned i = 0; i < mountpoints.size(); i++ )
	{
		RageFileManager::DriverLocation *l = &mountpoints[i];

		if( l->Type != "dir" )
			continue;

		if( l->MountPoint != "/" )
			continue;

		sMountPath = l->Root;
		break;
	}

	CollapsePath( sMountPath );
	LOG->Trace( "DataMountPath resolved to \"%s\".", sMountPath.c_str() );

	return sMountPath;
}

// needed in order to get the actual path for chmod.
static CString ResolveTempFilePath( const CString &sFile )
{
	CString ret;
#ifdef ITG_ARCADE
	ret = TEMP_PATCH_DIR;
	ret.Replace( "Data/", "/stats/" );
	ret += sFile;
#else
	ret = GetDataMountPath() + "/" + TEMP_PATCH_DIR + sFile;
#endif // ITG_ARCADE

	return ret;
}

#endif // LINUX

void ScreenArcadePatch::PatchMain()
{
	m_State = PATCH_CHECKING;

        /* NOTE: we used to look for OpenITG prefixed updates also, but the
         * original ITG binary does not, so if we want people to be able to
         * upgrade directly from ITG to OITG, we have to continue to use the
         * "ITG 2 " prefix.  Sad times. */

	/* set up patterns to match */
	CStringArray vsPatterns;
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
	PATCH_TEXT( "" );

	/* set up the key paths we want to verify against */
	CStringArray vsRSAPaths;
	vsRSAPaths.push_back( "Data/Patch-OpenITG.rsa" );
	vsRSAPaths.push_back( "Data/Patch.rsa" );

	/* create the path for the patch file */
	CString sPatchFile = m_sProfileDir + m_vsPatches[0];

	// copy the patch file into memory
	// TODO: see if we can FILEMAN->GetFileDriver( "/@mem/" ).
	m_MemDriver = new RageFileDriverMem;

	int iError = 0;
	m_PatchFile = m_MemDriver->Open( ITG_TEMP_FILE, RageFile::WRITE, iError );
	
	if( iError != 0 )
	{
		STATE_TEXT( ssprintf("Failed to open temporary file:\n%s", strerror(iError)) );
		m_State = PATCH_ERROR;
	}

	/* give up to an hour for the data to copy */
	MEMCARDMAN->MountCard( pn, 3600 );

	RageFile patch;
	if( !patch.Open(sPatchFile) )
	{
		STATE_TEXT( ssprintf("Failed to open patch file (%s):\n%s",
			sPatchFile.c_str(), patch.GetError().c_str()) );

		m_State = PATCH_ERROR;
	}

	if( m_State == PATCH_ERROR )
	{
		MEMCARDMAN->UnmountCard(pn);
		return;
	}

	CString sError;
	if( FileCopy( patch, *m_PatchFile, sError, NULL, UpdateProgress) )
	{
		STATE_TEXT( "Patch copied! Checking..." );
		PATCH_TEXT( "" );
	}
	else
	{
		STATE_TEXT( ssprintf("Patch copying failed:\n" "%s", sError.c_str()) );
		m_State = PATCH_ERROR;
	}

	MEMCARDMAN->UnmountCard( pn );

	if( m_State == PATCH_ERROR )
		return;

	/* re-open the patch file read-open. this should not fail. */
	m_PatchFile = m_MemDriver->Open( ITG_TEMP_FILE, RageFile::READ, iError );

	if( !this->VerifyPatch(m_PatchFile, vsRSAPaths) )
	{
		PATCH_TEXT( "" );
		m_State = PATCH_ERROR;
		return;
	}

	/* reset the error checker */
	iError = 0;

	/* check the XML data */
	CString sGame, sSuccessMessage;
	int iPatchRevision;

	/* open our new copy and read from it */
	RageFileDriverZip *fZip = new RageFileDriverZip( m_PatchFile );

	// we'll catch this in a bit, after we've freed our memory
	if( !this->GetXMLData(fZip, sGame, sSuccessMessage, iPatchRevision) )
		m_State = PATCH_ERROR;

	SAFE_DELETE( fZip );

	// if the XML get earlier failed, return now.
	if( m_State == PATCH_ERROR )
		return;

	// accept patches for oITG or ITG2
	if( sGame != "OpenITG" && sGame != "In The Groove 2" )
	{
		sError = ssprintf( "revision is for another game\n" "(\"%s\")", sGame.c_str() );
		STATE_TEXT( ssprintf("Cannot proceed: %s", sError.c_str()) );
		m_State = PATCH_ERROR;
		return;
	}

	int iCurrentRevision = DiagnosticsUtil::GetRevision();

	// HACK: allow any patch at all if it's revision 1.
	if( iCurrentRevision != 1 && iCurrentRevision == iPatchRevision )
	{
		sError = ssprintf( "patch revision (%d) matches the machine revision.", iPatchRevision );
		STATE_TEXT( ssprintf("Cannot proceed: %s", sError.c_str()) );
		m_State = PATCH_ERROR;
		return;
	}

	/* wipe any unsucessful or unused patch data */
	DeleteRecursive( TEMP_PATCH_DIR );
	DeleteRecursive( FINAL_PATCH_DIR );

	FILEMAN->CreateDir( TEMP_PATCH_DIR );

	/* re-open the ZIP file now */
	fZip = new RageFileDriverZip;
	if( !fZip->Load(m_PatchFile) )
	{
		PATCH_TEXT( "Failed to re-open ZIP file!" );
		m_State = PATCH_ERROR;
		return;
	}

	CStringArray vsDirs, vsFiles;
	vsDirs.push_back( "/" );

	/* find all the files we're going to write with a recursive check */
	while( vsDirs.size() )
	{
		CString sDir = vsDirs.back();
		vsDirs.pop_back();

		fZip->GetDirListing( sDir + "/*", vsFiles, false, true );
		fZip->GetDirListing( sDir + "/*", vsDirs, true, true );
	}

	PATCH_TEXT( "Extracting files..." );

	/* write them now */
	for( unsigned i = 0; i < vsFiles.size(); i++ )
	{
		const CString &sPath = vsFiles[i];
		CString sCleanPath = sPath;
		TrimLeft( sCleanPath, "/" );

		if( fZip->GetFileType(sPath) != RageFileManager::TYPE_FILE )
			continue;

		LOG->Trace( "ScreenArcadePatch: copying file \"%s\"", sCleanPath.c_str() );
		PATCH_TEXT( ssprintf("Extracting files...\n" "%s", sCleanPath.c_str()) );

		RageFileBasic *fCopyFrom = fZip->Open( sPath, RageFile::READ, iError );

		RageFile fCopyTo;
		fCopyTo.Open( TEMP_PATCH_DIR + sCleanPath, RageFile::WRITE );

		if( !FileCopy(*fCopyFrom, fCopyTo, sError) )
		{
			PATCH_TEXT( ssprintf("Could not copy \"%s\":\n" "%s", sCleanPath.c_str(), sError.c_str()) );

			m_State = PATCH_ERROR;
			SAFE_DELETE( fCopyFrom );
			SAFE_DELETE( fZip );
			return;
		}

		SAFE_DELETE( fCopyFrom );
		fCopyTo.Close();

/* set CHMOD info */
#ifdef LINUX
		/* get the actual path to the files */
		CString sRealPath = ResolveTempFilePath( sCleanPath );

		const RageFileDriverZip::FileInfo *fi = fZip->GetFileInfo( sPath );
		int ret = chmod( sRealPath.c_str(), fi->m_iFilePermissions );

		LOG->Trace( "chmod( %s, %#o ) returned %i", sRealPath.c_str(), fi->m_iFilePermissions, ret );
#endif // LINUX
	}

	SAFE_DELETE( fZip );

	/* clear the previous copying text */
	STATE_TEXT( "Finalizing patch data..." );
	PATCH_TEXT( "" );

#ifdef LINUX
	sync(); sleep(5);
#endif

	/* we've successfully copied everything. now, move the directory and we're done. */
	if( FILEMAN->Move(TEMP_PATCH_DIR, FINAL_PATCH_DIR) )
	{
		STATE_TEXT( sSuccessMessage );
		m_State = PATCH_INSTALLED;
	}
	else
	{
		STATE_TEXT( "Failed to finalize patch data!\nCheck your system permissions" );
		m_State = PATCH_ERROR;
	}
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

