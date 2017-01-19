/* vim: set noet */
#define __STDC_CONSTANT_MACROS
//#include <stdint.h>
#include "global.h"
#include "MovieTexture_FFMpeg.h"

#include "RageLog.h"
#include "RageTextureManager.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "RageFile.h"
#include "RageSurface.h"
#include "PrefsManager.h"

#include <cerrno>

#if defined(WIN32) && !defined(XBOX)
#include <windows.h>
#endif

#if defined(HAVE_LEGACY_FFMPEG)
#include "FFMpeg_Helper_Legacy.h"
#else
#include "FFMpeg_Helper.h"
#endif

void MovieTexture_FFMpeg::ConvertFrame()
{
	ASSERT_M( m_ImageWaiting == FRAME_DECODED, ssprintf("%i", m_ImageWaiting ) );

    decoder->RenderFrame(m_img, m_AVTexfmt);
	m_ImageWaiting = FRAME_WAITING;
}

MovieTexture_FFMpeg::MovieTexture_FFMpeg( RageTextureID ID ):
	RageMovieTexture( ID ),
	m_BufferFinished( "BufferFinished", 0 )
{
	LOG->Trace( "MovieTexture_FFMpeg::MovieTexture_FFMpeg(%s)", ID.filename.c_str() );

	FixLilEndian();

	m_uTexHandle = 0;
	m_bLoop = true;
    m_State = DECODER_QUIT; /* it's quit until we call StartThread */
	m_img = NULL;
	m_ImageWaiting = FRAME_NONE;
	m_Rate = 1;
	m_bWantRewind = false;
	m_Clock = 0;
	m_FrameSkipMode = false;
	m_bThreaded = PREFSMAN->m_bThreadedMovieDecode.Get();
	decoder = NULL;
}

CString MovieTexture_FFMpeg::Init()
{
	CString sError = CreateDecoder();
	if( sError != "" )
		return sError;

	/* Decode one frame, to guarantee that the texture is drawn when this function returns. */
	int ret = decoder->GetFrame();
	if( ret == -1 )
		return ssprintf( "%s: error getting first frame", GetID().filename.c_str() );
	if( ret == 0 )
	{
		/* There's nothing there. */
		return ssprintf( "%s: EOF getting first frame", GetID().filename.c_str() );
	}

	m_ImageWaiting = FRAME_DECODED;

	CreateTexture();
	LOG->Trace( "Resolution: %ix%i (%ix%i, %ix%i)",
			m_iSourceWidth, m_iSourceHeight,
			m_iImageWidth, m_iImageHeight, m_iTextureWidth, m_iTextureHeight );
	LOG->Trace( "Texture pixel format: %i", m_AVTexfmt );

	CreateFrameRects();

	ConvertFrame();
	UpdateFrame();

	//CHECKPOINT;

	StartThread();

	return "";
}

MovieTexture_FFMpeg::~MovieTexture_FFMpeg()
{
	StopThread();
	DestroyDecoder();
	DestroyTexture();
}

CString MovieTexture_FFMpeg::CreateDecoder()
{
	ASSERT( decoder == NULL );
	decoder = new FFMpeg_Helper;
	decoder->RegisterProtocols();
	return decoder->Open( GetID().filename );
}


/* Delete the decoder.  The decoding thread must be stopped. */
void MovieTexture_FFMpeg::DestroyDecoder()
{
	decoder->Close();
	SAFE_DELETE( decoder );
}

/* Delete the surface and texture.  The decoding thread must be stopped, and this
 * is normally done after destroying the decoder. */
void MovieTexture_FFMpeg::DestroyTexture()
{
	if( m_img )
	{
		delete m_img;
		m_img=NULL;
	}
	if(m_uTexHandle)
	{
		DISPLAY->DeleteTexture( m_uTexHandle );
		m_uTexHandle = 0;
	}
}


