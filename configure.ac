# vim: set noet:
# only tested with autoconf 2.57
AC_PREREQ(2.53)

AC_INIT([OpenITG], [1.0])
AC_CONFIG_SRCDIR([src/StepMania.cpp])

AC_CONFIG_AUX_DIR(autoconf)
AC_CANONICAL_TARGET
AM_INIT_AUTOMAKE([subdir-objects])
AM_CONFIG_HEADER([src/config.h])
AM_MAINTAINER_MODE

# We don't want PROG_CC/CXX default settings, but don't ignore explicit settings.
test -z "$CFLAGS"    && DEFAULT_CFLAGS=yes
test -z "$CXXFLAGS"  && DEFAULT_CXXFLAGS=yes
test -z "$LDFLAGS"   && DEFAULT_LDFLAGS=yes

AC_PROG_CC
AC_PROG_CXX
AC_PROG_RANLIB

test "$DEFAULT_CFLAGS" = "yes" && CFLAGS="-Wall -W -Wno-unused -Wno-switch"
test "$DEFAULT_CXXFLAGS" = "yes" && CXXFLAGS="-Wall -W -Wno-unused -Wno-switch"
test "$DEFAULT_LDFLAGS" = "yes" && LDFLAGS=""

compile=release
AC_ARG_WITH(debug, AC_HELP_STRING([--with-debug], [Enable debug mode]), with_debug=$withval, with_debug=no)
AC_ARG_WITH(fast-compile, AC_HELP_STRING([--with-fast-compile], [Enable fast compile]), with_fast_compile=$withval, with_fast_compile=no)
if test "$with_debug" = "yes"; then
	compile=debug
fi
if test "$with_fast_compile" = "yes"; then
	compile=fast
fi

case $compile in
	release)
	test "$DEFAULT_CFLAGS" = "yes" && CFLAGS="$CFLAGS -O3"
	test "$DEFAULT_CXXFLAGS" = "yes" && CXXFLAGS="$CXXFLAGS -O3"
	;;
	debug)
	test "$DEFAULT_CFLAGS" = "yes" && CFLAGS="$CFLAGS -O0 -g3"
	test "$DEFAULT_CXXFLAGS" = "yes" && CXXFLAGS="$CXXFLAGS -O0 -g3"
	;;
	fast)
	test "$DEFAULT_CFLAGS" = "yes" && CFLAGS="$CFLAGS -O2 -fno-inline"
	test "$DEFAULT_CXXFLAGS" = "yes" && CXXFLAGS="$CXXFLAGS -O2 -fno-inline"
	;;
esac

# Define UNIX for all Unix-like systems.  Don't define it for cross-compiling to
# non-Unix-like systems.  (-DUNIX selects the archutils and ArchHooks to use; if
# your platform doesn't use the Unix ones, you probably don't want to define UNIX.)
case $host in
*-linux-*)
	AC_DEFINE(LINUX, 1, [Linux])
	AC_DEFINE(UNIX, 1, [Unix])
	linux=yes
	unix=yes
	;;

*)
	AC_DEFINE(UNIX, 1, [Unix])
	unix=yes
	;;
esac
AM_CONDITIONAL(UNIX, test "$unix" = "yes" )
AM_CONDITIONAL(LINUX, test "$linux" = "yes" )

# Define macros for individual CPU types, for a few bits of inline assembly.
# This is for major, compatible CPU classes--"CPU_X86" includes P2, P3, P4,
# AMD, etc.  If you need CPU-specific assembly, check at runtime--don't create
# separate binaries for each CPU if the binaries themselves are otherwise
# compatible.
case $host in
i?86-*)
	AC_DEFINE(CPU_X86, 1, [x86])
	;;
x86_64-*)
	AC_DEFINE(CPU_X86_64, 1, [x86-64])
	;;
powerpc-*)
	AC_DEFINE(CPU_PPC, 1, [PPC])
	;;
esac


