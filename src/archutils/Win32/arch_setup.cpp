#include "global.h"
#include "arch_setup.h"
#include "RageThreads.h"
#include <time.h>
#ifdef _WINDOWS
#  include <windows.h>
#endif
#include "StepMania.h"

struct tm *my_localtime_r( const time_t *timep, struct tm *result )
{
	static RageMutex mut("my_localtime_r");
	LockMut(mut);

	*result = *localtime( timep );
	return result;
}

struct tm *my_gmtime_r( const time_t *timep, struct tm *result )
{
	static RageMutex mut("my_gmtime_r");
	LockMut(mut);

	*result = *gmtime( timep );
	return result;
}

void my_usleep( unsigned long usec )
{
	Sleep( usec/1000 );
}

#if defined(WINDOWS)
/* Ugh.  Windows doesn't give us the argv[] parser; all it gives is CommandLineToArgvW,
 * which is NT-only, so we have to do this ourself.  Don't be fancy; only handle double
 * quotes. */
int GetWin32CmdLine( char** &argv )
{
	char *pCmdLine = GetCommandLine();
	int argc = 0;
	argv = NULL;

	int i = 0;
	while( pCmdLine[i] )
	{
		argv = (char **) realloc( argv, (argc+1) * sizeof(char *) );
		argv[argc] = pCmdLine+i;
		++argc;

		/* Skip to the end of this argument. */
		while( pCmdLine[i] && pCmdLine[i] != ' ' )
		{
			if( pCmdLine[i] == '"' )
			{
				/* Erase the quote. */
				memmove( pCmdLine+i, pCmdLine+i+1, strlen(pCmdLine+i+1)+1 );

				/* Skip to the close quote. */
				while( pCmdLine[i] && pCmdLine[i] != '"' )
					++i;

				/* Erase the close quote. */
				if( pCmdLine[i] == '"' )
					memmove( pCmdLine+i, pCmdLine+i+1, strlen(pCmdLine+i+1)+1 );
			}
			else
				++i;
		}

		if( pCmdLine[i] == ' ' )
		{
			pCmdLine[i] = '\0';
			++i;
		}
	}

	return argc;
}

int __stdcall WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, char *pCmdLine, int nCmdShow )
{
	char **argv;
	int argc = GetWin32CmdLine( argv );

	return main( argc, argv );
}
#endif

/*
 * (c) 2004 Glenn Maynard
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
