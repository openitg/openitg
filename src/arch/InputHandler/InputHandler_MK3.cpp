#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "InputMapper.h"

/* TODO: add lights support */

#include "InputHandler_MK3.h"

/* All ports are 16 bits. */
const short INPUT_PORT_1 = 0x2A4;
const short INPUT_PORT_2 = 0x2A6;

const short OUTPUT_PORT_1 = 0x2A0;
const short OUTPUT_PORT_2 = 0x2A2;

#ifdef LINUX
#include <sys/io.h>

inline void Write( uint32_t iData )
{
	outw_p( (uint16_t)(iData>>16),	OUTPUT_PORT_1 );
	outw_p( (uint16_t)(iData),		OUTPUT_PORT_2 );
}

inline void Read( uint32_t *pData )
{
	*pData = inw_p(INPUT_PORT_1) << 16 | inw_p(INPUT_PORT_2);
}
#else
#error InputHandler_MK3 needs fixing up to work on a non-*nix OS.
#endif

InputHandler_MK3::InputHandler_MK3()
{
	m_bFoundDevice = false;
	m_bShutdown = false;

	// XXX: non-portable command
	if( iopl(3) == -1 )
	{
		LOG->Warn( "InputHandler_MK3 requires root privileges to run." );
		return;
	}

	// TO DO: figure out what kind of values we read when MK3 isn't
	// initted, and more intelligently decide we have a device there.
	m_bFoundDevice = true;

	InputThread.SetName( "MK3 thread" );
	InputThread.Create( InputThread_Start, this );
}

InputHandler_MK3::~InputHandler_MK3()
{
	if( InputThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down MK3 thread..." );
		InputThread.Wait();
		LOG->Trace( "MK3 thread shut down." );
	}

	// reset all lights
	if( m_bFoundDevice )
		Write( 0 );
}

void InputHandler_MK3::GetDevicesAndDescriptions( vector<InputDevice> &vDevicesOut, vector<CString> &vDescriptionsOut )
{
	if( m_bFoundDevice )
	{
		vDevicesOut.push_back( InputDevice(DEVICE_JOY1) );
		vDescriptionsOut.push_back( "MK3" );
	}
}

void InputHandler_MK3::InputThreadMain()
{
	while( !m_bShutdown )
	{
		/* Figure out the lights states and set them */
		UpdateLights();

		/* Read input and sensors, write the above data */
		HandleInput();
	}
}

void InputHandler_MK3::HandleInput()
{
	ZERO( m_iInputField );
	ZERO( m_iInputData );

	for( uint32_t i = 0; i < 4; i++ )
	{
		// write which sensors to report from
		m_iLightData &= 0xFFFCFFFC;
		m_iLightData |= (i | (i << 16));

		// do one write/read cycle to get this sensor set
		Write( m_iLightData );
		Read( &m_iInputData[i] );

		// PIUIO opens high - invert it
		m_iInputData[i] = ~m_iInputData[i];

		// OR it into the input field
		m_iInputField |= m_iInputData[i];
	}

	DeviceInput di(DEVICE_JOY1, JOY_1);

	for( int iBtn = 0; iBtn < 32; iBtn++ )
	{
		di.button = JOY_1+iBtn;

		if( InputThread.IsCreated() )
			di.ts.Touch();

		// TODO: implement per-sensor reporting

		// report state of all button presses
		ButtonPressed( di, (bool)(m_iInputField & (1 << (31-iBtn))) );
	}
}

// TODO: lol
void InputHandler_MK3::UpdateLights()
{
	m_iLightData = 0xFFFFFFFF;
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
