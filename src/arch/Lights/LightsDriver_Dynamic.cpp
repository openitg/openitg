#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "LightsDriver_Dynamic.h"

CLightsState LightsDriver_Dynamic::m_CLightsState;

LightsDriver_Dynamic::LightsDriver_Dynamic( const CString &sLibPath )
{
	m_bLoaded = false;
	m_sLibraryPath = sLibPath;
}

bool LightsDriver_Dynamic::LoadInternal()
{
	ASSERT( !m_sLibraryPath.empty() );

	const struct LightsModuleInfo *info = Module_GetModuleInfo();
	CHECKPOINT;

	if( info == NULL )
	{
		LOG->Warn( "Could not get LightsModuleInfo from LightsDriver \"%s\".", m_sLibraryPath.c_str() );
		return false;
	}

	CHECKPOINT;
	if( LIGHTS_API_VERSION_MAJOR != info->mi_api_ver_major ||
		LIGHTS_API_VERSION_MINOR != info->mi_api_ver_minor )
	{
		LOG->Warn( "LightsDriver \"%s\" uses API version %d.%d, binary uses %d.%d. Disabled.",
			info->mi_name, info->mi_api_ver_major, info->mi_api_ver_minor,
			LIGHTS_API_VERSION_MAJOR, LIGHTS_API_VERSION_MINOR );
		return false;
	}
	CHECKPOINT;

	LOG->Trace( "%s: attempting to load LightsDriver \"%s\".", __FUNCTION__, info->mi_name );
	CHECKPOINT;
	int iError = Module_Load();
	CHECKPOINT;

	CHECKPOINT;
	if( iError != 0 )
	{
	CHECKPOINT;
		CString sError = ssprintf( "Error initializing LightsDriver \"%s\": %s", info->mi_name,
			(Module_GetError != NULL) ? Module_GetError(iError) : "(no error message)" );
		LOG->Warn( sError );
	CHECKPOINT;
		Module_Unload();
	CHECKPOINT;
		return false;
	}

	CHECKPOINT;
	LOG->Debug( "Established connection with LightsDriver \"%s\".", info->mi_name );
	LOG->Info( "LightsDriver module: %s, author: %s, version: %d.%d",
		info->mi_name, info->mi_author, info->mi_api_ver_major, info->mi_api_ver_minor );

	m_bLoaded = true;
	CHECKPOINT;

	return true;
}

LightsDriver_Dynamic::~LightsDriver_Dynamic()
{
	/* bit of a hack. force all lights off. */
	if( m_bLoaded )
	{
		CLightsState blank;
		ZERO( blank );
		Module_Update( &blank );
		Module_Unload();
	}
}

void LightsDriver_Dynamic::Set( const LightsState *ls )
{
	if( !m_bLoaded )
		return;

	MakeCLightsState( ls );
	Module_Update( &m_CLightsState );
}

void LightsDriver_Dynamic::MakeCLightsState( const LightsState *ls )
{
	FOREACH_CabinetLight( cl )
		m_CLightsState.m_bCabinetLights[cl] = uint8_t(ls->m_bCabinetLights[cl]);

	FOREACH_GameController( gc )
		FOREACH_GameButton( gb )
			m_CLightsState.m_bGameButtonLights[gc][gb] = uint8_t(ls->m_bGameButtonLights[gc][gb]);
}
