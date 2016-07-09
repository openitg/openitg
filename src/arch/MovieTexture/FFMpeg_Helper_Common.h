#ifndef FFMPEG_HELPER_COMMON
#define FFMPEG_HELPER_COMMON

#include "global.h"
#include "RageDisplay.h"
#include "RageUtil.h"

namespace avcodec
{
	#if defined(_WIN32)
		#include "ffmpeg/include/ffmpeg/avformat.h"
	#else
	extern "C"
	{
		#if defined(HAVE_LEGACY_FFMPEG)
			#include "ffmpeg/include/ffmpeg/avformat.h"
		#else
			#include <libavformat/avformat.h>
			#include <libswscale/swscale.h>
			#include <libavutil/avutil.h>
			#include <libavutil/pixdesc.h>
		#endif
	}
	#endif
	
	#if !defined (AV_VERSION_INT)
		#define AV_VERSION_INT(a, b, c) (a<<16 | b<<8 | c)
	#endif
};

#if defined(_MSC_VER)
	#pragma comment(lib, "ffmpeg/lib/avcodec.lib")
	#pragma comment(lib, "ffmpeg/lib/avformat.lib")
#endif

struct AVPixelFormat_t
{
	int bpp;
	unsigned masks[4];
	OITG_AV_PIXFMT_NAME pf;
	bool HighColor;
	bool ByteSwapOnLittleEndian;
};

extern AVPixelFormat_t AVPixelFormats[5];

static void FixLilEndian()
{
#if defined(ENDIAN_LITTLE)
	static bool Initialized = false;
	if( Initialized )
		return; 
	Initialized = true;

	for( int i = 0; i < AVPixelFormats[i].bpp; ++i )
	{
		AVPixelFormat_t &pf = AVPixelFormats[i];

		if( !pf.ByteSwapOnLittleEndian )
			continue;

		for( int mask = 0; mask < 4; ++mask)
		{
			int m = pf.masks[mask];
			switch( pf.bpp )
			{
				case 24: m = Swap24(m); break;
				case 32: m = Swap32(m); break;
				default: ASSERT(0);
			}
			pf.masks[mask] = m;
		}
	}
#endif
}

static int FindCompatibleAVFormat( RageDisplay::RagePixelFormat &pixfmt, bool HighColor )
{
	for( int i = 0; AVPixelFormats[i].bpp; ++i )
	{
		AVPixelFormat_t &fmt = AVPixelFormats[i];
		if( fmt.HighColor != HighColor )
			continue;

		pixfmt = DISPLAY->FindPixelFormat( fmt.bpp,
				fmt.masks[0],
				fmt.masks[1],
				fmt.masks[2],
				fmt.masks[3],
				true /* realtime */
				);

		if( pixfmt == RageDisplay::NUM_PIX_FORMATS )
			continue;

		return i;
	}

	return -1;
}
#endif // FFMPEG_HELPER_COMMON
