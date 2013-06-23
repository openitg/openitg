/* ProductInfo - Branding strings. */

#ifndef PRODUCT_INFO_H
#define PRODUCT_INFO_H

#ifdef LINUX
#include "config.h" // for ITG_ARCADE
#endif

#undef OFFICIAL_RELEASE
//#define OFFICIAL_RELEASE 1

/* (x?)xyy - x is release type, y is release version */
/* 101 - beta 1, 102 - beta 1.1, 103 - beta 2, 104 - beta 3 */
#define PRODUCT_TOKEN 104

/* The name of the build and its current version */
#define PRODUCT_NAME "OpenITG"
#define PRODUCT_VER "beta 3"

#if defined(ITG_ARCADE)
#define PRODUCT_PLATFORM "AC"
#elif defined(XBOX)
#define PRODUCT_PLATFORM "CS" 
#else
#define PRODUCT_PLATFORM "PC"
#endif

#ifndef OFFICIAL_RELEASE
#define PRODUCT_NAME_VER PRODUCT_NAME " " PRODUCT_PLATFORM " " PRODUCT_VER " DEV"
#else
#define PRODUCT_NAME_VER PRODUCT_NAME " " PRODUCT_PLATFORM " " PRODUCT_VER
#endif

/* A central location from which we can update crash handler data... */
#define CRASH_REPORT_URL "https://sourceforge.net/tracker2/?atid=1110556&group_id=239714"

#endif

/*
 * (c) 2003-2009 Chris Danford, BoXoRRoXoRs
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

