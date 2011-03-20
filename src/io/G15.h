#ifndef IOG15_H
#define IOG15_H

#include "io/USBDriver.h"

enum G15DisplayState
{
	G15_LIGHTS_MODE,
	G15_STATS_MODE,
	NUM_G15_DISPLAY_STATES
};

class G15: public USBDriver
{
public:
	bool Open();

	bool Write( const uint32_t iData, G15DisplayState throwaway );

protected:
	bool Matches( int idVendor, int idProduct ) const;

private:
	bool WriteLCD( unsigned char *data );
};

#endif /* IOG15_H */
