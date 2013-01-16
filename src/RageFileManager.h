/* RageFileManager - File utilities and high-level manager for RageFile objects. */

#ifndef RAGE_FILE_MANAGER_H
#define RAGE_FILE_MANAGER_H

#include "RageFile.h"

extern CString InitialWorkingDirectory;
extern CString DirOfExecutable;

class RageFileDriver;

class RageFileManager
{
public:
	RageFileManager( const CString &argv0 );
	~RageFileManager();
	void MountInitialFilesystems();

	void GetDirListing( const CString &sPath, CStringArray &AddTo, bool bOnlyDirs, bool bReturnPathToo );
	bool Move( const CString &sOldPath, const CString &sNewPath );
	bool Remove( const CString &sPath );
	void CreateDir( const CString &sDir );

	enum FileType { TYPE_FILE, TYPE_DIR, TYPE_NONE };
	FileType GetFileType( const CString &sPath );

	bool IsAFile( const CString &sPath );
	bool IsADirectory( const CString &sPath );
	bool DoesFileExist( const CString &sPath );

	int GetFileSizeInBytes( const CString &sPath );
	int GetFileHash( const CString &sPath );

	CString ResolvePath( const CString &sPath );

	bool Mount( const CString &Type, const CString &RealPath, const CString &MountPoint, bool bAddToEnd = true );
	void Unmount( const CString &Type, const CString &Root, const CString &MountPoint );

	/* Change the root of a filesystem.  Only a couple drivers support this; it's
	 * used to change memory card mountpoints without having to actually unmount
	 * the driver. */
	static void Remount( CString sMountpoint, CString sPath );
	bool IsMounted( const CString &MountPoint );
	struct DriverLocation
	{
		CString Type, Root, MountPoint;
	};
	void GetLoadedDrivers( vector<DriverLocation> &Mounts );

	void FlushDirCache( const CString &sPath );

	/* Used only by RageFile: */
	RageFileBasic *Open( const CString &sPath, int mode, int &err );

	/* Retrieve or release a reference to the low-level driver for a mountpoint. */
	RageFileDriver *GetFileDriver( const CString &sMountpoint );
	void ReleaseFileDriver( RageFileDriver *pDriver );

private:
	RageFileBasic *OpenForReading( const CString &sPath, int mode, int &err );
	RageFileBasic *OpenForWriting( const CString &sPath, int mode, int &err );
};

extern RageFileManager *FILEMAN;

#endif

/*
 * Copyright (c) 2001-2004 Glenn Maynard, Chris Danford
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
