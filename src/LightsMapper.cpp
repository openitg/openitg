/* Please note:
 * Cabinet mappings (lights + counter) are global ([PIUIO]).
 * Game buttons are divided by game and player ([PIUIO-dance-P1]).
 * It is possible to overlap buttons, and it is
 * possible to specify numbers that are never used.
 * (I may implement something to limit this later.)
 */

#include "global.h"
#include "RageLog.h"
#include "GameState.h"
#include "LightsManager.h"
#include "GameInput.h"
#include "Game.h"
#include "IniFile.h"
#include "PlayerNumber.h"
#include "LightsMapper.h"

#define LIGHTS_INI_PATH CString("Data/LightsMaps.ini")

/* It is important to note that ToMapping writes to the
 * input; FromMapping does not. This is because we never
 * have a reason to convert a LightsMapping value to a
 * proper number from a bit mapping. */
void ToMapping( CString &sBits, uint32_t &iByte )
{
	// reset
	iByte = 0;
	CStringArray sArray;
	split( sBits, ",", sArray );

	if( sArray.empty() )
		return;

	for( unsigned char i = 0; i < sArray.size(); i++ )
	{
		unsigned shift = atoi( sArray[i].c_str() );
		LOG->Debug( "atoi(%s) = %u", sArray[i].c_str(), shift );

		if( shift > 0 || shift <= 32 )
			iByte |= (1 << (32-shift));
		else
			LOG->Debug( "ToMapping(): invalid value \"%u\" (1 << %u)", shift, (1 << (31-shift)) );
	}

	LOG->Debug( "ToMapping(): translated %s to %u", sBits.c_str(), iByte );
}

CString FromMapping( uint32_t iMap )
{
	CStringArray sArray;

	for( int i = 0; i < 32; i++ )
		if( iMap & (1 << (31-i)) )
		{
			// offset: our test starts at 0, ini values start at 1
			sArray.push_back( ssprintf("%u", i+1) );
			LOG->Warn( "FromMapping(): match at i=%u (%u)", i, i+1 );
		}

	LOG->Debug( "FromMapping(): translated %u to \"%s\"", iMap, join(",",sArray).c_str() );

	return join( ",", sArray );
}

void LightsMapper::LoadMappings( CString sDevice, LightsMapping &mapping )
{
	IniFile ini;

	const Game* pGame = GAMESTATE->GetCurrentGame();

	// check for a pre-existing key set
	if( !ini.ReadFile( LIGHTS_INI_PATH ) || ini.GetChild( sDevice ) == NULL )
	{
		LOG->Warn( "Mappings not set for device \"%s\", game \"%s\". Writing default mappings.",
			sDevice.c_str(), pGame->m_szName );

		LightsMapper::WriteMappings( sDevice, mapping );
		return;
	}

	// load the INI values into this, then convert them as needed.
	CString sBuffer;

	// load cabinet-light data and convert the string to a uint32_t
	FOREACH_CabinetLight( cl )
	{
		ini.GetValue( sDevice, CabinetLightToString(cl), sBuffer );
		ToMapping( sBuffer, mapping.m_iCabinetLights[cl] );
	}

	ini.GetValue( sDevice, "CoinCounterOn", sBuffer );
	ToMapping( sBuffer, mapping.m_iCoinCounterOn );
	ini.GetValue( sDevice, "CoinCounterOff", sBuffer );
	ToMapping( sBuffer, mapping.m_iCoinCounterOff );

	// "PIUIO-dance-", "PIUIO-pump-", etc.
	CString sBaseKey = sDevice + "-" + pGame->m_szName + "-";

	FOREACH_GameController( gc )
	{
		// [PIUIO-dance-P1], [PIUIO-dance-P2]
		CString sPlayerKey = sBaseKey + PlayerNumberToString((PlayerNumber)gc);

		// only read up to the last-set game button, which is the operator button
		FOREACH_ENUM( GameButton, pGame->ButtonNameToIndex("Operator"), gb )
		{
			ini.GetValue( sPlayerKey, GameButtonToString(pGame, gb), sBuffer );
			ToMapping( sBuffer, mapping.m_iGameLights[gc][gb] );
		}
	}

	// re-write, to update all the values
	LightsMapper::WriteMappings( sDevice, mapping );
}

void LightsMapper::WriteMappings( CString sDevice, LightsMapping &mapping )
{
	IniFile ini;
	ini.ReadFile( LIGHTS_INI_PATH );

	const Game* pGame = GAMESTATE->GetCurrentGame();

	// set cabinet and counter data
	ini.SetValue( sDevice, "CoinCounterOn", FromMapping(mapping.m_iCoinCounterOn) );
	ini.SetValue( sDevice, "CoinCounterOff", FromMapping(mapping.m_iCoinCounterOff) );

	FOREACH_CabinetLight( cl )
		ini.SetValue( sDevice, CabinetLightToString(cl), FromMapping(mapping.m_iCabinetLights[cl]) );

	// "PIUIO-dance-", "PIUIO-pump-", etc.
	CString sBaseKey = sDevice + "-" + pGame->m_szName + "-";

	FOREACH_GameController( gc )
	{
		// [PIUIO-dance-P1], [PIUIO-dance-P2]
		CString sPlayerKey = sBaseKey + PlayerNumberToString((PlayerNumber)gc);

		// only read up to the last-set game button, which is the operator button
		FOREACH_ENUM( GameButton, pGame->ButtonNameToIndex("Operator"), gb )
			ini.SetValue( sPlayerKey, GameButtonToString(pGame, gb),
				FromMapping(mapping.m_iGameLights[gc][gb]) );
	}

	ini.WriteFile( LIGHTS_INI_PATH );
}