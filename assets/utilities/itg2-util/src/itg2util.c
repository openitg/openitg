#include <stdlib.h>
#include <stdio.h>
#include <gcrypt.h>
#include "keydump.h"
#include "aes.h"
#include "getkey.h"
#include "itg2util.h"
#include "itg2-patch-constants.h"

int keydump_itg2_encrypt_file(const char *srcFile, const char *destFile, const unsigned char *subkey, const unsigned int subkeySize, const unsigned char *aesKey, const int type, char *keyFile) {
	FILE *fd, *dfd;
	unsigned int numcrypts, got, fileSize;
	int i,j;
	aes_encrypt_ctx ctx[1];
	unsigned char encbuf[4080], decbuf[4080], backbuffer[16], verifyBlock[16], *plaintext = ":DSPIGWITMERDIKS";

	aes_encrypt_key(aesKey, 24, ctx);

	aes_encrypt(plaintext,verifyBlock,ctx);

	if ((fd = fopen(srcFile, "rb")) == NULL) {
		fprintf(stderr, "%s: fopen(%s) failed D=\n", __FUNCTION__, srcFile);
		return -1;
	}

	if ((dfd = fopen(destFile, "wb")) == NULL) {
		fprintf(stderr, "%s: fopen(%s) failed D=\n", __FUNCTION__, destFile);
		fclose(fd);
		return -1;
	}
	fseek(fd, 0, SEEK_END);
	fileSize = ftell(fd);
	fseek(fd, 0, SEEK_SET);
	fwrite(ITG2FileMagic[type], 2, 1, dfd);
	fwrite(&fileSize, 1, 4, dfd);
	fwrite(&subkeySize, 1, 4, dfd);
	fwrite(subkey, 1, subkeySize, dfd);
	fwrite(verifyBlock, 1, 16, dfd);
	printf("verifyBlock: %02X %02X %02X %02X\n", verifyBlock[0], verifyBlock[1], verifyBlock[2], verifyBlock[3]);

	do {
		if ((got = fread(decbuf, 1, 4080, fd)) == -1) {
			fprintf(stderr, "%s: error: fread(%s) returned -1, exiting...\n", __FUNCTION__, srcFile);
			fclose(fd);
			fclose(dfd);
			return -1;
		}
		numcrypts = got / 16;
		if (got % 16 > 0) { 
			numcrypts++;
		}
		if (got > 0) {
			memset(backbuffer, '\0', 16);
			for (i = 0; i < numcrypts; i++) {
				for (j = 0; j < 16; j++) {
					((unsigned char*)(decbuf+(16*i)))[j] ^= (((unsigned char)backbuffer[j]) - j);
				}
				aes_encrypt(decbuf+(16*i), encbuf+(16*i), ctx);
				memcpy(backbuffer, encbuf+(16*i), 16);
			}
			fwrite(encbuf, 1, numcrypts*16, dfd);
		}
	} while (got > 0);
	fclose(dfd);
	fclose(fd);

	return 0;
}

