/* ProductInfo - Branding strings. */

#ifndef PRODUCT_INFO_H
#define PRODUCT_INFO_H

// I'll make this more elegant later...if set 1,
// the SVN revision is ignored. we can drop "DEV"
// labels from releases and use this now. - Vyhd
#define OFFICIAL_RELEASE 1

#ifdef LINUX
#include "config.h" // for ITG_ARCADE
#include "svnver.h" // for revision number
#define PRODUCT_SVN "r" SVN_VERSION
#endif

/* We need to make this more intelligent. */
#ifdef WIN32
#define SVN_VERSION "SVN"
#define PRODUCT_SVN SVN_VERSION
#endif

/* The name of the build and its current version */
#define PRODUCT_NAME "OpenITG"
#define PRODUCT_VER "prebeta"

#if defined(ITG_ARCADE)
#define PRODUCT_PLATFORM "AC"
#elif defined(XBOX)
#define PRODUCT_PLATFORM "CS" 
#else
#define PRODUCT_PLATFORM "PC"
#endif

// Don't forget to also change ProductInfo.inc!
#ifdef OFFICIAL_RELEASE
#define PRODUCT_NAME_VER PRODUCT_NAME " " PRODUCT_PLATFORM " " PRODUCT_VER
#else
#define PRODUCT_NAME_VER PRODUCT_NAME " " PRODUCT_PLATFORM " " PRODUCT_VER " " PRODUCT_SVN
#endif

/* A central location from which we can update crash handler data... */
#define CRASH_REPORT_URL "http://boxorroxors.net/forum/viewtopic.php?t=971"

#endif

/*
 * (c) 2003-2005 Chris Danford
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

