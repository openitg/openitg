#include <cstdio>
#include <cstdlib>
#include "USB-classes.h"

// Service, test buttons - increments the counter
const uint32_t COIN_EVENT = (1 << 14) | (1 << 9);
// P1 and P2 Start - ends the program
const uint32_t END_EVENT = (1 << 4) | (1 << 20);

int main()
{
	PIUIO Board;

	if( !Board.Open() )
	{
		printf( "Could not open PIUIO.\n" );
		return 1;
	}

	uint32_t InputData, OutputData;

	// initialization write
	Board.Write( 0xE7FCFFFC );

	while( 1 )
	{
		InputData = 0;
		OutputData = 0xEFFCFFFC;

		Board.Read( &InputData );

		// invert, for logical processing
		InputData = ~InputData;

		if( InputData & COIN_EVENT )
			OutputData |= (1 << 28);

		Board.Write( OutputData );

		if( InputData & END_EVENT )
			break;
	}

	Board.Close();
	return 0;
}
