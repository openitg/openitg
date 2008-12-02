// Ehhh....
//
// I think I'll just put this one in the public domain
// (with no warranty as usual).
//
// --Avery

// Rewritten for OpenITG pre-build event... -- Vyhd

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <time.h>

// strftime() formatted string
#define DATE_FORMAT "%Y%m%d"

int main()
{
	FILE *verinfo;
	char datebuf[256];

	// get the current time and create a string
	time_t curtime = time(NULL);
	strftime( datebuf, 255, DATE_FORMAT, localtime(&curtime) );

	/* TODO: more intelligent BUILD_VERSION thing */
	if( verinfo = fopen("verinfo.h", "w") )
	{
		fprintf( verinfo,
			"#define BUILD_VERSION \"SVN\"\n"
			"#define BUILD_DATE \"%s\"",
			datebuf
			);

		fclose(verinfo);
	}

	return 0;
}
