#include "global.h"
#include "RageFileDriverCrypt.h"

static struct FileDriverEntry_PATCH: public FileDriverEntry
{
	FileDriverEntry_PATCH(): FileDriverEntry( "PATCH" ) { }
	RageFileDriver *Create( CString Root ) const { return new RageFileDriverCrypt( Root, 
		"58691958710496814910943867304986071324198643072" ); }
} const g_RegisterDriver;
