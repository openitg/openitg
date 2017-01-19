#include "global.h"
#include "UserPackManager.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageFileDriverZip.h"
#include "arch/ArchHooks/ArchHooks.h"

UserPackManager* UPACKMAN = NULL; // global and accessable from anywhere in our program

void UserPackManager::GetUserPacks( CStringArray &sAddTo )
{
	GetDirListing( USER_PACK_SAVE_PATH+"/*.zip", sAddTo, false, false );
}

void UserPackManager::MountAll()
{
	CStringArray asPacks;
	CString sError;

	/* get a listing of all packs in the directory to mount */
	GetUserPacks( asPacks );

	for( unsigned i = 0; i < asPacks.size(); i++ )
	{
		const CString sPackPath = USER_PACK_SAVE_PATH + asPacks[i];
		LOG->Info( "Loading user pack: %s", sPackPath.c_str() );

		if ( !IsPackMountable( sPackPath, sError ) ) // if it can't load it, forget it
		{
			LOG->Warn( "Error adding user pack %s: %s", sPackPath.c_str(), sError.c_str() );
			continue;
		}

		FILEMAN->Mount( "zip", sPackPath, GetPackMountPoint( sPackPath ), false );
	}
}

bool UserPackManager::Remove( const CString &sPack )
{
	/* it's okay if either one of these fails */
	FILEMAN->Unmount( "zip", sPack, "/Songs" );
	FILEMAN->Unmount( "zip", sPack, "/" );

	bool bDeleted = FILEMAN->Remove( sPack );

	/* refresh directory data after deletion */
	FILEMAN->FlushDirCache( USER_PACK_SAVE_PATH );

	return bDeleted;
}

/* Any packs containing these folders will be rejected from addition
 * due to possible conflicts, problems, or stability issues, */
static const unsigned NUM_BLACKLISTED_FOLDERS = 4;
static const char *BLACKLISTED_FOLDERS[] = { "Data", "Program", "Themes/default", "Themes/home" };

bool UserPackManager::IsPackMountable( const CString &sPack, CString &sError )
{
	RageFileDriverZip *pZip = new RageFileDriverZip;

	if ( !pZip->Load(sPack) )
	{
		sError = "not a valid zip file";
		SAFE_DELETE( pZip );
		return false;
	}

	for( unsigned i = 0; i < NUM_BLACKLISTED_FOLDERS && BLACKLISTED_FOLDERS[i] != NULL; i++ )
	{
		CString sDir = CString("/") + BLACKLISTED_FOLDERS[i];
		CStringArray sDirListing;

		pZip->GetDirListing( sDir, sDirListing, false, true );

		int iListSize = sDirListing.size();

		// if any blacklisted folders exist, reject the pack
		if ( iListSize > 0 )
		{ 
			sError = ssprintf( "blacklisted folder: %s", BLACKLISTED_FOLDERS[i] );
			SAFE_DELETE( pZip );
			return false;
		}
	}

	// do not add if it's just folders of individual songs without a group folder
	CStringArray asRootFolders;
	pZip->GetDirListing( "/*", asRootFolders, true, false );
	for( unsigned i = 0; i < asRootFolders.size(); i++ )
	{
		CStringArray asFiles;
		pZip->GetDirListing( "/" + asRootFolders[i] + "/*.sm", asFiles, false, false );
		pZip->GetDirListing( "/" + asRootFolders[i] + "/*.dwi", asFiles, false, false );
		if ( asFiles.size() > 0 )
		{
			sError = "Package is not a group folder package.\nPlease add songs to a single group folder.\n(i.e. {Group Name}/{Song Folder}/Song.sm)";
			SAFE_DELETE( pZip );
			return false;
		}
	}

	SAFE_DELETE( pZip );
	return true;
}

bool UserPackManager::IsPackTransferable( const CString &sPack, const CString &sPath, CString &sError )
{
	/* Check for duplicate names. */
	{
		CStringArray asSavedPacks;
		GetUserPacks( asSavedPacks );

		for( unsigned i = 0; i < asSavedPacks.size(); ++i )
		{
			if ( asSavedPacks[i].CompareNoCase(sPack) == 0 )
			{
				sError = ssprintf("%s is already on the machine", asSavedPacks[i].c_str());
				return false;
			}
		}
	}

	/* Do we have enough disk space? */
	{
		uint64_t iFree = HOOKS->GetDiskSpaceFree( USER_PACK_TRANSFER_PATH );
		uint64_t iFileSize = (uint64_t) FILEMAN->GetFileSizeInBytes( sPath );
		if( iFree < iFileSize )
		{
			sError = "Insufficient disk space";
			return false;
		}
	}

	return true;
}

bool UserPackManager::TransferPack( const CString &sPack, const CString &sDestPack, FileCopyFn CopyFn, CString &sError )
{
	bool bSuccess = FileCopy( sPack, USER_PACK_SAVE_PATH + "/" + sDestPack, sError, CopyFn );
	if (!bSuccess)
	{
		FILEMAN->FlushDirCache( USER_PACK_SAVE_PATH );
		FILEMAN->Remove( USER_PACK_SAVE_PATH + "/" + sDestPack );
	}

	return bSuccess;
}

/* use "" as a sentinel for the end of the list */
const CString asRootDirs[] = { "Themes", "Songs", "BackgroundEffects", "BGAnimations", "BackgroundTransitions", 
				"Courses", "NoteSkins", "RandomMovies", "" };

/* seemingly good start of an automagical way to mount a user pack based on the folder structure */
CString UserPackManager::GetPackMountPoint( const CString &sPack )
{
	enum UserPackMountType { UPACK_MOUNT_ROOT, UPACK_MOUNT_SONGS };

	RageFileDriverZip *pZip = new RageFileDriverZip;
	CHECKPOINT_M( sPack );
	// it should already be a valid zip by now...
	ASSERT( pZip->Load( sPack ) );

	CStringArray asRootEntries;
	pZip->GetDirListing( "/", asRootEntries, true, false );
	SAFE_DELETE( pZip );

	// if we find a StepMania root folder, mount it as one
	for( unsigned i = 0; i < asRootEntries.size(); ++i )
	{
		for( unsigned j = 0; j < ARRAYLEN(asRootDirs); j++ )
		{
			if ( asRootEntries[i].CompareNoCase( asRootDirs[j] ) == 0 )
				return "/";
		}
	}

	/* for now, assume a Songs-only pack if the root dirs aren't there */
	return "/Songs";
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

