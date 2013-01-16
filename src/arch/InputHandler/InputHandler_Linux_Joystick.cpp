#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "ScreenManager.h"
#include "InputMapper.h"
#include "InputHandler_Linux_Joystick.h"

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/joystick.h>

#include <set>

REGISTER_INPUT_HANDLER2( Joystick, Linux_Joystick );

InputHandler_Linux_Joystick::InputHandler_Linux_Joystick()
{
	LOG->Trace( "InputHandler_Linux_Joystick::InputHandler_Linux_Joystick" );
	
	for(int i = 0; i < NUM_JOYSTICKS; ++i)
		m_Devs[i].fd = -1;

	// try attaching all possible joysticks
	for(int i = 0; i < NUM_JOYSTICKS; ++i)
		Attach(ssprintf("/dev/input/js%d", i));

	m_bShutdown = false;

	m_DebugTimer.m_sName = "Linux_Joystick";

	m_InputThread.SetName( "Joystick thread" );
	m_InputThread.Create( InputThread_Start, this );

#if HAVE_INOTIFY
	m_HotplugThread.SetName( "Joystick hotplug thread" );
	m_HotplugThread.Create( HotplugThread_Start, this );
#endif
}
	
InputHandler_Linux_Joystick::~InputHandler_Linux_Joystick()
{
#if HAVE_INOTIFY
	if( m_HotplugThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down joystick hotplug thread ..." );
		m_HotplugThread.Wait();
		LOG->Trace( "Joystick hotplug thread shut down." );
	}
#endif

	if( m_InputThread.IsCreated() )
	{
		m_bShutdown = true;
		LOG->Trace( "Shutting down joystick thread ..." );
		m_InputThread.Wait();
		LOG->Trace( "Joystick thread shut down." );
	}

	for(int i = 0; i < NUM_JOYSTICKS; ++i)
	{
		if (m_Devs[i].fd)
			close(m_Devs[i].fd);
	}
}

int InputHandler_Linux_Joystick::InputThread_Start( void *p )
{
	((InputHandler_Linux_Joystick *) p)->InputThread();
	return 0;
}

#if HAVE_INOTIFY
int InputHandler_Linux_Joystick::HotplugThread_Start( void *p )
{
	((InputHandler_Linux_Joystick *) p)->HotplugThread();
	return 0;
}
#endif

void InputHandler_Linux_Joystick::InputThread()
{
	while( !m_bShutdown )
	{
		m_DebugTimer.StartUpdate();

		fd_set fdset;
		FD_ZERO(&fdset);
		int max_fd = -1;
		
		for(int i = 0; i < NUM_JOYSTICKS; ++i)
		{
			if (m_Devs[i].fd < 0)
				continue;

			FD_SET(m_Devs[i].fd, &fdset);
			max_fd = max(max_fd, m_Devs[i].fd);
		}

		struct timeval zero = {0,100000};
		if( select(max_fd+1, &fdset, NULL, NULL, &zero) <= 0 )
			continue;

		for(int i = 0; i < NUM_JOYSTICKS; ++i)
		{
			if (m_Devs[i].fd < 0)
				continue;

			if(!FD_ISSET(m_Devs[i].fd, &fdset))
				continue;

			js_event event;
			int ret = read(m_Devs[i].fd, &event, sizeof(event));
			if(ret != sizeof(event))
			{
				LOG->Warn("Unexpected packet (size %i != %i) from joystick %i; disabled", ret, (int)sizeof(event), i);
				Detach(m_Devs[i].path);
				continue;
			}

			InputDevice id = InputDevice(DEVICE_JOY1 + i);
			DeviceInput di = DeviceInput( id, JOY_1 );

			event.type &= ~JS_EVENT_INIT;
			switch (event.type) {
			case JS_EVENT_BUTTON: {
				int iNum = event.number;
				// In 2.6.11 using an EMS USB2, the event number for P1 Tri (the first button)
				// is being reported as 32 instead of 0.  Correct for this.
				wrap( iNum, 32 );	// max number of joystick buttons.  Make this a constant?

				// set the DeviceInput timestamp and button
				di.button = JOY_1 + iNum;
				di.ts.Touch();
				ButtonPressed( di, event.value );
				break;
			}
				
			case JS_EVENT_AXIS: {
				DeviceButton neg = (JoystickButton)(JOY_LEFT+2*event.number);
				DeviceButton pos = (JoystickButton)(JOY_RIGHT+2*event.number);

				di.ts.Touch();
				di.button = neg; ButtonPressed( di, event.value < -16000 );
				di.button = pos; ButtonPressed( di, event.value > +16000 );
				break;
			}
				
			default:
				LOG->Warn("Unexpected packet (type %i) from joystick %i; disabled", event.type, i);
				Detach(m_Devs[i].path);
				continue;
			}

		}

		InputHandler::UpdateTimer();

		m_DebugTimer.EndUpdate();
	}
}

