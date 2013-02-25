#include "global.h" // also pulls in config.h
#include "ProductInfo.h"

#if defined(HAVE_CONFIG_H)
	/* included in global.h */
#elif defined(WIN32)
#include "verinfo.h"
#else
#define BUILD_VERSION "unknown version"
#define BUILD_REV_DATE "unknown date"
#define BUILD_REV_TAG "unknown revision"
#warning No build information is available.
#endif

/*
 * Helpers
 */

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

/*
 * Version strings and getters
 */

#define VERSION_STRING( id, value ) \
	static const CString g_s##id = CString( value ); \
	const CString& ProductInfo::Get##id() { return g_s##id; }

VERSION_STRING( Name, "OpenITG" );
VERSION_STRING( Version, BUILD_VERSION );

VERSION_STRING( BuildDate, BUILD_DATE );
VERSION_STRING( BuildRevision, BUILD_REVISION_TAG );

VERSION_STRING( CrashReportURL, "https://github.com/openitg/openitg/issues" );

VERSION_STRING( FullVersion, g_sName + " " + GetPlatform() + " " + g_sVersion );

#undef VERSION_STRING

/*
 * Lua version helpers
 */

#include "RageLog.h"
#include "LuaManager.h"
#include "EnumHelper.h"

namespace
{
	enum Version
	{
		VERSION_ALPHA,
		VERSION_BETA,
		VERSION_GAMMA,
		VERSION_OMEGA,
		NUM_VERSIONS,
		VERSION_INVALID
	};

	static const CString VersionNames[] =
	{
		"alpha",
		"beta",
		"gamma",
		"omega"
	};

	int ToToken( Version t )
	{
		return t * 100;
	}

	/* This is a dumb implementation to get the transition done;
	 * clean up later. */
	int GetVersionTokenFromBuildInfo()
	{
		CString sVersion = ProductInfo::GetVersion();
		sVersion.MakeLower();

		Version ver = VERSION_INVALID;

		if( sVersion.find("alpha") != CString::npos )
			ver = VERSION_ALPHA;
		else if( sVersion.find("beta") != CString::npos )
			ver = VERSION_BETA;
		else if( sVersion.find("gamma") != CString::npos || sVersion.find("rc") != CString::npos )
			ver = VERSION_GAMMA;
		else
			ver = VERSION_OMEGA;

		unsigned iVersion = 0;

		if( sscanf(sVersion, "%*[A-Za-z] %d", &iVersion) != 1 )
			LOG->Warn( "GetVersionTokenFromBuildInfo(): couldn't parse version string \"%s\"", sVersion.c_str() );

		unsigned iToken = ToToken(ver) + iVersion;
		return iToken;
	}
}

void SetVersionGlobals( lua_State* L )
{
	/* Boolean flag that says this engine is OpenITG */
	LUA->SetGlobal( "OPENITG", true );

	/* Integer flag that allows for new Lua bindings to be used without
	 * breaking older engine builds; see below for tokens and values. */
	LUA->SetGlobal( "OPENITG_VERSION", GetVersionTokenFromBuildInfo() );

	/* Tokens to compare the above values against */
	FOREACH_ENUM( Version, NUM_VERSIONS, v )
	{
		CString s = VersionNames[v];
		s.MakeUpper();
		LUA->SetGlobal( "VERSION_" + s, ToToken(v) );
	}
}

REGISTER_WITH_LUA_FUNCTION( SetVersionGlobals );

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
