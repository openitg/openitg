#ifndef RAGE_CRYPT_INTERFACE_ITG2_H
#define RAGE_CRYPT_INTERFACE_ITG2_H

#include "RageCryptInterface.h"
#include "aes/aes.h"
#include <map>

class crypt_file_ITG2: public crypt_file
{
public:
	aes_decrypt_ctx ctx[1];
};

typedef std::map<const char *, unsigned char*> tKeyMap;

// a new crypto driver must define the following functions
// as needed. not all of them need to be filled in.
namespace RageCryptInterface_ITG2
{
	static tKeyMap KnownKeys;

	// return a special version of crypt_file used for this driver
	crypt_file *crypt_create_internal() { return new crypt_file_ITG2; }
	crypt_file *crypt_copy_internal( crypt_file *cf );

	// true if file is readable, false if it isn't
	bool crypt_open_internal( crypt_file *cf, CString secret );
	int crypt_read_internal( crypt_file *cf, void *buf, int size );

	// unused in this implementation
	int crypt_seek_internal( crypt_file *cf, int where )	{ return cf->filepos; }
	int crypt_tell_internal( crypt_file *cf )		{ return cf->filepos; }
	bool crypt_close_internal( crypt_file *cf )		{ return true; }
};

// UGLY: refer to the ITG2 versions of the internal calls
using namespace RageCryptInterface_ITG2;

#ifdef CRYPT_INTERFACE
#error More than one crypt interface selected
#endif

#define CRYPT_INTERFACE ITG2

#endif // CRYPT_INTERFACE_ITG2_H

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
