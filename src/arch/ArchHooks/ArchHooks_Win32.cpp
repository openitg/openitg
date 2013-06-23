#include "global.h"
#include "ArchHooks_Win32.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageThreads.h"
#include "PrefsManager.h"
#include "StepMania.h"
#include "ProductInfo.h"
#include "GameState.h"

#include "archutils/win32/AppInstance.h"
#include "archutils/win32/crash.h"
#include "archutils/win32/DebugInfoHunt.h"
#include "archutils/win32/GotoURL.h"
#include "archutils/win32/RestartProgram.h"
#include "archutils/win32/VideoDriverInfo.h"
#include "archutils/win32/WindowsResources.h"

#include <mmsystem.h>
#include <winsock2.h>
#if defined(_MSC_VER)
#pragma comment(lib, "winmm.lib") // for timeGetTime
#pragma comment(lib, "ws2_32.lib") // for Winsock functions
#endif

/* Workaround for 'undefined' problem */
#ifndef EWX_FORCEIFHUNG
#define EWX_FORCEIFHUNG 0x10
#endif

ArchHooks_Win32::ArchHooks_Win32()
{
	SetUnhandledExceptionFilter(CrashHandler);
	TimeCritMutex = new RageMutex("TimeCritMutex");

	/* Disable critical errors, and handle them internally.  We never want the
	 * "drive not ready", etc. dialogs to pop up. */
	SetErrorMode( SetErrorMode(0) | SEM_FAILCRITICALERRORS );

	/* Windows boosts priority on keyboard input, among other things.  Disable that for
	 * the main thread. */
	SetThreadPriorityBoost( GetCurrentThread(), TRUE );
}

ArchHooks_Win32::~ArchHooks_Win32()
{
	delete TimeCritMutex;
}

void ArchHooks_Win32::DumpDebugInfo()
{
	/* This is a good time to do the debug search: before we actually
	 * start OpenGL (in case something goes wrong). */
	SearchForDebugInfo();

	CheckVideoDriver();
}

static CString g_sDriverVersion, g_sURL;
static bool g_Hush;
static BOOL CALLBACK DriverWndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
	case WM_INITDIALOG:
		{
			g_Hush = false;
			CString sMessage = ssprintf(
				"The graphics drivers you are running, %s, are very old and are known "
				"to cause problems.  Upgrading to the latest drivers is recommended.\n",
					g_sDriverVersion.c_str() );

			sMessage.Replace( "\n", "\r\n" );

			SendDlgItemMessage( hWnd, IDC_MESSAGE, WM_SETTEXT, 
				0, (LPARAM)(LPCTSTR)sMessage );
		}
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			g_Hush = !!IsDlgButtonChecked(hWnd, IDC_HUSH);
			GotoURL( g_sURL );
			EndDialog( hWnd, 1 );
			break;

		case IDCANCEL:
			g_Hush = !!IsDlgButtonChecked(hWnd, IDC_HUSH);
			EndDialog( hWnd, 0 );
			break;
		}
	}
	return FALSE;
}

static bool MessageIsIgnored( CString ID )
{
	vector<CString> list;
	split( PREFSMAN->m_sIgnoredMessageWindows, ",", list );
	for( unsigned i = 0; i < list.size(); ++i )
		if( !ID.CompareNoCase(list[i]) )
			return true;
	return false;
}

static void IgnoreMessage( CString ID )
{
	if( ID == "" )

	if( MessageIsIgnored(ID) )
		return;

	vector<CString> list;
	split( PREFSMAN->m_sIgnoredMessageWindows, ",", list );
	list.push_back( ID );
	PREFSMAN->m_sIgnoredMessageWindows.Set( join(",",list) );
	PREFSMAN->SaveGlobalPrefsToDisk();
}

/*
 * This simply does a few manual checks for known bad driver versions.  Only nag the
 * user if it's a driver that we receive many complaints about--we don't want to
 * tell users to upgrade if there's no problem, since it's likely to cause new problems.
 */
void ArchHooks_Win32::CheckVideoDriver()
{
	if( MessageIsIgnored( "OLD_DRIVER_WARNING" ) )
		return;

	CString sPrimaryDeviceName = GetPrimaryVideoName();
	if( sPrimaryDeviceName == "" )
		return;

	g_sDriverVersion = "";
	for( int i=0; true; i++ )
	{
		VideoDriverInfo info;
		if( !GetVideoDriverInfo(i, info) )
			break;
		if( info.sDescription != sPrimaryDeviceName )
			continue;

		/* "IntelR 810 Chipset Graphics Driver PV 2.1".  I get a lot of crash reports
		 * with people using this version. */
		if( Regex( "Intel.* 810 Chipset Graphics Driver PV 2.1").Compare( info.sDescription ) )
		{
			g_sDriverVersion = info.sDescription;
			g_sURL = "http://www.intel.com/design/software/drivers/platform/810.htm";
			break;
		}
	}

	if( g_sDriverVersion == "" )
		return;

	bool bExit = !!DialogBox( AppInstance(), MAKEINTRESOURCE(IDD_DRIVER), NULL, DriverWndProc );

	if( g_Hush )
		IgnoreMessage( "OLD_DRIVER_WARNING" );
	if( bExit )
		ExitProcess(0);
}


