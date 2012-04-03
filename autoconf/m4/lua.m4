AC_DEFUN([SM_LUA], [

AC_CHECK_PROGS(LUA_CONFIG, [lua-config50 lua-config], "")
if test "$LUA_CONFIG" != ""; then
	LUA_CFLAGS=`$LUA_CONFIG --include`
	LUA_LIBS=`$LUA_CONFIG --static`

	old_LIBS=$LIBS
	LIBS="$LIBS $LUA_LIBS"
	
	AC_CHECK_FUNC(lua_open, , LUA_MISSING=yes)

	# If lua-config exists, we should at least have Lua; if it fails to build,
	# something other than it not being installed is wrong.
	if test "$LUA_MISSING" = "yes"; then
		echo
		echo "*** $LUA_CONFIG was found, but a Lua test program failed to build."
		echo "*** Please check your installation."
		exit 1;
	fi
	AC_CHECK_FUNC(luaopen_base, , LUA_LIB_MISSING=yes)

	LIBS="$old_LIBS"
else
	if test "$LIB_LUA" = ""; then
		AC_CHECK_LIB(lua, lua_open, LIB_LUA=-llua)
	fi
	if test "$LIB_LUA" = ""; then
		AC_CHECK_LIB(lua, luaL_newstate, LIB_LUA=-llua)
		if test "$LIB_LUA" != ""; then
			AC_DEFINE([HAVE_LUA51], [1], [The system has Lua 5.1 instead of Lua 5.0])
		fi
	fi
	if test "$LIB_LUA" = ""; then
		AC_CHECK_LIB(lua50, lua_open, LIB_LUA=-llua50)
	fi
	if test "$LIB_LUA" = ""; then
		LUA_MISSING=yes
	fi

	# TODO: lualib50 much?

	LUA_CFLAGS=
	LUA_LIBS="$LIB_LUA"
fi
if test "$LUA_MISSING" = "yes"; then
	LUA_CFLAGS=
	LUA_LIBS=
	#PKG_PROG_PKG_CONFIG
	#PKG_CHECK_MODULES(LUA, lua >= 5.1, [LUA_MISSING=no])

	if test "$LUA_MISSING" = "yes"; then
		## http://lua-users.org/lists/lua-l/2006-07/msg00329.html
		AC_CHECK_LIB(lua5.1, luaL_newstate, LUA_MISSING=no)
		if test "$LUA_MISSING" = "no"; then
			LUA_LIBS=-llua5.1
			if test -d /usr/include/lua5.1; then
				LUA_CFLAGS="-I/usr/include/lua5.1"
			fi
		fi
	fi

	if test "$LUA_MISSING" = "yes"; then
		echo
		echo "*** liblua is required to build StepMania; please make sure that"
		echo "*** it is installed to continue the installation process."
		exit 1
	else
		AC_DEFINE([HAVE_LUA51], [1], [The system has Lua 5.1 instead of Lua 5.0])
	fi
fi

AC_SUBST(LUA_CFLAGS)
AC_SUBST(LUA_LIBS)

])
