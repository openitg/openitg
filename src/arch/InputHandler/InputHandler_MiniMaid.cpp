// Thanks to buzzert for starting this code
// Finished and tested by MooingLemur 2014-09-05

#include "global.h"

#include "RageLog.h"
#include "InputFilter.h"
#include "DiagnosticsUtil.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "arch/Lights/LightsDriver_External.h"

#include "InputHandler_MiniMaid.h"

#define DEVICE_NAME "MiniMaid"
#define DEVICE_INPUTDEVICE DEVICE_JOY1 // arbitrary
#define INPUT_BIT_OFFSET 24
#define INPUT_RANGE 23

// Lights stuff
static const uint64_t CABINET_LIGHTS = (1 * 8);
static const uint64_t PAD_LIGHTS     = (2 * 8);
static const uint64_t BASS_LIGHTS    = (4 * 8);

// This constant isn't used in code but it's being
// left in as comment, a potentially useful hint to
// future maintainers
/* static const uint64_t ALL_ON = 0x10F0FFC00ULL; */

REGISTER_INPUT_HANDLER( MiniMaid );

InputHandler_MiniMaid::InputHandler_MiniMaid() : 
	m_bShouldStop(false), m_iLastInputField(0)
{
	if( !m_pBoard.Open() )
	{
		LOG->Warn( "InputHandler_MiniMaid: Could not establish a connection with the MiniMaid device." );
		return;
	}

	SetLightsMappings();
	DiagnosticsUtil::SetInputType( DEVICE_NAME );
	
	InputThread.SetName( "MiniMaid I/O thread" );
	InputThread.Create( InputThread_Start, this );
}

InputHandler_MiniMaid::~InputHandler_MiniMaid()
{
	if ( InputThread.IsCreated() )
	{
		m_bShouldStop = true;
		InputThread.Wait();
	}
}

void InputHandler_MiniMaid::Reconnect()
{
	while ( !m_pBoard.Open() )
	{
		m_pBoard.Close();
		usleep( 1000 );
	}
}

void InputHandler_MiniMaid::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_pBoard.m_bInitialized )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_INPUTDEVICE) );
		vDescriptionsOut.push_back( DEVICE_NAME );
	}
}

void InputHandler_MiniMaid::HandleInput()
{
	m_iInputField = 0;
	
	// Read input from board
	while ( !m_pBoard.Read( &m_iInputField ) )
		Reconnect();
	
	// Generate input events bitfield (1 = change, 0 = no change)
	uint64_t iChanged = m_iInputField ^ m_iLastInputField;
	m_iLastInputField = m_iInputField;
	
	// Reuse these inside loop
	DeviceInput deviceInput( DEVICE_INPUTDEVICE, KEY_SPACE );
	RageTimer now;
	
	for ( unsigned i = INPUT_BIT_OFFSET; i <= (INPUT_BIT_OFFSET + INPUT_RANGE); i++ )
	{
		// Don't report anything if this bit didn't change
		if ( likely( !IsBitSet(iChanged, i) ) )
			continue;
		
		deviceInput.button = JOY_1 + (i - INPUT_BIT_OFFSET);
		deviceInput.ts = now;
		
		// Report this button's status
		ButtonPressed( deviceInput, IsBitSet( m_iInputField, i ) );
	}
}

void InputHandler_MiniMaid::SetLightsMappings()
{
	uint32_t iCabinetLights[NUM_CABINET_LIGHTS] = {
		(1 << 7), // LIGHT_MARQUEE_UP_LEFT
		(1 << 5), // LIGHT_MARQUEE_UP_RIGHT
		(1 << 6), // LIGHT_MARQUEE_LR_LEFT
		(1 << 4), // LIGHT_MARQUEE_LR_RIGHT
		(1 << 2), // LIGHT_BUTTONS_LEFT
		(1 << 3), // LIGHT_BUTTONS_RIGHT
		
		// These actually overflow our 32-bit integer, so we'll need to add
		// a special case for them in Set().
		(0x01), // LIGHT_BASS_LEFT
		(0x01), // LIGHT_BASS_RIGHT
	};
	
	uint32_t iGameLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS] = {
		// { L, R, U, D }
		{ (1 << 2) , (1 << 3) , (1 << 0), (1 << 1) }, // Player 1
		{ (1 << 10), (1 << 11), (1 << 8), (1 << 9) }  // Player 2
	};
	
	m_LightsMappings.SetCabinetLights(iCabinetLights);
	m_LightsMappings.SetCustomGameLights(iGameLights);
}

void InputHandler_MiniMaid::UpdateLights()
{
	static const LightsState *ls = LightsDriver_External::Get();

	m_iLightsField = 0x0;
	
	// Cabinet Lights
	FOREACH_CabinetLight(cl) {
		if (true == ls->m_bCabinetLights[cl]) {
			if (cl == LIGHT_BASS_LEFT || cl == LIGHT_BASS_RIGHT) {
				m_iLightsField |= ((uint64_t)0x01 << BASS_LIGHTS);
			}
			else
			{
				m_iLightsField |= ((uint64_t)m_LightsMappings.m_iCabinetLights[cl] << CABINET_LIGHTS);
			}
		}
	}
	
	// Pad Lights
        FOREACH_GameController( gc )
		FOREACH_GameButton( gb )
			if( ls->m_bGameButtonLights[gc][gb] )
				m_iLightsField |= (uint64_t)m_LightsMappings.m_iGameLights[gc][gb] << PAD_LIGHTS;
	
	while ( !m_pBoard.Write( m_iLightsField ) )
		Reconnect();
}

int InputHandler_MiniMaid::InputThreadMain()
{
	while( !m_bShouldStop )
	{
		HandleInput();
		UpdateLights();
	}
	
	// Turn off all lights when loop exits
	m_pBoard.Write( 0x0 );

	return 0;
}