AC_C_BIGENDIAN( 
	AC_DEFINE(ENDIAN_BIG, 1, [Big endian]),
	AC_DEFINE(ENDIAN_LITTLE, 1, [Little endian]),
	AC_MSG_ERROR([Can't determine endianness]) )

AC_ARG_WITH(prof, AC_HELP_STRING([--with-prof], [Enable profiling]), with_prof=$withval, with_prof=no)
if test "$with_prof" = "yes"; then
	test "$DEFAULT_CFLAGS" = "yes" && CFLAGS="$CFLAGS -pg"
	test "$DEFAULT_CXXFLAGS" = "yes" && CXXFLAGS="$CXXFLAGS -pg"
	test "$DEFAULT_LDFLAGS" = "yes" && LDFLAGS="$LDFLAGS -pg"
fi

SM_X_WITH_OPENGL

AC_ARG_ENABLE(sdl, AC_HELP_STRING([--enable-sdl], [Enable SDL support]), enable_sdl=$enableval, enable_sdl=no)
if test "$enable_sdl" = "yes"; then
	AM_PATH_SDL(1.2.6,have_sdl=yes,have_sdl=no)
fi
AM_CONDITIONAL(HAVE_SDL, test "$have_sdl" = "yes")
if test "$have_sdl" = "yes"; then
	AC_DEFINE(HAVE_SDL, 1, [SDL is available])
fi

# sdl-config puts -L/usr/lib in the library search path, which reorders things
# in a way that breaks some configurations.
# Does this still need to be in?
# not sure
# SDL_LIBS="`echo $SDL_LIBS | sed 's_-L/usr/lib/\?[[ $]]__'`"

have_libpng=yes
PNG_LIBS=
PNG_CFLAGS=
AC_CHECK_LIB(png12, png_create_read_struct, have_libpng=yes, have_libpng=no, 
             [-lz -lm])
if test "$have_libpng" = "yes"; then
    png_header_found=yes
    AC_CHECK_HEADER(/usr/include/libpng12/png.h, , png_header_found=no)
    if test "$png_header_found" = "yes"; then
        CXXFLAGS="$CXXFLAGS -I/usr/include/libpng12"
    else
        AC_CHECK_HEADER(png.h, , have_libpng=no)
    fi
    LIBS="$LIBS -lpng12 -lz -lm"
else
    AC_CHECK_LIB(png, png_create_read_struct, have_libpng=yes, have_libpng=no, [-lz -lm]) # x=y to stop autoconf from messing with LIBS
    AC_CHECK_HEADER(png.h, , have_libpng=no)
    LIBS="$LIBS -lpng -lz -lm"
fi
if test "$have_libpng" = "no"; then
  echo
  echo "*** libpng is required to build StepMania; please make sure that"
  echo "*** it is installed to continue the build process."
  exit 1
fi

AC_ARG_WITH(jpeg, AC_HELP_STRING([--without-jpeg], [Disable JPEG support]), with_jpeg=$withval, with_jpeg=yes)

if test "$with_jpeg" = "yes"; then
	have_libjpeg=yes
	AC_CHECK_LIB(jpeg, jpeg_read_scanlines, [x=y], have_libjpeg=no) # x=y to stop autoconf from messing with LIBS
	AC_CHECK_HEADER(jpeglib.h, , have_libjpeg=no)

	if test "$have_libjpeg" = "no"; then
		# Require JPEG by default, so people don't compile without it
		# by accident and then come asking us why files won't load.
		AC_MSG_ERROR(
[libjpeg is required to build StepMania; please make sure that it is installed
to continue the build process.  If you really want to compile without JPEG
support, pass the "--without-jpeg" flag to configure.])
	fi
	have_libjpeg=

	LIBS="$LIBS -ljpeg"
else
	AC_DEFINE(NO_JPEG_SUPPORT, 1, [JPEG support not available])
fi

AC_ARG_WITH(network, AC_HELP_STRING([--without-network], [Disable networking]), with_network=$withval, with_network=yes)
if test "$with_network" = "no"; then
	AC_DEFINE(WITHOUT_NETWORKING, 1, [Networking support not available])
fi
AM_CONDITIONAL(WITHOUT_NETWORKING, test "$with_network" = "no")


AC_ARG_ENABLE(itg-arcade, AC_HELP_STRING([--enable-itg-arcade], [Enable arcade-specific behavior]), itg_arcade=$enableval, itg_arcade=no)
if test "$itg_arcade" = "yes"; then
	AC_DEFINE(ITG_ARCADE, 1, [Use arcade mountpoints and behavior])
fi
AM_CONDITIONAL(ITG_ARCADE, test "$itg_arcade" = "yes")

SM_LUA
SM_ZLIB
SM_AUDIO
SM_TLS
SM_LIBUSB
SM_FFMPEG

# for linux joystick hotplug
have_inotify=no
AC_CHECK_HEADER([sys/inotify.h], 
	have_inotify=yes
	AC_DEFINE(HAVE_INOTIFY, 1, [inotify support in kernel])
,)

# search for system libs if not building for arcade
if test "$itg_arcade" = "no"; then
	AC_CHECK_LIB(resample, resample_open, [have_libresample=yes], [have_libresample=static], [-lm])
	if test "$have_libresample" = "yes"; then
		LIBS="$LIBS -lresample"
	fi
else
	echo "Skipping libresample check due to arcade build"
	have_libresample=static
fi

AM_CONDITIONAL(HAVE_LIBRESAMPLE, test "$have_libresample" = "yes")

# test if processor supports SSE2
have_sse2=no
AC_TRY_RUN([int main() { int a,d; asm ("cpuid":"=a"(a),"=d"(d):"0"(1)); return d & (1 << 26) ? 0 : 1; }], have_sse2=yes, have_sse2=no)

# Don't use SSE2 by default; some GCC versions misoptimise the code, which
# causes Heisenbugs, and we don't want to inflict that pain unnecessarily.
# Only supply it if the user specifically asks.

AC_ARG_WITH(sse2, AC_HELP_STRING([--with-sse2], [Enable SSE2 optimization]), with_sse2=$withval, with_sse2=no)
if test "$with_sse2" = "yes"; then
	CFLAGS="$CFLAGS -msse2"
	CXXFLAGS="$CXXFLAGS -msse2"
fi

SM_CHECK_CRASH_HANDLER

alsa=yes
AM_PATH_ALSA(0.9.0,AC_DEFINE([HAVE_ALSA],1,[Define presence of ALSA]),alsa=no)

AM_ICONV

AC_ARG_ENABLE(gtk2, AC_HELP_STRING([--disable-gtk2], [Disable GTK support]), enable_gtk2=$enableval, enable_gtk2=yes)

if test x$enable_gtk2 = xyes; then
  AM_PATH_GTK_2_0(2.0.0,AC_DEFINE([HAVE_GTK],1,[Define presence of GTK]),enable_gtk2=no)
fi

AC_CHECK_HEADER(sys/soundcard.h, [AC_DEFINE(HAVE_OSS, 1, [OSS support available])])
AC_CHECK_DECL(OSS_GETVERSION, AC_DEFINE([HAVE_OSS_GETVERSION],1,[OSS_GETVERSION is defined]), , [#include <sys/soundcard.h>])
AC_ARG_ENABLE(force-oss, AC_HELP_STRING([--enable-force-oss], [Force OSS]), force_oss=$enableval, force_oss=no)
AC_CHECK_HEADER(stdint.h, , [AC_DEFINE(MISSING_STDINT_H, 1, [stdint.h is missing])])
AC_CHECK_HEADERS([inttypes.h endian.h])

AC_MSG_CHECKING(if cstdlib breaks llabs)
AC_TRY_COMPILE(	[#include <stdlib.h>
		 #include <cstdlib> ],
		[int foo() { return llabs(1); }],
		[AC_MSG_RESULT(no)],
		[AC_MSG_RESULT(yes)
		 AC_DEFINE([NEED_CSTDLIB_WORKAROUND], 1, [cstdlib breaks llabs])]
	      )

AC_CHECK_LIB( pthread, pthread_create, AC_DEFINE([HAVE_LIBPTHREAD],1,[libpthread is available]) )
AC_CHECK_LIB( pthread, pthread_mutex_timedlock, AC_DEFINE([HAVE_PTHREAD_MUTEX_TIMEDLOCK],1,[pthreads has pthread_mutex_timedlock()]) )
AC_CHECK_LIB( pthread, pthread_cond_timedwait, AC_DEFINE([HAVE_PTHREAD_COND_TIMEDWAIT],1,[pthreads has pthread_cond_timedwait()]) )

# Always:
AC_DEFINE(_GNU_SOURCE, 1, [Use GNU extensions])

AM_CONDITIONAL(HAVE_ALSA, test x$alsa != xno )
AM_CONDITIONAL(HAVE_GTK, test "$enable_gtk2" != "no" )
AM_CONDITIONAL(HAVE_OSS, test x$ac_cv_header_sys_soundcard_h = xyes )
AM_CONDITIONAL(USE_CRASH_HANDLER, test "$use_crash_handler" = "yes" )

if test x$force_oss = xyes && test x$ac_cv_header_sys_soundcard_h = xyes; then
	AC_DEFINE([FORCE_OSS], 1, [Force OSS Usage])
fi

AC_CHECK_DECL(powf, , AC_DEFINE([NEED_POWF],1,[Need powf]), [#include <math.h>])
AC_CHECK_DECL(sqrtf, , AC_DEFINE([NEED_SQRTF],1,[Need sqrtf]), [#include <math.h>])
AC_CHECK_DECL(sinf, , AC_DEFINE([NEED_SINF],1,[Need sinf]), [#include <math.h>])
AC_CHECK_DECL(tanf, , AC_DEFINE([NEED_COSF],1,[Need tanf]), [#include <math.h>])
AC_CHECK_DECL(cosf, , AC_DEFINE([NEED_TANF],1,[Need cosf]), [#include <math.h>])
AC_CHECK_DECL(acosf, , AC_DEFINE([NEED_ACOSF],1,[Need acosf]), [#include <math.h>])
AC_CHECK_DECL(roundf, , AC_DEFINE([NEED_ROUNDF],1,[Need roundf]), [#include <math.h>])
AC_CHECK_DECL(truncf, , AC_DEFINE([NEED_TRUNCF],1,[Need truncf]), [#include <math.h>])

# This doesn't work on glibc math functions:
# AC_CHECK_FUNCS([sqrtf sinf tanf cosf acosf roundf truncf])

AC_CHECK_DECLS([SIGPWR],,,[#include <signal.h>])

AC_ARG_ENABLE(tests, AC_HELP_STRING([--enable-tests], [Build test programs]), enable_tests=$enableval, enable_tests=no)
AM_CONDITIONAL(BUILD_TESTS, test "$enable_tests" = "yes" )

AC_CONFIG_FILES(Makefile)
AC_CONFIG_FILES(src/Makefile)

if test "$have_libresample" != "yes"; then
	# use our own libresample in vain
	cd src/libresample && autoconf && ./configure && cd ../..
fi

AC_CHECK_PROG([have_git], [git], yes, no)

have_git_repo=no
if test "$have_git" = "yes"; then
	if test -d .git; then
		have_git_repo=yes
	else
		echo "Warning: This is not a git checkout, can't determine version!"
	fi
else
	echo "Warning: git is not installed, can't determine version!"
fi

if test "$have_git_repo" = "yes"; then
	build_version=`git describe --abbrev=0`
	build_rev_date=`git log -1 --pretty="format:%ad" --date="short"`
	build_rev_tag=`git describe`

	# If the rev_tag and the version are the same, this is
	# an official release; otherwise, it's a dev build.
	if test "$build_version" != "$build_rev_tag"; then
		build_version="$build_version DEV"
	fi

	# HACK: the arcade chroot's git version is too old to understand our
	# desired YYYY-MM-DD format. If we don't get a date in that format,
	# fall back to today's date.

	# HACK x2 COMBO!!: 'expr', in bash 2.05, doesn't seem to parse
	# {n} for matching n repeats, hence the brain-dead regex.

	if ! [ expr "$build_rev_date" : '[0-9][0-9][0-9][0-9]-[0-9][0-9]-[0-9][0-9]$' > /dev/null ]; then
		build_rev_date=`date +%Y-%m-%d`
	fi
else
	build_version="unknown version"
	build_rev_date="unknown date"
	build_rev_tag="unknown revision"
fi

AC_DEFINE_UNQUOTED(BUILD_VERSION, "$build_version", [Version tag])
AC_DEFINE_UNQUOTED(BUILD_DATE, "$build_rev_date", [Date of source revision])
AC_DEFINE_UNQUOTED(BUILD_REVISION_TAG, "$build_rev_tag", [ID of source revision])

AC_SUBST(CFLAGS)
AC_SUBST(CXXFLAGS)
AC_SUBST(LDFLAGS)

AC_OUTPUT

echo
echo

echo "OpenITG $build_version (revision $build_rev_tag, $build_rev_date)"

echo

echo "Arcade build		$itg_arcade"
echo "SSE2 optimizations	$with_sse2 (supported: $have_sse2)"
echo "SDL			$enable_sdl"
echo "FFMPEG			$have_ffmpeg ($ffmpeg_option)"
echo "libresample		$have_libresample"
echo "GTK2			$enable_gtk2"
echo "ALSA			$alsa"
echo "OSS			$ac_cv_header_sys_soundcard_h (forced: $force_oss)"
echo "inotify hotplug		$have_inotify"

echo
