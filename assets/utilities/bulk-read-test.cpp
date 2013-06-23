#include <cstdio>
#include <cstdlib>
#include <climits>
#include <usb.h>

#include "USB-classes.h"

/* Exit codes for this program:
 * 1: board connection error
 * 2: invalid arguments
 */

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

const int VERSION = 1;

/* the length of the data arrays */
#define MAX_LEN 8

int main( int argc, char *argv[] )
{
	printf( "bulk-read-test, version %i, starting.\n\n", VERSION );

	if( argc != 2 )
	{
		printf( "Usage: ./bulk-read-test [value]\n" );
		printf( "Value is read as 32-bit, unsigned hexadecimal.\n" );
		exit( 2 );
	}

	uint32_t iWriteData[MAX_LEN];
	uint32_t iLastData[MAX_LEN];

	// assign the read value and clear the sensor bits
	uint32_t iLightData = strtoul( argv[1], NULL, 16 );
	iLightData &= 0xE7FCFFFC;

	/* assign the given values in hex format */
	for( int i = 0; i < 4; i++ )
	{
		iWriteData[i*2+0] = iLightData;
		iWriteData[i*2+0] |= (i | (i << 16));

		// clear the input field, so to speak...
		iWriteData[i*2+1] = 0xFFFFFFFF;

		// debug line
		printf( "Assigned %08x to iWriteData[%i]\n", iWriteData[i*2+0], i*2+0 );
	}

	/* since we write directly to iWriteData, save a backup. */
	for( int i = 0; i < MAX_LEN; i++ )
		iLastData[i] = iWriteData[i];

	for( int i = 0; i < MAX_LEN; i++ )
		printf( "Data %i: Input: %08x, output: %08x (inverted: %08x)\n", i+1, iLastData[i], iWriteData[i], ~iWriteData[i] );

	PIUIO Board;

	/* not as succinct as !, but more readable */
	if( Board.Open() == false )
	{
		printf( "I/O board could not be found or assigned.\n" );
		exit( 1 );
	}

	printf( "Opened and claimed I/O board.\n" );

	Board.BulkReadWrite( iWriteData );
	Board.Close();

	for( int i = 0; i < MAX_LEN; i++ )
		printf( "Data %i: Input: %08x, output: %08x\n", i+1, iLastData[i], iWriteData[i] );

	printf( "Program exited successfully.\n" );
	return 0;
}
