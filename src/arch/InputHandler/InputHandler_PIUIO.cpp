/* XXX: this could be cleaner. */

#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "DiagnosticsUtil.h"

#include "PrefsManager.h" // for m_bDebugUSBInput
#include "ScreenManager.h"
#include "LightsManager.h"
#include "InputMapper.h"

#include "arch/Lights/LightsDriver_External.h"
#include "InputHandler_PIUIO.h"

InputHandler_PIUIO::InputHandler_PIUIO()
{
	m_bFoundDevice = false;
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

// use the kernel hack code if the r16 module is seen
#ifdef LINUX
	if( IsAFile("/rootfs/stats/patch/modules/usbcore.ko") )
	{
		LOG->Debug( "Found usbcore.ko. Assuming r16 kernel hack." );
		InternalInputHandler = &InputHandler_PIUIO::HandleInputKernel;
	}
	else
#endif
	{
		LOG->Debug( "usbcore.ko not found - using normal handler." );
		InternalInputHandler = &InputHandler_PIUIO::HandleInputNormal;
	}

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

static CString GetSensorDescription( bool *bArray )
{
	CStringArray sensors;

	for( int i = 0; i < 4; i++ )
		if( bArray[i] ) sensors.push_back( SensorNames[i] );

	/* HACK: if all sensors are reporting, then don't return anything.
	 * On PIUIO, all buttons always return all sensors except pads. */
//	if( sensors.size() == 4 )
//		return "";

	return join(", ", sensors);
}

/* code to handle the r16 kernel hack */
void InputHandler_PIUIO::HandleInputKernel()
{
	ZERO( m_iBulkReadData );

	m_iLightData &= 0xFFFCFFFC;

	// write each light state at once - array members 0, 2, 4, and 6
	for (uint32_t i = 0; i < 4; i++)
		m_iBulkReadData[i*2] = m_iLightData | (i | (i << 16));

	/* WARNING: SCIENCE CONTENT!
	 * We write each output set in members 0, 2, 4, and 6 of a uint32_t array.
	 * The BulkReadWrite sends four asynchronous write/read requests that end
	 * up overwriting the data we write with the data that's read.
	 *
	 * I'm not sure why we need an 8-member array. Oh well. */

	Board.BulkReadWrite( m_iBulkReadData );

	// process the input we were given
	for (uint32_t i = 0; i < 4; i++)
	{
		/* PIUIO opens high - for more logical processing, invert it */
		m_iBulkReadData[i*2] = ~m_iBulkReadData[i*2];

		// add this set into the input field
		m_iInputField |= m_iBulkReadData[i*2];

		// figure out which sensors were enabled
		for( int j = 0; j < 32; j++ )
			if( m_iBulkReadData[i*2] & (1 << (32-j)) )
				m_bSensors[j][i] = true;
	}
}

/* this is the input-reading logic that we know works */
void InputHandler_PIUIO::HandleInputNormal()
{
	ZERO( m_iInputData );

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

		// add this set into the input field
		m_iInputField |= m_iInputData[i];

		/* Toggle sensor bits - Left, Right, Up, Down */
		for( int j = 0; j < 32; j++ )
			if( m_iInputData[i] & (1 << (32-j)) )
				m_bSensors[j][i] = true;
	}

}

void InputHandler_PIUIO::HandleInput()
{
	m_InputTimer.Touch();

	// reset our reading data
	ZERO( m_bSensors );
	ZERO( m_iInputField );

	// sets up m_iInputField for usage
	(this->*InternalInputHandler)();

	/* If they asked for it... */
	if( PREFSMAN->m_bDebugUSBInput && SCREENMAN )
		SCREENMAN->SystemMessageNoAnimate( InputToBinary(m_iInputField) );

	// construct outside the loop, to save some processor time
	DeviceInput di(DEVICE_JOY1, JOY_1);

	for( int iButton = 0; iButton < 32; iButton++ )
	{
		di.button = JOY_1+iButton;

		/* If we're in a thread, our timestamp is accurate */
		if( InputThread.IsCreated() )
			di.ts.Touch();

		/* Set a description of detected sensors to the arrows */
		INPUTFILTER->SetButtonComment( di, GetSensorDescription(m_bSensors[iButton]) );

		/* Is the button we're looking for flagged in the input data? */
		ButtonPressed( di, (m_iInputField & (1 << (31-iButton))) );
	}

	RunTimingCode();
}

/* Requires "LightsDriver=ext" */
void InputHandler_PIUIO::UpdateLights()
{
	// set a const pointer to the "ext" LightsState to read from
	static const LightsState *m_LightsState = LightsDriver_External::Get();

	static const uint32_t iCabinetLights[NUM_CABINET_LIGHTS] = 
	{
		/* UL, UR, LL, LR marquee lights */
		(1 << 23), (1 << 26), (1 << 25), (1 << 24),

		/* selection buttons (not used), bass lights */
		0, 0, (1 << 10), (1 << 10)
	};

	static const uint32_t iPadLights[2][4] = 
	{
		/* Left, Right, Up, Down */
		{ (1 << 20), (1 << 21), (1 << 18), (1 << 19) },	/* Player 1 */
		{ (1 << 4), (1 << 5), (1 << 2), (1 << 3) }	/* Player 2 */
	};

	// iCoinPulseOn is sent whenever a coin is recorded.
	// iCoinPulseOff is sent whenever a coin is not being recorded.
	static const uint32_t iCoinPulseOn = (1 << 28);
	static const uint32_t iCoinPulseOff = (1 << 27);

	// reset
	m_iLightData = 0;

	// update marquee lights
	FOREACH_CabinetLight( cl )
		if( m_LightsState->m_bCabinetLights[cl] )
			m_iLightData |= iCabinetLights[cl];

	// update the four pad lights on both game controllers
	FOREACH_GameController( gc )
		FOREACH_ENUM( GameButton, 4, gb )
			if( m_LightsState->m_bGameButtonLights[gc][gb] )
				m_iLightData |= iPadLights[gc][gb];

	/* The coin counter moves halfway if we send bit 4, then the
	 * rest of the way (or not at all) if we send bit 5. Send bit
	 * 5 unless we have a coin event being recorded. */
	m_iLightData |= m_LightsState->m_bCoinCounter ? iCoinPulseOn : iCoinPulseOff;
}

// temporary debug function
void InputHandler_PIUIO::RunTimingCode()
{
	if( !PREFSMAN->m_bDebugUSBInput )
		return;

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
