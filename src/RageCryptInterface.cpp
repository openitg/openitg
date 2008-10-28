#include <cerrno>
#include "global.h"
#include "RageLog.h"
#include "RageCryptInterface.h"

// This is the underlying crypto implementation being used.
#include "RageCryptInterface_ITG2.h"

// workaround so we don't need to #define in crypt_open
#ifndef O_BINARY
#define O_BINARY 0
#endif

// simple redirects to the underlying driver
crypt_file *RageCryptInterface::crypt_create()
{
	return crypt_create_internal();
}

crypt_file *RageCryptInterface::crypt_copy( crypt_file *cf )
{
	return crypt_copy_internal( cf );
}

crypt_file *RageCryptInterface::crypt_open( CString name, CString secret )
{
	LOG->Debug( "RageCryptInterface::crypt_open( %s )", name.c_str() );

	// create the implementation's derivation of crypt_file
	crypt_file *new_file = crypt_create();

	new_file->fd = open( name.c_str(), O_RDONLY | O_BINARY );

	if( new_file->fd == -1 )
	{
		LOG->Warn( "RageCryptInterface: could not open \"%s\": %s", name.c_str(), strerror(errno) );
		delete new_file;
		return NULL;
	}

	new_file->filepos = 0;
	new_file->path = name;

	// can this file be understood by the underlying crypt implementation?
	if( crypt_open_internal(new_file, secret) )
		return new_file;

	delete new_file;
	return NULL;
}

// this is a redirect - it does nothing on its own
int RageCryptInterface::crypt_read( crypt_file *cf, void *buf, int size )
{
	return crypt_read_internal( cf, buf, size );
}

int RageCryptInterface::crypt_seek( crypt_file *cf, int where )
{
	cf->filepos = where;
	return cf->filepos;
}

int RageCryptInterface::crypt_tell( crypt_file *cf )
{
	return cf->filepos;
}

int RageCryptInterface::crypt_close( crypt_file *cf )
{
	LOG->Debug( "RageCryptInterface::crypt_close( %s )", cf->path.c_str() );

	return close(cf->fd);
	delete cf;
}

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
