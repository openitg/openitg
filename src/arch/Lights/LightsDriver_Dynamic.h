#ifndef LIGHTS_DRIVER_DYNAMIC
#define LIGHTS_DRIVER_DYNAMIC

#include "LightsDriver.h"

/* included because we need CLightsState and API data. */
#include "../../../Docs/LightsDriver-API/LightsDriver_ModuleDefs.h"

class LightsDriver_Dynamic: public LightsDriver
{
public:
	LightsDriver_Dynamic( const CString &sLibPath );
	~LightsDriver_Dynamic();

	void Set( const LightsState *ls );

	virtual bool Load() = 0;
	bool IsLoaded()	{ return m_bLoaded; }
protected:
	virtual bool LoadInternal();

	LoadFn		Module_Load;
	UnloadFn	Module_Unload;
	UpdateFn	Module_Update;
	GetModuleInfoFn	Module_GetModuleInfo;
	GetErrorFn	Module_GetError;

	CString m_sLibraryPath;
private:
	bool m_bLoaded;

	static void MakeCLightsState( const LightsState *ls );

	static LightsState m_LastState;
	static CLightsState m_CLightsState;
};

#endif // LIGHTS_DRIVER_DYNAMIC
