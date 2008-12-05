/* Customize what bits for output are enabled, as set by the Outputs.ini. */

#ifndef LIGHTS_MAPPER_H
#define LIGHTS_MAPPER_H

#include "RageUtil.h"
#include "LightsManager.h"

// a list of mappings in (1 << x) format. it's up to the driver to define these.
// the cabinet lights and coin counter are always available: the game lights
// might not be. 
struct LightsMapping
{
	// set a default of eight outputs (one byte) unless otherwise noted
	LightsMapping()
	{
		ZERO(m_iCabinetLights);
		ZERO(m_iGameLights);
		ZERO(m_iCoinCounterOn);
		ZERO(m_iCoinCounterOff);
	}

	// XXX: coder is trusted to have the right amount of elements for these calls.
	void SetCabinetLights( uint32_t array[] )
	{
		for( int i = 0; i < NUM_CABINET_LIGHTS; i++ )
			m_iCabinetLights[i] = array[i];
	}

	void SetGameLights( uint32_t array1[], uint32_t array2[] )
	{
		for( int i = 0; i < MAX_GAME_BUTTONS; i++ )
		{
			m_iGameLights[GAME_CONTROLLER_1][i] = array1[i];
			m_iGameLights[GAME_CONTROLLER_2][i] = array2[i];
		}
	}

	uint32_t	m_iCabinetLights[NUM_CABINET_LIGHTS];
	uint32_t	m_iGameLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS];
	uint32_t	m_iCoinCounterOn, m_iCoinCounterOff;
};

namespace LightsMapper
{
	void LoadMappings( CString sDevice, LightsMapping &mapping );
	void WriteMappings( CString sDevice, LightsMapping &mapping );
};

#endif