/* XXX: this could be cleaner. */

#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "DiagnosticsUtil.h"

#include "PrefsManager.h" // for m_bDebugUSBInput
#include "ScreenManager.h"
#include "LightsManager.h"
#include "InputMapper.h"

#include "GameInput.h"
#include "Preference.h"

#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_PIUIO.h"

extern LightsState g_LightsState; // from LightsDriver_External

static bool g_bUnstableAvailable; // determine which handling code to use

InputHandler_PIUIO::InputHandler_PIUIO()
{
	m_bShutdown = false;
	DiagnosticsUtil::SetInputType( "PIUIO" );

	// device found and set
	if( !Board.Open() )
	{
		LOG->Warn( "OpenITG could not establish a connection with PIUIO." );
		return;
	}

	LOG->Trace( "Opened PIUIO board." );
	m_bFoundDevice = true;

	/* warn if "ext" isn't enabled */
	if( PREFSMAN->GetLightsDriver().Find("ext") == -1 )
		LOG->Warn( "\"ext\" is not an enabled LightsDriver. The I/O board cannot run lights." );

// HELL YES PAT FINALLY MADE A DECENT COMMITABLE CHANGE AFTER MONTHS OF NOT DOING SO :D
#ifdef LINUX
	// use the kernel hack code if the r16 module is loaded
	if( GetHashForFile("/rootfs/stats/patch/modules/usbcore.ko") == 2101124880 )
		this->InternalInputHandler = &InputHandler_PIUIO::HandleInputKernel;
	else
#endif
		InternalInputHandler = &InputHandler_PIUIO::HandleInputNormal;

	// figure out which inputs we should report sensors on - pads only
	// XXX: make this refresh whenever mappings have changed!
	for( int i = 0; i < 32; i++ )
		m_bReportSensor[i] = true;

	InputThread.SetName( "PIUIO thread" );
	InputThread.Create( InputThread_Start, this );
}

InputHandler_PIUIO::~InputHandler_PIUIO()
{
	if( InputThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down PIUIO thread..." );
		InputThread.Wait();
		LOG->Trace( "PIUIO thread shut down." );
	}

	// reset all lights and unclaim
	if( m_bFoundDevice )
	{
		Board.Write( 0 );
		Board.Close();
	}
}

void InputHandler_PIUIO::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_JOY1) );
		vDescriptionsOut.push_back( "PIUIO" );
	}
}

int InputHandler_PIUIO::InputThread_Start( void *p )
{
	((InputHandler_PIUIO *) p)->InputThreadMain();
	return 0;
}

void InputHandler_PIUIO::InputThreadMain()
{
	while( !m_bShutdown )
	{
		/* Figure out the lights and write them */
		UpdateLights();

		/* Find our sensors, report to RageInput */
		HandleInput();
	}
}

static CString InputToBinary( uint32_t array )
{
	CString result;
	uint32_t one = 1; // convenience hack
	for (int i = 31; i >= 0; i--)
	{
		if (one << i)
			result += "1";
		else
			result += "0";
	}
	return result;
}

static CString SensorNames[] = { "right", "left", "bottom", "top" };

static CString GetSensorDescription( bool *bSensorArray )
{
	CStringArray retSensors;

	for( int i = 0; i < 4; i++ )
		if( bSensorArray[i] ) retSensors.push_back( SensorNames[i] );

	return join(", ", retSensors);
}

/* code to handle the r16 kernel hack */
void InputHandler_PIUIO::HandleInputKernel()
{
	m_iLightData &= 0xFFFCFFFC;

	// write each light state at once - array members 0, 2, 4, and 6
	for (uint32_t i = 0; i < 4; i++)
		m_iBulkReadData[i*2] = m_iLightData | (i | (i << 16));

	Board.BulkReadWrite( m_iBulkReadData );

	// process the input we were given
	for (uint32_t i = 0; i < 4; i++)
	{
		/* PIUIO opens high - for more logical processing, invert it */
		m_iBulkReadData[i*2+1] = ~m_iBulkReadData[i*2+1];

		// add this set into the input field
		m_iInputField |= m_iBulkReadData[i*2+1];

		// figure out which sensors were enabled
		for( int j = 0; j < 32; j++ )
			if( m_iBulkReadData[i*2+1] & (1 << 32-j) )
				m_bInputs[j][i] = true;
	}
}

/* this is the input-reading logic that we know works */
void InputHandler_PIUIO::HandleInputNormal()
{
	for (uint32_t i = 0; i < 4; i++)
	{
		// write which sensors to report from
		m_iLightData &= 0xFFFCFFFC;
		m_iLightData |= (i | (i << 16));

		// do one write/read cycle to get this set of sensors
		Board.Write( m_iLightData );
		Board.Read( &m_iInputData[i] );

		/* PIUIO opens high - for more logical processing, invert it */
		m_iInputData[i] = ~m_iInputData[i];

		/* Toggle sensor bits - Left, Right, Up, Down */
		for( int j = 0; j < 32; j++ )
			if( m_iInputData[i] & (1 << 32-j) )
				m_bInputs[j][i] = true;
	}

}

