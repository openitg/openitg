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
			{"data", no_argument, 0, 'x'},
			{"patch", no_argument, 0, 'p'},
			{"decrypt", no_argument, 0, 'd'},
			{"static", required_argument, 0, 's'},
			/*{"source", required_argument, 0, 'i'},
			{"dest",  required_argument, 0, 'o'},*/
			{0, 0, 0, 0}
		};

		c = getopt_long(argc, argv, "xpds:", long_options, &option_index);
		option_index++;
		if (c == -1) break;

		switch(c) {
		case 0:
			/*if (!strcmp(long_options[option_index].name,"source") && optarg) {
				openFile = optarg;
			} else if (!strcmp(long_options[option_index].name,"dest") && optarg) {
				destFile = optarg;
			} else */
			if (!strcmp(long_options[option_index].name,"data")) {
				type = KEYDUMP_ITG2_FILE_DATA;
			} else if (!strcmp(long_options[option_index].name,"patch")) {
				type = KEYDUMP_ITG2_FILE_PATCH;
			} else if (!strcmp(long_options[option_index].name,"static")) {
				type = KEYDUMP_ITG2_FILE_STATIC;
				keyFile = optarg;
			} else if (!strcmp(long_options[option_index].name,"decrypt")) {
				direction = 1;
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
			type = KEYDUMP_ITG2_FILE_STATIC;
			keyFile = optarg;
			break;

/*
		case 'i':
			openFile = optarg;
			break;

		case 'o':
			destFile = optarg;
			break;
*/

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
	if (direction == 0) {
		int i;
		for (i = 0; i < 1024; i++)
			subkey[i] = (unsigned char)rand() * 255;

		if (keydump_itg2_retrieve_aes_key(subkey, 1024, aesKey, type, keyFile) == -1) {
			fprintf(stderr, "%s: could not retrieve AES key, exiting...\n", argv[0]);
			return -1;
		}

		if (keydump_itg2_encrypt_file(openFile, destFile, subkey, 1024, aesKey, type) == -1) {
			fprintf(stderr, "%s: could not encrypt %s, exiting...\n", argv[0], openFile);
			return -1;
		}
	// decrypt
	} else {
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
	//printf("\t--source (-f)\tSource File (required argument)\n");
	//printf("\t--dest (-w)\tDestination File (required argument)\n\n");
	printf("\t--data (-f)\tTreat source file as data file (default)\n");
	printf("\t--patch (-p)\tTreat source file as patch file\n");
	printf("\t--static (-s)\tStatic Encryption/Decryption: AES key is in a separate file (required argument as key file)\n\n");
}
