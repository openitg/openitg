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

//#define tell( handle ) lseek( handle, 0L, SEEK_CUR )
//#define _tell tell

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

RageFileDriverCrypt::RageFileDriverCrypt( CString root_, CString secret_):RageFileDriver( new CryptFilenameDB(root_) )
{
	secret = secret_;
	root = root_;
}

static CString MakeTempFilename( const CString &sPath )
{
	/* "Foo/bar/baz" -> "Foo/bar/new.baz.new".  Both prepend and append: we don't
	 * want a wildcard search for the filename to match (foo.txt.new matches foo.txt*),
	 * and we don't want to have the same extension (so "new.foo.sm" doesn't show up
	 * in *.sm). */
	return Dirname(sPath) + "new." + Basename(sPath) + ".new";
}

RageFileBasic *RageFileDriverCrypt::Open( const CString &path, int mode, int &err )
{
	CString sPath = root + path;

	/* This partially resolves.  For example, if "abc/def" exists, and we're opening
	 * "ABC/DEF/GHI/jkl/mno", this will resolve it to "abc/def/GHI/jkl/mno"; we'll
	 * create the missing parts below. */
	CHECKPOINT_M(sPath.c_str());
	FDB->ResolvePath( sPath );

	crypt_file *newFile = ITG2CryptInterface::crypt_open(sPath, secret);

	if (newFile == NULL)
		return NULL;

	return new RageFileObjCrypt(newFile);
}

RageFileBasic *RageFileObjCrypt::Copy() const
{
        int iErr;
	crypt_file *cpCf = new crypt_file;
	cpCf->path = cf->path;
	memcpy(cpCf->ctx, cf->ctx, sizeof(cf->ctx));
	//cpCf->ctx = cf->ctx;
	cpCf->filesize = cf->filesize;
	cpCf->headersize = cf->headersize;

	cpCf->fd = open(cf->path.c_str(), O_RDONLY);
	ITG2CryptInterface::crypt_seek(cpCf, ITG2CryptInterface::crypt_tell(cf));
	
	return new RageFileObjCrypt(cpCf);
}

RageFileObjCrypt::RageFileObjCrypt( crypt_file *cf_ )
{
	cf = cf_;
	ASSERT( cf != NULL );
}

int RageFileObjCrypt::SeekInternal( int iOffset )
{
        return ITG2CryptInterface::crypt_seek( cf, iOffset );
}

int RageFileObjCrypt::ReadInternal( void *pBuf, size_t iBytes )
{
	//if (LOG) LOG->Trace("RageFileObjCrypt::ReadInternal(%d) called", iBytes);
        int iRet = ITG2CryptInterface::crypt_read( cf, pBuf, iBytes );
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
	return cf->filesize;
}

RageFileObjCrypt::~RageFileObjCrypt()
{
	//if(LOG) LOG->Trace("~RageFileObjCrypt(%s) called", m_sPath.c_str());
	if (cf->fd != -1) {
		if (ITG2CryptInterface::crypt_close(cf) == -1) {
			LOG->Warn("~RageFileObjCrypt(): could not close file");
		}
	}
}

CryptFilenameDB::CryptFilenameDB( CString root_ )
{
        ExpireSeconds = 30;
        SetRoot( root_ );
}

void CryptFilenameDB::SetRoot( CString root_ )
{
	root = root_;

	/* "\abcd\" -> "/abcd/": */
	root.Replace( "\\", "/" );

	/* "/abcd/" -> "/abcd": */
	if( root.Right(1) == "/" )
		root.erase( root.size()-1, 1 );
}

void CryptFilenameDB::PopulateFileSet( FileSet &fs, const CString &path )
{
	CString sPath = path;

#if defined(XBOX)
	/* Xbox doesn't handle path names which end with ".", which are used when using an
	 * alternative song directory */
	if( sPath.size() > 0 && sPath.Right(1) == "." )
		sPath.erase( sPath.size() - 1 );
#endif

	/* Resolve path cases (path/Path -> PATH/path). */
	ResolvePath( sPath );

	fs.age.GetDeltaTime(); /* reset */
	fs.files.clear();

#if defined(WIN32)
	WIN32_FIND_DATA fd;

	if ( sPath.size() > 0  && sPath.Right(1) == "/" )
		sPath.erase( sPath.size() - 1 );

	HANDLE hFind = DoFindFirstFile( root+sPath+"/*", &fd );

	if( hFind == INVALID_HANDLE_VALUE )
		return;

	do {
		if( !strcmp(fd.cFileName, ".") || !strcmp(fd.cFileName, "..") )
			continue;

		File f;
		f.SetName( fd.cFileName );
		f.dir = !!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY);
		f.size = fd.nFileSizeLow;
		f.hash = fd.ftLastWriteTime.dwLowDateTime;

		fs.files.insert( f );
	} while( FindNextFile( hFind, &fd ) );
	FindClose( hFind );
#else
	/* Ugly: POSIX threads are not guaranteed to have per-thread cwds, and only
	 * a few systems have openat() or equivalent; one or the other is needed
	 * to do efficient, thread-safe directory traversal.  Instead, we have to
	 * use absolute paths, which forces the system to re-parse the directory
	 * for each file.  This isn't a major issue, since most large directory
	 * scans are I/O-bound. */
	 
	DIR *pDir = opendir(root+sPath);
	if( pDir == NULL )
	{
		LOG->MapLog("opendir " + root+sPath, "Couldn't opendir(%s%s): %s", root.c_str(), sPath.c_str(), strerror(errno) );
		return;
	}

	while( struct dirent *pEnt = readdir(pDir) )
	{
		if( !strcmp(pEnt->d_name, ".") )
			continue;
		if( !strcmp(pEnt->d_name, "..") )
			continue;
		
		File f;
		f.SetName( pEnt->d_name );
		
		struct stat st;
		if( DoStat(root+sPath + "/" + pEnt->d_name, &st) == -1 )
		{
			/* If it's a broken symlink, ignore it.  Otherwise, warn. */
			if( lstat(pEnt->d_name, &st) == 0 )
				continue;
			
			/* Huh? */
			if( LOG )
				LOG->Warn( "Got file '%s' in '%s' from list, but can't stat? (%s)",
					pEnt->d_name, sPath.c_str(), strerror(errno) );
			continue;
		} else {
			f.dir = (st.st_mode & S_IFDIR);
			f.size = st.st_size;
			f.hash = st.st_mtime;
		}

		fs.files.insert(f);
	}
	       
	closedir( pDir );
#endif
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

