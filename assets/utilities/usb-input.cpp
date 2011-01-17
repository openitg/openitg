#include <cstdio>
#include "USBIO.h"

/*
void LongToBitString( uint32_t data, char *str )
{
	for( int i = 0; i < 32; i++ )
		str[i] = (data & (1<<(32-i))) ? "1" : "0";
}
*/

int main()
{
	// probe for any readable boards
	USBDriver *pDriver;
	Board BoardType;

	pDriver = USBIO::FindBoard( BoardType );

	if( pDriver == NULL )
	{
		printf( "Error: no boards could be opened for input!\n" );
		return 2;
	}

	if( BoardType == BOARD_PACDRIVE )
	{
		printf( "The PacDrive cannot read input - terminating.\n" );
		return 4;
	}

	// mask the values as needed for each board
	if( !pDriver->Open() )
	{
		printf( "Crash reason: WinDEU\n" );
		return 2;
	}

	// initialization write
	uint32_t iWrite = 0xFFFFFFFF;
	USBIO::MaskOutput( BoardType, &iWrite );
	pDriver->Write( iWrite );

	uint32_t iData;
	char sBits[32];
	while( 1 )
	{
		pDriver->Read( &iData );
		iData = ~iData;
		printf( "%8#x\r", iData );
//		LongToBitString( iData, &sBits );
//		printf( "%s\n", sBits );
	}

	pDriver->Write( 0 );
	pDriver->Close();

	return 0;
}
