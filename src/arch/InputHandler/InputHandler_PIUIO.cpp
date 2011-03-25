#include "global.h"
#include "RageLog.h"
#include "InputFilter.h"
#include "DiagnosticsUtil.h"
#include "LightsManager.h"
#include "arch/Lights/LightsDriver_External.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "InputHandler_PIUIO.h"
#include "InputHandler_PIUIO_Helper.h"

REGISTER_INPUT_HANDLER( PIUIO );

bool InputHandler_PIUIO::s_bInitialized = false;

// simple helper function to automatically reopen PIUIO if a USB error occurs
static void Reconnect( PIUIO &board )
{
	LOG->Warn( "PIUIO connection lost! Retrying..." );

	while( !board.Open() )
	{
		board.Close();
		usleep( 100000 );
	}

	LOG->Warn( "PIUIO reconnected." );
}

InputHandler_PIUIO::InputHandler_PIUIO()
{
	m_bFoundDevice = false;

	/* if a handler has already been created (e.g. by ScreenArcadeStart)
	 * and it has claimed the board, don't try to claim it again. */
	if( s_bInitialized )
	{
		LOG->Warn( "InputHandler_PIUIO: Redundant driver loaded. Disabling..." );
		return;
	}

	// attempt to connect to the I/O board
	if( !Board.Open() )
	{
		LOG->Warn( "InputHandler_PIUIO: Could not establish a connection with the I/O device." );
		return;
	}

	// set the relevant global flags (static flag, input type)
	m_bFoundDevice = true;
	s_bInitialized = true;
	m_bShutdown = false;

	m_iLastInputField = 0;

	DiagnosticsUtil::SetInputType( "PIUIO" );

	/* If using the R16 kernel hack, low-level input is handled differently. */
	m_InputType = MK6Helper::HasKernelPatch() ? INPUT_KERNEL : INPUT_NORMAL;

	SetLightsMappings();

	// leave us to do the reporting.
	m_DebugTimer.SetName( "MK6" );
	m_DebugTimer.AutoReport( false );
	m_DebugTimer.SetInterval( 5 );

	InputThread.SetName( "PIUIO thread" );
	InputThread.Create( InputThread_Start, this );
}

