#include "StdString.h"
#include "ProductInfo.h"

#if HAVE_CONFIG_H
	#include "config.h"
#else
/* XXX: workaround until the Windows build can pull Git versioning. */
	#define BUILD_VERSION "beta3 DEV"
	#define BUILD_DATE "unknown date"
	#define BUILD_REVISION_TAG "unknown revision"
#endif

#define VERSION_STRING( id, value ) \
	static const CString g_s##id = CString( value ); \
	const CString& ProductInfo::Get##id() { return g_s##id; }

VERSION_STRING( Name, "OpenITG" );
VERSION_STRING( Version, BUILD_VERSION );

VERSION_STRING( BuildDate, BUILD_DATE );
VERSION_STRING( BuildRevision, BUILD_REVISION_TAG );

VERSION_STRING( CrashReportURL, "<to be determined>" );

namespace
{
	const CString GetPlatform()
	{
	#if defined(ITG_ARCADE)
		return "AC";
	#elif defined(XBOX)
		return "CS";
	#else
		return "PC";
	#endif
	}
}

VERSION_STRING( FullVersion, g_sName + " " + GetPlatform() + " " + g_sVersion );

#undef VERSION_STRING

/*
 * (c) 2013 Marc Cannon
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

