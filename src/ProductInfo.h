/* ProductInfo - Branding strings. */

#ifndef PRODUCT_INFO_H
#define PRODUCT_INFO_H

#ifdef LINUX
#include "config.h" // for ITG_ARCADE
#endif

/* The name of the build and its current version */
#define PRODUCT_NAME "OpenITG"
#define PRODUCT_VER "alpha 6 DEV"

/* Build date, in MMDDYYYY - used for serials */
#define PRODUCT_BUILD_DATE "05042008"

#if defined(ITG_ARCADE)
#define PRODUCT_PLATFORM "AC"
#elif defined(XBOX)
/* Not likely at all, but might as well be ready for it. */
#define PRODUCT_PLATFORM "CS" 
#else
#define PRODUCT_PLATFORM "PC"
#endif

// Don't forget to also change ProductInfo.inc!
#define PRODUCT_NAME_VER PRODUCT_NAME " " PRODUCT_PLATFORM " " PRODUCT_VER

/* A central location from which we can update crash handler data... */
#define CRASH_REPORT_URL "http://boxorroxors.net/forum/viewtopic.php?t=713"

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