InputHandler_PIUIO::~InputHandler_PIUIO()
{
	// give a final report
	m_DebugTimer.Report();

	if( InputThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down PIUIO thread..." );
		InputThread.Wait();
		LOG->Trace( "PIUIO thread shut down." );
	}

	// reset all lights and unclaim the device
	if( m_bFoundDevice )
	{
		Board.Write( 0 );	// it's okay if this fails
		Board.Close();

		s_bInitialized = false;
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

void InputHandler_PIUIO::SetLightsMappings()
{
	uint32_t iCabinetLights[NUM_CABINET_LIGHTS] = 
	{
		/* UL, UR, LL, LR marquee lights */
		(1 << 23), (1 << 26), (1 << 25), (1 << 24),

		/* selection buttons (not used), bass lights */
		0, 0, (1 << 10), (1 << 10)
	};

	uint32_t iGameLights[MAX_GAME_CONTROLLERS][MAX_GAME_BUTTONS] = 
	{
		/* Left, Right, Up, Down */
		{ (1 << 20), (1 << 21), (1 << 18), (1 << 19) },	/* Player 1 */
		{ (1 << 4), (1 << 5), (1 << 2), (1 << 3) }	/* Player 2 */
	};

	/* The coin counter moves halfway if we send bit 4, then the rest of
	 * the way when we send bit 5. If bit 5 is sent without bit 4 prior,
	 * the coin counter doesn't do anything, so we just keep it on and
	 * use bit 4 to pulse. */
	uint32_t iCoinTriggers[2] = { (1 << 27), (1 << 28) };

	m_LightsMappings.SetCabinetLights( iCabinetLights );
	m_LightsMappings.SetCustomGameLights( iGameLights );
	m_LightsMappings.SetCoinCounter( iCoinTriggers );
 
	LightsMapper::LoadMappings( "PIUIO", m_LightsMappings );
}

void InputHandler_PIUIO::InputThreadMain()
{
	// boost this thread priority past the priority of the binary;
	// if we don't, we might lose input data (e.g. coins) during loads.
	HOOKS->BoostThreadPriority();

	while( !m_bShutdown )
	{
		m_DebugTimer.StartUpdate();

		/* core I/O cycle - translate lights state to an output value,
		 * read the per-sensor inputs and dispatch input messages. */
		UpdateLights();
		HandleInput();

		m_DebugTimer.EndUpdate();

		/* export the I/O values to the helper, for LUA binding */
		MK6Helper::Import( m_iInputData, m_iLightData );

		/* dispatch debug messages if we're debugging */
		if( g_bDebugInputDrivers && m_DebugTimer.TimeToReport() )
		{
			MK6Helper::DebugOutput( m_DebugTimer );
			m_DebugTimer.Reset();
		}
	}

	HOOKS->UnBoostThreadPriority();
}
void InputHandler_PIUIO::HandleInput()
{
	// reset our reading data
	ZERO( m_iInputField );
	ZERO( m_iInputData );

	/* PIU02 only reports one set of sensors (with four total sets) on each
	 * I/O request. In order to report all sensors for all arrows, we must:
	 *
	 * 1. Specify the requested set at bits 15-16 and 31-32 of the output
	 * 2. Write lights data and sensor selection to PIUIO
	 * 3. Read from PIUIO and save that set of sensor data
	 * 4. Repeat 2-3 until all four sets of sensor data are obtained
	 * 5. Bitwise OR the sensor data together to produce the final input
	 *
	 * The R16 kernel hack simply does all of this in kernel space, instead
	 * of alternating between kernel space and user space on each call.
	 * We pass it an 8-member uint32_t array with the lights data and get
	 * input in its place. (Why 8? I have no clue. We just RE'd it...)
	 */

	switch( m_InputType )
	{
	case INPUT_NORMAL:
		/* Normal input: write light data (with requested sensor set);
		 * perform one Write/Read cycle to get input; invert. */
		{
			for ( uint32_t i = 0; i < 4; ++i )
			{
				// write a set of sensors to request
				m_iLightData &= 0xFFFCFFFC;
				m_iLightData |= (i | (i << 16));

				// request this set of sensors
				while( !Board.Write(m_iLightData) )
					Reconnect( Board );

				// read from this set of sensors
				while( !Board.Read(&m_iInputData[i]) )
					Reconnect( Board );

				// PIUIO opens high; invert the input
				m_iInputData[i] = ~m_iInputData[i];
			}
		}
		break;
	case INPUT_KERNEL:
		/* Kernel input: write light data (with desired sensor set)
		 * in array members 0, 2, 4, 6; call BulkReadWrite; invert
		 * input and copy it to our central data array. */
		{
			ZERO( m_iBulkReadData );

			m_iLightData &= 0xFFFCFFFC;

			for( uint32_t i = 0; i < 4; ++i )
				m_iBulkReadData[i*2] = m_iLightData | (i | (i << 16));

			Board.BulkReadWrite( m_iBulkReadData );

			/* PIUIO opens high, so invert the input data. */
			for ( uint32_t i = 0; i < 4; ++i )
				m_iInputData[i] = ~m_iBulkReadData[i*2];
		}
		break;
	}

	// combine the read data into a single field
	for( int i = 0; i < 4; ++i )
		m_iInputField |= m_iInputData[i];

	// generate our input events bit field (1 = change, 0 = no change)
	uint32_t iChanged = m_iInputField ^ m_iLastInputField;
	m_iLastInputField = m_iInputField;

	// Construct outside the loop and reassign as needed (it's cheaper).
	DeviceInput di(DEVICE_JOY1, JOY_1);
	RageTimer now;

	for( unsigned iBtn = 0; iBtn < 32; ++iBtn )
	{
		// if this button's status hasn't changed, don't report it.
		if( likely(!IsBitSet(iChanged, iBtn)) )
			continue;

		di.button = JOY_1+iBtn;
		di.ts = now;

		/* Set a description of detected sensors to the arrows */
		INPUTFILTER->SetButtonComment( di, MK6Helper::GetSensorDescription(m_iInputData, iBtn) );

		// report this button's status
		ButtonPressed( di, IsBitSet(m_iInputField,iBtn) );
	}
}

void InputHandler_PIUIO::UpdateLights()
{
	// set a const pointer to the "ext" LightsState to read from
	static const LightsState *ls = LightsDriver_External::Get();

	m_iLightData = m_LightsMappings.GetLightsField( ls );
}

/*
 * (c) 2010 BoXoRRoXoRs
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

