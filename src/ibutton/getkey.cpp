#include "global.h"
#include "RageLog.h"

extern "C" {
#include "ownet.h"
#include "shaib.h"
}

int getKey(const uchar *subkey, uchar *output) {
	uchar firstDataPage[32], firstScratchPad[32];
	SHACopr copr;
	int i;

	memcpy(firstDataPage, subkey, 32);
	
	// XXX: this looks suspiciously non-portable.
	if ((copr.portnum = owAcquireEx("/dev/ttyS0")) == -1) {
		LOG->Warn("getKey(): failed to acquire port.\n");
		return -1;
	}
	FindNewSHA(copr.portnum, copr.devAN, TRUE);
	owSerialNum(copr.portnum, copr.devAN, FALSE);

	WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
	memset(firstScratchPad, '\0', 32);
	memcpy(firstScratchPad+8, subkey+32, 15);

	WriteScratchpadSHA18(copr.portnum, 0, firstScratchPad, 32, 1);
	SHAFunction18(copr.portnum, 0xC3, 0, 1);
	ReadScratchpadSHA18(copr.portnum, 0, 0, firstScratchPad, 1);

	memset(firstDataPage, '\0', 32);
	WriteDataPageSHA18(copr.portnum, 0, firstDataPage, 0);
	memcpy(output, firstScratchPad+8, 24);

	owRelease(copr.portnum);	
	return 0;
}

