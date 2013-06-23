#include "global.h"
#include "GraphicsWindow.h"
#include "StepMania.h"
#include "ProductInfo.h"
#include "RageLog.h"
#include "RageUtil.h"

#include "archutils/Win32/AppInstance.h"
#include "archutils/Win32/WindowIcon.h"
#include "archutils/Win32/GetFileInformation.h"

#include <set>

static const CString g_sClassName = CString(PRODUCT_NAME) + " LowLevelWindow_Win32";

static HWND g_hWndMain;
static HDC g_HDC;
static RageDisplay::VideoModeParams g_CurrentParams;
static bool g_bResolutionChanged = false;
static bool g_bHasFocus = true;
static bool g_bLastHasFocus = true;
static HICON g_hIcon = NULL;
static bool m_bWideWindowClass;

CString GetNewWindow()
{
	HWND h = GetForegroundWindow();
	if( h == NULL )
		return "(NULL)";

	DWORD iProcessID;
	GetWindowThreadProcessId( h, &iProcessID );

	CString sName;
	GetProcessFileName( iProcessID, sName );

	sName = Basename(sName);

	return sName;
}

LRESULT CALLBACK GraphicsWindow::GraphicsWindow_WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_ACTIVATE:
	{
		const bool bInactive = (LOWORD(wParam) == WA_INACTIVE);
		const bool bMinimized = (HIWORD(wParam) != 0);
		LOG->Trace("WM_ACTIVATE (%i, %i)",
			bInactive, bMinimized );
		g_bHasFocus = !bInactive && !bMinimized;
		if( !g_bHasFocus )
		{
			CString sName = GetNewWindow();
			static set<CString> sLostFocusTo;
			sLostFocusTo.insert( sName );
			CString sStr;
			for( set<CString>::const_iterator it = sLostFocusTo.begin(); it != sLostFocusTo.end(); ++it )
				sStr += (sStr.size()?", ":"") + *it;

			LOG->MapLog( "LOST_FOCUS", "Lost focus to: %s", sStr.c_str() );
		}
		return 0;
	}

	/* Is there any reason we should care what size the user resizes the window to? */
//	case WM_GETMINMAXINFO:

	case WM_SETCURSOR:
		if( !g_CurrentParams.windowed )
		{
			SetCursor( NULL );
			return 1;
		}
		break;

	case WM_SYSCOMMAND:
		switch( wParam )
		{
		case SC_MONITORPOWER:
		case SC_SCREENSAVE:
			return 1;
		}
		break;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		BeginPaint( hWnd, &ps );
		EndPaint( hWnd, &ps );
		break;
	}

	case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
		/* We handle all input ourself, via DirectInput. */
		return 0;

	case WM_CLOSE:
		LOG->Trace("WM_CLOSE: shutting down");
		ExitGame();
		return 0;

	case WM_WINDOWPOSCHANGED:
	{
		RECT rect;
		GetClientRect( hWnd, &rect );

		int iWidth = rect.right - rect.left;
		int iHeight = rect.bottom - rect.top;
		if( g_CurrentParams.width != iWidth || g_CurrentParams.height != iHeight )
		{
			g_CurrentParams.width = iWidth;
			g_CurrentParams.height = iHeight;
			g_bResolutionChanged = true;
		}
		break;
	}
	}

	if( m_bWideWindowClass )
		return DefWindowProcW( hWnd, msg, wParam, lParam );
	else
		return DefWindowProcA( hWnd, msg, wParam, lParam );
}

void GraphicsWindow::SetVideoModeParams( const RageDisplay::VideoModeParams &p )
{
	g_CurrentParams = p;
}

CString GraphicsWindow::SetScreenMode( const RageDisplay::VideoModeParams &p )
{
	if( p.windowed )
	{
		/* We're going windowed.  If we were previously fullscreen, reset. */
		if( !g_CurrentParams.windowed )
			ChangeDisplaySettings( NULL, 0 );

		return "";
	}

	DEVMODE DevMode;
	ZERO( DevMode );
	DevMode.dmSize = sizeof(DEVMODE);
	DevMode.dmPelsWidth = p.width;
	DevMode.dmPelsHeight = p.height;
	DevMode.dmBitsPerPel = p.bpp;
	DevMode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

	if( p.rate != REFRESH_DEFAULT )
	{
		DevMode.dmDisplayFrequency = p.rate;
		DevMode.dmFields |= DM_DISPLAYFREQUENCY;
	}
	ChangeDisplaySettings( NULL, 0 );

	int ret = ChangeDisplaySettings( &DevMode, CDS_FULLSCREEN );
	if( ret != DISP_CHANGE_SUCCESSFUL && (DevMode.dmFields & DM_DISPLAYFREQUENCY) )
	{
		DevMode.dmFields &= ~DM_DISPLAYFREQUENCY;
		ret = ChangeDisplaySettings( &DevMode, CDS_FULLSCREEN );
	}

	/* XXX: append error */
	if( ret != DISP_CHANGE_SUCCESSFUL )
		return "Couldn't set screen mode";

	return "";
}

