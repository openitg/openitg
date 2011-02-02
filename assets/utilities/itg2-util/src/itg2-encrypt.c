#include <stdio.h>
#include <math.h>
#include "keydump.h"
#include "itg2util.h"

int main(int argc, char *argv[]) {
	int ret, i;
	char *openFile, *destFile, subkey[1024], aesKey[24];

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

	ret = keydump_itg2_retrieve_aes_key(subkey, 1024, aesKey, KEYDUMP_ITG2_FILE_DATA, NULL);

	ret = keydump_itg2_encrypt_file(openFile, destFile, subkey, 1024, aesKey, KEYDUMP_ITG2_FILE_DATA);

	return 0;
	
}

