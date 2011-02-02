#include <stdio.h>
#include <math.h>
#include <gcrypt.h>
#include "keydump.h"
#include "aes.h"
#include "patch-constants.h"

int main(int argc, char *argv[]) {
	int ret;
	unsigned char aesKey[24], subkey[1024];
	char *openFile, *destFile;

	srand(time(NULL));

	printIntro("ITG2");

	if (argc < 3) {
		printf("usage: %s <input file> <output file>\n", argv[0]);
		exit(0);
	}
	openFile = argv[1];
	destFile = argv[2];

	for (i = 0; i < 1024; i++)
		subkey[i] = rand() * 255;

	printf("deriving AES key...\n");
	if (keydump_itg2_retrieve_aes_key(subkey, 1024, aesKey, KEYDUMP_ITG2_FILE_PATCH, NULL)) == -1) {
		fprintf("%s: could not retrieve AES key, exiting...\n", argv[0]);
		return -1;
	}

	printf("encrypting %s into %s...\n", openFile, destFile);
	if (keydump_itg2_encrypt_file(openFile, destFile, subkey, 1024, aesKey, KEYDUMP_ITG2_FILE_PATCH) == -1) {
		fprintf("%s: could not encrypt %s, exiting...\n", argv[0], openFile);
		return -1;
	}

	return 0;
}

