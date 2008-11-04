#include "global.h"
#include "RageFileDriverCrypt.h"
#include "RageFileDriverDirectHelpers.h"
#include "RageUtil.h"
#include "RageUtil_FileDB.h"
#include "RageLog.h"

#include <fcntl.h>
#include <cerrno>
// XXX: is this stuff portable? I have no idea. - Vyhd
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(WIN32)
#include <dirent.h>
#include <fcntl.h>
#else
#if !defined(_XBOX)
#include <windows.h>
#endif
#include <io.h>
#endif

/* courtesy of random.org. This is for future testing. -- Vyhd */
#define DEFAULT_AES_KEY "65487573252940086457044055343188392138734144585"

static struct FileDriverEntry_KRY: public FileDriverEntry
{
	FileDriverEntry_KRY(): FileDriverEntry( "KRY" ) { }
	RageFileDriver *Create( CString Root ) const
	{
#ifdef ITG_ARCADE
		return new RageFileDriverCrypt( Root, "" );
#else
		return new RageFileDriverCrypt( Root, DEFAULT_AES_KEY );
#endif
	}
} const g_RegisterDriver;

RageFileDriverCrypt::RageFileDriverCrypt( CString root_, CString secret_):RageFileDriver( new DirectFilenameDB(root_) )
{
	secret = secret_;
	root = root_;
}

RageFileBasic *RageFileDriverCrypt::Open( const CString &path, int mode, int &err )
{
	CString sPath = root + path;

	/* This partially resolves.  For example, if "abc/def" exists, and we're opening
	 * "ABC/DEF/GHI/jkl/mno", this will resolve it to "abc/def/GHI/jkl/mno"; we'll
	 * create the missing parts below. */
	CHECKPOINT_M(sPath.c_str());
	FDB->ResolvePath( sPath );

	crypt_file *newFile = RageCryptInterface::crypt_open(sPath, secret);

	if (newFile == NULL)
		return NULL;

	return new RageFileObjCrypt(newFile);
}

RageFileBasic *RageFileObjCrypt::Copy() const
{
	crypt_file *cpCf = RageCryptInterface::crypt_copy( cf );

	cpCf->fd = open( cf->path.c_str(), O_RDONLY );
	RageCryptInterface::crypt_seek(cpCf, RageCryptInterface::crypt_tell(cf));
	
	return new RageFileObjCrypt(cpCf);
}

RageFileObjCrypt::RageFileObjCrypt( crypt_file *cf_ )
{
	cf = cf_;
	ASSERT( cf != NULL );
}

int RageFileObjCrypt::SeekInternal( int iOffset )
{
        return RageCryptInterface::crypt_seek( cf, iOffset );
}

int RageFileObjCrypt::ReadInternal( void *pBuf, size_t iBytes )
{
        int iRet = RageCryptInterface::crypt_read( cf, pBuf, iBytes );
        if( iRet == -1 )
        {
                SetError( strerror(errno) );
                return -1;
        }

        return iRet;
}

int RageFileObjCrypt::WriteInternal( const void *pBuf, size_t iBytes)
{
	if (LOG) LOG->Warn("RageFileObjCrypt: writing not implemented");
	return -1;
}

int RageFileObjCrypt::GetFileSize() const
{
	return cf->file_size;
}

RageFileObjCrypt::~RageFileObjCrypt()
{
	if (cf->fd != -1 && RageCryptInterface::crypt_close(cf) == -1)
		LOG->Warn("~RageFileObjCrypt(): could not close file.");

	SAFE_DELETE(cf);
}

/*
 * Copyright (c) 2005 Glenn Maynard re-implemented by infamouspat
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

