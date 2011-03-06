#ifndef INPUT_HANDLER_LINUX_JOYSTICK_H
#define INPUT_HANDLER_LINUX_JOYSTICK_H 1

#include "InputHandler.h"
#include "RageThreads.h"
#include "RageTimer.h"

#if HAVE_INOTIFY
#include <sys/inotify.h>
#endif

class InputHandler_Linux_Joystick: public InputHandler
{
public:
	InputHandler_Linux_Joystick();
	~InputHandler_Linux_Joystick();

	void GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut );

private:
	struct joystick_dev
	{
		/*
		joystick_dev(CString a, int b, CString c)
		{
			path = a;
			fd = b;
			name = c;
		};
		*/
		CString path;
		int fd;
		CString name;
	};

	joystick_dev m_Devs[NUM_JOYSTICKS];

	void Attach(CString device);
	void Detach(CString device);

	static int InputThread_Start( void *p );
	void InputThread();
#if HAVE_INOTIFY
	static int HotplugThread_Start( void *p );
	void HotplugThread();
#endif

	int fds[NUM_JOYSTICKS];
	CString m_sDescription[NUM_JOYSTICKS];
	RageThread m_InputThread;
	RageThread m_HotplugThread;
	bool m_bShutdown;

	// debugging
	RageTimer m_InputTimer;
	float m_fTotalReadTime;
	int m_iReadCount;
};

#endif

/*
 * (c) 2003-2004 Glenn Maynard
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
