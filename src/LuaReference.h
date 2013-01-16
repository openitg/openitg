/* LuaReference - a self-cleaning Lua reference. */

#ifndef LUA_REFERENCE_H
#define LUA_REFERENCE_H

struct lua_State;
typedef lua_State Lua;

class LuaReference
{
public:
	LuaReference();
	virtual ~LuaReference();

	/* Copying a reference makes a new reference pointing to the same object. */
	LuaReference( const LuaReference &cpy );
	LuaReference &operator=( const LuaReference &cpy );

	/* Create a reference pointing to the item at the top of the stack, and pop
	 * the stack. */
	void SetFromStack( Lua *L );
	void SetFromNil();

	/* Push the referenced object onto the stack.  If not set (or set to nil), push nil. */
	virtual void PushSelf( Lua *L ) const;

	/* Return true if set.  (SetFromNil() counts as being set.) */
	bool IsSet() const;
	bool IsNil() const;
	void Unset() { Unregister(); }

	/* Return the referenced type, or LUA_TNONE if not set. */
	int GetLuaType() const;

	static void BeforeResetAll();	// call this before resetting Lua
	static void AfterResetAll();	// call this after resetting Lua

protected:
	/* If this object needs to store state to recreate itself, overload this. */
	virtual void BeforeReset() { }

	/* If this object is able to recreate itself, overload this, and ReRegisterAll
	 * will work; the reference will still exist after a theme change.  If not
	 * implemented, the reference will be unset after ReRegister. */
	virtual void Register() { }

private:
	void Unregister();
	void ReRegister();
	int m_iReference;
};

/* Evaluate an expression that returns an object; store the object in a reference.
 * For example, evaluating "{ 1, 2, 3 }" will result in a reference to a table. */
class LuaExpression: public LuaReference
{
public:
	LuaExpression( const CString &sExpression = "", bool bSandbox = false )
	{
		m_bSandboxed = bSandbox;
		if( sExpression != "" ) SetFromExpression( sExpression );
	}

	void SetFromExpression( const CString &sExpression );
	const CString& GetExpression() const { return m_sExpression; }
protected:
	virtual void Register();

private:
	CString m_sExpression;
	bool m_bSandboxed;
};

/* Reference a trivially restorable Lua object (any object that Serialize can handle).
 * The object will be saved and restored across Lua resets. */
class LuaData: public LuaReference
{
public:
	virtual CString Serialize() const;
	virtual void LoadFromString( const CString &s, bool bSandbox = false );

protected:
	virtual void BeforeReset();
	virtual void Register();

	CString m_sSerializedData;
	bool m_bWasSet;
	bool m_bSandboxed;
};

class LuaTable: public LuaData
{
public:
	LuaTable();

	/* Set a key by the given name to a value on the stack, and pop the value
	 * off the stack. */
	void Set( Lua *L, const CString &sKey );

	/* Unset the given key (set it to nil). */
	void Unset( Lua *L, const CString &sKey );

	/* Set a key on the stack to a value on the stack; push the key first.  Pop
	 * both the key and the value off the stack. */
	void SetKeyAndValue( Lua *L );
};

#endif

/*
 * (c) 2005 Glenn Maynard, Chris Danford
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