static int GetWindowStyle( const RageDisplay::VideoModeParams &p )
{
	if( p.windowed )
		return WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
	else
		return WS_POPUP;
}

void GraphicsWindow::CreateGraphicsWindow( const RageDisplay::VideoModeParams &p )
{
	ASSERT( g_hWndMain == NULL );

	int iWindowStyle = GetWindowStyle(p);

	AppInstance inst;
	g_hWndMain = CreateWindow( g_sClassName, "app", iWindowStyle,
					0, 0, 0, 0, NULL, NULL, inst, NULL );
	if( g_hWndMain == NULL )
		RageException::Throw( "%s", werr_ssprintf( GetLastError(), "CreateWindow" ).c_str() );

//	SetForegroundWindow( g_hWndMain );

	g_HDC = GetDC( g_hWndMain );
}

void GraphicsWindow::RecreateGraphicsWindow( const RageDisplay::VideoModeParams &p )
{
	ASSERT( g_hWndMain != NULL );

	int iWindowStyle = GetWindowStyle(p);

	AppInstance inst;
	HWND hWnd = CreateWindow( g_sClassName, "app", iWindowStyle,
					0, 0, 0, 0, NULL, NULL, inst, NULL );
	if( hWnd == NULL )
		RageException::Throw( "%s", werr_ssprintf( GetLastError(), "CreateWindow" ).c_str() );

	SetForegroundWindow( hWnd );

	DestroyGraphicsWindow();

	g_hWndMain = hWnd;
	g_HDC = GetDC( g_hWndMain );
}

/* Set the final window size, set the window text and icon, and then unhide the
 * window. */
void GraphicsWindow::ConfigureGraphicsWindow( const RageDisplay::VideoModeParams &p )
{
	ASSERT( g_hWndMain );

	/* The window style may change as a result of switching to or from fullscreen;
	 * apply it.  Don't change the WS_VISIBLE bit. */
	int iWindowStyle = GetWindowStyle(p);
	if( GetWindowLong( g_hWndMain, GWL_STYLE ) & WS_VISIBLE )
		iWindowStyle |= WS_VISIBLE;
	SetWindowLong( g_hWndMain, GWL_STYLE, iWindowStyle );

	RECT WindowRect;
	SetRect( &WindowRect, 0, 0, p.width, p.height );
	AdjustWindowRect( &WindowRect, iWindowStyle, FALSE );

	const int iWidth = WindowRect.right - WindowRect.left;
	const int iHeight = WindowRect.bottom - WindowRect.top;

	/* If windowed, center the window. */
	int x = 0, y = 0;
	if( p.windowed )
	{
        x = GetSystemMetrics(SM_CXSCREEN)/2-iWidth/2;
        y = GetSystemMetrics(SM_CYSCREEN)/2-iHeight/2;
	}

	/* Move and resize the window.  SWP_FRAMECHANGED causes the above SetWindowLong
	 * to take effect. */
	SetWindowPos( g_hWndMain, HWND_NOTOPMOST, x, y, iWidth, iHeight, SWP_FRAMECHANGED );

	/* Update the window title. */
	do
	{
		if( m_bWideWindowClass )
		{
			if( SetWindowTextW( g_hWndMain, CStringToWstring(p.sWindowTitle).c_str() ) )
				break;
		}

		SetWindowTextA( g_hWndMain, ConvertUTF8ToACP(p.sWindowTitle) );
	} while(0);

	/* Update the window icon. */
	if( g_hIcon != NULL )
	{
		SetClassLong( g_hWndMain, GCL_HICON, (LONG) NULL );
		DestroyIcon( g_hIcon );
		g_hIcon = NULL;
	}
	g_hIcon = IconFromFile( p.sIconFile );
	if( g_hIcon != NULL )
		SetClassLong( g_hWndMain, GCL_HICON, (LONG) g_hIcon );

	ShowWindow( g_hWndMain, SW_SHOW );

	SetForegroundWindow( g_hWndMain );

	/* Pump messages quickly, to make sure the window is completely set up.
	 * If we don't do this, then starting up in a D3D fullscreen window may
	 * cause all other windows on the system to be resized. */
	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		GetMessage( &msg, NULL, 0, 0 );
		DispatchMessage( &msg );
	}
}

