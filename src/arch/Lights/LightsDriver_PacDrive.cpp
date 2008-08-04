// Cross-platform, libusb-based driver for outputting lights
// via a PacDrive (http://www.ultimarc.com/pacdrive.html)

#include "global.h"
#include "RageLog.h"
#include "io/PacDrive.h"
#include "LightsDriver_PacDrive.h"

LightsDriver_PacDrive::LightsDriver_PacDrive()
{
	m_bHasDevice = Board.Open();

	if( m_bHasDevice == false )
	{
		LOG->Warn( "Could not establish a connection with PacDrive." );
		return;
	}

	// clear all lights
	Board.Write( 0 );
}

LightsDriver_PacDrive::~LightsDriver_PacDrive()
{
	if( !m_bHasDevice )
		return;

	// clear all lights and close the connection
	Board.Write( 0 );
	Board.Close();
}

void LightsDriver_PacDrive::Set( const LightsState *ls )
{
	if( !m_bHasDevice )
		return;

	uint8_t iCabinetData = 0;
	uint8_t iPadData = 0;

	// Lights 1 - 8 are used for the cabinet lights
	FOREACH_CabinetLight( cl )
		if( ls->m_bCabinetLights[cl] )
			iCabinetData |= (1 << cl);

	LOG->Debug( "iCabinetData: %#2x", iCabinetData );

	// Lights 9-13 for P1 pad, 14-18 for P2 pad
	// FIXME: make this work for all game-types?
	FOREACH_GameController( gc )
		FOREACH_ENUM( GameButton, 4, gb )
			if( ls->m_bGameButtonLights[gc][gb] )
				iPadData |= (1 << (gb + gc*4));

	LOG->Debug( "iPadData: %#2x", iPadData );

	// combine the data set above
	uint16_t iData = (uint16_t)(iCabinetData << 8) | iPadData;

	LOG->Debug( "Writing %#4x to PacDrive.", iData );

	// write the data - if it fails, stop updating
	if( !Board.Write(iData) )
	{
		LOG->Warn( "Lost connection with PacDrive." );
		m_bHasDevice = false;
	}
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
