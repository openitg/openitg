#include <stdio.h>
#include <getopt.h>
#include "keydump.h"
#include "itg2util.h"

int main(int argc, char **argv) {
	// direction: 0 = encrypt, 1 = decrypt
	int type = KEYDUMP_ITG2_FILE_DATA, ret, direction = 0;
	char c, *openFile = NULL, *destFile = NULL, *keyFile = NULL;
	unsigned char subkey[1024], aesKey[24];

	srand(time(NULL));

	printIntro("ITG2");

	if ( argc < 2 ) {
		printHelp(argv[0]);
		return 0;
	}

	int option_index = 0;
	while (1) {
		static struct option long_options[] = {
			{"patch", no_argument, 0, 'p'},
			{"decrypt", no_argument, 0, 'd'},
			{"static", required_argument, 0, 's'},
			{"help", no_argument, 0, 'h'},
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "hxpds:", long_options, &option_index);
		option_index++;
		if (c == -1) break;

		switch(c) {
		case 0:
			if (!strcmp(long_options[option_index].name,"patch")) {
				type = KEYDUMP_ITG2_FILE_PATCH;
			} else if (!strcmp(long_options[option_index].name,"static")) {
				keyFile = optarg;
				++option_index;
			} else if (!strcmp(long_options[option_index].name,"decrypt")) {
				direction = 1;
			} else if (!strcmp(long_options[option_index].name,"help")) {
				printHelp(argv[0]);
				exit(0);
			}
			break;

		case 'd':
			direction = 1;
			break;

		case 'x':
			type = KEYDUMP_ITG2_FILE_DATA;
			break;

		case 'p':
			type = KEYDUMP_ITG2_FILE_PATCH;
			break;

		case 's':
			keyFile = optarg;
			++option_index;
			break;

		case 'h':
			printHelp(argv[0]);
			exit(0);

		default:
			return -1;
		}
	}

	if ( option_index+2 > argc ) {
		printHelp(argv[0]);
		return 0;
	}

	openFile = argv[option_index];
	destFile = argv[option_index+1];

	// sanity checks
	if (openFile == NULL) {
		printf("please specify a source file\n");
		printHelp(argv[0]);
		return 0;
	}
	if (destFile == NULL) {
		printf("please specify a destination file\n");
		printHelp(argv[0]);
		return 0;
	}
	if (type == KEYDUMP_ITG2_FILE_STATIC && keyFile == NULL) {
		fprintf(stderr, "%s: static method with no key file specified, exiting...\n", argv[0]);
		return -1;
	}

	// encrypt
	if (direction == 0 && keyFile != NULL) {
		// TODO: everything about this is terrible -- should be moved into itg2util.c somewhere
		FILE *fd, *dfd, *kf;
		char magic[2];
		unsigned int fileSize = 0, subkeySize = 0, totalBytes = 0, numCrypts = 0, padMisses = 0, got = 0;
		unsigned char *subkey, aesKey[24], verifyBlock[16], plaintext[16], backbuffer[16];

		if ((fd = fopen(openFile, "rb")) == NULL) {
			fprintf(stderr, "%s: fopen(%s) failed D=\n", __FUNCTION__, openFile);
			return -1;
		}

		fread(magic, 1, 2, fd);
		fread(&fileSize, 1, 4, fd);
		fread(&subkeySize, 1, 4, fd);
		subkey = (unsigned char*)malloc(subkeySize * sizeof(unsigned char));
		got = fread(subkey, 1, subkeySize, fd);
		if (keydump_itg2_retrieve_aes_key(subkey, 1024, aesKey, type, direction, keyFile) == -1) {
			fprintf(stderr, "%s: could not retrieve AES key, exiting...\n", argv[0]);
			return -1;
		}
		kf = fopen(keyFile, "wb");
		if ( kf == NULL ) {
			fprintf(stderr, "%s: could not open key file for output, exiting...\n", argv[0]);
			return -1;
		}
		fwrite(aesKey, 1, 24, kf);
		fclose(kf);
	} else if (direction == 0) {
		int i;
		for (i = 0; i < 1024; i++)
			subkey[i] = (unsigned char)rand() * 255;
		
		if (keydump_itg2_retrieve_aes_key(subkey, 1024, aesKey, type, direction, keyFile) == -1) {
			fprintf(stderr, "%s: could not retrieve AES key, exiting...\n", argv[0]);
			return -1;
		}

		if (keydump_itg2_encrypt_file(openFile, destFile, subkey, 1024, aesKey, type, keyFile) == -1) {
			fprintf(stderr, "%s: could not encrypt %s, exiting...\n", argv[0], openFile);
			return -1;
		}

	// decrypt
	} else {
		printf("keyFile before: %s\n", keyFile);
		if (keydump_itg2_decrypt_file(openFile, destFile, type, keyFile) == -1) {
			fprintf(stderr, "%s: could not decrypt %s, exiting...\n", argv[0], openFile);
			return -1;
		}
	}

	return 0;
}

void printHelp( const char *argv0 ) {
	printf("Usage: %s -f <source file> -w <dest file> [extra args]\n", argv0);
	printf("\t--decrypt (-d)\tDecrypt mode\n\n");
	printf("\t--data (-f)\tTreat source file as data file (default)\n");
	printf("\t--patch (-p)\tTreat source file as patch file\n");
	printf("\t--static (-s)\tStatic Encryption/Decryption: AES key is in a separate file (required argument as key file)\n\n");
}
