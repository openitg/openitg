#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageThreads.h"
#include "StepMania.h"

#include "ArchHooks_Unix.h"
#include "archutils/Unix/SignalHandler.h"
#include "archutils/Unix/GetSysInfo.h"
#include "archutils/Unix/LinuxThreadHelpers.h"
#include "archutils/Unix/EmergencyShutdown.h"
#include "archutils/Unix/AssertionHandler.h"

#include <sys/io.h>
#include <sys/time.h>
#include <sys/reboot.h>
#include <sys/resource.h>
#include <sys/io.h>
#include <unistd.h>
#include <cerrno>
#include <mntent.h>

extern "C"
{
// Include Unix/Linux networking types
#include <ifaddrs.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// Include statvfs, for disk space info
#include <sys/statvfs.h>
}


#if defined(CRASH_HANDLER)
#include "archutils/Unix/CrashHandler.h"
#endif

int64_t ArchHooks_Unix::m_iStartTime = 0;

static bool IsFatalSignal( int signal )
{
	switch( signal )
	{
	case SIGINT:
	case SIGTERM:
	case SIGHUP:
		return false;
	default:
		return true;
	}
}

static bool IsReadOnlyMountPoint( const CString &mountPoint )
{
	CHECKPOINT;
	struct mntent *ent;
	bool found = false;
	bool isReadOnly = false;
	
	FILE *aFile;
	aFile = setmntent( "/proc/mounts", "r" );
	if( aFile == NULL )
	{
		LOG->Warn( "Can't open /proc/mounts to determine if " + mountPoint + " is a readonly filesystem mountpoint" );
		return false;
	}
	
	while( (ent = getmntent( aFile )) != NULL )
	{
		CString mountDir = ent->mnt_dir;
		
		if( mountDir == mountPoint )
		{
			found = true;
			isReadOnly = hasmntopt( ent, "ro" ) != NULL;
			break;
		}
	}
	
	endmntent( aFile );
	return found && isReadOnly;
}

void ArchHooks_Unix::MountInitialFilesystems( const CString &sDirOfExecutable )
{
	/* Mount the root filesystem, so we can read files in /proc, /etc, and so on.
	 * This is /rootfs, not /root, to avoid confusion with root's home directory. */
	FILEMAN->Mount( "dirro", "/", "/rootfs" );

	/* Mount /proc, so Alsa9Buf::GetSoundCardDebugInfo() and others can access it.
	 * (Deprecated; use rootfs.) */
	FILEMAN->Mount( "dirro", "/proc", "/proc" );

	/* FileDB cannot accept relative paths, so Root must be absolute */
	/* using DirOfExecutable for now  --infamouspat */
	CString Root = sDirOfExecutable;

/*	XXX - do we really need this?

struct stat st;
	if( Root == "" && !stat( sDirOfExecutable + "/Songs", &st ) && st.st_mode&S_IFDIR )
		Root = sDirOfExecutable;
	if( Root == "" && !stat( InitialWorkingDirectory + "/Songs", &st ) && st.st_mode&S_IFDIR )
		Root = InitialWorkingDirectory;
*/

#ifdef ITG_ARCADE
	/* Stock ITG2 filesystem configuration has /stats and /itgdata as their own xfs partitions.
	 * /stats is writable and /itgdata is readonly.
	 * Because disk space is sparse in /stats, OpenITG depends on /itgdata to be writable aswell.
	 * Primarily userpacks and cache is stored by OpenITG in /itgdata.
	 * Some users may not have configured /itgdata as its own partition, but rather just a directory.
	 * For them, assume its writable. */
	if( IsReadOnlyMountPoint( "/itgdata" ) )
		system( "mount -o remount,rw /itgdata" );

	/* ITG-specific arcade paths */
	FILEMAN->Mount( "prb", "/itgdata", "/Packages" );
	FILEMAN->Mount( "dir", "/stats", "/Data" );

	/* OpenITG-specific arcade paths */
	FILEMAN->Mount( "dir", "/itgdata/UserPacks", "/UserPacks" );
	FILEMAN->Mount( "dir", "/itgdata/cache-sink", "/Cache" );
#else
	/* OpenITG-specific paths */
	FILEMAN->Mount( "oitg", Root + "/CryptPackages", "/Packages" );

	/*
	* Mount an OpenITG root in the home directory.
	* This is where custom data (songs, themes, etc) should go. 
	* Any files OpenITG tries to modify will be written here.
	*/
	CString home = CString( getenv( "HOME" ) ) + "/";
	FILEMAN->Mount( "dir", home + ".openitg", "/" );

	/* This mounts everything else, including Cache, Data, UserPacks, etc. */
	FILEMAN->Mount( "dir", Root, "/" );
#endif // ITG_ARCADE
}