void MovieTexture_FFMpeg::CreateTexture()
{
    if( m_uTexHandle )
        return;

	//CHECKPOINT;

	RageTextureID actualID = GetID();
	actualID.iAlphaBits = 0;

	/* Cap the max texture size to the hardware max. */
	actualID.iMaxSize = min( actualID.iMaxSize, DISPLAY->GetMaxTextureSize() );

    m_iSourceWidth = decoder->m_width;
    m_iSourceHeight = decoder->m_height;

	/* image size cannot exceed max size */
	m_iImageWidth = min( m_iSourceWidth, actualID.iMaxSize );
	m_iImageHeight = min( m_iSourceHeight, actualID.iMaxSize );

	/* Texture dimensions need to be a power of two; jump to the next. */
	m_iTextureWidth = power_of_two(m_iImageWidth);
	m_iTextureHeight = power_of_two(m_iImageHeight);

	/* Bogus assignment to shut gcc up. */
    RageDisplay::RagePixelFormat pixfmt = RageDisplay::FMT_RGBA8;
	bool PreferHighColor = (TEXTUREMAN->GetPrefs().m_iMovieColorDepth == 32);
	m_AVTexfmt = FindCompatibleAVFormat( pixfmt, PreferHighColor );

	if( m_AVTexfmt == -1 )
		m_AVTexfmt = FindCompatibleAVFormat( pixfmt, !PreferHighColor );

	if( m_AVTexfmt == -1 )
	{
		/* No dice.  Use the first avcodec format of the preferred bit depth,
		 * and let the display system convert. */
		for( m_AVTexfmt = 0; AVPixelFormats[m_AVTexfmt].bpp; ++m_AVTexfmt )
			if( AVPixelFormats[m_AVTexfmt].HighColor == PreferHighColor )
				break;
		ASSERT( AVPixelFormats[m_AVTexfmt].bpp );

		switch( TEXTUREMAN->GetPrefs().m_iMovieColorDepth )
		{
		default:
			ASSERT(0);
		case 16:
			if( DISPLAY->SupportsTextureFormat(RageDisplay::FMT_RGB5) )
				pixfmt = RageDisplay::FMT_RGB5;
			else
				pixfmt = RageDisplay::FMT_RGBA4; // everything supports RGBA4

			break;

		case 32:
			if( DISPLAY->SupportsTextureFormat(RageDisplay::FMT_RGB8) )
				pixfmt = RageDisplay::FMT_RGB8;
			else if( DISPLAY->SupportsTextureFormat(RageDisplay::FMT_RGBA8) )
				pixfmt = RageDisplay::FMT_RGBA8;
			else if( DISPLAY->SupportsTextureFormat(RageDisplay::FMT_RGB5) )
				pixfmt = RageDisplay::FMT_RGB5;
			else
				pixfmt = RageDisplay::FMT_RGBA4; // everything supports RGBA4
			break;
		}
	}
	
	if( !m_img )
	{
		const AVPixelFormat_t *pfd = &AVPixelFormats[m_AVTexfmt];

		LOG->Trace("format %i, %08x %08x %08x %08x",
			pfd->bpp, pfd->masks[0], pfd->masks[1], pfd->masks[2], pfd->masks[3]);

		m_img = CreateSurface( m_iTextureWidth, m_iTextureHeight, pfd->bpp,
			pfd->masks[0], pfd->masks[1], pfd->masks[2], pfd->masks[3] );
	}

    m_uTexHandle = DISPLAY->CreateTexture( pixfmt, m_img, false );
}

/* Handle decoding for a frame.  Return true if a frame was decoded, false if not
 * (due to pause, EOF, etc).  If true is returned, we'll be in FRAME_DECODED. */
bool MovieTexture_FFMpeg::DecodeFrame()
{
	ASSERT_M( m_ImageWaiting == FRAME_NONE, ssprintf("%i", m_ImageWaiting) );

	if( m_State == DECODER_QUIT )
		return false;
	//CHECKPOINT;

	/* Read a frame. */
	int ret = decoder->GetFrame();
	if( ret == -1 )
		return false;

	if( m_bWantRewind && decoder->GetTimestamp() == 0 )
		m_bWantRewind = false; /* ignore */

	if( ret == 0 )
	{
		/* EOF. */
		if( !m_bLoop )
			return false;

		LOG->Trace( "File \"%s\" looping", GetID().filename.c_str() );
		m_bWantRewind = true;
	}

	if( m_bWantRewind )
	{
		m_bWantRewind = false;

		/* When resetting the clock, set it back by the length of the last frame,
		 * so it has a proper delay. */
		float fDelay = decoder->LastFrameDelay;

		/* Restart. */
		DestroyDecoder();
		CString sError = CreateDecoder();
		if( sError != "" )
			RageException::Throw( "Error rewinding stream %s: %s", GetID().filename.c_str(), sError.c_str() );

		m_Clock = -fDelay;
		return false;
	}

	/* We got a frame. */
	m_ImageWaiting = FRAME_DECODED;

	return true;
}

