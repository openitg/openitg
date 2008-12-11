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

/*
 * (c) 2008 BoXoRRoXoRs
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
