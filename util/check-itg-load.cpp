#include <cstdio>
#include <cassert>
#include "USB-classes.h"

/* Exit codes:
 * 0 - no button press, no error
 * 1 - button press, no error
 * 2 - board could not be found
 */

//typedef unsigned int uint32_t;
//typedef unsigned short uint16_t;

enum BoardType
{
	BOARD_ITGIO,
	BOARD_PIUIO,
	BOARD_NONE
};

// convenient helper function
template <class T>
void safe_delete( T *pointer )
{
	delete pointer;
	pointer = NULL;
}

template<class T>
bool bit_set( T iData, uint8_t iBit )
{
	int iLength = sizeof(T) * 8;
	assert( iLength >= iBit );

	printf( "iLength: %i, iBit: %i, shift: %i\n", iLength, iBit, iLength - iBit );

	// in some cases, this must be cast (if 1's assumed type is smaller than iData)
	return iData & ((T)1 << (iLength-iBit));
}

int main()
{
	// load the appropriate driver
	USBDriver *pDriver;
	BoardType Board = BOARD_NONE;

	// Attempt to find a PIUIO device
	pDriver = new PIUIO;

	if( pDriver->Open() )
		Board = BOARD_PIUIO;
	else
		safe_delete<USBDriver>( pDriver );

	// Attempt to find an ITGIO device otherwise
	if( Board == BOARD_NONE ) 
	{
		pDriver = new ITGIO;

		if( pDriver->Open() )
			Board = BOARD_ITGIO;
		else
			safe_delete<USBDriver>( pDriver );
	}

	// couldn't find any boards 
	if( Board == BOARD_NONE )
		return 2;

	{
		uint32_t iData = 0;

		// initialization write
		if( Board == BOARD_PIUIO )
			pDriver->Write( 0xE7FCFFFC );
		else
			pDriver->Write( 0xFFFFFFFF );

		pDriver->Read( &iData );

		// we have our data, so we can clean up here
		pDriver->Close(); safe_delete<USBDriver>( pDriver );

		// both I/O boards open high - invert the data
		iData = ~iData;

		// check for the start button bits on each board
		if( Board == BOARD_ITGIO )
			if( bit_set<uint32_t>(iData, 5) || bit_set<uint32_t>(iData,12) )
				return 1;
		else if( Board == BOARD_PIUIO )
			if( bit_set<uint32_t>(iData, 12) || bit_set<uint32_t>(iData,28) )
				return 1;
	}

	return 0;
}