/*
 * Call when m_ImageWaiting == FRAME_DECODED.
 * Returns:
 *  == 0 if the currently decoded frame is ready to be displayed
 *   > 0 (seconds) if it's not yet time to display;
 *  == -1 if we're behind and the frame should be skipped
 */
float MovieTexture_FFMpeg::CheckFrameTime()
{
	ASSERT_M( m_ImageWaiting == FRAME_DECODED, ssprintf("%i", m_ImageWaiting) );

	if( m_Rate == 0 )
		return 1;	// "a long time until the next frame"

	const float Offset = (decoder->GetTimestamp() - m_Clock) / m_Rate;

	/* If we're ahead, we're decoding too fast; delay. */
	if( Offset > 0.00001f )
	{
		if( m_FrameSkipMode )
		{
			/* We're caught up; stop skipping frames. */
			LOG->Trace( "stopped skipping frames" );
			m_FrameSkipMode = false;
		}
		return Offset;
	}

	/*
	 * We're behind by -Offset seconds.  
	 *
	 * If we're just slightly behind, don't worry about it; we'll simply
	 * not sleep, so we'll move as fast as we can to catch up.
	 *
	 * If we're far behind, we're short on CPU.  Skip texture updates; this
	 * is a big bottleneck on many systems.
	 *
	 * If we hit a threshold, start skipping frames via #1.  If we do that,
	 * don't stop once we hit the threshold; keep doing it until we're fully
	 * caught up.
	 *
	 * We should try to notice if we simply don't have enough CPU for the video;
	 * it's better to just stay in frame skip mode than to enter and exit it
	 * constantly, but we don't want to do that due to a single timing glitch.
	 */
	const float FrameSkipThreshold = 0.5f;

	if( -Offset >= FrameSkipThreshold && !m_FrameSkipMode )
	{
		LOG->Trace( "(%s) Time is %f, and the movie is at %f.  Entering frame skip mode.",
			GetID().filename.c_str(), m_Clock, decoder->GetTimestamp());
		m_FrameSkipMode = true;
	}

    if( m_FrameSkipMode && decoder->GetRawFrameNumber() % 2 )
		return -1; /* skip */
	
	return 0;
}

void MovieTexture_FFMpeg::DiscardFrame()
{
	ASSERT_M( m_ImageWaiting == FRAME_DECODED, ssprintf("%i", m_ImageWaiting) );
	m_ImageWaiting = FRAME_NONE;
}

void MovieTexture_FFMpeg::DecoderThread()
{
#if defined(_WINDOWS)
	/* Windows likes to boost priority when processes come out of a wait state.  We don't
	 * want that, since it'll result in us having a small priority boost after each movie
	 * frame, resulting in skips in the gameplay thread. */
	if( !SetThreadPriorityBoost(GetCurrentThread(), TRUE) && GetLastError() != ERROR_CALL_NOT_IMPLEMENTED )
		LOG->Warn( werr_ssprintf(GetLastError(), "SetThreadPriorityBoost failed") );
#endif

	//CHECKPOINT;

	while( m_State != DECODER_QUIT )
	{
		if( m_ImageWaiting == FRAME_NONE )
			DecodeFrame();

		/* If we still have no frame, we're at EOF and we didn't loop. */
		if( m_ImageWaiting != FRAME_DECODED )
		{
			usleep( 10000 );
			continue;
		}

		const float fTime = CheckFrameTime();
		if( fTime == -1 )	// skip frame
		{
			DiscardFrame();
		}
		else if( fTime > 0 )		// not time to decode a new frame yet
		{
			/* This needs to be relatively short so that we wake up quickly 
			 * from being paused or for changes in m_Rate. */
			usleep( 10000 );
		}
		else // fTime == 0
		{
			{
				/* The only reason m_BufferFinished might be non-zero right now (before
				 * ConvertFrame()) is if we're quitting. */
				int n = m_BufferFinished.GetValue();
				ASSERT_M( n == 0 || m_State == DECODER_QUIT, ssprintf("%i, %i", n, m_State) );
			}
			ConvertFrame();

			/* We just went into FRAME_WAITING.  Don't actually check; the main thread
			 * will change us back to FRAME_NONE without locking, and poke m_BufferFinished.
			 * Don't time out on this; if a new screen has started loading, this might not
			 * return for a while. */
			m_BufferFinished.Wait( false );

			/* If the frame wasn't used, then we must be shutting down. */
			ASSERT_M( m_ImageWaiting == FRAME_NONE || m_State == DECODER_QUIT, ssprintf("%i, %i", m_ImageWaiting, m_State) );
		}
	}
	//CHECKPOINT;
}

