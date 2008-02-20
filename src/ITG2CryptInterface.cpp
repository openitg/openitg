#include "global.h"
#include "RageException.h"
#include "RageLog.h"
#include "crypto/aes.h"
#include "crypto/CryptSH512.h"
#include "ITG2CryptInterface.h"
#include "ibutton/getkey.h"
#include <fcntl.h>
#include <cstdio>

crypt_file *ITG2CryptInterface::crypt_open(CString name, CString secret) {
	crypt_file *newfile = new crypt_file;
	char header[2];
	unsigned char *subkey, verifyblock[16];
	size_t got, subkeysize;
	unsigned char *SHABuffer, *AESKey, plaintext[16], AESSHABuffer[64];
	newfile->fd = open(name.c_str(), O_RDONLY);
	if (newfile->fd == -1) {
		LOG->Warn("ITG2CryptInterface: Could not open %s: %s", name.c_str(), strerror(errno));
		return NULL;
	}
	if (read(newfile->fd, header, 2) < 2) {
		LOG->Warn("ITG2CryptInterface: Could not open %s: unexpected header size", name.c_str());
	}

	if (secret.length() == 0) {
		if (header[0] != ':' || header[1] != '|') {
			LOG->Warn("ITG2CryptInterface: no secret given and %s is not an ITG2 arcade encrypted file", name.c_str());
			return NULL;
		}
	} else {
		if (header[0] != '8' || header[1] != 'O') {
			LOG->Warn("ITG2CryptInterface: secret given, but %s is not an ITG2 patch file", name.c_str());
			return NULL;
		}
	}

	if (read(newfile->fd, &(newfile->filesize), 4) < 4) {
		LOG->Warn("ITG2CryptInterface: Could not open %s: unexpected file size", name.c_str());
		return NULL;
	}
	if (read(newfile->fd, &subkeysize, 4) < 4) {
		LOG->Warn("ITG2CryptInterface: Could not open %s: unexpected subkey size", name.c_str());
		return NULL;
	}
	subkey = new unsigned char[subkeysize];
	if ((got = read(newfile->fd, subkey, subkeysize) < subkeysize)) {
		LOG->Warn("ITG2CryptInterface: %s: subkey: expected %i, got %i", name.c_str(), subkeysize, got);
		return NULL;
	}
	if ((got = read(newfile->fd, verifyblock, 16)) < 16) {
		LOG->Warn("ITG2CryptInterface: %s: verifyblock: expected 16, got %i", name.c_str(), got);
		return NULL;
	}
	
	tKeyMap::iterator iter = ITG2CryptInterface::KnownKeys.find(name.c_str());
	if (iter != ITG2CryptInterface::KnownKeys.end()) {
		CHECKPOINT_M("cache");
		AESKey = iter->second;
	} else {
		if (secret.length() > 0) {
			CHECKPOINT_M("patch");
			AESKey = new unsigned char[24];
			SHABuffer = new unsigned char[subkeysize + 47];
			memcpy(SHABuffer, subkey, subkeysize);
			memcpy(SHABuffer+subkeysize, secret.c_str(), 47);
			SHA512_Simple(SHABuffer, subkeysize+47, AESSHABuffer);
			memcpy(AESKey, AESSHABuffer, 24);
		} else {
			CHECKPOINT_M("dongle");
			AESKey = new unsigned char[24];
			GetKeyFromDongle(subkey, AESKey);
		}
		unsigned char *cAESKey = new unsigned char[24];
		memcpy(cAESKey, AESKey, 24);
		ITG2CryptInterface::KnownKeys[name.c_str()] = cAESKey;
	}

	aes_decrypt_key(AESKey, 24, newfile->ctx);
	aes_decrypt(verifyblock, plaintext, newfile->ctx);
	if (plaintext[0] != ':' || plaintext[1] != 'D') {
		LOG->Warn("ITG2CryptInterface: %s: decrypt failed, unexpected decryption magic", name.c_str());
		return NULL;
	}

	newfile->filepos = 0;
	newfile->headersize = lseek(newfile->fd, 0, SEEK_CUR);
	newfile->path = name;

	return newfile;
}

int ITG2CryptInterface::GetKeyFromDongle(const unsigned char *subkey, unsigned char *aeskey) {
	return getKey(subkey, aeskey);
}

int ITG2CryptInterface::crypt_read(crypt_file *cf, void *buf, int size) {
	unsigned char backbuffer[16], decbuf[16], *crbuf, *dcbuf;
	size_t bufsize = ((size/16)+1)*16;
	if (size % 16 > 0) bufsize += 16;
	unsigned int got;
	unsigned int oldpos = cf->filepos;
	crbuf = new unsigned char[bufsize]; // not an efficient way to do this, but oh well
	dcbuf = new unsigned char[bufsize];
	unsigned int cryptpos = (cf->filepos/16)*16; // position in file to make decryption ready
	unsigned int difference = cf->filepos - cryptpos;
	lseek( cf->fd, cf->headersize + cryptpos, SEEK_SET );
	read( cf->fd, crbuf, bufsize);
	if (cryptpos % 4080 == 0) {
		memset(backbuffer, '\0', 16);
	} else {
		lseek( cf->fd, cf->headersize + (cryptpos-16), SEEK_SET );
		read(cf->fd, backbuffer, 16);
	}
	for (unsigned int i = 0; i < bufsize / 16; i++) {
		aes_decrypt(crbuf+(i*16), decbuf, cf->ctx);
		for (int j = 0; j < 16; j++) {
			dcbuf[(i*16)+j] = decbuf[j] ^ (backbuffer[j] - j);
		}
		if (((cryptpos + i*16) + 16) % 4080 == 0) {
			memset(backbuffer, '\0', 16);
		} else {
			memcpy(backbuffer, crbuf+(i*16), 16);
		}
	}
	memcpy(buf, dcbuf+difference, size);
	memset(backbuffer, '\0', 16);
	memcpy(backbuffer, dcbuf, 15);
	cf->filepos = oldpos + size;

	delete dcbuf;
	delete crbuf;
	return size;
}

int ITG2CryptInterface::crypt_seek(crypt_file *cf, int where) {
	cf->filepos = where;
	return cf->filepos;
}

int ITG2CryptInterface::crypt_tell(crypt_file *cf) {
	return cf->filepos;
}


int ITG2CryptInterface::crypt_close(crypt_file *cf) {
	return close(cf->fd);
}