static void GetDiskSpace( const CString &sDir, uint64_t *pSpaceFree, uint64_t *pSpaceTotal )
{
	CString sResolvedDir = FILEMAN->ResolvePath( sDir );

	struct statvfs fsdata;
	if( statvfs(sResolvedDir.c_str(), &fsdata) != 0 )
	{
		LOG->Warn( "GetDiskSpace(): statvfs() failed: %s", strerror(errno) );
		return;
	}

	// block size * blocks available to user
	if ( pSpaceFree )
		*pSpaceFree = uint64_t(fsdata.f_bsize) * uint64_t(fsdata.f_bavail);

	// fragment size * blocks on the FS
	if( pSpaceTotal )
		*pSpaceTotal = uint64_t(fsdata.f_frsize) * uint64_t(fsdata.f_blocks);
}

uint64_t ArchHooks_Unix::GetDiskSpaceFree( const CString &sDir )
{
	uint64_t iSpaceFree = 0;
	GetDiskSpace( sDir, &iSpaceFree, NULL );
	return iSpaceFree;
}

uint64_t ArchHooks_Unix::GetDiskSpaceTotal( const CString &sDir )
{
	uint64_t iSpaceTotal = 0;
	GetDiskSpace( sDir, NULL, &iSpaceTotal );
	return iSpaceTotal;
}

bool ArchHooks_Unix::OpenMemoryRange( unsigned short start_port, unsigned short bytes )
{
	LOG->Trace( "ArchHooks_Unix::OpenMemoryRange( %#x, %d )", start_port, bytes );

	int ret = iopl(3);

	if( ret != 0 )
		LOG->Warn( "OpenMemoryRange(): iopl error: %s", strerror(errno) );

	return (ret == 0);
}

void ArchHooks_Unix::CloseMemoryRange( unsigned short start_port, unsigned short bytes )
{
	if( (start_port+bytes) <= 0x3FF )
	{
		if( ioperm( start_port, bytes, 0 ) != 0 )
			LOG->Warn( "CloseMemoryRange(): ioperm error: %s", strerror(errno) );

		return;
	}

	if( iopl(0) != 0 )
		LOG->Warn( "CloseMemoryRange(): iopl error: %s", strerror(errno) );
}

bool ArchHooks_Unix::GetNetworkAddress( CString &sIP, CString &sNetmask, CString &sError )
{
	struct ifaddrs *ifaces;

	if ( getifaddrs(&ifaces) != 0 )
	{
		sError = "Network interface error (getifaddrs() failed)";
		return false;
	}

	for ( struct ifaddrs *iface = ifaces; iface; iface = iface->ifa_next )
	{
		// 0x1000 = uses broadcast
		if ((iface->ifa_flags & 0x1000) == 0 || (iface->ifa_addr->sa_family != AF_INET))
			continue;

		struct sockaddr_in *sad = NULL;
		struct sockaddr_in *snm = NULL;

		sad = (struct sockaddr_in *)iface->ifa_addr;
		snm = (struct sockaddr_in *)iface->ifa_netmask;
		sIP = inet_ntoa(((struct sockaddr_in *)sad)->sin_addr);
		sNetmask = inet_ntoa(((struct sockaddr_in *)snm)->sin_addr);
	}

	freeifaddrs(ifaces);

	if( sIP.empty() && sNetmask.empty() )
	{
		sError = "Networking interface disabled";
		return false;
	}

	return true;
}

void ArchHooks_Unix::SystemReboot( bool bForceSync )
{
#ifdef ITG_ARCADE
	/* Important: flush to disk first */
	if( bForceSync )
	{
		sync();
		sleep(5);
	}

	/* If /tmp/no-crash-reboot exists, don't reboot. Just exit. */
	if( !IsAFile("/rootfs/tmp/no-crash-reboot") )
		if( reboot(RB_AUTOBOOT) != 0 )
			LOG->Warn( "Could not reboot: %s", strerror(errno) );
#endif

	// Should we try to develop a RestartProgram for Unix?
	ExitGame();
}

void ArchHooks_Unix::BoostThreadPriority()
{
	if( setpriority(PRIO_PROCESS, 0, -15) != 0 )
		LOG->Warn( "BoostThreadPriority failed: %s", strerror(errno) );
}

void ArchHooks_Unix::UnBoostThreadPriority()
{
	if( setpriority(PRIO_PROCESS, 0, 0) != 0 )
		LOG->Warn( "UnBoostThreadPriority failed: %s", strerror(errno) );
}

static void DoCleanShutdown( int signal, siginfo_t *si, const ucontext_t *uc )
{
	if( IsFatalSignal(signal) )
		return;

	/* ^C. */
	ExitGame();
}

