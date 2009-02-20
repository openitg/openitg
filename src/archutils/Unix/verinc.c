// Ehhh....
//
// I think I'll just put this one in the public domain
// (with no warranty as usual).
//
// --Avery

// Modified for OpenITG pre-build event... -- Vyhd

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <time.h>

typedef unsigned long ulong;

/* UGLY HACK: we've got no particularly good way to get the
 * SVN version input, aside from stdin. So, we do that. */
int main( int argc, char **argv )
{
	FILE *f;
	int has_version = 0;
	ulong build = 0;
	char strdate[10], strtime[25];
	time_t tm;

	// try to read the last version from stdin
	if( argc >= 2 )
		build = strtol( argv[1], NULL, 10 );

	if( build != 0 )
		has_version = 1;

	// get the current time
	time(&tm);

	// print the debug serial date/time
	strftime( strdate, 15, "%Y%m%d", localtime(&tm) );
	memcpy( strtime, asctime(localtime(&tm)), 24 );

	// zero out the newline character
	strtime[24] = 0;

	if ( f = fopen("verstub.cpp","w") )
	{
		fprintf(f,
			"unsigned long VersionNumber = %ld;\n"
			"extern const char *const VersionTime = \"%s\";\n"
			"extern const char *const VersionDate = \"%s\";\n"
			"extern const bool VersionSVN = %s;\n",
			build, strtime, strdate,
			has_version == 1 ? "true" : "false"
			);

		fclose( f );
	}

	return 0;
}
