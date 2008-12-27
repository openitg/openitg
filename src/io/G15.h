#ifndef IOG15_H
#define IOG15_H

#include "io/USBDriver.h"

class G15: public USBDriver
{
public:
	bool Read( uint32_t *pData ) { return false; }
	bool Write( uint32_t iData );

protected:
	bool Matches( int idVendor, int idProduct ) const;

private:
	bool WriteLCD( unsigned char *data );
};

#endif /* IOG15_H */
