#include "global.h"
#include "RageFileDriverDirect.h"
#include "RageFileDriverDirectHelpers.h"
#include "RageUtil.h"
#include "RageLog.h"

#include <fcntl.h>
#include <cerrno>
#include <sys/types.h>
#include <sys/stat.h>

#if !defined(WIN32)
#include <dirent.h>
#include <fcntl.h>
#else
#define open _open
#define lseek _lseek
#define read _read
#define write _write
#define close _close
#define rmdir _rmdir
#if !defined(_XBOX)
#include <windows.h>
#endif
#include <io.h>
#endif

REGISTER_FILE_DRIVER( Direct, "DIR" );
REGISTER_FILE_DRIVER( DirectReadOnly, "DIRRO" );

// 64 KB buffer
static const unsigned int BUFFER_SIZE = 1024*64;

RageFileDriverDirect::RageFileDriverDirect( const CString &sRoot ):
	RageFileDriver( new DirectFilenameDB(sRoot) )
{
	Remount( sRoot );
}


static CString MakeTempFilename( const CString &sPath )
{
	/* "Foo/bar/baz" -> "Foo/bar/new.baz.new".  Both prepend and append: we don't
	 * want a wildcard search for the filename to match (foo.txt.new matches foo.txt*),
	 * and we don't want to have the same extension (so "new.foo.sm" doesn't show up
	 * in *.sm). */
	return Dirname(sPath) + "new." + Basename(sPath) + ".new";
}

RageFileBasic *RageFileDriverDirect::Open( const CString &sPath_, int iMode, int &iError )
{
	CString sPath = sPath_;
	ASSERT( sPath.size() && sPath[0] == '/' );

	/* This partially resolves.  For example, if "abc/def" exists, and we're opening
	 * "ABC/DEF/GHI/jkl/mno", this will resolve it to "abc/def/GHI/jkl/mno"; we'll
	 * create the missing parts below. */
	FDB->ResolvePath( sPath );

	if( iMode & RageFile::WRITE )
	{
		const CString dir = Dirname(sPath);
		if( this->GetFileType(dir) != RageFileManager::TYPE_DIR )
			CreateDirectories( m_sRoot + dir );
	}

	RageFileObjDirect *ret = this->CreateInternal( m_sRoot + sPath );

	if( ret->OpenInternal( m_sRoot + sPath, iMode, iError) )
		return ret;

	SAFE_DELETE( ret );
	return NULL;
}

bool RageFileDriverDirect::Move( const CString &sOldPath_, const CString &sNewPath_ )
{
	CString sOldPath = sOldPath_;
	CString sNewPath = sNewPath_;
	FDB->ResolvePath( sOldPath );
	FDB->ResolvePath( sNewPath );

	if( this->GetFileType(sOldPath) == RageFileManager::TYPE_NONE )
		return false;

	{
		const CString sDir = Dirname(sNewPath);
		CreateDirectories( m_sRoot + sDir );
	}

	LOG->Trace( ssprintf("rename \"%s\" -> \"%s\"", (m_sRoot + sOldPath).c_str(), (m_sRoot + sNewPath).c_str()) );

	if( DoRename(m_sRoot + sOldPath, m_sRoot + sNewPath) == -1 )
	{
		LOG->Warn( ssprintf("rename(%s,%s) failed: %s", (m_sRoot + sOldPath).c_str(), (m_sRoot + sNewPath).c_str(), strerror(errno)) );
		return false;
	}

	return true;
}

bool RageFileDriverDirect::Remove( const CString &sPath_ )
{
	CString sPath = sPath_;
	FDB->ResolvePath( sPath );
	switch( this->GetFileType(sPath) )
	{
	case RageFileManager::TYPE_FILE:
		LOG->Trace( "remove '%s'", (m_sRoot + sPath).c_str() );
		if( DoRemove(m_sRoot + sPath) == -1 )
		{
			WARN( ssprintf("remove(%s) failed: %s", (m_sRoot + sPath).c_str(), strerror(errno)) );
			return false;
		}
		FDB->DelFile( sPath );
		return true;

	case RageFileManager::TYPE_DIR:
		LOG->Trace( "rmdir '%s'", (m_sRoot + sPath).c_str() );
		if( DoRmdir(m_sRoot + sPath) == -1 )
		{
			WARN( ssprintf("rmdir(%s) failed: %s", (m_sRoot + sPath).c_str(), strerror(errno)) );
			return false;
		}
		FDB->DelFile( sPath );
		return true;

	case RageFileManager::TYPE_NONE: return false;
	default: ASSERT(0); return false;
	}
}

