#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(itg2-util, 1.0, itgpmc@gmail.com)
AC_CANONICAL_SYSTEM
AC_CONFIG_SRCDIR(src/itg2-decrypt.c)
AM_CONFIG_HEADER(config.h)
AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_RANLIB

GETTEXT_PACKAGE=itg2-util
AC_SUBST(GETTEXT_PACKAGE)
# Checks for libraries.
AC_CHECK_LIB([gcrypt], [gcry_md_hash_buffer])

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h sys/ioctl.h sys/time.h termios.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_HEADER_TIME
AC_HEADER_STDBOOL

AC_ARG_ENABLE(kd-debug, [  --enable-kd-debug             Enable debug printout (key and scratchpad data)],
 	USE_KD_DEBUG="$enableval", USE_KD_DEBUG="no")

if test $USE_KD_DEBUG = yes; then
	AC_DEFINE(KD_DEBUG, [], [Debug printout])
fi

# Checks for library functions.
AC_FUNC_MALLOC
AC_FUNC_MEMCMP
AC_FUNC_SELECT_ARGTYPES
AC_CHECK_FUNCS([gettimeofday memset select])

AC_CONFIG_FILES([Makefile
                 src/Makefile])
AC_OUTPUT
