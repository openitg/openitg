#ifndef IO_MINIMAID_H
#define IO_MINIMAID_H

#include <stdint.h>
#include "USBDriver.h"

class MiniMaid: public USBDriver
{
public:
	static bool DeviceMatches( int iVendorID, int iProductID );
	bool Open();
	
	bool Read( uint64_t *pData );
	bool Write( uint64_t data );
	
	bool m_bInitialized;
};

#endif /* IO_MINIMAID_H */
