#ifndef ITG2_CRYPT_INTERFACE_H
#define ITG2_CRYPT_INTERFACE_H

#include <cerrno>
#include "aes/aes.h"
#include "StdString.h"
#include <map>

typedef struct {
	int fd;
	CString path;
	char header[2];
	size_t filesize;
	size_t subkeysize;
	unsigned char *subkey;
	unsigned char verifyblock[16];
	aes_decrypt_ctx ctx[1];
	size_t headersize;

	//unsigned char decbuf[4080];
	//unsigned char encbuf[4080];

	unsigned char backbuffer[16];
	unsigned int filepos;
} crypt_file;

typedef std::map<const char *, unsigned char*> tKeyMap;

namespace ITG2CryptInterface
{
	static tKeyMap KnownKeys;

	crypt_file *crypt_open(CString fname, CString secret);
	int crypt_read(crypt_file *cf, void *buf, int size);
	int crypt_seek(crypt_file *cf, int where);
	int crypt_tell(crypt_file *cf);
	int crypt_close(crypt_file *cf);
	
	int GetKeyFromDongle(const unsigned char *subkey, unsigned char *aeskey);
}

#endif // ITG2_CRYPT_INTERFACE_H

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
