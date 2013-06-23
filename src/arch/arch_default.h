#ifndef ARCH_DEFAULT_H
#define ARCH_DEFAULT_H

/* Define the default driver sets. */
#if defined(WINDOWS)
#include "ArchHooks/ArchHooks_Win32.h"
#include "LoadingWindow/LoadingWindow_Win32.h"
#include "LowLevelWindow/LowLevelWindow_Win32.h"
#include "MemoryCard/MemoryCardDriverThreaded_Windows.h"
#define DEFAULT_INPUT_DRIVER_LIST "DirectInput,Pump,Para"
#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,DShow,Null"
#define DEFAULT_SOUND_DRIVER_LIST "DirectSound-sw,WDMKS,WaveOut,Null"


#elif defined(MACOSX)
#include "ArchHooks/ArchHooks_MacOSX.h"
#include "LoadingWindow/LoadingWindow_MacOSX.h"
#include "LowLevelWindow/LowLevelWindow_MacOSX.h"
#define DEFAULT_INPUT_DRIVER_LIST "SDL"
#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,Null"
#define DEFAULT_SOUND_DRIVER_LIST "CoreAudio,Null"


#elif defined(_XBOX)
#include "ArchHooks/ArchHooks_Xbox.h"
#include "LoadingWindow/LoadingWindow_Xbox.h"
#include "LowLevelWindow/LowLevelWindow_Win32.h"
#define DEFAULT_INPUT_DRIVER_LIST "Xbox"
#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,DShow,Null"
#define DEFAULT_SOUND_DRIVER_LIST "DirectSound,DirectSound-sw,Null"


#elif defined(UNIX)
#include "ArchHooks/ArchHooks_Unix.h"
#include "LowLevelWindow/LowLevelWindow_X11.h"

#if defined(LINUX)
#include "MemoryCard/MemoryCardDriverThreaded_Linux.h"
#endif

#if defined(HAVE_GTK)
#include "LoadingWindow/LoadingWindow_Gtk.h"
#endif
#if defined(LINUX)
#define DEFAULT_INPUT_DRIVER_LIST "X11,Joystick"
#else
#define DEFAULT_INPUT_DRIVER_LIST "X11"
#endif
#define DEFAULT_MOVIE_DRIVER_LIST "FFMpeg,Null"
#define DEFAULT_SOUND_DRIVER_LIST "ALSA-sw,OSS,Null"
#else
#error Which arch?
#endif

/* All use these. */
#include "LoadingWindow/LoadingWindow_Null.h"
#include "MemoryCard/MemoryCardDriver_Null.h"

#endif

/*
 * (c) 2002-2006 Glenn Maynard, Ben Anderson, Steve Checkoway
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
