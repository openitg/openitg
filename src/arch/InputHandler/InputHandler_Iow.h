#ifndef INPUT_HANDLER_LINUX_IOW_H
#define INPUT_HANDLER_LINUX_IOW_H

#ifdef WIN32
#include <windows.h>
#else
#include <stdint.h>
#endif

#include "InputHandler.h"
#include "RageThreads.h"
#include "io/ITGIO.h"

class InputHandler_Linux_Iow: public InputHandler
{
public:
	InputHandler_Linux_Iow();
	~InputHandler_Linux_Iow();

//	for non-threaded input...should we even bother?
//	void Update( float fDeltaTime );
	void GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut );
private:
	ITGIO IOBoard;
	RageThread InputThread;
	RageThread LightsThread;

	bool m_bFoundDevice;
	bool m_bShutdown;

	uint32_t m_iInputData;
	uint32_t m_iLastInput;

	uint32_t m_iWriteData;
	uint32_t m_iLastWrite;

	static int InputThread_Start( void *p );
	static int LightsThread_Start( void *p );

	void InputThreadMain();
	void LightsThreadMain();

	void HandleInput();
	// allow this driver to update lights with "ext"
	void UpdateLights();
};

#define USE_INPUT_HANDLER_LINUX_IOW

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