#if defined(CRASH_HANDLER)
static void DoCrashSignalHandler( int signal, siginfo_t *si, const ucontext_t *uc )
{
        /* Don't dump a debug file if the user just hit ^C. */
	if( !IsFatalSignal(signal) )
		return;

	CrashSignalHandler( signal, si, uc );
}
#endif

static void EmergencyShutdown( int signal, siginfo_t *si, const ucontext_t *uc )
{
	if( !IsFatalSignal(signal) )
		return;

	DoEmergencyShutdown();

#if defined(CRASH_HANDLER)
	/* If we ran the crash handler, then die. */
	kill( getpid(), SIGKILL );
#else
	/* We didn't run the crash handler.  Run the default handler, so we can dump core. */
	SignalHandler::ResetSignalHandlers();
	raise( signal );
#endif
}
	
#if defined(HAVE_TLS)
static thread_local int g_iTestTLS = 0;

static int TestTLSThread( void *p )
{
	g_iTestTLS = 2;
	return 0;
}

static void TestTLS()
{
	/* TLS won't work on older threads libraries, and may crash. */
	if( !UsingNPTL() )
		return;

	/* TLS won't work on older Linux kernels.  Do a simple check. */
	g_iTestTLS = 1;

	RageThread TestThread;
	TestThread.SetName( "TestTLS" );
	TestThread.Create( TestTLSThread, NULL );
	TestThread.Wait();

	if( g_iTestTLS == 1 )
		RageThread::SetSupportsTLS( true );
}
#endif

static int64_t GetMicrosecondsSinceEpoch()
{
	struct timeval tv;
	gettimeofday( &tv, NULL );

	return int64_t(tv.tv_sec) * 1000000 + int64_t(tv.tv_usec);
}

int64_t ArchHooks::GetMicrosecondsSinceStart( bool bAccurate )
{
	if( ArchHooks_Unix::m_iStartTime == 0 )
    	        ArchHooks_Unix::m_iStartTime = GetMicrosecondsSinceEpoch();

	int64_t ret = GetMicrosecondsSinceEpoch() - ArchHooks_Unix::m_iStartTime;
	if( bAccurate )
		ret = FixupTimeIfBackwards( ret );
	return ret;
}

ArchHooks_Unix::ArchHooks_Unix()
{
	/* First, handle non-fatal termination signals. */
	SignalHandler::OnClose( DoCleanShutdown );

#if defined(CRASH_HANDLER)
	CrashHandlerHandleArgs( g_argc, g_argv );
	InitializeCrashHandler();
	SignalHandler::OnClose( DoCrashSignalHandler );
#endif

	/* Set up EmergencyShutdown, to try to shut down the window if we crash.
	 * This might blow up, so be sure to do it after the crash handler. */
	SignalHandler::OnClose( EmergencyShutdown );

	InstallExceptionHandler();
	
#if defined(HAVE_TLS)
	TestTLS();
#endif
}

#ifndef _CS_GNU_LIBC_VERSION
#define _CS_GNU_LIBC_VERSION 2
#endif

static CString LibcVersion()
{	
	char buf[1024] = "(error)";
	int ret = confstr( _CS_GNU_LIBC_VERSION, buf, sizeof(buf) );
	if( ret == -1 )
		return "(unknown)";

	return buf;
}

void ArchHooks_Unix::DumpDebugInfo()
{
	CString sys;
	int vers;
	GetKernel( sys, vers );
	LOG->Info( "OS: %s ver %06i", sys.c_str(), vers );

#if defined(CRASH_HANDLER)
	LOG->Info( "Crash backtrace component: %s", BACKTRACE_METHOD_TEXT );
	LOG->Info( "Crash lookup component: %s", BACKTRACE_LOOKUP_METHOD_TEXT );
#if defined(BACKTRACE_DEMANGLE_METHOD_TEXT)
	LOG->Info( "Crash demangle component: %s", BACKTRACE_DEMANGLE_METHOD_TEXT );
#endif
#endif

	LOG->Info( "Runtime library: %s", LibcVersion().c_str() );
	LOG->Info( "Threads library: %s", ThreadsVersion().c_str() );
}

void ArchHooks_Unix::SetTime( tm newtime )
{
	CString sCommand = ssprintf( "date %02d%02d%02d%02d%04d.%02d",
		newtime.tm_mon+1,
		newtime.tm_mday,
		newtime.tm_hour,
		newtime.tm_min,
		newtime.tm_year+1900,
		newtime.tm_sec );

	LOG->Trace( "executing '%s'", sCommand.c_str() ); 
	system( sCommand );

	system( "hwclock --systohc" );
}

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
