#include "global.h"
#include "LowLevelWindow_X11.h"
#include "RageLog.h"
#include "RageException.h"
#include "archutils/Unix/X11Helper.h"
#include "PrefsManager.h" // XXX

using namespace X11Helper;

#include <stack>
#include <math.h>	// ceil()
#define GLX_GLXEXT_PROTOTYPES
#include <GL/glx.h>	// All sorts of stuff...
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/extensions/Xrandr.h>

#if defined(HAVE_LIBXTST)
#include <X11/extensions/XTest.h>
#endif

static GLXContext g_pContext = NULL;
static GLXContext g_pBackgroundContext = NULL;
static Rotation g_OldRotation;
static int g_iOldSize;
XRRScreenConfiguration *g_pScreenConfig = NULL;

// XXX HACK: RageDisplay_OGL is expecting us to set this for it so it can do
// GLX-specific queries and whatnot. It's one ugly hackish mess, but hey,
// LLW_SDL is in on it, and I'm feeling lazy.
extern Display *g_X11Display;

LowLevelWindow_X11::LowLevelWindow_X11()
{
	g_X11Display = NULL;
	if( !OpenXConnection() )
		RageException::Throw( "Failed to establish a connection with the X server." );

	g_X11Display = Dpy;

	const int iScreen = DefaultScreen( Dpy );

	LOG->Info( "Display: %s (screen %i)", DisplayString(Dpy), iScreen );
	LOG->Info( "Direct rendering: %s", glXIsDirect( Dpy, glXGetCurrentContext() )? "yes":"no" );

	int iXServerVersion = XVendorRelease( Dpy ); /* eg. 40201001 */
	int iMajor = iXServerVersion / 10000000; iXServerVersion %= 10000000;
	int iMinor = iXServerVersion / 100000;   iXServerVersion %= 100000;
	int iRevision = iXServerVersion / 1000;  iXServerVersion %= 1000;
	int iPatch = iXServerVersion;

	LOG->Info( "X server vendor: %s [%i.%i.%i.%i]", XServerVendor( Dpy ), iMajor, iMinor, iRevision, iPatch );
	LOG->Info( "Server GLX vendor: %s [%s]", glXQueryServerString( Dpy, iScreen, GLX_VENDOR ), glXQueryServerString( Dpy, iScreen, GLX_VERSION ) );
	LOG->Info( "Client GLX vendor: %s [%s]", glXGetClientString( Dpy, GLX_VENDOR ), glXGetClientString( Dpy, GLX_VERSION ) );
	
	m_bWasWindowed = true;
	g_pScreenConfig = XRRGetScreenInfo( Dpy, RootWindow(Dpy, DefaultScreen(Dpy)) );
}

LowLevelWindow_X11::~LowLevelWindow_X11()
{
	// Reset the display
	if( !m_bWasWindowed )
	{
		XRRSetScreenConfig( Dpy, g_pScreenConfig, RootWindow(Dpy, DefaultScreen(Dpy)), g_iOldSize, g_OldRotation, CurrentTime );
		
		XUngrabKeyboard( Dpy, CurrentTime );
	}
	if( g_pContext )
	{
		glXDestroyContext( Dpy, g_pContext );
		g_pContext = NULL;
	}
	if( g_pBackgroundContext )
	{
		glXDestroyContext( Dpy, g_pBackgroundContext );
		g_pBackgroundContext = NULL;
	}
	XRRFreeScreenConfigInfo( g_pScreenConfig );
	g_pScreenConfig = NULL;

	XDestroyWindow( Dpy, Win );
	Win = None;
	CloseXConnection();
}

void *LowLevelWindow_X11::GetProcAddress( CString s )
{
	// XXX: We should check whether glXGetProcAddress or
	// glXGetProcAddressARB is available, and go by that, instead of
	// assuming like this.
	return (void*) glXGetProcAddressARB( (const GLubyte*) s.c_str() );
}

