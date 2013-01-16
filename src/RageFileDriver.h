/* RageFileDriver - File driver base classes. */

#ifndef RAGE_FILE_DRIVER_H
#define RAGE_FILE_DRIVER_H

#include "RageFileManager.h"

#define REGISTER_FILE_DRIVER(name,type) \
	static struct FileDriverEntry_##name : public FileDriverEntry \
	{	\
		FileDriverEntry_##name(): FileDriverEntry( type ) { } \
		RageFileDriver *Create( const CString &sRoot ) const { return new RageFileDriver##name( sRoot ); } \
	} const g_Register##name##Driver;

class RageFileBasic;
class FilenameDB;
class RageFileDriver
{
public:
	RageFileDriver( FilenameDB *db ) { FDB = db; }
	virtual ~RageFileDriver();
	virtual RageFileBasic *Open( const CString &path, int mode, int &err ) = 0;
	virtual void GetDirListing( const CString &sPath, CStringArray &AddTo, bool bOnlyDirs, bool bReturnPathToo );
	virtual RageFileManager::FileType GetFileType( const CString &sPath );
	virtual int GetFileSizeInBytes( const CString &sFilePath );
	virtual int GetFileHash( const CString &sPath );
	virtual int GetPathValue( const CString &path );
	virtual void FlushDirCache( const CString &sPath );
	virtual bool Move( const CString &sOldPath, const CString &sNewPath ) { return false; }
	virtual bool Remove( const CString &sPath ) { return false; }

	/* Optional: Move to a different place, as if reconstructed with a different path. */
	virtual bool Remount( const CString &sPath ) { return false; }

	/* Possible error returns from Open, in addition to standard errno.h values: */
	enum { ERROR_WRITING_NOT_SUPPORTED = -1 };
// protected:
	FilenameDB *FDB;
};

/* This is used to register the driver, so RageFileManager can see it. */
struct FileDriverEntry
{
	FileDriverEntry( const CString &sType );
	virtual ~FileDriverEntry();
	virtual RageFileDriver *Create( const CString &sRoot ) const = 0;

	CString m_Type;
	const FileDriverEntry *m_Link;
};
RageFileDriver *MakeFileDriver( const CString &sType, const CString &sRoot );

#endif

/*
 * Copyright (c) 2003-2004 Glenn Maynard
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
