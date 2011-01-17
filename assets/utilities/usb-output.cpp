#include <cstdio>
#include <cstdlib>
#include "USBIO.h"

int main( int argc, char *argv[] )
{
	if( argc != 2 )
	{
		printf( "Usage: %s [32-bit hex]\n", argv[0] );
		return 1;
	}

	uint32_t iData = strtoul( argv[1], NULL, 16 );
	printf( "Output data: %#8x\n", iData );

	// try to find a board we can actually output this to.

	USBDriver *pDriver;
	Board BoardType;
	pDriver = USBIO::FindBoard( BoardType );

	if( pDriver == NULL )
	{
		printf( "Error: no boards could be opened for output!\n" );
		return 2;
	}

	// mask the values as needed for each board
	USBIO::MaskOutput( BoardType, &iData );
	printf( "New output data: %#8x\n", iData );

	// not sure why this would fail now, but better check.
	if( !pDriver->Open() )
	{
		printf( "Crash reason: WinDEU\n" );
		return 2;
	}

	pDriver->Write( iData );
	pDriver->Close();

	return 0;
}
