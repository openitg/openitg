#include "global.h"
#include "io/G15.h"

/* right now, only supports G15 V2 keyboards */
bool G15::Matches( int idVendor, int idProduct ) const
{
	if ( idVendor != 0x046d ) return false;
	if ( idProduct != 0xc227 ) return false;
	return true;
}

bool G15::WriteLCD( unsigned char *pData )
{
	int iReturn = usb_interrupt_write( m_pHandle, 0x02, (char*)pData, 992, 1000 );
	if (iReturn == 992) return true;

	printf( "G15 LCD Write failed, returned %i: %s\n", iReturn, usb_strerror() );

	return false;
}

bool G15::Write( uint32_t iData )
{
	unsigned char data[32+(160*7)];
	memset(data, 0, 992);

	// magic byte
	data[0] = 0x03;

	// for now, just shows bars on the LCD to indicate lights
	for (unsigned i = 0; i < 32; i++) {
		if ( iData & (1 << i) ) {
			// 32 is the data offset (a.k.a. LCD data has a 32 byte header)
			for (unsigned j = 0; j < 5; j++)
			{
				data[32+(5*i)+(160*j)] = 0xff;
				data[32+(5*i)+1+(160*j)] = 0xff;
				data[32+(5*i)+2+(160*j)] = 0xff;
				data[32+(5*i)+3+(160*j)] = 0xff;
				data[32+(5*i)+4+(160*j)] = 0xff;
			}
		}
	}
	return WriteLCD(data);
}

