#include "global.h"
#include "RageLog.h"
#include "RageCryptInterface_ITG2.h"

#include "aes/aes.h"
#include "crypto/CryptSH512.h"
#include "ibutton/ibutton.h"

crypt_file_ITG2::crypt_file_ITG2():crypt_file() {}

crypt_file_ITG2::crypt_file_ITG2(crypt_file *cf):crypt_file(cf) {}

crypt_file *RageCryptInterface_ITG2::crypt_create_internal() { return new crypt_file_ITG2; }

int RageCryptInterface_ITG2::crypt_tell_internal( crypt_file *cf ) { return cf->filepos; }
int RageCryptInterface_ITG2::crypt_close_internal( crypt_file *cf ) { return close(cf->fd); }

// we can safely assume that all instances of crypt_file
// will match ours, due to the crypt_create_internal call.
crypt_file *RageCryptInterface_ITG2::crypt_copy_internal( crypt_file *cf )
{
	crypt_file_ITG2 *cf_new = new crypt_file_ITG2( cf );

	// copy the ctx value
	memcpy( cf_new->ctx, ((crypt_file_ITG2*)cf)->ctx, sizeof(((crypt_file_ITG2*)cf)->ctx) );

	return (crypt_file*)cf_new;
}

bool RageCryptInterface_ITG2::crypt_open_internal(crypt_file *newfile, CString secret)
{
	CHECKPOINT;
	LOG->Debug( "RageCryptInterface_ITG2::crypt_open( %s, \"%s\" )", newfile->path.c_str(), secret.c_str() );

	char header[2];
	unsigned char *subkey, verifyblock[16];
	size_t got, subkeysize;
	unsigned char *SHABuffer, *AESKey, plaintext[16], AESSHABuffer[64];

	if (read(newfile->fd, header, 2) < 2)
	{
		LOG->Warn("RageCryptInterface_ITG2: Could not open %s: unexpected header size", newfile->path.c_str());
		return false;
	}
	newfile->header_size = 2;

	if (secret.length() == 0)
	{
		if (header[0] != ':' || header[1] != '|')
		{
			LOG->Warn("RageCryptInterface_ITG2: no secret given and %s is not an ITG2 arcade encrypted file", newfile->path.c_str());
			return false;
		}
	}
	else
	{
		if (header[0] != '8' || header[1] != 'O')
		{
			LOG->Warn("RageCryptInterface_ITG2: secret given, but %s is not an ITG2 patch file", newfile->path.c_str());
			return false;
		}
	}

	if (read(newfile->fd, &(newfile->file_size), 4) < 4)
	{
		LOG->Warn("RageCryptInterface_ITG2: Could not open %s: unexpected file size", newfile->path.c_str());
		return false;
	}
	
	if (read(newfile->fd, &subkeysize, 4) < 4)
	{
		LOG->Warn("RageCryptInterface_ITG2: Could not open %s: unexpected subkey size", newfile->path.c_str());
		return false;
	}
	
	subkey = new unsigned char[subkeysize];

	if ((got = read(newfile->fd, subkey, subkeysize)) < subkeysize)
	{
		LOG->Warn("RageCryptInterface_ITG2: %s: subkey: expected %i, got %i", newfile->path.c_str(), subkeysize, got);
		return false;
	}

	if ((got = read(newfile->fd, verifyblock, 16)) < 16)
	{
		LOG->Warn("RageCryptInterface_ITG2: %s: verifyblock: expected 16, got %i", newfile->path.c_str(), got);
		return false;
	}
	
	tKeyMap::iterator iter = RageCryptInterface_ITG2::KnownKeys.find(newfile->path.c_str());
	if (iter != RageCryptInterface_ITG2::KnownKeys.end())
	{
		LOG->Debug( "Decryption key found: %s", iter->second );
		CHECKPOINT_M("cache");
		AESKey = iter->second;
	}
	else
	{
		if (secret.length() > 0)
		{
			CHECKPOINT_M("patch");
			AESKey = new unsigned char[24];
			SHABuffer = new unsigned char[subkeysize + 47];
			memcpy(SHABuffer, subkey, subkeysize);
			memcpy(SHABuffer+subkeysize, secret.c_str(), 47);
			SHA512_Simple(SHABuffer, subkeysize+47, AESSHABuffer);
			memcpy(AESKey, AESSHABuffer, 24);
		}
		else
		{
			CHECKPOINT_M("dongle");
			AESKey = new unsigned char[24];
			iButton::GetAESKey(subkey, AESKey);
		}

		unsigned char *cAESKey = new unsigned char[24];
		memcpy(cAESKey, AESKey, 24);
		RageCryptInterface_ITG2::KnownKeys[newfile->path.c_str()] = cAESKey;
		LOG->Debug( "Key saved: %s", cAESKey );
	}
	delete subkey;

	aes_decrypt_key(AESKey, 24, ((crypt_file_ITG2*)newfile)->ctx);
	aes_decrypt(verifyblock, plaintext, ((crypt_file_ITG2*)newfile)->ctx);
	if (plaintext[0] != ':' || plaintext[1] != 'D')
	{
		LOG->Warn("RageCryptInterface_ITG2: %s: decrypt failed, unexpected decryption magic", newfile->path.c_str());
		return false;
	}
	else
	{
		LOG->Debug("RageCryptInterface_ITG2: %s: decrypt succeeded", newfile->path.c_str() );
	}

	newfile->filepos = 0;
	newfile->header_size = lseek(newfile->fd, 0, SEEK_CUR);

	return true;
}

int RageCryptInterface_ITG2::crypt_read_internal(crypt_file *cf, void *buf, int size)
{
	unsigned char backbuffer[16], decbuf[16], *crbuf, *dcbuf;

	size_t bufsize = ((size/16)+1)*16;

	if (size % 16 > 0)
		bufsize += 16;

	unsigned int oldpos = cf->filepos;

	crbuf = new unsigned char[bufsize]; // not an efficient way to do this, but oh well
	dcbuf = new unsigned char[bufsize];

	unsigned int cryptpos = (cf->filepos/16)*16; // position in file to make decryption ready
	unsigned int difference = cf->filepos - cryptpos;

	lseek( cf->fd, cf->header_size + cryptpos, SEEK_SET );
	read( cf->fd, crbuf, bufsize);

	if (cryptpos % 4080 == 0)
	{
		memset(backbuffer, '\0', 16);
	}
	else
	{
		lseek( cf->fd, cf->header_size + (cryptpos-16), SEEK_SET );
		read(cf->fd, backbuffer, 16);
	}

	for (unsigned i = 0; i < bufsize / 16; i++)
	{
		aes_decrypt(crbuf+(i*16), decbuf, ((crypt_file_ITG2*)cf)->ctx);

		for (unsigned char j = 0; j < 16; j++)
			dcbuf[(i*16)+j] = decbuf[j] ^ (backbuffer[j] - j);

		if (((cryptpos + i*16) + 16) % 4080 == 0)
			memset(backbuffer, '\0', 16);
		else
			memcpy(backbuffer, crbuf+(i*16), 16);
	}

	memcpy(buf, dcbuf+difference, size);
	//memset(backbuffer, '\0', 16);
	//memcpy(backbuffer, dcbuf, 15);

	cf->filepos = oldpos + size;

	delete dcbuf;
	delete crbuf;
	return size;
}

int RageCryptInterface_ITG2::crypt_seek_internal(crypt_file *cf, int where)
{
	cf->filepos = where;
	return cf->filepos;
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