// XXX: phase out as soon as possible
const bool IsPadInput( int iButton )
{
	switch( iButton+1 )
	{
	case 13: case 14: case 15: case 16: /* Player 2 */
	case 29: case 30: case 31: case 32: /* Player 1 */
		return true;
		break;
	default:
		return false;
	}

	return false;
}

// XXX fixed 4/7/08.  Game.  Set.  Match.  --infamouspat
// ITT history :D  -- vyhd
void InputHandler_PIUIO::HandleInput()
{
	m_InputTimer.Touch();
	uint32_t i = 1; // convenience hack

	bool bInputIsNonZero = false, bInputChanged = false;

	// reset
	ZERO( m_iInputData );
	ZERO( m_bInputs );

	// sets up m_iInputField for usage
	this->InternalInputHandler;

	/* Flag coin events */
//	if( m_iInputData[0] & )
//		m_bCoinEvent = true;

	/* If they asked for it... */
	if( PREFSMAN->m_bDebugUSBInput && SCREENMAN )
			SCREENMAN->SystemMessageNoAnimate( InputToBinary(m_iInputField) );

	InputDevice id = DEVICE_JOY1;

	/* Actually handle the input now */
	for( int iButton = 0; iButton < 32; iButton++ )
	{
		DeviceInput di(id, JOY_1+iButton);

		/* If we're in a thread, our timestamp is accurate */
		if( InputThread.IsCreated() )
			di.ts.Touch();

		/* Set a description of detected sensors to the arrows */
		// XXX: set this to pad-only ( StyleI.IsValid()? )
		if( IsPadInput(iButton) )
			INPUTFILTER->SetButtonComment(di, GetSensorDescription( m_bInputs[iButton] ));

		/* Is the button we're looking for flagged in the input data? */
		ButtonPressed( di, m_iInputField & (1 << (31-iButton)) );
	}

	/* from here on, it's all debug/timing stuff */
	if( !PREFSMAN->m_bDebugUSBInput )
		return;

	/*
	 * Begin debug code!
	 */

	float fReadTime = m_InputTimer.GetDeltaTime();

	/* loading latency or something similar - discard */
	if( fReadTime > 0.1f )
		return;

	m_fTotalReadTime += fReadTime;
	m_iReadCount++;

	/* we take the average every 1,000 reads */
	if( m_iReadCount < 1000 )
		return;

	/* even if the count is off for some value,
	 * this will still work as expected. */
	float fAverage = m_fTotalReadTime / (float)m_iReadCount;

	LOG->Info( "PIUIO read average: %f seconds in %i reads. (approx. %i reads per second)", fAverage, m_iReadCount, (int)(1.0f/fAverage) );

	/* reset */
	m_iReadCount = 0;
	m_fTotalReadTime = 0;
}

/* Requires "LightsDriver=ext" */
void InputHandler_PIUIO::UpdateLights()
{
	static const uint32_t iCabinetBits[NUM_CABINET_LIGHTS] = 
	{
		/* UL, UR, LL, LR marquee lights */
		(1 << 23), (1 << 26), (1 << 25), (1 << 24),

		/* selection buttons (not used), bass lights */
		0, 0, (1 << 10), (1 << 10)
	};

	static const uint32_t iPadBits[2][4] = 
	{
		/* Left, Right, Up, Down */
		{ (1 << 20), (1 << 21), (1 << 18), (1 << 19) },	/* Player 1 */
		{ (1 << 4), (1 << 5), (1 << 2), (1 << 3) }	/* Player 2 */
	};

	// XXX: this should work, but doesn't. Why not?
	static const uint32_t iCoinBit = (1 << 28);

	// reset
	m_iLightData = 0;

	// update marquee lights
	FOREACH_CabinetLight( cl )
		if( g_LightsState.m_bCabinetLights[cl] )
			m_iLightData |= iCabinetBits[cl];

	// update the four pad lights on both game controllers
	FOREACH_GameController( gc )
		FOREACH_ENUM( GameButton, 4, gb )
			if( g_LightsState.m_bGameButtonLights[gc][gb] )
				m_iLightData |= iPadBits[gc][gb];

	// pulse the coin counter if we have an event
	if( m_bCoinEvent )
		m_iLightData |= iCoinBit;
}

/*
 * (c) 2005 Chris Danford, Glenn Maynard.  Re-implemented by vyhd, infamouspat
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
