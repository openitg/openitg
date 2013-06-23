#include "StdString.h"
#include "sha512.h"
#include "aescpp.h"
#include "aesopt.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>

using namespace std;

const char *patchkey = "58691958710496814910943867304986071324198643072";

void encusage(char*);
inline void printEncIntro();


int main(int argc, char *argv[])
{
	FILE *fdin, *fdout;
	unsigned char subkey[1024], buffer[4080];
	char *workingDir, *chrBuf1, *chrBuf2, *outFile;
	int workingDirLen;
	CString SHASecret("");
	unsigned char SHADigest[64];
	unsigned char AESKey[24], backbuffer[16], backbuffer2[16], cryptbuf[16];
	long filesize;
	int got, numcrypts, writesize;
	AESencrypt ct;
	long long totalbytes = 0;

	printEncIntro();

	if (argc < 2) { encusage(argv[0]); return 0; }

	if (strstr(argv[1], "patch.zip"))
	{
		cerr << "the source file cannot be named patch.zip :P\n";
		return -1;
	}
	
	chrBuf1 = strchr(argv[1], '\\');
	if (chrBuf1 != NULL) {
		while (chrBuf1 != NULL) {
			chrBuf2 = chrBuf1;
			chrBuf1 = strchr(chrBuf1+1, '\\');
		}
		workingDirLen = chrBuf2 - argv[1] + 1;
		workingDir = (char*)malloc(sizeof(char) * workingDirLen);
		strncpy(workingDir, argv[1], workingDirLen);
		outFile = (char*)malloc(sizeof(char) * workingDirLen + 10);
		strcpy(outFile, workingDir);
		strcpy(outFile+workingDirLen, "patch.zip");
		outFile[workingDirLen+9] = '\0';
	} else {
		outFile = "patch.zip";
	}
	
	if ((fdin = fopen(argv[1], "rb")) == NULL)
	{
		cerr << "The source file could not be opened (typo?) :(\n";
		return -1;
	}

	fseek(fdin, 0, SEEK_END);
	filesize = ftell(fdin);
	rewind(fdin);

	if ((fdout = fopen(outFile, "wb")) == NULL)
	{
		cerr << "The destination file could not be opened, this is bad :/\n";
		return -1;
	}

	// first is first...
	fwrite("8O", 2, 1, fdout);
	// size of the plaintext zip file
	fwrite(&filesize, 4, 1, fdout);
	// size of the subkey, which we'll always make 1024
	fwrite("\x00\x04\x00\x00", 4, 1, fdout);

	// generate subkey
	for (int i = 0; i < 1024; i++)
		subkey[i] = (unsigned char)rand();
	// write actual subkey
	fwrite(subkey, 1024, 1, fdout);
	
	SHASecret.append((char*)subkey, 1024);
	SHASecret.append(patchkey, 47);
	SHA512_Simple(SHASecret.c_str(), 1071 /*1024 + 47*/, SHADigest);
	memcpy(AESKey, SHADigest, 24);

	ct.key(AESKey, 24);

	// verification block
	unsigned char *verification_block = (unsigned char*)":DSPIGWITMERDIKS";
	ct.encrypt(verification_block,backbuffer2);
	fwrite(backbuffer2, 16, 1, fdout);

	got = fread(buffer, 1, 4080, fdin);

	do
	{
		printf("Encrypting %s: %d%%    ", argv[1], (totalbytes * 100) / filesize);
		putchar(0x0d);

		numcrypts = (got / 16);
		if ((got % 16) > 0)
		{
			numcrypts++;
			filesize += got % 16;
		}
		writesize = numcrypts * 16;
		memset(backbuffer, '\0', 16);
		for (int j = 0; j < numcrypts; j++)
		{
			memcpy(backbuffer2, buffer+(j*16), 16);
			memcpy(cryptbuf, buffer+(j*16), 16);
			for (int i = 0; i < 16; i++)
			{
				cryptbuf[i] ^= (backbuffer[i] - i);
			}
			ct.encrypt(cryptbuf, buffer+(j*16));
			memcpy(backbuffer, buffer+(j*16), 16);
		}
		fwrite(buffer, 1, writesize, fdout);
		totalbytes += writesize;
		got = fread(buffer, 1, 4080, fdin);
	} while (got > 0);

	cout << "Encrypting " << argv[1] << ": done\n";

	fclose(fdin);
	fclose(fdout);
	return 0;
}

inline void printEncIntro()
{
	cout << "ITG2 Arcade patch.zip encrypter (c) 2007 infamouspat\n\n";
}


void encusage( char *argv0 )
{
	cout << "Usage: " << argv0 << " <zip file>\n\nFile created will be named patch.zip\nDO NOT have the source file be named patch.zip (use something like patchdec.zip)\n";
}