void MovieTexture_FFMpeg::Update(float fDeltaTime)
{
	/* We might need to decode more than one frame per update.  However, there
	 * have been bugs in ffmpeg that cause it to not handle EOF properly, which
	 * could make this never return, so let's play it safe. */
	int iMax = 4;
	while( --iMax )
	{
		if( !m_bThreaded )
		{
			/* If we don't have a frame decoded, decode one. */
			if( m_ImageWaiting == FRAME_NONE )
				DecodeFrame();

			/* If we have a frame decoded, see if it's time to display it. */
			if( m_ImageWaiting == FRAME_DECODED )
			{
				float fTime = CheckFrameTime();
				if( fTime > 0 )
					return;
				else if( fTime == -1 )
					DiscardFrame();
				else
					ConvertFrame();
			}
		}

		/* Note that if there's an image waiting, we *must* signal m_BufferFinished, or
		* the decoder thread may sit around waiting for it, even though Pause and Play
		* calls, causing the clock to keep running. */
		if( m_ImageWaiting != FRAME_WAITING )
			return;
		//CHECKPOINT;

		UpdateFrame();
		
		if( m_bThreaded )
			m_BufferFinished.Post();
	}

	LOG->MapLog( "ffmpeg_looping", "MovieTexture_FFMpeg::Update looping" );
}

/* Call from the main thread when m_ImageWaiting == FRAME_WAITING to update the
 * texture.  Sets FRAME_NONE.  Does not signal m_BufferFinished. */
void MovieTexture_FFMpeg::UpdateFrame()
{
	ASSERT_M( m_ImageWaiting == FRAME_WAITING, ssprintf("%i", m_ImageWaiting) );

    /* Just in case we were invalidated: */
    CreateTexture();

	//CHECKPOINT;
	DISPLAY->UpdateTexture(
        m_uTexHandle,
        m_img,
        0, 0,
        m_iImageWidth, m_iImageHeight );
    //CHECKPOINT;

	m_ImageWaiting = FRAME_NONE;
}

void MovieTexture_FFMpeg::Reload()
{
}

void MovieTexture_FFMpeg::StartThread()
{
	ASSERT( m_State == DECODER_QUIT );
	m_State = DECODER_RUNNING;
	m_DecoderThread.SetName( ssprintf("MovieTexture_FFMpeg(%s)", GetID().filename.c_str()) );
	
	if( m_bThreaded )
		m_DecoderThread.Create( DecoderThread_start, this );
}

void MovieTexture_FFMpeg::StopThread()
{
	if( !m_DecoderThread.IsCreated() )
		return;

	LOG->Trace("Shutting down decoder thread ...");

	m_State = DECODER_QUIT;

	/* Make sure we don't deadlock waiting for m_BufferFinished. */
	m_BufferFinished.Post();
	//CHECKPOINT;
	m_DecoderThread.Wait();
	//CHECKPOINT;
	
	m_ImageWaiting = FRAME_NONE;

	/* Clear the above post, if the thread didn't. */
	m_BufferFinished.TryWait();

	LOG->Trace("Decoder thread shut down.");
}

void MovieTexture_FFMpeg::SetPosition( float fSeconds )
{
    ASSERT( m_State != DECODER_QUIT );

	/* We can reset to 0, but I don't think this API supports fast seeking
	 * yet.  I don't think we ever actually seek except to 0 right now,
	 * anyway. XXX */
	if( fSeconds != 0 )
	{
		LOG->Warn( "MovieTexture_FFMpeg::SetPosition(%f): non-0 seeking unsupported; ignored", fSeconds );
		return;
	}

	LOG->Trace( "Seek to %f", fSeconds );
	m_bWantRewind = true;
}

/* This is used to decode data. */
void MovieTexture_FFMpeg::DecodeSeconds( float fSeconds )
{
	m_Clock += fSeconds * m_Rate;

	/* If we're not threaded, we want to be sure to decode any new frames now,
	 * and not on the next frame.  Update() may have already been called for this
	 * frame; call it again to be sure. */
	Update(0);
}

/*
 * (c) 2003-2004 Glenn Maynard
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
