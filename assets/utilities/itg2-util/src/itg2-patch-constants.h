#ifndef ITG2_PATCH_CONSTANTS_H
#define ITG2_PATCH_CONSTANTS_H
#include "itg2util.h"

const char *ITG2SubkeySalt = "58691958710496814910943867304986071324198643072";

const char *ITG2FileMagic[2] = {
	[KEYDUMP_ITG2_FILE_DATA] = ":|",
	[KEYDUMP_ITG2_FILE_PATCH] = "8O"
};

const char *ITG2FileDesc[2] = {
	[KEYDUMP_ITG2_FILE_DATA] = "data",
	[KEYDUMP_ITG2_FILE_PATCH] = "patch"
};

const char *ITG2DecMagic = ":D";

#endif /* ITG2_PATCH_CONSTANTS_H */