/* The DIRRO driver is just like DIR, except writes are disallowed. */
RageFileDriverDirectReadOnly::RageFileDriverDirectReadOnly( const CString &sRoot ) :
	RageFileDriverDirect( sRoot ) { }

RageFileBasic *RageFileDriverDirectReadOnly::Open( const CString &sPath, int iMode, int &iError )
{
	if( iMode & RageFile::WRITE )
	{
		iError = EROFS;
		return NULL;
	}

	return RageFileDriverDirect::Open( sPath, iMode, iError );
}
bool RageFileDriverDirectReadOnly::Move( const CString &sOldPath, const CString &sNewPath ) { return false; }
bool RageFileDriverDirectReadOnly::Remove( const CString &sPath ) { return false; }


bool RageFileObjDirect::OpenInternal( const CString &sPath, int iMode, int &iError )
{
	m_sPath = sPath;
	m_iMode = iMode;

	if( iMode & RageFile::READ )
	{
		m_iFD = DoOpen( sPath, O_BINARY|O_RDONLY, 0666 );
		/* XXX: Windows returns EACCES if we try to open a file on a CDROM that isn't
		 * ready, instead of something like ENODEV.  We want to return that case as
		 * ENOENT, but we can't distinguish it from file permission errors. */
	}
	else
	{
		CString sOut;
		/* open a temporary file if we're not streaming */
		if( iMode & RageFile::STREAMED )
			sOut = sPath;
		else
			sOut = MakeTempFilename(sPath);

		/* Open a temporary file for writing. */
		m_iFD = DoOpen( sOut, O_BINARY|O_WRONLY|O_CREAT|O_TRUNC, 0666 );
	}

	if( m_iFD == -1 )
	{
		iError = errno;
		return false;
	}

	/* enable the write buffer if we're writing data */
	if( iMode & RageFile::WRITE )
		this->EnableWriteBuffering( BUFFER_SIZE );

	return true;
}

RageFileObjDirect *RageFileObjDirect::Copy() const
{
	int iErr;
	RageFileObjDirect *ret = new RageFileObjDirect;

	if( ret->OpenInternal( m_sPath, m_iMode, iErr ) )
	{
		ret->Seek( lseek( m_iFD, 0, SEEK_CUR ) );
		return ret;
	}

	RageException::Throw( "Couldn't reopen \"%s\": %s", m_sPath.c_str(), strerror(iErr) );
	delete ret;
	return NULL;
}

bool RageFileDriverDirect::Remount( const CString &sPath )
{
	m_sRoot = sPath;
	((DirectFilenameDB *) FDB)->SetRoot( sPath );

	/* If the root path doesn't exist, create it. */
	CreateDirectories( m_sRoot );

	return true;
}

RageFileObjDirect::RageFileObjDirect()
{
	m_bWriteFailed = false;
	m_iFD = -1;
	m_iMode = 0;
}

bool RageFileObjDirect::FinalFlush()
{
	if( !(m_iMode & RageFile::WRITE) )
		return true;

	/* Flush the output buffer. */
	if( Flush() == -1 )
		return false;

	/* Only do the rest of the flushes if SLOW_FLUSH is enabled. */
	if( !(m_iMode & RageFile::SLOW_FLUSH) )
		return true;
	
	/* Force a kernel buffer flush. */
	if( fsync( m_iFD ) == -1 )
	{
		WARN( ssprintf("Error synchronizing %s: %s", this->m_sPath.c_str(), strerror(errno)) );
		SetError( strerror(errno) );
		return false;
	}

#if !defined(WIN32)
	/* Wait for the directory to be flushed. */
	int dirfd = open( Dirname(m_sPath), O_RDONLY );
	if( dirfd == -1 )
	{
		WARN( ssprintf("Error synchronizing open(%s dir): %s", this->m_sPath.c_str(), strerror(errno)) );
		SetError( strerror(errno) );
		return false;
	}

	if( fsync( dirfd ) == -1 )
	{
		WARN( ssprintf("Error synchronizing fsync(%s dir): %s", this->m_sPath.c_str(), strerror(errno)) );
		SetError( strerror(errno) );
		close( dirfd );
		return false;
	}

	close( dirfd );
#endif

	return true;
}

