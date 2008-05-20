/* XXX: this could be cleaner. */

#include "global.h"
#include "RageLog.h"
#include "RageInput.h" // for g_sInputType
#include "RageUtil.h"

#include "PrefsManager.h" // for m_bDebugUSBInput
#include "ScreenManager.h"
#include "LightsManager.h"

#include "Preference.h"

#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_PIUIO.h"

extern LightsState g_LightsState; // from LightsDriver_External
extern CString g_sInputType; // from RageInput

const unsigned NUM_SENSORS = 12;

Preference<bool>	g_bIOCoinTest( "IOCoinTest", false );

InputHandler_PIUIO::InputHandler_PIUIO()
{
	m_bShutdown = false;

	/* Mark the input type, for theme purposes */
	g_sInputType = "PIUIO";

	// device found and set
	if( !IOBoard.Open() )
	{
		LOG->Warn( "OpenITG could not establish a connection with PIUIO." );
		return;
	}

	LOG->Trace( "Opened PIUIO board." );
	m_bFoundDevice = true;

	/* warn if "ext" isn't enabled */
	if( PREFSMAN->GetLightsDriver().Find("ext") == -1 )
		LOG->Warn( "\"ext\" is not an enabled LightsDriver. The I/O board cannot run lights." );

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

	IOBoard.Close();
}

void InputHandler_PIUIO::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_PIUIO) );
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

static CString ShortArrayToBin( uint32_t arr[8] )
{
	CString result;
	uint32_t one = 1;
	for (int i = 0; i < 8; i++)
	{
		for (int j = 31; j >= 0; j--)
		{
			if (arr[i] & (one << j))
				result += "1";
			else
				result += "0";
		}
		result += "\n";
	}
	return result;
}

// P1: right left bottom top, P2: right left bottom top
int sensor_bits[NUM_SENSORS];

static CString SensorDescriptions[] = { "right", "left", "bottom", "top" };

