#ifndef LIGHTS_DRIVER_DYNAMIC_UNIX_H
#define LIGHTS_DRIVER_DYNAMIC_UNIX_H

#include "LightsDriver_Dynamic.h"

class LightsDriver_Dynamic_Unix: public LightsDriver_Dynamic
{
public:
	LightsDriver_Dynamic_Unix( const CString &sLibPath ) : LightsDriver_Dynamic( sLibPath ) { }
	bool Load();

private:
	void *pLib;
};

#endif // LIGHTS_DRIVER_DYNAMIC_UNIX_H