RageFileObjDirect::~RageFileObjDirect()
{
	bool bFailed = !FinalFlush();
	
	if( m_iFD != -1 )
	{
		if( close( m_iFD ) == -1 )
		{
			WARN( ssprintf("Error closing %s: %s", this->m_sPath.c_str(), strerror(errno)) );
			SetError( strerror(errno) );
			bFailed = true;
		}
	}

	if( !(m_iMode & RageFile::WRITE) || (m_iMode & RageFile::STREAMED) )
		return;

	/* We now have path written to MakeTempFilename(m_sPath).  Rename the temporary
	 * file over the real path. */

	do
	{
		if( bFailed || WriteFailed() )
			break;

		/*
		 * We now have path written to MakeTempFilename(m_sPath).  Rename the temporary
		 * file over the real path.  This should be an atomic operation with a journalling
		 * filesystem.  That is, there should be no intermediate state a JFS might restore
		 * the file we're writing (in the case of a crash/powerdown) to an empty or partial
		 * file.
		 */

		CString sOldPath = MakeTempFilename(m_sPath);
		CString sNewPath = m_sPath;

#if defined(WIN32)
		if( WinMoveFile(sOldPath, sNewPath) )
			return;

		/* We failed. */
		int err = GetLastError();
		const CString error = werr_ssprintf( err, "Error renaming \"%s\" to \"%s\"", sOldPath.c_str(), sNewPath.c_str() );
		WARN( ssprintf("%s", error.c_str()) );
		SetError( error );
		break;
#else
		if( rename( sOldPath, sNewPath ) == -1 )
		{
			WARN( ssprintf("Error renaming \"%s\" to \"%s\": %s", 
					sOldPath.c_str(), sNewPath.c_str(), strerror(errno)) );
			SetError( strerror(errno) );
			break;
		}
#endif

		/* Success. */
		return;
	} while(0);

	/* The write or the rename failed.  Delete the incomplete temporary file. */
	DoRemove( MakeTempFilename(m_sPath) );
}

int RageFileObjDirect::ReadInternal( void *pBuf, size_t iBytes )
{
	int iRet = read( m_iFD, pBuf, iBytes );
	if( iRet == -1 )
	{
		SetError( strerror(errno) );
		return -1;
	}

	return iRet;
}

/* write(), but retry a couple times on EINTR. */
static int RetriedWrite( int iFD, const void *pBuf, size_t iCount )
{
	int iTries = 3, iRet;
	do
	{
		iRet = write( iFD, pBuf, iCount );
	}
	while( iRet == -1 && errno == EINTR && iTries-- );

	return iRet;
}


int RageFileObjDirect::FlushInternal()
{
	if( WriteFailed() )
	{
		SetError( "previous write failed" );
		return -1;
	}

	return 0;
}

int RageFileObjDirect::WriteInternal( const void *pBuf, size_t iBytes )
{
	if( WriteFailed() )
	{
		SetError( "previous write failed" );
		return -1;
	}

	/* The buffer is cleared. If we still don't have space, it's bigger than
	 * the buffer size, so just write it directly. */

	int iRet = RetriedWrite( m_iFD, pBuf, iBytes );

	if( iRet == -1 )
	{
		SetError( strerror(errno) );
		m_bWriteFailed = true;
		return -1;
	}

	return iBytes;
}

int RageFileObjDirect::SeekInternal( int iOffset )
{
	return lseek( m_iFD, iOffset, SEEK_SET );
}

int RageFileObjDirect::GetFileSize() const
{
	const int iOldPos = lseek( m_iFD, 0, SEEK_CUR );
	ASSERT_M( iOldPos != -1, ssprintf("\"%s\": %s", m_sPath.c_str(), strerror(errno)) );
	const int iRet = lseek( m_iFD, 0, SEEK_END );
	ASSERT_M( iRet != -1, ssprintf("\"%s\": %s", m_sPath.c_str(), strerror(errno)) );
	lseek( m_iFD, iOldPos, SEEK_SET );
	return iRet;
}

/*
 * Copyright (c) 2003-2004 Glenn Maynard, Chris Danford
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

