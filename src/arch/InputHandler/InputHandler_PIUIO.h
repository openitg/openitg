#ifndef INPUT_HANDLER_LINUX_PIUIO_H
#define INPUT_HANDLER_LINUX_PIUIO_H

/*  I've heard bad things about portability relating to stdint.  /
/   If you run into any problems, let me know. - Vyhd		 */
#ifdef WIN32
#include <windows.h>
#else
#include <stdint.h>
#endif

#include "InputHandler.h"
#include "RageThreads.h"
#include "RageTimer.h"
#include "io/PIUIO.h"

class InputHandler_PIUIO: public InputHandler
{
public:
	/* If bPumpMode, some mappings change */
//	InputHandler_PIUIO( bool bPumpMode = false );
	InputHandler_PIUIO();
	~InputHandler_PIUIO();

//	for non-threaded input...should we even bother?
//	void Update( float fDeltaTime );
	void GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut );
private:
	PIUIO IOBoard;
	RageTimer m_InputTimer;
	RageThread InputThread;

	bool m_bFoundDevice;
	bool m_bShutdown;
	bool m_bCoinEvent;
	bool m_bPumpMode;

	/* one uint32_t per sensor set */
	uint32_t m_iInputData[4];
	uint32_t m_iLastInputData[4];

	uint32_t m_iLightData;
	uint32_t m_iLastLightData;

	static int InputThread_Start( void *p );
	void InputThreadMain();

	void HandleInput();

	// allow this driver to update lights with "ext"
	void UpdateLights();
};

#define USE_INPUT_HANDLER_PIUIO

#endif

/*
 * (c) 2008 BoXoRRoXoRs
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

