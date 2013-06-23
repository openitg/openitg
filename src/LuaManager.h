#ifndef LUA_MANAGER_H
#define LUA_MANAGER_H

struct lua_State;
typedef lua_State Lua;
typedef void (*RegisterWithLuaFn)(lua_State*);
class RageMutex;

#ifdef HAVE_LUA51
#include <lua.hpp>
#else
extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}
#endif

class LuaManager;
extern LuaManager *LUA;

namespace LuaHelpers
{
	template<class T>
	void Push( T *pObject, lua_State *L );

	void Push( const bool &Object, lua_State *L );
	void Push( const float &Object, lua_State *L );
	void Push( const int &Object, lua_State *L );
	void Push( const CString &Object, lua_State *L );


	bool FromStack( bool &Object, int iOffset, lua_State *L );
	bool FromStack( float &Object, int iOffset, lua_State *L );
	bool FromStack( int &Object, int iOffset, lua_State *L );
	bool FromStack( CString &Object, int iOffset, lua_State *L );

	template<class T>
	void ReadArrayFromTable( vector<T> &aOut, lua_State *L );
	template<class T>
	void PushStack( const T &val, lua_State *L );
	template<class T>
	bool PopStack( T &val, lua_State *L );
	template<class T>
	void CreateTableFromArray( const vector<T> &aIn, lua_State *L );
};

class LuaManager
{
public:
	// Every Actor should register its class at program initialization.
	static void Register( RegisterWithLuaFn pfn );

	LuaManager();
	~LuaManager();

	Lua *Get();
	void Release( Lua *&p );

	/* Reset the environment, freeing any globals left over by previously executed scripts. */
	void ResetState();

	void SetGlobal( const CString &sName, int val );
	void SetGlobal( const CString &sName, bool val );
	void SetGlobal( const CString &sName, const CString &val );
	void UnsetGlobal( const CString &sName );
private:
	lua_State *L;

	RageMutex *m_pLock;

	/* Register all subscribing types.  There's no harm in registering when already registered. */
	void RegisterTypes();
};


namespace LuaHelpers
{
	/* Run the function with arguments at the top of the stack, with the given
	 * number of arguments.  The specified number of return values are left on
	 * the Lua stack.  On error, nils are left on the stack, sError is set and
	 * false is returned. */
	bool RunScriptOnStack( Lua *L, CString &sError, int iArgs = 0, int iReturnValues = 0 );

	/* Run a script with the given name.  Return values are left on the Lua stack.
	 * Returns false on error, with sError set. */
	bool RunScript( Lua *L, const CString &sScript, const CString &sName, CString &sError, int iReturnValues = 0 );

	/* Convenience: run a script with one return value, displaying an error on failure.
	 * The return value is left on the Lua stack. */
	bool RunScript( Lua *L, const CString &sExpression, const CString &sName = "", int iReturnValues = 0 );

	bool RunScriptFile( const CString &sFile );

	/* Strip "//" comments and "+". */
	void PrepareExpression( CString &sInOut );

	/* Create a Lua array (a table with indices starting at 1) of the given vector,
	 * and push it on the stack. */
	void CreateTableFromArrayB( Lua *L, const vector<bool> &aIn );

	/* Read the table at the top of the stack back into a vector. */
	void ReadArrayFromTableB( Lua *L, vector<bool> &aOut );

	/* Run an expression in the global environment, returning the given type. */
	bool RunExpressionB( const CString &str );
	float RunExpressionF( const CString &str );
	int RunExpressionI( const CString &str );
	bool RunExpressionS( const CString &str, CString &sOut );

	/* If sStr begins with @, evaluate the rest as an expression and store the result over sStr. */
	bool RunAtExpressionS( CString &sStr );

	template<class T>
	void ReadArrayFromTable( vector<T> &aOut, lua_State *L )
	{
		luaL_checktype( L, -1, LUA_TTABLE );

		unsigned iCount = luaL_getn( L, -1 );

		for( unsigned i = 0; i < iCount; ++i )
		{
			lua_rawgeti( L, -1, i+1 );
			T value = T();
			LuaHelpers::FromStack( value, -1, L );
			aOut.push_back( value );
			lua_pop( L, 1 );
		}
	}
	template<class T>
	void PushStack( const T &val, lua_State *L )
	{
		LuaHelpers::Push( val, L );
	}
	template<class T>
	bool PopStack( T &val, lua_State *L )
	{
		bool bRet = LuaHelpers::FromStack( val, -1, L );
		lua_pop( L, 1 );
		return bRet;
	}
	template<class T>
	void CreateTableFromArray( const vector<T> &aIn, lua_State *L )
	{
		lua_newtable( L );
		for( unsigned i = 0; i < aIn.size(); ++i )
		{
			LuaHelpers::Push( aIn[i], L );
			lua_rawseti( L, -2, i+1 );
		}
	}

	int TypeError( Lua *L, int narg, const char *tname );
}


#define REGISTER_WITH_LUA_FUNCTION( Fn ) \
	class Register##Fn { public: Register##Fn() { LuaManager::Register( Fn ); } }; \
	static Register##Fn register##Fn;

#define SArg(n) (luaL_checkstring(L,n))
#define IArg(n) (luaL_checkint(L,n))
#define BArg(n) (MyLua_checkboolean(L,n))
#define FArg(n) ((float) luaL_checknumber(L,n))

#endif

/*
 * (c) 2004 Glenn Maynard
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
