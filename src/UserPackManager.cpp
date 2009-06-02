#include "global.h"
#include "UserPackManager.h"
#include "RageFileManager.h"
#include "RageUtil.h"
#include "Foreach.h"
#include "RageLog.h"
#include "RageFileDriverZip.h"

UserPackManager* UPACKMAN = NULL; // global and accessable from anywhere in our program

UserPackManager::UserPackManager( const CString sTransferPath, const CString sSavePath )
{
	m_sTransferPath = sTransferPath;
	m_sSavePath = sSavePath;
}

void UserPackManager::GetCurrentUserPacks( CStringArray &sAddTo )
{
	GetDirListing( m_sSavePath+"/*.zip", sAddTo, false, false ); /**/
}

void UserPackManager::MergePacksToVFS()
{
	CStringArray asPacks;
	CString sError;

	GetCurrentUserPacks( asPacks );

	for( unsigned i = 0; i < asPacks.size(); i++ )
	{
		const CString sPackPath = m_sSavePath + asPacks[i];

		LOG->Info( "Loading User Pack: %s", sPackPath.c_str() );

		if ( !IsPackAddable( sPackPath, sError ) ) // if it can't load it, forget it
		{
			LOG->Warn( "Error adding user pack %s: %s", sPackPath.c_str(), sError.c_str() );
			continue;
		}

		FILEMAN->Mount( "zip", sPackPath, GetPackMountPoint( sPackPath ), false );
	}
}

void UserPackManager::AddBlacklistedFolder( const CString sBlacklistedFolder )
{
	m_asBlacklistedFolders.push_back( sBlacklistedFolder );
}

bool UserPackManager::UnlinkAndRemovePack( const CString sPack )
{
	FILEMAN->Unmount("zip", sPack, "/Songs");
	FILEMAN->Unmount("zip", sPack, "/");
	bool bRet = FILEMAN->Remove( sPack );
	FILEMAN->FlushDirCache( m_sSavePath );
	return bRet;
}

bool UserPackManager::IsPackAddable( const CString sPack, CString &sError )
{
	RageFileDriverZip *pZip = new RageFileDriverZip;

	if ( ! pZip->Load( sPack ) )
	{
		sError = "not a valid zip file";
		SAFE_DELETE( pZip );
		return false;
	}

	for( unsigned i = 0; i < m_asBlacklistedFolders.size(); i++ )
	{
		/* XXX: might have to use GetDirListing() instead */
		if ( pZip->GetFileInfo( m_asBlacklistedFolders[i] ) != NULL )
		{ // blacklisted file/folder, don't add
			sError = ssprintf("blacklisted content: %s", m_asBlacklistedFolders[i].c_str());
			SAFE_DELETE( pZip );
			return false;
		}
	}

	// do not add if it's just folders of individual songs without a group folder
	CStringArray asSMFiles;
	pZip->GetDirListing( "*/*.sm", asSMFiles, false, false ); /**/
	if ( asSMFiles.size() > 0 )
	{
		sError = "Package is not a group folder.\nPlease add songs to a single group folder.\n(i.e. {Group Name}/{Song Folder}/Song.sm)";
		SAFE_DELETE( pZip );
		return false;
	}

	SAFE_DELETE( pZip );
	return true;
}

bool UserPackManager::IsPackTransferable( const CString sPack, CString &sError )
{
	CStringArray asSavedPacks;
	GetCurrentUserPacks( asSavedPacks );
	for ( unsigned i = 0; i < asSavedPacks.size(); i++ )
	{
		if ( asSavedPacks[i].CompareNoCase(sPack) == 0 )
		{
			sError = ssprintf("%s is already on the machine", asSavedPacks[i].c_str());
			return false;
		}
	}
	return true;
}

bool UserPackManager::TransferPack( const CString sPack, const CString sDestPack, void(*OnUpdate)(float), CString &sError )
{
	return FileCopy( sPack, m_sSavePath + "/" + sDestPack, sError, OnUpdate );
}

#define ROOT_DIRS_SIZE 8
const CString asRootDirs[ROOT_DIRS_SIZE] = { "Themes", "Songs", "BackgroundEffects", "BGAnimations", "BackgroundTransitions", 
					"Courses", "NoteSkins", "RandomMovies" };

/* seemingly good start of an automagical way to mount a user pack based on the folder structure */
CString UserPackManager::GetPackMountPoint( const CString sPack )
{
	enum UserPackMountType { UPACK_MOUNT_ROOT, UPACK_MOUNT_SONGS };

	RageFileDriverZip *pZip = new RageFileDriverZip;
	CHECKPOINT_M( sPack );
	// it should already be a valid zip by now...
	ASSERT( pZip->Load( sPack ) );
	UserPackMountType upmt = UPACK_MOUNT_SONGS;

	CStringArray asRootEntries;
	pZip->GetDirListing( "/", asRootEntries, true, false );
	SAFE_DELETE( pZip );

	for( unsigned i = 0; i < ROOT_DIRS_SIZE; i++ )
	{
		for( unsigned j = 0; j < asRootEntries.size(); j++ )
		{
			if ( asRootEntries[j].CompareNoCase( asRootDirs[i] ) == 0 )
			{ // found a StepMania root folder, mount it as one
				return "/";
			}
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
