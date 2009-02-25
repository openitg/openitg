#ifndef LIGHTSDRIVER_G15_H
#define LIGHTSDRIVER_G15_H

#include "LightsDriver.h"
#include "LightsMapper.h"
#include "io/G15.h"
#include "RageThreads.h"

class LightsDriver_G15: public LightsDriver
{
public:
	LightsDriver_G15();
	~LightsDriver_G15();

	void Set( const LightsState *ls );
private:
	void SetLightsMappings();
	LightsMapping m_LightsMappings;

	void ThreadLoop();
	void KeysThreadLoop();
	static int ThreadStart( void *pG15 );
	static int KeysThreadStart( void *pG15 );

	void ReadSpecialKeys( unsigned char *pData );

	G15 LCD;
	bool m_bHasDevice;

	RageThread m_WriteThread;
	RageThread m_ReadThread;
	bool m_bThreadStop;

	uint32_t m_iSavedLightData;
	uint32_t m_iLastLightData;
	LCDDisplayState m_DisplayState;
};

#define USE_LIGHTS_DRIVER_G15

#endif // LIGHTSDRIVER_G15_H

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
