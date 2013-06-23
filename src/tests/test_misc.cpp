#include "global.h"
#include "test_misc.h"

#include "RageFileManager.h"
#include "RageLog.h"
#include "arch/arch.h"
#include "arch/ArchHooks/ArchHooks.h"
void ExitGame() { }

CString g_Driver = "dir", g_Root = ".";

CString argv0;
int g_argc = 0;
char **g_argv = NULL;

void HandleException( CString sErr )
{

}

void test_handle_args( int argc, char *argv[] )
{
	g_argc = argc;
	g_argv = argv;
	argv0 = argv[0];

	while( 1 )
	{
		opterr = 0;
		int opt = getopt( argc, argv, "cd:r:h" );
		if( opt == -1 )
			break;
		switch( opt )
		{
		case 'd':
			g_Driver = optarg;
			break;
		case 'r':
			g_Root = optarg;
			break;

		case 'h':
			printf( "options:\n" );
			printf( " -c: create test files\n" );
			printf( " -d driver: choose file driver to test (default \"Dir\")\n" );
			printf( " -r root: set file driver root (default \".\")\n" );
			exit(1);
		}

	}
}

void test_init()
{
	HOOKS = MakeArchHooks();

	FILEMAN = new RageFileManager( argv0 );
	FILEMAN->Mount( "dir", InitialWorkingDirectory, "/" );
	FILEMAN->Mount( g_Driver, g_Root, "/" );

	LOG = new RageLog();
	LOG->SetLogToDisk( false );
	LOG->SetShowLogOutput( true );
	LOG->SetFlushing( true );
}

void test_deinit()
{
	delete LOG;
	delete FILEMAN;
	delete HOOKS;
}