static CString GetSensorDescription( int iBits )
{
	if ( iBits == 0 )
		return "";

	CStringArray retSensors;
	for( int i = 0; i < 4; i++ )
		if ( iBits & (1 << i) )
			retSensors.push_back(SensorDescriptions[i]);

	return join(", ", retSensors);
}
// XXX fixed 4/7/08.  Game.  Set.  Match.  --infamouspat
// ITT history :D  -- vyhd
void InputHandler_PIUIO::HandleInput()
{
	m_InputTimer.Touch();
	uint32_t i = 1; // convenience hack

	/* Easy to access if needed --Vyhd */
	static const uint32_t iInputBits[NUM_IO_BUTTONS] =
	{
		/* Player 1 */
		//Left, right, up, down arrows
		(i << 2), (i << 3), (i << 0), (i << 1),

		// Select, Start, MenuLeft, MenuRight
		(i << 5), (i << 4), (i << 6), (i << 7),

		/* Player 2 */
		// Left, right, up, down arrows
		(i << 18), (i << 19), (i << 16), (i << 17),

		// Select, Start, MenuLeft, MenuRight
		(i << 21), (i << 20), (i << 22), (i << 23),

		/* General input */
		// Service button, Coin event
		(i << 9) | (i << 14), (i << 10)
	};

	uint32_t iNewLightData = 0;

	bool bInputIsNonZero = false, bInputChanged = false;

	// reset
	for (unsigned j = 0; j < NUM_SENSORS; j++)
		ZERO( sensor_bits );
	for (unsigned j = 0; j < 8; j++)
		ZERO( m_iInputData );

	/* Explanation of this logic:
	 * ==========================
	 * Output bits 15-16 and 31-32 tell PIUIO which
	 * sensor set to report from when we read input.
	 *
	 * We AND out these bits, so they're guaranteed 0, then write
	 * the number we want for this read (re-writing the lights state).
	 *
	 * After this is written, we read the input into our input
	 * data array, add the sensor bits read into an array used to
	 * show which sensors are being pressed, and combine the data.
	 *
	 * The resultant iInputBitField, combined from four reads, is 
	 * finally used to report input when compared against the maps.
	 */

	for (uint32_t j = 0; j < 4; j++)
	{
		iNewLightData = m_iLightData & 0xfffcfffc;
		iNewLightData |= (j | (j << 16));

		m_iInputData[j*2] = iNewLightData;
	}

		//IOBoard.Write( iNewLightData );
		/* Get the data and send it to RageInput */
		//IOBoard.Read( &(m_iInputData[j]) );

		IOBoard.BulkReadWrite( m_iInputData );

	for (uint32_t j = 0; j < 4; j++)
	{
		/* PIUIO opens high - for more logical processing, invert it */
		m_iInputData[j] = ~m_iInputData[j*2];

		/* Toggle sensor bits - Left, Right, Up, Down */
		// P1
		/* Don't read past the P2 arrows */
		if (m_iInputData[j] & (i << 2)) sensor_bits[0] |= (i << j);
		if (m_iInputData[j] & (i << 3)) sensor_bits[1] |= (i << j);
		if (m_iInputData[j] & (i << 0)) sensor_bits[2] |= (i << j);
		if (m_iInputData[j] & (i << 1)) sensor_bits[3] |= (i << j);

		// P2
		if (m_iInputData[j] & (i << 18)) sensor_bits[8] |= (i << j);
		if (m_iInputData[j] & (i << 19)) sensor_bits[9] |= (i << j);
		if (m_iInputData[j] & (i << 16)) sensor_bits[10] |= (i << j);
		if (m_iInputData[j] & (i << 17)) sensor_bits[11] |= (i << j);
	}

	/* Handle coin events now */
	// XXX: probably should have a way to refer to the I/O fields
	if( m_iInputData[0] & iInputBits[IO_INSERT_COIN] )
	{
		if( SCREENMAN )
			SCREENMAN->SystemMessageNoAnimate( "PIUIO detected coin insert." );

		m_bCoinEvent = true;
	}

	for (int j = 0; j < 4; j++)
	{
		if (m_iInputData[j] != 0) bInputIsNonZero = true;
		if (m_iInputData[j] != m_iLastInputData[j]) bInputChanged = true;
	}

	/* If they asked for it... */
	if( PREFSMAN->m_bDebugUSBInput)
	{
		if ( bInputIsNonZero && bInputChanged )
			LOG->Trace( "Input: %s", ShortArrayToBin(m_iInputData).c_str() );

		if( SCREENMAN )
			SCREENMAN->SystemMessageNoAnimate(ShortArrayToBin(m_iInputData));

		/*
		CString sInputs;
		
		for( unsigned x = 0; x < 64; x++ )
		{
			if( !(m_iInputData & (i << x)) )
				continue;

			if( sInputs == "" )
				sInputs	= ssprintf( "Inputs: (1 << %i)", x );
			else
				sInputs += ssprintf( ", (1 << %i)", x );
		}

		if( LOG )
			LOG->Info( sInputs );
		*/
	}

	uint32_t iInputBitField = 0;

	for (int j = 0; j < 4; j++)
		iInputBitField |= m_iInputData[j];

	InputDevice id = DEVICE_PIUIO;

	/* Actually handle the input now */
	for( int iButton = 0; iButton < NUM_IO_BUTTONS; iButton++ )
	{
		DeviceInput di(id, iButton);

		/* If we're in a thread, our timestamp is accurate */
		if( InputThread.IsCreated() )
			di.ts.Touch();

		/* Set a description of detected sensors to the arrows */
		if (iButton < 4 || (iButton > 7 && iButton < 12))
			INPUTFILTER->SetButtonComment(di, GetSensorDescription(sensor_bits[iButton]));

		/* Is the button we're looking for flagged in the input data? */
		ButtonPressed( di, iInputBitField & iInputBits[iButton] );

	}

	/* assign our last input */
	for (int j = 0; j < 4; j++)
		m_iLastInputData[j] = m_iInputData[j];

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

	int iBit = 0;
	if( g_bIOCoinTest )
		iBit = (1 << 10);
	else
		iBit = (1 << 28);

	static const uint32_t iCoinBit = iBit;

	// reset
	m_iLightData = 0;

	if( !g_bIOCoinTest )
	{
		// update marquee lights
		FOREACH_CabinetLight( cl )
			if( g_LightsState.m_bCabinetLights[cl] )
				m_iLightData |= iCabinetBits[cl];

		// update the four pad lights on both game controllers
		FOREACH_GameController( gc )
			FOREACH_ENUM( GameButton, 4, gb )
				if( g_LightsState.m_bGameButtonLights[gc][gb] )
					m_iLightData |= iPadBits[gc][gb];
	}

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
