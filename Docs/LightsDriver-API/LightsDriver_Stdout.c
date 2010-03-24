/* Example LightsDriver, shared library, using printf().
 * Compile with:
 * gcc -O2 -Wall -fPIC -shared LightsDriver_Stdout.c -o LightsDriver_Stdout.so
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "LightsDriver_ModuleDefs.h"

struct LightsModuleInfo info =
{
	LIGHTS_API_VERSION_MAJOR,	/* API major version */
	LIGHTS_API_VERSION_MINOR,	/* API minor version */
	1,				/* Lib major version */
	0,				/* Lib minor version */
	"Stdout",			/* Driver description */
	"Vyhd",				/* Driver author */
};

const struct LightsModuleInfo* GetModuleInfo()
{
	return &info;
}

const char *GetError( int code )
{
	switch( code )
	{
	case 1:
		return "You got rand()'d (equal to zero)!";
	};

	return NULL;
}

int Load()
{
	printf( "Stdout init\n" );

	if( rand() % 5 == 0 )
		return 1;

	return 0;
}

void Unload()
{
	printf( "Stdout de-init\n" );
}

int Update( struct CLightsState *state )
{
	enum CabinetLight cl	= (enum CabinetLight) 0;
	enum GameController gc	= (enum GameController) 0;
	enum GameButton gb	= (enum GameButton) 0;

	FOREACH_CabinetLight( cl )
		printf( "%c", (state->m_bCabinetLights[cl]) == 0 ? '0' : '1' );
	printf( " " );

	FOREACH_GameController( gc )
	{
		FOREACH_GameButton( gb )
			printf( "%c", (state->m_bGameButtonLights[gc][gb]) == 0 ? '0' : '1' );

		printf( " " );
	}

	printf( "\n" );
	return 0;
}