void InputHandler_Linux_Joystick::Attach(CString path)
{
	struct stat st;
	if( stat( path.c_str(), &st ) == -1 )
	{
		if( errno != ENOENT )
			LOG->Warn( "Couldn't stat %s: %s", path.c_str(), strerror(errno) );
		return;
	}

	if( !S_ISCHR( st.st_mode ) )
	{
		LOG->Warn( "Ignoring %s: not a character device", path.c_str() );
		return;
	}

	int fd = open( path.c_str(), O_RDONLY );

	if (fd != -1)
	{
		CString name;

		char szName[1024];
		ZERO( szName );
		if( ioctl(fd, JSIOCGNAME(sizeof(szName)), szName) < 0 )
			name = ssprintf( "Unknown joystick at %s", path.c_str() );
		else
			name = szName;

		for (int i = 0; i < NUM_JOYSTICKS; ++i)
		{
			if (m_Devs[i].fd < 0)
			{
				m_Devs[i].path = path;
				m_Devs[i].fd = fd;
				m_Devs[i].name = name;

				/* ScreenManager is not loaded when the joysticks are first initialized */
				if (SCREENMAN)
				{
					SCREENMAN->SystemMessageNoAnimate(ssprintf("Joystick %d \"%s\" attached", i+1, name.c_str()));
					if (INPUTMAPPER)
					{
						LOG->Info("Remapping joysticks after hotplug.");
						INPUTMAPPER->AutoMapJoysticksForCurrentGame();
					}
				}
				return;
			}
		}

		LOG->Warn("Couldn't find a free device slot for new joystick!");
	} else {
		LOG->Warn("Couldn't open %s: %s", path.c_str(), strerror(errno));
	}
}

void InputHandler_Linux_Joystick::Detach(CString path)
{
	for (int i = 0; i < NUM_JOYSTICKS; ++i)
	{
		if (m_Devs[i].fd < 0)
			continue;

		if (m_Devs[i].path == path)
		{
			SCREENMAN->SystemMessageNoAnimate(ssprintf("Joystick %d \"%s\" detached", i+1, m_Devs[i].name.c_str()));
			close(m_Devs[i].fd);
			m_Devs[i].fd = -1;
			break;
		}
	}
}

#if HAVE_INOTIFY
void InputHandler_Linux_Joystick::HotplugThread()
{
	int ifd = inotify_init();
	if (ifd < 0)
	{
		LOG->Warn("Error initializing inotify: %s", strerror(errno));
		return;
	}

	int wfd = inotify_add_watch(ifd, "/dev/input", IN_CREATE|IN_DELETE);
	if (wfd < 0)
	{
		close(ifd);
		LOG->Warn("Error watching /dev/input");
		return;
	}

	LOG->Info("Watching /dev/input for changes");

	while( !m_bShutdown )
	{
		fd_set fdset;
		FD_ZERO(&fdset);
		FD_SET(ifd, &fdset);

		struct timeval tv = {0,100000};
		if( select(ifd+1, &fdset, NULL, NULL, &tv) <= 0 )
			continue;

		char buf[1024];
		memset(buf, 0, 1024);
		int ret = read(ifd, buf, 1024);
		if (ret < 1)
		{
			LOG->Warn("Error reading inotify: %s", strerror(errno));
			break;
		}

		char *ptr = buf;

		while (ptr) {
			struct inotify_event *ev = (struct inotify_event *)ptr;

			if (ev->mask & IN_CREATE)
			{
				if (CString(ev->name).find("js") == 0)
				{
					SCREENMAN->PlayCoinSound();
					struct timeval tv = {0,500000};
					select(1, NULL, NULL, NULL, &tv);
					Attach("/dev/input/" + CString(ev->name));
				}
			}

			else if (ev->mask & IN_DELETE)
			{
				if (CString(ev->name).find("js") == 0)
				{
					Detach("/dev/input/" + CString(ev->name));
				}
			}

			ptr += sizeof(struct inotify_event) + ev->len;
			if (ptr > buf + ret)
				ptr = NULL;
		}
	}

	inotify_rm_watch(ifd, wfd);
	close(ifd);
}
#endif

void InputHandler_Linux_Joystick::GetDevicesAndDescriptions( vector<InputDevice>& vDevicesOut, vector<CString>& vDescriptionsOut )
{
	for (int i = 0; i < NUM_JOYSTICKS; i++)
	{
		if (m_Devs[i].fd < 0)
			continue;

		vDevicesOut.push_back( InputDevice(DEVICE_JOY1+i) );
		vDescriptionsOut.push_back( m_Devs[i].name );
	}
}

/*
 * (c) 2003-2004 Glenn Maynard, 2011 Toni Spets <toni.spets@iki.fi>
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
