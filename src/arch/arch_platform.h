#ifndef ARCH_PLATFORM_H
#define ARCH_PLATFORM_H

// In here, you define which APIs are guaranteed to be available on which OSes.
// Don't ever actually #include anything here -- that's for */Selector_*.h.
#if defined(UNIX) && !defined(DARWIN)
#define HAVE_UNIX
#endif
#if defined(DARWIN)
#define HAVE_DARWIN
#define HAVE_COCOA
#define HAVE_COREAUDIO
#define HAVE_SDL
#define HAVE_CRYPTOPP
#endif
#if defined(_WINDOWS)
#define HAVE_DIRECTX
#define HAVE_FFMPEG
#define HAVE_WIN32 // XXX: Better name for this API?
#define HAVE_CRYPTOPP
#endif
#if defined(_XBOX)
#define HAVE_DIRECTX
#define HAVE_XBOX // XXX: Better name for this API?
#endif
#if defined(LINUX)
#define HAVE_UNIX	// Here just to be explicit.
#define HAVE_LINUXKERNEL
#define HAVE_CRYPTOPP
#endif

#endif

/*
 * (c) 2002-2005 Glenn Maynard, Ben Anderson
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