CString LowLevelWindow_X11::TryVideoMode( RageDisplay::VideoModeParams p, bool &bNewDeviceOut )
{
#if defined(UNIX)
	/*
	 * nVidia cards:
	 *
	 * This only works the first time we set up a window; after that, the
	 * drivers appear to cache the value, so you have to actually restart
	 * the program to change it again.
	 */
	static char buf[128];
	strcpy( buf, "__GL_SYNC_TO_VBLANK=" );
	strcat( buf, p.vsync?"1":"0" );
	putenv( buf );
#endif

	if( g_pContext == NULL || p.bpp != CurrentParams.bpp || m_bWasWindowed != p.windowed )
	{
		// Different depth, or we didn't make a window before. New context.
		bNewDeviceOut = true;

		int visAttribs[32];
		int i = 0;
		ASSERT( p.bpp == 16 || p.bpp == 32 );

		if( p.bpp == 32 )
		{
			visAttribs[i++] = GLX_RED_SIZE;		visAttribs[i++] = 8;
			visAttribs[i++] = GLX_GREEN_SIZE;	visAttribs[i++] = 8;
			visAttribs[i++] = GLX_BLUE_SIZE;	visAttribs[i++] = 8;
		}
		else
		{
			visAttribs[i++] = GLX_RED_SIZE;		visAttribs[i++] = 5;
			visAttribs[i++] = GLX_GREEN_SIZE;	visAttribs[i++] = 6;
			visAttribs[i++] = GLX_BLUE_SIZE;	visAttribs[i++] = 5;
		}

		visAttribs[i++] = GLX_DEPTH_SIZE;	visAttribs[i++] = 16;
		visAttribs[i++] = GLX_RGBA;
		visAttribs[i++] = GLX_DOUBLEBUFFER;

		visAttribs[i++] = None;

		XVisualInfo *xvi = glXChooseVisual( Dpy, DefaultScreen(Dpy), visAttribs );
		if( xvi == NULL )
			return "No visual available for that depth.";

		// I get strange behavior if I add override redirect after creating the window.
		// So, let's recreate the window when changing that state.
		if( !MakeWindow(Win, xvi->screen, xvi->depth, xvi->visual, p.width, p.height, !p.windowed) )
			return "Failed to create the window.";
		
		char *szWindowTitle = const_cast<char *>( p.sWindowTitle.c_str() );
		XChangeProperty( Dpy, Win, XA_WM_NAME, XA_STRING, 8, PropModeReplace,
				reinterpret_cast<unsigned char*>(szWindowTitle), strlen(szWindowTitle) );

		if( g_pContext )
			glXDestroyContext( Dpy, g_pContext );
		if( g_pBackgroundContext )
			glXDestroyContext( Dpy, g_pBackgroundContext );
		g_pContext = glXCreateContext( Dpy, xvi, NULL, True );
		g_pBackgroundContext = glXCreateContext( Dpy, xvi, g_pContext, True );

		glXMakeCurrent( Dpy, Win, g_pContext );

		XWindowAttributes winAttrib;
		XGetWindowAttributes( Dpy, Win, &winAttrib );
		XSelectInput( Dpy, Win, winAttrib.your_event_mask | StructureNotifyMask );
		XMapWindow( Dpy, Win );

		// XXX: Why do we need to wait for the MapNotify event?
		while( true )
		{
			XEvent event;
			XMaskEvent( Dpy, StructureNotifyMask, &event );
			if( event.type == MapNotify )
				break;
		}
		XSelectInput( Dpy, Win, winAttrib.your_event_mask );

	}
	else
	{
		// We're remodeling the existing window, and not touching the
		// context.
		bNewDeviceOut = false;
	}
	
	g_iOldSize = XRRConfigCurrentConfiguration( g_pScreenConfig, &g_OldRotation );
	
	if( !p.windowed )
	{
		// Find a matching mode.
		int iSizesXct;
		XRRScreenSize *pSizesX = XRRSizes( Dpy, DefaultScreen(Dpy), &iSizesXct );
		ASSERT_M( iSizesXct != 0, "Couldn't get resolution list from X server" );
	
		int iSizeMatch = -1;
		
		for( int i = 0; i < iSizesXct; ++i )
		{
			if( pSizesX[i].width == p.width && pSizesX[i].height == p.height )
			{
				iSizeMatch = i;
				break;
			}
		}

		// Set this mode.
		// XXX: This doesn't handle if the config has changed since we queried it (see man Xrandr)
		XRRSetScreenConfig( Dpy, g_pScreenConfig, RootWindow(Dpy, DefaultScreen(Dpy)), iSizeMatch, 1, CurrentTime );
		
		// Move the window to the corner that the screen focuses in on.
		XMoveWindow( Dpy, Win, 0, 0 );
		
		XRaiseWindow( Dpy, Win );
		
		if( m_bWasWindowed )
		{
			// We want to prevent the WM from catching anything that comes from the keyboard.
			XGrabKeyboard( Dpy, Win, True, GrabModeAsync, GrabModeAsync, CurrentTime );
			m_bWasWindowed = false;
		}
	}
	else
	{
		if( !m_bWasWindowed )
		{
			XRRSetScreenConfig( Dpy, g_pScreenConfig, RootWindow(Dpy, DefaultScreen(Dpy)), g_iOldSize, g_OldRotation, CurrentTime );
			// In windowed mode, we actually want the WM to function normally.
			// Release any previous grab.
			XUngrabKeyboard( Dpy, CurrentTime );
			m_bWasWindowed = true;
		}
	}
	int rate = XRRConfigCurrentRate( g_pScreenConfig );

	// Do this before resizing the window so that pane-style WMs (Ion,
	// ratpoison) don't resize us back inappropriately.
	{
		XSizeHints hints;

		hints.flags = PBaseSize;
		hints.base_width = p.width;
		hints.base_height = p.height;

		XSetWMNormalHints( Dpy, Win, &hints );
	}

	// Do this even if we just created the window -- works around Ion2 not
	// catching WM normal hints changes in mapped windows.
	XResizeWindow( Dpy, Win, p.width, p.height );

	CurrentParams = p;
	CurrentParams.rate = rate;
	return ""; // Success
}

