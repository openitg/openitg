AC_DEFUN([SM_FFMPEG], [

AC_ARG_WITH(ffmpeg, AS_HELP_STRING([--without-ffmpeg],[Disable ffmpeg support]), with_ffmpeg=$withval, with_ffmpeg=yes)
AC_ARG_WITH(legacy-ffmpeg, AS_HELP_STRING([--with-legacy-ffmpeg],[Use bundled static legacy ffmpeg libraries]), with_legacy_ffmpeg=$withval, with_legacy_ffmpeg=no)

have_ffmpeg=no

if test "$with_ffmpeg" = "yes"; then
    if test "$with_legacy_ffmpeg" = "no"; then

        if pkg-config --libs libavcodec &> /dev/null; then AVCODEC_LIBS="`pkg-config --libs libavcodec`"; else AVCODEC_LIBS="-lavcodec"; fi
		if pkg-config --libs libavformat &> /dev/null; then AVFORMAT_LIBS="`pkg-config --libs libavformat`"; else AVFORMAT_LIBS="-lavcodec"; fi
		if pkg-config --libs libswscale &> /dev/null; then SWSCALE_LIBS="`pkg-config --libs libswscale`"; else SWSCALE_LIBS="-swscale"; fi
        if pkg-config --libs libavutil &> /dev/null; then AVUTIL_LIBS="`pkg-config --libs libavutil`"; else AVUTIL_LIBS="-lavutil"; fi
        
        if pkg-config --cflags libavcodec &> /dev/null; then AVCODEC_CFLAGS="`pkg-config --cflags libavcodec`"; else AVCODEC_CFLAGS=""; fi
		if pkg-config --cflags libavformat &> /dev/null; then AVFORMAT_CFLAGS="`pkg-config --cflags libavformat`"; else AVFORMAT_CFLAGS=""; fi
		if pkg-config --cflags libswscale &> /dev/null; then SWSCALE_CFLAGS="`pkg-config --cflags libswscale`"; else SWSCALE_CFLAGS=""; fi
        if pkg-config --cflags libavutil &> /dev/null; then AVUTIL_CFLAGS="`pkg-config --cflags libavutil`"; else AVUTIL_CFLAGS="-lavutil"; fi
        
        ffmpeg_save_CFLAGS="$CFLAGS"
		ffmpeg_save_CXXFLAGS="$CXXFLAGS"
		ffmpeg_save_LIBS="$LIBS"

        FFMPEG_CFLAGS="$AVUTIL_CFLAGS $AVCODEC_CFLAGS $AVFORMAT_CFLAGS $SWSCALE_CFLAGS"
		FFMPEG_CXXFLAGS="$AVUTIL_CFLAGS $AVCODEC_CFLAGS $AVFORMAT_CFLAGS $SWSCALE_CFLAGS"
		FFMPEG_LIBS="$AVUTIL_LIBS $AVCODEC_LIBS $AVFORMAT_LIBS $SWSCALE_LIBS"

        CFLAGS="$FFMPEG_CFLAGS $CFLAGS"
		CXXFLAGS="$FFMPEG_CXXFLAGS $CXXFLAGS"
		LIBS="$FFMPEG_LIBS $LIBS"

		AC_CHECK_FUNC([avcodec_find_decoder], have_libavcodec=yes, have_libavcodec=no)
		AC_CHECK_FUNC([avformat_open_input], have_libavformat=yes, have_libavformat=no)
		AC_CHECK_FUNC([swscale_version], have_libswscale=yes, have_libswscale=no)
        AC_CHECK_FUNC([av_frame_free], have_libavutil=yes, have_libavutil=no)

        if test "$have_libavcodec" = "yes"; then
        AC_MSG_CHECKING([for libavcodec >= 0.5.2])
        AC_TRY_RUN([
            #include <libavcodec/avcodec.h>
            int main()
            {
                return ( LIBAVCODEC_VERSION_INT < 0x341401 )? 1:0;
            }
            ],,have_libavcodec=no,)
        AC_MSG_RESULT($have_libavcodec)
        fi

        if test "$have_libavformat" = "yes"; then
        AC_MSG_CHECKING([for libavformat >= 0.5.2])
        AC_TRY_RUN([
            #include <libavformat/avformat.h>
            int main()
            {
                return ( LIBAVFORMAT_VERSION_INT < 0x341F00 )? 1:0;
            }
            ],,have_libavformat=no,)
        AC_MSG_RESULT($have_libavformat)
        fi

        if test "$have_libswscale" = "yes"; then
        AC_MSG_CHECKING([for libswscale >= 0.5.2])
        AC_TRY_RUN([
            #include <libswscale/swscale.h>
            int main()
            {
                return ( LIBSWSCALE_VERSION_INT < 0x000701 )? 1:0;
            }
            ],,have_libswscale=no,)
        AC_MSG_RESULT($have_libswscale)
        fi

        CFLAGS="$ffmpeg_save_CFLAGS"
		CXXFLAGS="$ffmpeg_save_CXXFLAGS"
		LIBS="$ffmpeg_save_LIBS"

        if test "$have_libavformat" = "yes" -a "$have_libavcodec" = "yes" \
                -a "$have_libswscale" = "yes" -a "$have_libavutil" = "yes"; then
            have_ffmpeg=yes
            ffmpeg_option=shared
            AC_DEFINE(HAVE_FFMPEG, 1, [FFMPEG support available])
            
            CFLAGS="$FFMPEG_CFLAGS $CFLAGS"
		    CXXFLAGS="$FFMPEG_CXXFLAGS $CXXFLAGS"
		    LIBS="$FFMPEG_LIBS $LIBS"
        fi
    else
        have_ffmpeg=yes
        ffmpeg_option=legacy
        AC_DEFINE(HAVE_LEGACY_FFMPEG, 1, [Legacy FFMPEG support available])
        AC_DEFINE(HAVE_FFMPEG, 1, [FFMPEG support available])

		LIBS="-L$PWD/src/ffmpeg/lib -lavformat -lavcodec $LIBS"
    fi
fi

AM_CONDITIONAL(HAVE_FFMPEG, test "$have_ffmpeg" = "yes")
AM_CONDITIONAL(HAVE_LEGACY_FFMPEG, test "$have_ffmpeg" = "yes" -a "$ffmpeg_option" = "legacy")

])