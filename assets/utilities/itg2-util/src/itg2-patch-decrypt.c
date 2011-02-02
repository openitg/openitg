#include <stdio.h>
#include "keydump.h"
#include "itg2util.h"

int main(int argc, char *argv[]) {
	char *openFile, *destFile;
	int ret;

	printIntro("ITG2");

	if (argc < 3) {
		printf("usage: %s <input file> <output file>\n", argv[0]);
		exit(0);
	}
	openFile = argv[1];
	destFile = argv[2];

	printf("decrypting %s into %s...\n", srcFile, destFile);
	ret = keydump_itg2_decrypt_file(openFile, destFile, KEYDUMP_ITG2_FILE_PATCH, NULL);
	if (ret == -1) {
		fprintf(stderr, "%s: failed to decrypt %s, exiting...\n", argv[0], openFile);
		return -1;
	}

	return 0;
	
}

