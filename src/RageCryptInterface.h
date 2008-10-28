/* RageCryptInterface - a standard entry point for file-level
 * cryptography using RageFileDriverCrypt. */

#ifndef RAGE_CRYPT_INTERFACE_H
#define RAGE_CRYPT_INTERFACE_H

#include "StdString.h"

// POSIX file functions and Win32 compatibility layer
#include <fcntl.h>

#ifdef WIN32
#include <io.h>
#define open(a,b)	_open(a,b)
#define read(a,b,c)	_read(a,b,c)
#define lseek(a,b,c)	_lseek(a,b,c)
#endif

class crypt_file
{
public:
	crypt_file() { }
	crypt_file( crypt_file *cf )
	{
		path = cf->path;
		file_size = cf->file_size;
		header_size = cf->header_size;
	}

	CString path;
	int fd;
	unsigned int filepos;

	size_t file_size;
	size_t header_size;
};

// shared calls for all crypto interfaces
namespace RageCryptInterface
{
	// redirects to driver implementations
	crypt_file *crypt_create();
	crypt_file *crypt_copy( crypt_file *cf );

	crypt_file *crypt_open( CString name, CString secret );
	int crypt_read( crypt_file *cf, void *buf, int size );
	int crypt_seek( crypt_file *cf, int where );
	int crypt_tell( crypt_file *cf );
	int crypt_close( crypt_file *cf );
};

#endif

/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