void ArchHooks_Win32::RestartProgram()
{
	Win32RestartProgram();
}

void ArchHooks_Win32::MountInitialFilesystems( const CString &sDirOfExecutable )
{
	/* All Windows data goes in the directory one level above the executable. */
	CHECKPOINT_M( ssprintf( "DOE \"%s\"", sDirOfExecutable.c_str()) );

	CStringArray parts;
	split( sDirOfExecutable, "/", parts );

	CHECKPOINT_M( ssprintf( "... %i parts", parts.size()) );
	ASSERT_M( parts.size() > 1, ssprintf("Strange DirOfExecutable: %s", sDirOfExecutable.c_str()) );

	CString Dir = join( "/", parts.begin(), parts.end()-1 );

	/* XXX: how are directories going to be arranged on a Windows arcade machine? */
	if (parts.size() > 2 &&
		!parts[parts.size()-2].CompareNoCase("Data") &&
		!parts[parts.size()-1].CompareNoCase("patch"))
	{
		Dir = join( "/", parts.begin(), parts.end()-2 );
	}

	// OpenITG-specific file paths
	FILEMAN->Mount( "oitg", Dir + "/CryptPackages", "/Packages" );

	/* This mounts everything else, including Data, etc. */
	FILEMAN->Mount( "dir", Dir, "/" );
}

void ArchHooks_Win32::SystemReboot( bool bForceSync )
{
#ifndef ITG_ARCADE
	Win32RestartProgram();

	return; // pretend there's nothing else here right now -- enable if you need it
#endif

	/* Reboot, kill any hung processes, "Application Maintenance: Planned" */
	bool bRestart = ExitWindowsEx( EWX_REBOOT | EWX_FORCEIFHUNG,
	SHTDN_REASON_MAJOR_APPLICATION | SHTDN_REASON_MINOR_MAINTENANCE
	| SHTDN_REASON_FLAG_PLANNED );

	if( bRestart ) // success
		return;

	LOG->Warn( "Could not restart StepMania: %#x", (unsigned int)GetLastError() );

	ExitGame();
}

static void GetDiskSpace( const CString &sDir, uint64_t *pSpaceFree, uint64_t *pSpaceTotal )
{
	if( GetDiskFreeSpaceEx( sDir.c_str(), (PULARGE_INTEGER)pSpaceFree, (PULARGE_INTEGER)pSpaceTotal, NULL ) == 0 )
		LOG->Warn( werr_ssprintf(GetLastError(), "GetDiskSpace() failed") );
}

uint64_t ArchHooks_Win32::GetDiskSpaceFree( const CString &sDir_ )
{
	CString sDir = FILEMAN->ResolvePath( sDir_ );

	// remove beginning slash:
	//    /D:/path/to/stepmania
	sDir = sDir.substr(1);
	for( unsigned i = 0; i < sDir.size(); ++i )
		if( sDir[i] == '/' )
			sDir[i] = '\\';

	uint64_t iDiskSpaceFree = 0;
	GetDiskSpace( sDir, &iDiskSpaceFree, NULL );
	return iDiskSpaceFree;
}

uint64_t ArchHooks_Win32::GetDiskSpaceTotal( const CString &sDir_ )
{
	CString sDir = FILEMAN->ResolvePath( sDir_ );

	// remove beginning slash:
	//    /D:/path/to/stepmania
	sDir = sDir.substr(1);
	for( unsigned i = 0; i < sDir.size(); ++i )
		if( sDir[i] == '/' )
			sDir[i] = '\\';

	uint64_t iDiskSpaceTotal = 0;
	GetDiskSpace( sDir, NULL, &iDiskSpaceTotal );
	return iDiskSpaceTotal;
}

