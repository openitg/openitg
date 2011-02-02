#include <stdio.h>

#ifndef OW_UCHAR
typedef unsigned char uchar;
#endif

void printbuffer(const char *name, uchar buf[16]) {
	int i = 0;
	printf("%s: ", name);
	for (i; i < 16; i++)
		printf("%02x ", buf[i]);
	printf("\n");
}

void printKey(uchar aesKey[24]) {
	int i = 0;
	for (i; i < 24; i++) {
		printf("%02x ", aesKey[i]);
	}
	printf("\n");
}

void printScratchpad(uchar scratchPad[32]) {
        int i = 0, j = 0;
        for (i = 0; i < 2; i++) {
                for (j = 0; j < 16; j++) {
                        printf("%02x ", scratchPad[(i*16)+j]);
                }
                printf("\n");
        }
}



