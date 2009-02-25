// Libusb-based driver for controlling the LCD
// of a Logitech G15 gaming keyboard.

#include "global.h"
#include "RageLog.h"
#include "LightsMapper.h"
#include "io/G15.h"
#include "LightsDriver_G15.h"
#include "RageThreads.h"

RageMutex g_G15LightMutex("G15LightMutex");

LightsDriver_G15::LightsDriver_G15()
{
	m_bHasDevice = LCD.Open();

	if( m_bHasDevice == false )
	{
		LOG->Warn( "Could not establish a connection with G15." );
		return;
	}

	// load any alternate lights mappings
	SetLightsMappings();

	// clear all lights
	LCD.Write( 0, G15_LIGHTS_MODE );
	m_DisplayState = G15_LIGHTS_MODE;

	/* this thing lags like crazy, so threads are a must */
	m_bThreadStop = false;
	m_WriteThread.SetName("G15 LCD Thread");
	m_WriteThread.Create( ThreadStart, this );
}

int LightsDriver_G15::ThreadStart( void *pG15 )
{
	((LightsDriver_G15 *)pG15)->ThreadLoop();
	return 1;
}

void LightsDriver_G15::ThreadLoop()
{
	bool ret;
	while ( !m_bThreadStop && m_bHasDevice )
	{
		g_G15LightMutex.Lock();
		ret = LCD.Write( m_iSavedLightData, m_DisplayState );
		g_G15LightMutex.Unlock();
		usleep(50000);
	
		if( !ret )
		{
			LOG->Warn( "Lost connection with G15 LCD." );
			m_bHasDevice = false;
		}
	}
}

void LightsDriver_G15::SetLightsMappings()
{
	uint32_t iCabinetLights[NUM_CABINET_LIGHTS] =
	{
		// up-left, up-right, down-left, down-right marquees
		(1 << 0), (1 << 1), (1 << 2), (1 << 3),

		// left buttons, right buttons, left bass, right bass
		(1 << 4), (1 << 5), (1 << 6), (1 << 7)
	};

	uint32_t iGameLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS] =
	{
		// left, right, up, down
		{ ( 1 << 8), (1 << 9), (1 << 10), (1 << 11) },		// player 1
		{ ( 1 << 12), (1 << 13), (1 << 14), (1 << 15) },	// player 2
	};

	m_LightsMappings.SetCabinetLights( iCabinetLights );
	m_LightsMappings.SetGameLights( iGameLights[GAME_CONTROLLER_1],
		iGameLights[GAME_CONTROLLER_2] );

	LightsMapper::LoadMappings( "G15", m_LightsMappings );
}

LightsDriver_G15::~LightsDriver_G15()
{
	m_bThreadStop = true;

	if( m_WriteThread.IsCreated() )
	{
		LOG->Trace( "Shutting down G15 thread..." );
		m_WriteThread.Wait();
		LOG->Trace( "G15 thread shut down." );
	}

	if( !m_bHasDevice )
		return;

	// clear all lights and close the connection
	LCD.Write( 0, G15_LIGHTS_MODE );
	LCD.Close();
}

void LightsDriver_G15::Set( const LightsState *ls )
{
	if( !m_bHasDevice )
	{
		m_bThreadStop = true;
		return;
	}

	uint32_t iWriteData = 0;

	// Lights 1 - 8 are used for the cabinet lights
	FOREACH_CabinetLight( cl )
		if( ls->m_bCabinetLights[cl] )
			iWriteData |= m_LightsMappings.m_iCabinetLights[cl];

	// Lights 9-12 for P1 pad, 13-16 for P2 pad
	// FIXME: make this work for all game-types?
	FOREACH_GameController( gc )
		FOREACH_GameButton( gb )
			if( ls->m_bGameButtonLights[gc][gb] )
				iWriteData |= m_LightsMappings.m_iGameLights[gc][gb];

	g_G15LightMutex.Lock();
	m_iSavedLightData = iWriteData;
	g_G15LightMutex.Unlock();
}

/*
 * Copyright (c) 2008 BoXoRRoXoRs
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
