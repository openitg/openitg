#include "global.h"
#include "LightsDriver_Dynamic_Unix.h"
#include "RageLog.h"

#include <dlfcn.h>

bool LightsDriver_Dynamic_Unix::Load()
{
	CHECKPOINT;
	LOG->Debug( "Loading LightsDriver_Dynamic_Unix (%s)", m_sLibraryPath.c_str() );

	CHECKPOINT;
	pLib = dlopen( m_sLibraryPath.c_str(), RTLD_LOCAL | RTLD_NOW );
	CHECKPOINT;

	if( pLib == NULL )
	{
	CHECKPOINT;
		const char *error = dlerror();
		CString sError = ssprintf( "Error loading \"%s\": %s", m_sLibraryPath.c_str(), (error != NULL) ? error : "(none)" );
		LOG->Warn( sError );
	CHECKPOINT;
		return false;
	}

	CHECKPOINT;
	Module_Load		= (LoadFn) 	dlsym( pLib, "Load" );
	Module_Unload		= (UnloadFn)	dlsym( pLib, "Unload" );
	Module_Update		= (UpdateFn)	dlsym( pLib, "Update" );

	Module_GetError		= (GetErrorFn)	dlsym( pLib, "GetError" );
	Module_GetModuleInfo	= (GetLightsModuleInfoFn) dlsym( pLib, "GetModuleInfo" );
	CHECKPOINT;

	if( !Module_Load || !Module_Unload || !Module_Update || !Module_GetModuleInfo )
	{
		#define PRINT_POS(blah) LOG->Debug( "%s: %p", #blah, blah )

		PRINT_POS( Module_Load );
		PRINT_POS( Module_Unload );
		PRINT_POS( Module_Update );
		PRINT_POS( Module_GetError );
		PRINT_POS( Module_GetModuleInfo );

	CHECKPOINT;
		LOG->Warn( "Necessary module functions are missing. Cannot load." );
	CHECKPOINT;
		dlclose( pLib );
		return false;
	}
	CHECKPOINT;

	/* once the low-level operations have been completed,
	 * start processing at the abstracted driver level. */
	return LightsDriver_Dynamic::LoadInternal();
}
