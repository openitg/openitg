#include <stdio.h>
#include <gcrypt.h>
#include "keydump.h"
#include "ownet.h"
#include "shaib.h"
#include "getkey.h"

int getKey(const unsigned char *subkey, unsigned char *output, int game, unsigned int subkeySize) {
	unsigned char firstDataPage[32], firstScratchPad[32], shA1steaksauce[60];
	SHACopr copr; // honestly, I don't have to go through using a structure, but I will anyway...
	int i;
	unsigned char *toSign; // if it's Pump Pro, it'll be the SHA1 crap, if ITG2, just the subkey

	// Pump It Up Pro does this weirdo SHA1 hashing scheme of the subkey data before it gets sent to the dongle
	//  for hash generation...
	if (game == KEYDUMP_GETKEY_PPRO) {
		unsigned char *SHAworkspace = (unsigned char *)malloc(sizeof(unsigned char) * (subkeySize+40));

		// first SHA1
		gcry_md_hash_buffer(GCRY_MD_SHA1, shA1steaksauce, subkey, subkeySize);

		// second SHA1
		memcpy(SHAworkspace, subkey, subkeySize);
		memcpy(SHAworkspace+subkeySize, shA1steaksauce, 20);
		gcry_md_hash_buffer(GCRY_MD_SHA1, shA1steaksauce+20, SHAworkspace, subkeySize+20);

		// third SHA1
		memcpy(SHAworkspace, subkey, subkeySize);
		memcpy(SHAworkspace+subkeySize, shA1steaksauce, 40);
		gcry_md_hash_buffer(GCRY_MD_SHA1, shA1steaksauce+40, SHAworkspace, subkeySize+40);

		memcpy(firstDataPage, shA1steaksauce, 32);
		toSign = shA1steaksauce;
	} else { // ...whereas ITG2 just gives the dongle the subkey
		toSign = subkey;
	}

	// TODO: if subkey is less than 47 chars (would that even be an issue anyway)?

	memcpy(firstDataPage, toSign, 32);
	
#ifdef KD_DEBUG
	printf("KD_DEBUG enabled...\n");
	printf("Acquiring port...");
#endif



	if ((copr.portnum = owAcquireEx("/dev/ttyS0")) == -1) {
		printf("getKey(): failed to acquire port.\nCheck to see that:\n=== you have the correct serial drivers loaded in the kernel\nOR\n=== that the security dongle connected to the boxor has not become loose\n");
		return -1;
	}




#ifdef KD_DEBUG
	printf(" done.\n");
	printf("portnum = %d\n", copr.portnum);
	printf("Finding device address/serial...");
#endif


	FindNewSHA(copr.portnum, copr.devAN, TRUE);
	owSerialNum(copr.portnum, copr.devAN, FALSE);



#ifdef KD_DEBUG
	printf("serial: ");
	for (i = 7; i >= 0; i--) printf("%02x", copr.devAN[i]);
	printf("\n");

	printf("written to data page:\n");
	printScratchpad(firstDataPage);
#endif


	WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
	memset(firstScratchPad, '\0', 32);

	memcpy(firstScratchPad+8, toSign+32, 15);


#ifdef KD_DEBUG
	printf("BEFORE:\n");
	printScratchpad(firstScratchPad);
#endif



	WriteScratchpadSHA18(copr.portnum, 0, firstScratchPad, 32, 1);
	SHAFunction18(copr.portnum, SHA_SIGN_DATA_PAGE, 0, 1);
	ReadScratchpadSHA18(copr.portnum, 0, 0, firstScratchPad, 1);



#ifdef KD_DEBUG
	printf("AFTER:\n");
	printScratchpad(firstScratchPad);
#endif


	// erase any leftover evidence
	memset(firstDataPage, '\0', 32);
	WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
	memcpy(output, firstScratchPad+8, 24);
	owRelease(copr.portnum);
	
	return 0;
}

