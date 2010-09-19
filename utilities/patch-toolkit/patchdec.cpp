#include <iostream>
#include <fstream>
#include "StdString.h"
#include "sha512.h"
#include "aescpp.h"
#include "aesopt.h"
#include <memory.h>

typedef unsigned char uchar;

using namespace std;

const char *patchkey = "58691958710496814910943867304986071324198643072";

inline void printIntro();
void usage(char *);

int main(int argc, char *argv[])
{
	char *header, *workingDir, *chrBuf1, *chrBuf2, *outFile;
	int numcrypts, workingDirLen = 0;
	ifstream fdin;
	ofstream fdout;
	unsigned int fileSize, subkeySize;
	CString SHASecret("");
	uchar SHADigest[65];
	uchar AESKey[24];
	uchar encbuf[4081], decbuf[4081];
	char var_178[16];
	unsigned int dec_recv = 0, headerSize = 0;
	long long totalbytes = 0;

	AESdecrypt ct;
	uchar checkbuf_in[16], checkbuf_out[16], subkey[1024];

	printIntro();

	encbuf[4080] = '\0';
	decbuf[4080] = '\0';

	if (argc < 2) {
		usage(argv[0]);
		return 0;
	}

	if (strstr(argv[1], "patch-dec.zip"))
	{
		cerr << "the source file cannot be named patch-dec.zip\n";
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
		outFile = (char*)malloc(sizeof(char) * workingDirLen + 14);
		strcpy(outFile, workingDir);
		strcpy(outFile+workingDirLen, "patch-dec.zip");
		outFile[workingDirLen+13] = '\0';
	} else {
		outFile = "patch-dec.zip";
	}

	fdin.open(argv[1], ios::in | ios::binary);
	fdout.open(outFile, ios::out | ios::binary);

	header = new char(3);
	header[2] = '\0';

	if (!fdin.is_open())
	{
		cerr << "cannot open input file " << argv[1] << " (typo perhaps?) :(\n";
		return -1;
	}
	if (!fdout.is_open()) {
		cerr << "cannot open output file " << outFile << " :(\n";
		return -1;
	}

	fdin.read(header, 2);
	if (strcmp(header, "8O") != 0)
	{
		cerr << argv[1] << " is not an encrypted patch file :\\\n";
		return -1;
	}
	headerSize += 2;

	fdin.read((char*)&fileSize, 4);
	cout << "file size: " << fileSize << " bytes" << endl;
	headerSize += 4;

	fdin.read((char*)&subkeySize, 4);
	cout << "subkey size: " << subkeySize << endl;
	headerSize += 4;

	fdin.read((char *)subkey, subkeySize);
	headerSize += subkeySize;
	SHASecret.append((char *)subkey, subkeySize);
	SHASecret.append(patchkey, 47);
	cout << "SHA512 Message length: " << SHASecret.length() << endl;
	SHA512_Simple(SHASecret.c_str(), subkeySize+47, SHADigest);
	SHADigest[64] = '\0';

	strncpy((char*)AESKey, (char*)SHADigest, 24);
	ct.key(AESKey, 24);

	cout << "verifying encryption key magic...";
	fdin.read((char*)checkbuf_in, 16);
	headerSize += 16;
	ct.decrypt(checkbuf_in, checkbuf_out);

	if (strstr((char*)checkbuf_out, ":D") != NULL)
	{
		cout << "verified :D\n";
	} else {
		cout << "VERIFICATION FAILED D:\n";
		return 1;
	}

	cout << endl;

	while (!fdin.eof())
	{
		printf("Decrypting (%d%%)", ((totalbytes * 100) / fileSize));
		putchar(0x0d);

		if (fdin.read((char *)encbuf, (unsigned int)min((unsigned int)4080,(unsigned int)fileSize-(unsigned int)totalbytes) ).fail())
		{
			fdin.close();
			fdout.close();
			return 1;
		}
		dec_recv = fdin.gcount();
		if ( dec_recv == 0 )
			break;

		numcrypts = dec_recv / 16;
		totalbytes += dec_recv;

		memset(var_178, '\0', 16);

		for (int j = 0; j < numcrypts; j++)
		{
			ct.decrypt(encbuf+(j*16), checkbuf_out);
			for (int i = 0; i < 16; i++)
			{
				checkbuf_out[i] ^= (((unsigned char)var_178[i]) - i);
			}
			memcpy(var_178, encbuf+(j*16), 16);
			memcpy(decbuf+(j*16),checkbuf_out,16);
		}

		fdout.write((char*)decbuf, dec_recv);
		fdout.flush();
	}

	cout << "Decrypting done\n";

	fdin.close();
	fdout.close();
	return 0;
}

inline void printIntro()
{
	cout << "ITG2 Arcade patch.zip decrypter (c) 2007 infamouspat\n\n";
}

void usage(char *argv0)
{
	cout << "Usage: " << argv0 << " <patch.zip location>\n--- decrypted contents will be placed in patch-dec.zip";
}
