#include "global.h"
#include "ProductInfo.h"
#include "RageUtil.h"	// for ARRAYLEN
#include "EnumHelper.h"

/* change these as needed between releases */
const BuildVersion BUILD_VERSION = VERSION_BETA;
const unsigned BUILD_REVISION = 3;

XToString( BuildVersion, NUM_BUILD_VERSIONS );

CString ProductInfo::getName() {
	return CString("OpenITG");
}

CString ProductInfo::getVersion() {
#if defined(_WIN32)
	return CString("b3-win32");
#else
	return CString(OITG_VERSION);
#endif
}

CString ProductInfo::getDate() {
#if defined(_WIN32)
	return CString("04-08-2011");
#else
	return CString(OITG_DATE);
#endif
}

CString ProductInfo::getPlatform() {
#if defined(ITG_ARCADE)
	return CString("AC");
#elif defined(XBOX)
	return CString("CS");
#else
	return CString("PC");
#endif
}

CString ProductInfo::getCrashReportUrl() {
	return CString("http://wush.net/bugzilla/terabyte/");
}

CString ProductInfo::getFullVersionString() {
	return getVersion() + "-" + getPlatform();
}

CString ProductInfo::getSerial() {
	return CString("OITG-") + getVersion() + "-" + getPlatform();
}

/* begin Lua bindings */

#include "LuaManager.h"
#include "LuaBinding.h"

static void LuaBuildVersion( lua_State *L )
{
	FOREACH_ENUM( BuildVersion, NUM_BUILD_VERSIONS, ver )
	{
		CString s = BuildVersionToString( ver );
		s.MakeUpper();
		LUA->SetGlobal( "BUILD_VERSION_" + s, ver );
	}
}

REGISTER_WITH_LUA_FUNCTION( LuaBuildVersion );

// backwards compatibility with OPENITG_VERSION token
static unsigned MakeVersionToken( BuildVersion ver, unsigned rev )
{
	return int(ver) * 100 + rev;
}

/* Adds field 'name' with value 'val' to a table on top of the stack. */
static void SetKeyVal( lua_State *L, const char *name, int val )
{
	lua_pushstring( L, name );
	lua_pushnumber( L, val );
	lua_settable( L, -3 );
}

static void LuaVersionInfo( lua_State *L )
{
	/* Create an OpenITG table and stuff our tokens into it. This should
	 * be fine, since the previous implementation was a boolean and the
	 * table will be considered 'true' in a branching context. */

	lua_newtable( L );			// create table
	lua_setglobal( L, "OPENITG" );		// register as global, pop
	lua_getglobal( L, "OPENITG" );		// push back on the stack

	SetKeyVal( L, "TYPE", BUILD_VERSION );	// set our subtable values
	SetKeyVal( L, "REVISION", BUILD_REVISION );

	lua_pop( L, -1 );			// pop the main table

	/* Set the old OPENITG_VERSION token (for compatibility) */
	int token = MakeVersionToken( BUILD_VERSION, BUILD_REVISION );

	lua_pushnumber( L, token );		// push the token
	lua_setglobal( L, "OPENITG_VERSION" );	// set it global, pop

	ASSERT( lua_gettop(L) == 0 );
}

REGISTER_WITH_LUA_FUNCTION( LuaVersionInfo );