int keydump_itg2_decrypt_file(const char *srcFile, const char *destFile, int type, const char *keyFile) {
	FILE *fd, *dfd;
	char magic[2];
	unsigned int fileSize = 0, subkeySize = 0, totalBytes = 0, numCrypts = 0, padMisses = 0;
	unsigned char *subkey, aesKey[24], verifyBlock[16], plaintext[16], backbuffer[16];
	unsigned char encbuf[4080], decbuf[4080];
	aes_decrypt_ctx ctx[1];
	int i, j, got, numcrypts;

	printf("keyFile after: %s\n", keyFile);
	if ((fd = fopen(srcFile, "rb")) == NULL) {
		fprintf(stderr, "%s: fopen(%s) failed D=\n", __FUNCTION__, srcFile);
		return -1;
	}

	fread(magic, 1, 2, fd);
	if ( strncmp(magic, ITG2FileMagic[type], 2) ) {
		fprintf(stderr, "%s: source file is not an ITG2 %s zip\n",  __FUNCTION__, ITG2FileDesc[type]);
		fclose(fd);
		return -1;
	}
	fread(&fileSize, 1, 4, fd);
	fread(&subkeySize, 1, 4, fd);
	subkey = (unsigned char*)malloc(subkeySize * sizeof(unsigned char));
	got = fread(subkey, 1, subkeySize, fd);
	printf("got: %d\n",got);
	printf("first 4 bytes of subkey: %02x %02x %02x %02x\n", subkey[0], subkey[1], subkey[2], subkey[3]);
	fread(verifyBlock, 1, 16, fd);

	printf("keyFile before retrieve_aes_key: %s\n", keyFile);
	if ( keydump_itg2_retrieve_aes_key(subkey, subkeySize, aesKey, type, 1, keyFile) == -1 ) {
		fprintf(stderr, "%s: could not retrieve AES key\n", __FUNCTION__);
		fclose(fd);
		return -1;
	}

	aes_decrypt_key(aesKey, 24, ctx);

	aes_decrypt(verifyBlock, plaintext, ctx);
	if (strncmp(plaintext, ITG2DecMagic, 2) != 0) {
		fprintf(stderr, "%s: unexpected decryption magic (wrong AES key)\n", __FUNCTION__);
		fclose(fd);
		return -1;
	}


	if ((dfd = fopen(destFile, "wb")) == NULL) {
		fprintf(stderr, "%s: fopen(%s) failed D=\n", __FUNCTION__, destFile);
		fclose(fd);
		return -1;
	}

	do {
		if ((got = fread(encbuf, 1, 4080, fd)) == -1) {
			fprintf(stderr, "%s: error: fread(%s) returned -1, exiting...\n", __FUNCTION__, srcFile);
			fclose(dfd);
			fclose(fd);
			return -1;
		}
		totalBytes += got;
		numcrypts = got / 16;
		if (got % 16 > 0) { 
			padMisses++; // it means the encrypted zip wasn't made properly, FAIL
			numcrypts++;
		}
		if (totalBytes > fileSize) {
			got -= totalBytes-fileSize;
			totalBytes -= totalBytes-fileSize;
		}
		if (got > 0) {
			memset(backbuffer, '\0', 16);
			for (i = 0; i < numcrypts; i++) {
				aes_decrypt(encbuf+(16*i), decbuf+(16*i), ctx);
				for (j = 0; j < 16; j++) {
					((unsigned char*)(decbuf+(16*i)))[j] ^= (((unsigned char)backbuffer[j]) - j);
				}
				memcpy(backbuffer, encbuf+(16*i), 16);
			}
			fwrite(decbuf, 1, got, dfd);
		}
	} while (got > 0);
	fclose(dfd);
	fclose(fd);
	return padMisses;
}

int keydump_itg2_retrieve_aes_key(const unsigned char *subkey, const unsigned int subkeySize, unsigned char *out, int type, int direction, const char *keyFile) {
	FILE *kfd;
	unsigned char *SHAworkspace, *SHAout;
	int numread = 0;

	printf("keyFile after retrieve_aes_key: %s\n", keyFile);
	if ( direction == 1 && keyFile != NULL ) {	
		kfd = fopen(keyFile, "rb");
		if (kfd == NULL) {
			fprintf(stderr, "%s: cannot open key file (you sure you typed the right file?)\n", __FUNCTION__);
			return -1;
		}
		if ((numread = fread(out, 1, 24, kfd)) < 24) {
			fprintf(stderr, "%s: unexpected key file size (sure it\'s the right file?)\n", __FUNCTION__);
			fclose(kfd);
			return -1;
		}
		printf("numread: %d\n", numread);
		fclose(kfd);
	} else {
		switch (type) {
		case KEYDUMP_ITG2_FILE_DATA:
			if (getKey(subkey, out, KEYDUMP_GETKEY_ITG2, subkeySize) != 0) {
				fprintf(stderr, "%s: getKey() failed\n", __FUNCTION__);
				return -1;
			}
			break;

		case KEYDUMP_ITG2_FILE_PATCH:
			SHAworkspace = (unsigned char*)malloc(sizeof(unsigned char) * (subkeySize+47));
			SHAout = (unsigned char*)malloc(sizeof(unsigned char) * 64);
			memcpy(SHAworkspace, subkey, subkeySize);
			memcpy(SHAworkspace+subkeySize, ITG2SubkeySalt, 47);
			gcry_md_hash_buffer(GCRY_MD_SHA512, SHAout, SHAworkspace, subkeySize+47);
			memcpy(out, SHAout, 24);
			free(SHAworkspace); free(SHAout);
			break;
		default:
			fprintf("%s: huh?\n", __FUNCTION__);
			return -1;
		}
	}

	return 0;
}