/* Shut down the window, but don't reset the video mode. */
void GraphicsWindow::DestroyGraphicsWindow()
{
	if( g_HDC != NULL )
	{
		ReleaseDC( g_hWndMain, g_HDC );
		g_HDC = NULL;
	}

	if( g_hWndMain != NULL )
	{
		DestroyWindow( g_hWndMain );
		g_hWndMain = NULL;
	}

	if( g_hIcon != NULL )
	{
		DestroyIcon( g_hIcon );
		g_hIcon = NULL;
	}

	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		GetMessage( &msg, NULL, 0, 0 );
		DispatchMessage( &msg );
	}

}

void GraphicsWindow::Initialize()
{
	AppInstance inst;
	do
	{
		const wstring wsClassName = CStringToWstring( g_sClassName );
		WNDCLASSW WindowClassW =
		{
			CS_OWNDC | CS_BYTEALIGNCLIENT,
			GraphicsWindow_WndProc,
			0,				/* cbClsExtra */
			0,				/* cbWndExtra */
			inst,			/* hInstance */
			NULL,			/* set icon later */
			LoadCursor( NULL, IDC_ARROW ),	/* default cursor */
			NULL,			/* hbrBackground */
			NULL,			/* lpszMenuName */
			wsClassName.c_str()	/* lpszClassName */
		}; 

		m_bWideWindowClass = true;
		if( RegisterClassW( &WindowClassW ) )
			break;

		WNDCLASS WindowClassA =
		{
			CS_OWNDC | CS_BYTEALIGNCLIENT,
			GraphicsWindow_WndProc,
			0,				/* cbClsExtra */
			0,				/* cbWndExtra */
			inst,			/* hInstance */
			NULL,			/* set icon later */
			LoadCursor( NULL, IDC_ARROW ),	/* default cursor */
			NULL,			/* hbrBackground */
			NULL,			/* lpszMenuName */
			g_sClassName	/* lpszClassName */
		}; 

		m_bWideWindowClass = false;
		if( !RegisterClassA( &WindowClassA ) )
			RageException::Throw( "%s", werr_ssprintf( GetLastError(), "RegisterClass" ).c_str() );
	} while(0);
}

void GraphicsWindow::Shutdown()
{
	DestroyGraphicsWindow();

	/*
	 * Return to the desktop resolution, if needed.
	 *
	 * It'd be nice to not do this: Windows will do it when we quit, and if we're
	 * shutting down OpenGL to try D3D, this will cause extra mode switches.  However,
	 * we need to do this before displaying dialogs.
	 */
	ChangeDisplaySettings( NULL, 0 );

	AppInstance inst;
	UnregisterClass( g_sClassName, inst );
}

HDC GraphicsWindow::GetHDC()
{
	ASSERT( g_HDC != NULL );
	return g_HDC;
}

RageDisplay::VideoModeParams GraphicsWindow::GetParams()
{
	return g_CurrentParams;
}

void GraphicsWindow::Update()
{
	ASSERT( DISPLAY );

	MSG msg;
	while( PeekMessage( &msg, NULL, 0, 0, PM_NOREMOVE ) )
	{
		GetMessage( &msg, NULL, 0, 0 );
		DispatchMessage( &msg );
	}

	if( g_bHasFocus != g_bLastHasFocus )
	{
		FocusChanged( g_bHasFocus );
		g_bLastHasFocus = g_bHasFocus;
	}

	if( g_bResolutionChanged )
	{
		/* Let DISPLAY know that our resolution has changed. */
		DISPLAY->ResolutionChanged();
		g_bResolutionChanged = false;
	}
}

HWND GraphicsWindow::GetHwnd()
{
	return g_hWndMain;
}

/*
 * (c) 2004 Glenn Maynard
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
