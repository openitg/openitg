// LightsDriver_LinuxParallel - driver for lights on the Linux parallel por
// To use, StepMania must be run with root permissions (for ioperm and outb)
//
// Rewritten to accommodate pad lighting

#include "global.h"
#include <sys/io.h>
#include "LightsDriver_LinuxParallel.h"

REGISTER_LIGHTS_DRIVER( LinuxParallel );

static unsigned const int MAX_PORTS = 3;
static const short PORT_ADDRESS[MAX_PORTS] =
{
	0x378,		// cabinet lighting
	0x278,		// P1 controller lighting
	0x3bc,		// P2 controller lighting
};

unsigned char OUT_BYTE[MAX_PORTS] = {};

LightsDriver_LinuxParallel::LightsDriver_LinuxParallel()
{
	// Give port permissions and set to full
	for( unsigned i = 0; i < MAX_PORTS; i++)
	{
		ioperm( PORT_ADDRESS[i], 1, 1 );
		outb( 255, PORT_ADDRESS[i] );
	}
}

LightsDriver_LinuxParallel::~LightsDriver_LinuxParallel()
{
	for( unsigned i = 0; i < MAX_PORTS; i++)
	{
		ioperm( PORT_ADDRESS[i], 1, 1 );
		outb( 0, PORT_ADDRESS[i] );
	}
}

void LightsDriver_LinuxParallel::Set( const LightsState *ls )
{
	// Reset our byte values.
	int i = 0; // iterator for adding byte values
	int j = 1; // iterator for switching controller pads

	for( unsigned i = 0; i < MAX_PORTS; i++ )
		OUT_BYTE[i] = 0;

	//Set the cabinet light values
	FOREACH_CabinetLight( cl )
	{
		if( ls->m_bCabinetLights[cl] )
			// cabinet byte
			OUT_BYTE[0] += (unsigned char)pow((double)2,i);
		i++;
	}

	// reset for new out bytes and initialise our controller iterator
	i = 0;
	j = 0;

	//Set the pad light values
	FOREACH_GameController( gc )
	{
		FOREACH_GameButton( gb )
		{
			if( ls->m_bGameButtonLights[gc][gb] )
				// start at 1 and move on per pad
				OUT_BYTE[1+j] += (unsigned char)pow((double)2,i);
			i++;
		}
		j++;
	}

	//Output the information
	for( unsigned i = 0; i < MAX_PORTS; i++)
		outb( OUT_BYTE[i], PORT_ADDRESS[i] );
}

/*
 * (c) 2004 Hugo Hromic M. <hhromic@udec.cl>
 * (c) 2007 "Mark" <vyhdycokio@gmail.com>
 *
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
