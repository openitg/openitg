#ifndef ITG2_CRYPT_INTERFACE_H
#define ITG2_CRYPT_INTERFACE_H

#include <cerrno>
#include "crypto/aes.h"
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
