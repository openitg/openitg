#ifndef ARCH_HOOKS_H
#define ARCH_HOOKS_H

/* All ArchHooks require this header for filesystem mounts. */
#include "RageFileManager.h"

class ArchHooks
{
public:
	/* This is called as soon as SDL is set up, the loading window is shown
	 * and we can safely log and throw. */
	virtual void DumpDebugInfo() { }

	virtual ~ArchHooks() { }
	/* This is called once each time through the game loop */
	virtual void Update(float delta) { }

	/* Mount StepMania's virtual file systems. This is called early, so
	 * assume only RageFileManager routines are available here. */
	virtual void MountInitialFilesystems( const CString &sDirOfExecutable );

	/* Re-exec the game.  If this is implemented, it doesn't return. */
	virtual void RestartProgram() { }

	/* Restart the entire computer, in case of installation updates, etc. */
	virtual void SystemReboot( bool bForceSync = true ) { }

	/* Get disk space from the device that contains the given directory.
	 * The path given is in the VFS; we resolve the actual path here. */
	virtual uint64_t GetDiskSpaceTotal( const CString &sDir ) { return 0; }
	virtual uint64_t GetDiskSpaceFree( const CString &sDir ) { return 0; }

	/* Call this to temporarily enter a high-priority or realtime scheduling (depending
	 * on the implementation) mode.  This is used to improve timestamp accuracy.  Do as
	 * little as possible in this mode; hanging in it might hang the system entirely. */
	virtual void EnterTimeCriticalSection() { }
	virtual void ExitTimeCriticalSection() { }

	virtual void SetTime( tm newtime ) { }

	/* Boost the priority of this process. */
	virtual void BoostPriority() { }
	virtual void UnBoostPriority() { }

	/* Boost the priority of the calling thread. */
	virtual void BoostThreadPriority() { }
	virtual void UnBoostThreadPriority() { }

	/* Call this to open access to attempt to claim the given privileged I/O port range. */
	virtual bool OpenMemoryRange( unsigned short start_port, unsigned short bytes ) { return false; }
	virtual void CloseMemoryRange( unsigned short start_port, unsigned short bytes ) { }

	/* Get the IP and netmask, in StdString format, of the system. */
	virtual bool GetNetworkAddress( CString &sIP, CString &sNetmask, CString &sError )
	{
		sError = "Not implemented";
		return false;
	}

	/*
	 * Return the amount of time since the program started.  (This may actually be
	 * since the initialization of HOOKS.
	 *
	 * Full microsecond accuracy may not be available.
	 *
	 * bAccurate is a hint: it specifies whether to prefer short-term precision
	 * or long-term accuracy.  If false, the implementation may give higher resolution
	 * results, but not be as stable over long periods (eg. may drift depending on
	 * clock speed shifts on laptops).  If true, lower precision results (usually with
	 * no less than a 1ms granularity) are returned, but the results should be stable
	 * over long periods of time.
	 *
	 * Note that bAccurate may change the result significantly; it may use a different
	 * timer, and may have a different concept of when the program "started".
	 *
	 * This is a static function, implemented in whichever ArchHooks source is used,
	 * so it can be used at any time (such as in global constructors), before HOOKS
	 * is initialized.
	 *
	 * RageTimer layers on top of this, and attempts to correct wrapping, as the
	 * underlying timers may be 32-bit, but implementations should try to avoid
	 * wrapping if possible.
	 */
	static int64_t GetMicrosecondsSinceStart( bool bAccurate );

private:
	/* This are helpers for GetMicrosecondsSinceStart on systems with a timer
	 * that may loop or move backwards. */
	static uint64_t FixupTimeIfLooped( uint64_t usecs );
	static uint64_t FixupTimeIfBackwards( uint64_t usecs );
};

ArchHooks* MakeArchHooks();

#endif

extern ArchHooks *HOOKS;	// global and accessable from anywhere in our program

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
