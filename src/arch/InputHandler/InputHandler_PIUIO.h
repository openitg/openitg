#ifndef INPUT_HANDLER_PIUIO_H
#define INPUT_HANDLER_PIUIO_H

#include "InputHandler.h"
#include "RageThreads.h"
#include "RageTimer.h"

#include "LightsMapper.h"
#include "io/PIUIO.h"

struct lua_State;

class InputHandler_PIUIO: public InputHandler
{
public:
	InputHandler_PIUIO();
	~InputHandler_PIUIO();

	void GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut );

private:
	/* Allow only one handler to control the board at a time. More than one
	 * handler may be loaded due to startup and Static.ini interactions, so
	 * we need this to prevent obscure I/O problems. */
	static bool s_bInitialized;

	PIUIO Board;
	RageThread InputThread;

	void SetLightsMappings();
	LightsMapping m_LightsMappings;

	/* Low-level processing changes if using the R16 kernel hack. */
	enum InputType { INPUT_NORMAL, INPUT_KERNEL } m_InputType;

	void HandleInput();
	void UpdateLights();

	void InputThreadMain();
	static int InputThread_Start( void *p ) { ((InputHandler_PIUIO *) p)->InputThreadMain(); return 0; }

	bool m_bShutdown;
	bool m_bFoundDevice;

	// input field is a combination of each sensor set in m_iInputData
	uint32_t m_iInputField, m_iLastInputField;
	uint32_t m_iInputData[4];

	// used for the r16 kernel hack and translates to m_iInputData
	uint32_t m_iBulkReadData[8];

	// data that will be written to PIUIO (lights, sensors)
	uint32_t m_iLightData;
};

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
