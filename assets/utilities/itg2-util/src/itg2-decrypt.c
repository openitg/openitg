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

	printf("decrypting %s into %s...\n", openFile, destFile);
	ret = keydump_itg2_decrypt_file(openFile, destFile, KEYDUMP_ITG2_FILE_DATA, NULL);
	if (ret > -1) {
		printf("done :D\n");
		printf("padMisses: %u\n", ret);
	}
	fclose(dfd);
	fclose(fd);
	return 0;
	
}