bool ArchHooks_Win32::OpenMemoryRange( unsigned short start_port, unsigned short bytes )
{
	LOG->Trace( "ArchHooks_Win32::OpenMemoryRange( %u, %u )", start_port, bytes );
	
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize=sizeof(version);
	if( !GetVersionEx(&version) )
	{
		LOG->Warn( werr_ssprintf(GetLastError(), "OpenMemoryRange(): GetVersionEx failed") );
		return false;
	}

	// pre-NT Windows systems always have I/O port access open;
	// there's no need to request access for those systems.
	if( version.dwPlatformId == 1 )
	{
		LOG->Trace( "OpenMemoryRange(): detected pre-NT kernel, returning true." );
	       	return true;
	}

	LOG->Warn( "OpenMemoryRange() not fully implemented!\nIf this crashes, disable the ISA driver." );
	return true;
}

void ArchHooks_Win32::CloseMemoryRange( unsigned short start_port, unsigned short bytes )
{
	LOG->Trace( "CloseMemoryRange( %u, %u )", start_port, bytes );
	return;
}

bool ArchHooks_Win32::GetNetworkAddress( CString &sIP, CString &sNetmask, CString &sError )
{
	/* This doesn't work at all right now. */
	sError = "Not implemented";
	return false;

	// initialise the winsock layer
	WSADATA wsaData;
	WSAStartup( MAKEWORD(2,0), &wsaData );

	char host_name[128];
	if( ::gethostname( host_name, 128 ) )
	{
		WSACleanup();
		sError = "Couldn't get host name";
		return false;
	}

	struct hostent *host_entry = gethostbyname( host_name );
	if( host_entry == NULL )
	{
		WSACleanup();
		sError = "Couldn't get host entry";
		return false;
	}

	sIP = inet_ntoa(*(struct in_addr *)host_entry->h_addr_list);
	WSACleanup();

	// not yet implemented
	sNetmask = "Unknown";

	return true;
}


void ArchHooks_Win32::EnterTimeCriticalSection()
{
	TimeCritMutex->Lock();

	OldThreadPriority = GetThreadPriority( GetCurrentThread() );
	SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL );
}

void ArchHooks_Win32::ExitTimeCriticalSection()
{
	SetThreadPriority( GetCurrentThread(), OldThreadPriority );
	OldThreadPriority = 0;
	TimeCritMutex->Unlock();
}

void ArchHooks_Win32::SetTime( tm newtime )
{
	SYSTEMTIME st;
	ZERO( st );
	st.wYear = (WORD)newtime.tm_year+1900;
    st.wMonth = (WORD)newtime.tm_mon+1;
    st.wDay = (WORD)newtime.tm_mday;
    st.wHour = (WORD)newtime.tm_hour;
    st.wMinute = (WORD)newtime.tm_min;
    st.wSecond = (WORD)newtime.tm_sec;
    st.wMilliseconds = 0;
	SetLocalTime( &st ); 
}

void ArchHooks_Win32::BoostPriority()
{
	/* We just want a slight boost, so we don't skip needlessly if something happens
	 * in the background.  We don't really want to be high-priority--above normal should
	 * be enough.  However, ABOVE_NORMAL_PRIORITY_CLASS is only supported in Win2000
	 * and later. */
	OSVERSIONINFO version;
	version.dwOSVersionInfoSize=sizeof(version);
	if( !GetVersionEx(&version) )
	{
		LOG->Warn( werr_ssprintf(GetLastError(), "GetVersionEx failed") );
		return;
	}

#ifndef ABOVE_NORMAL_PRIORITY_CLASS
#define ABOVE_NORMAL_PRIORITY_CLASS 0x00008000
#endif

	DWORD pri = HIGH_PRIORITY_CLASS;
	if( version.dwMajorVersion >= 5 )
		pri = ABOVE_NORMAL_PRIORITY_CLASS;

	/* Be sure to boost the app, not the thread, to make sure the
	 * sound thread stays higher priority than the main thread. */
	SetPriorityClass( GetCurrentProcess(), pri );
}

void ArchHooks_Win32::UnBoostPriority()
{
	SetPriorityClass( GetCurrentProcess(), NORMAL_PRIORITY_CLASS );
}

static bool g_bTimerInitialized;
static DWORD g_iStartTime;

static void InitTimer()
{
	if( g_bTimerInitialized )
		return;
	g_bTimerInitialized = true;

	timeBeginPeriod( 1 );
	g_iStartTime = timeGetTime();
}

int64_t ArchHooks::GetMicrosecondsSinceStart( bool bAccurate )
{
	if( !g_bTimerInitialized )
		InitTimer();

	int64_t ret = (timeGetTime() - g_iStartTime) * int64_t(1000);
	if( bAccurate )
	{
		ret = FixupTimeIfLooped( ret );
		ret = FixupTimeIfBackwards( ret );
	}
	
	return ret;
}

/*
 * (c) 2003-2004 Glenn Maynard, Chris Danford
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
