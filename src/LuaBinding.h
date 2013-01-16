/* Based on Lua Wiki example: http://lua-users.org/wiki/SimplerCppBinding
 * Modified by Chris.
 */

#ifndef LuaBinding_H
#define LuaBinding_H

#ifdef HAVE_LUA51
#include <lua.hpp>
#else
extern "C"
{
#include <lua.h>
#include <lauxlib.h>
}
#endif

#include "LuaManager.h"
#include <vector>

inline bool MyLua_checkboolean (lua_State *L, int numArg)
{
	luaL_checktype(L,numArg,LUA_TBOOLEAN);
	return !!lua_toboolean(L,numArg);
}

template <typename T>
struct RegType
{
	const char *name; 
	int (*mfunc)(T *p, lua_State *L);
};

template <typename T>
class Luna 
{
	typedef struct { T *pT; } userdataType;
public:
	
	static void Register(lua_State *L)
	{
		lua_newtable(L);
		int methods = lua_gettop(L);
		
		luaL_newmetatable(L, s_className);
		int metatable = lua_gettop(L);
		
		// store method table in globals so that
		// scripts can add functions written in Lua.
		lua_pushstring(L, s_className);
		lua_pushvalue(L, methods);
		lua_settable(L, LUA_GLOBALSINDEX);
		
		lua_pushliteral(L, "__metatable");
		lua_pushvalue(L, methods);
		lua_settable(L, metatable);  // hide metatable from Lua getmetatable()
		
		lua_pushliteral(L, "__index");
		lua_pushvalue(L, methods);
		lua_settable(L, metatable);
		
		lua_pushliteral(L, "__tostring");
		lua_pushcfunction(L, tostring_T);
		lua_settable(L, metatable);
		
		lua_pushliteral(L, "__eq");
		lua_pushcfunction(L, equal);
		lua_settable(L, metatable);

		// fill method table with methods from class T
		for (unsigned i=0; s_pvMethods && i<s_pvMethods->size(); i++ )
		{
			const MyRegType *l = &(*s_pvMethods)[i];
			lua_pushstring(L, l->name);
			lua_pushlightuserdata(L, (void*)l);
			lua_pushcclosure(L, thunk, 1);
			lua_settable(L, methods);
		}
		
		lua_pop(L, 2);  // drop metatable and method table
	}

	// get userdata from Lua stack and return pointer to T object
	static T *check( lua_State *L, int narg, bool bIsSelf = false )
	{
		userdataType *pUserdata = static_cast<userdataType*>( luaL_checkudata(L, narg, s_className) );
		if( pUserdata == NULL )
		{
			if( bIsSelf )
				luaL_typerror( L, narg, s_className );
			else
				LuaHelpers::TypeError( L, narg, s_className );
		}

		return pUserdata->pT;  // pointer to T object
	}
	
private:
	
	// member function dispatcher
	static int thunk(lua_State *L) 
	{
		// stack has userdata, followed by method args
		T *obj = check( L, 1, true );  // get self
		lua_remove(L, 1);  // remove self so member function args start at index 1
		// get member function from upvalue
		MyRegType *l = static_cast<MyRegType*>(lua_touserdata(L, lua_upvalueindex(1)));
		return (*(l->mfunc))(obj,L);  // call member function
	}
	
	/* Two objects are equal if the underlying object is the same. */
	static int equal( lua_State *L )
	{
		userdataType *obj1 = static_cast<userdataType*>(lua_touserdata(L, 1));
		userdataType *obj2 = static_cast<userdataType*>(lua_touserdata(L, 2));
		lua_pushboolean( L, obj1->pT == obj2->pT );
		return 1;
	}

public:
	// create a new T object and
	// push onto the Lua stack a userdata containing a pointer to T object
	static int Push(lua_State *L, T* p )
	{
		userdataType *ud = static_cast<userdataType*>(lua_newuserdata(L, sizeof(userdataType)));
		ud->pT = p;  // store pointer to object in userdata
		luaL_getmetatable(L, s_className);  // lookup metatable in Lua registry
		lua_setmetatable(L, -2);
		return 1;  // userdata containing pointer to T object
	}
	
	typedef RegType<T> MyRegType;
	typedef vector<MyRegType> RegTypeVector;
	static RegTypeVector *s_pvMethods;
	
	static void CreateMethodsVector()
	{
		if(s_pvMethods==NULL) 
			s_pvMethods = new RegTypeVector;
	}
private:
	static const char s_className[];
	
	static int tostring_T (lua_State *L)
	{
		char buff[32];
		userdataType *ud = static_cast<userdataType*>(lua_touserdata(L, 1));
		T *obj = ud->pT;
		sprintf(buff, "%p", obj);
		lua_pushfstring(L, "%s (%s)", s_className, buff);
		return 1;
	}
};

#define LUA_REGISTER_CLASS( T ) \
	template<> const char Luna<T>::s_className[] = #T; \
	template<> Luna<T>::RegTypeVector* Luna<T>::s_pvMethods = NULL; \
	static Luna##T<T> registera##T; \
void T::PushSelf( lua_State *L ) { Luna##T<T>::Push( L, this ); } \
/* Call PushSelf, so we always call the derived Luna<T>::Push. */ \
namespace LuaHelpers { template<> void Push( T *pObject, lua_State *L ) { pObject->PushSelf( L ); } }

#define ADD_METHOD( method_name ) \
	{ Luna<T>::CreateMethodsVector(); RegType<T> r = {#method_name,method_name}; Luna<T>::s_pvMethods->push_back(r); }


#endif

/*
 * (c) 2001-2005 lua-users.org, Chris Danford
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