void LowLevelWindow_X11::SwapBuffers()
{
	glXSwapBuffers( Dpy, Win );

	if( PREFSMAN->m_bDisableScreenSaver )
	{
		/* Disable the screensaver. */
#if defined(HAVE_LIBXTST)
		/* This causes flicker. */
		// XForceScreenSaver( Dpy, ScreenSaverReset );
		
		/*
		 * Instead, send a null relative mouse motion, to trick X into thinking there has been
		 * user activity. 
		 *
		 * This also handles XScreenSaver; XForceScreenSaver only handles the internal X11
		 * screen blanker.
		 *
		 * This will delay the X blanker, DPMS and XScreenSaver from activating, and will
		 * disable the blanker and XScreenSaver if they're already active (unless XSS is
		 * locked).  For some reason, it doesn't un-blank DPMS if it's already active.
		 */

		XLockDisplay( Dpy );

		int event_base, error_base, major, minor;
		if( XTestQueryExtension( Dpy, &event_base, &error_base, &major, &minor ) )
		{
			XTestFakeRelativeMotionEvent( Dpy, 0, 0, 0 );
			XSync( Dpy, False );
		}

		XUnlockDisplay( Dpy );
#endif
	}
}

#if 0
void LowLevelWindow_X11::GetDisplayResolutions( DisplayResolutions &out ) const
{
	int iSizesXct;
	XRRScreenSize *pSizesX = XRRSizes( Dpy, DefaultScreen( Dpy ), &iSizesXct );
	ASSERT_M( iSizesXct != 0, "Couldn't get resolution list from X server" );
	
	for( int i = 0; i < iSizesXct; ++i )
	{
		DisplayResolution res = { pSizesX[i].width, pSizesX[i].height, true };
		out.insert( res );
	}
}
#endif

/*
 * (c) 2005 Ben Anderson
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
