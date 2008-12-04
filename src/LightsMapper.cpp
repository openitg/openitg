#include "global.h"
#include "RageLog.h"
#include "GameState.h"
#include "LightsManager.h"
#include "Game.h"
#include "GameInput.h"
#include "IniFile.h"
#include "PlayerNumber.h"
#include "LightsMapper.h"

#define LIGHTS_INI_PATH CString("Data/Outputs.ini")

/* It is important to note that ToMapping writes to the
 * input; FromMapping does not. This is because we never
 * have a reason to convert a LightsMapping value to a
 * proper number from a bit mapping. */
void ToMapping( uint32_t &iByte )
{
	// invalid sentinel
	if( iByte == 0 || iByte > 32 )
		iByte = 0;

	iByte = 1 << (32-iByte);
}

uint32_t FromMapping( uint32_t iMap )
{
	// XXX: is there a better way to do this?
	for( int i = 0; i < 32; i++ )
		if( iMap >> i == 1 )
			return i;

	return 0;	// set invalid
}

void LightsMapper::LoadMappings( CString sDevice, LightsMapping &mapping )
{
	IniFile ini;

	if( !ini.ReadFile( LIGHTS_INI_PATH ) )
	{
		LOG->Warn( "LoadMappings(): Could not open lights mappings file \"%s\"", LIGHTS_INI_PATH.c_str() );
		return;
	}

	// "PIUIO-dance", "PIUIO-pump", etc.
	const Game* pGame = GAMESTATE->GetCurrentGame();
	CString sKey = sDevice + pGame->m_szName;

	// check for a pre-existing key set
	if( ini.GetChild( sKey ) == NULL )
	{
		LOG->Warn( "Mappings not set for device \"%s\", game \"%s\". Writing default mappings.",
			sDevice.c_str(), pGame->m_szName );

		LightsMapper::WriteMappings( sDevice, mapping );
		return;
	}

	// load cabinet-light data and convert the data to a mapping
	FOREACH_CabinetLight( cl )
	{
		ini.GetValue( sKey, CabinetLightToString(cl), mapping.m_iCabinetLights[cl] );
		ToMapping( mapping.m_iCabinetLights[cl] );
	}

	FOREACH_GameController( gc )
	{
		GameInput GameI( gc, GAME_BUTTON_INVALID );

		FOREACH_GameButton( gb )
		{
			GameI.button = gb;
			CString sPlayerButton = PlayerNumberToString((PlayerNumber)gc) + GameButtonToString(pGame, gb);
			ini.GetValue( sKey, sPlayerButton, mapping.m_iGameLights[gc][gb] );
		}
	}

	ini.GetValue( sKey, "CoinCounterOn", mapping.m_iCoinCounterOn );
	ini.GetValue( sKey, "CoinCounterOff", mapping.m_iCoinCounterOff );
}

void LightsMapper::WriteMappings( CString sDevice, LightsMapping &mapping )
{
	IniFile ini;

	if( !ini.ReadFile( LIGHTS_INI_PATH ) )
	{
		LOG->Warn( "LoadMappings(): Could not open lights mappings file \"%s\"", LIGHTS_INI_PATH.c_str() );
		return;
	}

	// "PIUIO-dance", "PIUIO-pump", etc.
	const Game* pGame = GAMESTATE->GetCurrentGame();
	CString sKey = sDevice + pGame->m_szName;

	if( !ini.ReadFile( LIGHTS_INI_PATH ) )
	{
		LOG->Warn( "WriteMappings(): Could not open lights mappings file \"%s\"", LIGHTS_INI_PATH.c_str() );
		return;
	}

	FOREACH_CabinetLight( cl )
		ini.SetValue( sKey, CabinetLightToString(cl), FromMapping(mapping.m_iCabinetLights[cl]) );

	FOREACH_GameController( gc )
	{
		GameInput GameI( gc, GAME_BUTTON_INVALID );

		FOREACH_GameButton( gb )
		{
			GameI.button = gb;
			CString sPlayerButton = PlayerNumberToString((PlayerNumber)gc) + GameButtonToString(pGame, gb);
			ini.SetValue( sKey, sPlayerButton, FromMapping( mapping.m_iGameLights[gc][gb]) );
		}
	}

	ini.SetValue( sKey, "CoinCounter", FromMapping(mapping.m_iCoinCounterOn) );
	ini.SetValue( sKey, "CoinCounterOff", FromMapping(mapping.m_iCoinCounterOff) );

	ini.WriteFile( LIGHTS_INI_PATH );
}