#include "global.h"
#include "RageLog.h"
#include "FFMpeg_Helper_Legacy.h"
#include "RageFile.h"
#include "RageSurface.h"

#include <cerrno>

void img_convert__(avcodec::AVPicture *dst, int dst_pix_fmt,
        const avcodec::AVPicture *src, int pix_fmt,
        int width, int height)
{
    #if defined(HAVE_LEGACY_FFMPEG)
        avcodec::img_convert(dst, dst_pix_fmt, src, pix_fmt, width, height);
    #else
        static avcodec::SwsContext* context = 0;
        context = avcodec::sws_getCachedContext(context, width, height, (avcodec::PixelFormat)pix_fmt, width, height, (avcodec::PixelFormat)dst_pix_fmt, avcodec::SWS_BICUBIC, NULL, NULL, NULL);
        avcodec::sws_scale(context, const_cast<uint8_t**>(src->data), const_cast<int*>(src->linesize), 0, height, dst->data, dst->linesize);
    #endif // HAVE_LEGACY_FFMPEG
}

AVPixelFormat_t AVPixelFormats[5] = {
	{ 
		/* This format is really ARGB, and is affected by endianness, unlike PIX_FMT_RGB24
		 * and PIX_FMT_BGR24. */
		32,
		{ 0x00FF0000,
		  0x0000FF00,
		  0x000000FF,
		  0xFF000000 },
#if defined(HAVE_LEGACY_FFMPEG)
		avcodec::PIX_FMT_RGBA32,
		true,
		false
#else
		avcodec::PIX_FMT_ARGB,
 		true,
		true
#endif
	},
	{ 
		24,
		{ 0xFF0000,
		  0x00FF00,
		  0x0000FF,
		  0x000000 },
		avcodec::PIX_FMT_RGB24,
		true,
		true
	},
	{ 
		24,
		{ 0x0000FF,
		  0x00FF00,
		  0xFF0000,
		  0x000000 },
		avcodec::PIX_FMT_BGR24,
		true,
		true
	},
	{
		16,
		{ 0x7C00,
		  0x03E0,
		  0x001F,
		  0x8000 },
		avcodec::PIX_FMT_RGB555,
		false,
		false
	},
	{ 0, { 0,0,0,0 }, avcodec::PIX_FMT_NB, true, false }
};

static CString averr_ssprintf( int err, const char *fmt, ... )
{
	ASSERT( err < 0 );

	va_list     va;
	va_start(va, fmt);
	CString s = vssprintf( fmt, va );
	va_end(va); 

	CString Error;
	switch( err )
	{
	case AVERROR_IO:			Error = "I/O error"; break;
	case AVERROR_NUMEXPECTED:	Error = "number syntax expected in filename"; break;
	case AVERROR_INVALIDDATA:	Error = "invalid data found"; break;
	case AVERROR_NOMEM:			Error = "not enough memory"; break;
	case AVERROR_NOFMT:			Error = "unknown format"; break;
	default: Error = ssprintf( "unknown error %i", err ); break;
	}

	return s + " (" + Error + ")";
}

int URLRageFile_open( avcodec::URLContext *h, const char *filename, int flags )
{
	if( strncmp( filename, "rage://", 7 ) )
	{
		LOG->Warn("URLRageFile_open: Unexpected path \"%s\"", filename );
	    return -EIO;
	}
	filename += 7;

	int mode = 0;
	switch( flags )
	{
	case URL_RDONLY: mode = RageFile::READ; break;
	case URL_WRONLY: mode = RageFile::WRITE | RageFile::STREAMED; break;
	case URL_RDWR: FAIL_M( "O_RDWR unsupported" );
	}

	RageFile *f = new RageFile;
	if( !f->Open(filename, mode) )
	{
		LOG->Trace("Error opening \"%s\": %s", filename, f->GetError().c_str() );
		delete f;
	    return -EIO;
	}

	h->is_streamed = false;
	h->priv_data = f;
	return 0;
}

int URLRageFile_read( avcodec::URLContext *h, unsigned char *buf, int size )
{
	RageFile *f = (RageFile *) h->priv_data;
	return f->Read( buf, size );
}

#if LIBAVFORMAT_BUILD > AV_VERSION_INT(52, 67, 0)
int URLRageFile_write( avcodec::URLContext *h, const unsigned char *buf, int size )
#else
int URLRageFile_write( avcodec::URLContext *h, unsigned char *buf, int size )
#endif
{
	RageFile *f = (RageFile *) h->priv_data;
	return f->Write( buf, size );
}

int64_t URLRageFile_seek( avcodec::URLContext *h, int64_t pos, int whence )
{
	RageFile *f = (RageFile *) h->priv_data;
	return f->Seek( (int) pos, whence );
}

int URLRageFile_close( avcodec::URLContext *h )
{
	RageFile *f = (RageFile *) h->priv_data;
	delete f;
	return 0;
}

static avcodec::URLProtocol RageProtocol =
{
	"rage",
	URLRageFile_open,
	URLRageFile_read,
	URLRageFile_write,
	URLRageFile_seek,
	URLRageFile_close,
	NULL
};

FFMpeg_Helper::FFMpeg_Helper()
{
	m_fctx=NULL;
	m_stream=NULL;
	current_packet_offset = -1;
	Init();
}

FFMpeg_Helper::~FFMpeg_Helper()
{
	if( current_packet_offset != -1 )
	{
		avcodec::av_free_packet( &pkt );
		current_packet_offset = -1;
	}
}

void FFMpeg_Helper::Init()
{
	eof = 0;
	GetNextTimestamp = true;
	CurrentTimestamp = 0, Last_IP_Timestamp = 0;
	LastFrameDelay = 0;
	pts = -1;
	FrameNumber = -1; /* decode one frame and you're on the 0th */
	TimestampOffset = 0;

	if( current_packet_offset != -1 )
	{
		avcodec::av_free_packet( &pkt );
		current_packet_offset = -1;
	}
}

static avcodec::AVStream *FindVideoStream( avcodec::AVFormatContext *m_fctx )
{
    for( int stream = 0; stream < m_fctx->nb_streams; ++stream )
	{
		avcodec::AVStream *enc = m_fctx->streams[stream];
#if (LIBAVCODEC_BUILD >= 4754)
        if( enc->codec->codec_type == avcodec::CODEC_TYPE_VIDEO )
			return enc;
#else
        if( enc->codec.codec_type == avcodec::CODEC_TYPE_VIDEO )
			return enc;
#endif
	}
	return NULL;
}

CString FFMpeg_Helper::Open(CString what)
{
	int ret = avcodec::av_open_input_file( &this->m_fctx, "rage://" + what, NULL, 0, NULL );
	if ( ret < 0 )
	{
		return ssprintf( averr_ssprintf(ret, "AVCodec: Couldn't open \"%s\"", what.c_str()) );
	}

	ret = avcodec::av_find_stream_info( this->m_fctx );
	if ( ret < 0 )
	{
		return ssprintf( averr_ssprintf(ret, "AVCodec (%s): Couldn't find codec parameters", what.c_str()) );
	}

	avcodec::AVStream *stream = FindVideoStream( this->m_fctx );
	if ( stream == NULL )
	{
		return ssprintf( "AVCodec (%s): Couldn't find any video streams", what.c_str() );
	}

#if (LIBAVCODEC_BUILD >= 4754)
	if( stream->codec->codec_id == avcodec::CODEC_ID_NONE )
	{
		return ssprintf( "AVCodec (%s): Unsupported codec %08x", what.c_str(), stream->codec->codec_tag );
	}

	avcodec::AVCodec *codec = avcodec::avcodec_find_decoder( stream->codec->codec_id );
	if( codec == NULL )
	{
		return ssprintf( "AVCodec (%s): Couldn't find decoder %i", what.c_str(), stream->codec->codec_id );
	}

	LOG->Trace("Opening codec %s", codec->name );
	ret = avcodec::avcodec_open( stream->codec, codec );
#else
	if( stream->codec.codec_id == avcodec::CODEC_ID_NONE )
	{
		return ssprintf( "AVCodec (%s): Unsupported codec %08x", what.c_str(), stream->codec.codec_tag );
	}

	avcodec::AVCodec *codec = avcodec::avcodec_find_decoder( stream->codec.codec_id );
	if( codec == NULL )
	{
		return ssprintf( "AVCodec (%s): Couldn't find decoder %i", what.c_str(), stream->codec.codec_id );
	}

	LOG->Trace("Opening codec %s", codec->name );
	ret = avcodec::avcodec_open( &stream->codec, codec );
#endif
	if ( ret < 0 )
	{
		return ssprintf( averr_ssprintf(ret, "AVCodec (%s): Couldn't open codec \"%s\"", what.c_str(), codec->name) );
	}

#if (LIBAVCODEC_BUILD >= 4754)
	m_width  = stream->codec->width;
	m_height = stream->codec->height;
#else
	m_width  = stream->codec.width;
	m_height = stream->codec.height;
#endif
	/* Don't set this until we successfully open stream->codec, so we don't try to close it
	 * on an exception unless it was really opened. */
	m_stream = stream;

#if (LIBAVCODEC_BUILD >= 4754)
	LOG->Trace("Bitrate: %i", this->m_stream->codec->bit_rate );
	LOG->Trace("Codec pixel format: %s", avcodec::avcodec_get_pix_fmt_name(this->m_stream->codec->pix_fmt) );
#else
	LOG->Trace("Bitrate: %i", this->m_stream->codec.bit_rate );
	LOG->Trace("Codec pixel format: %s", avcodec::avcodec_get_pix_fmt_name(this->m_stream->codec.pix_fmt) );
#endif

	return "";
}

void FFMpeg_Helper::Close()
{
	if( m_stream )
	{
#if (LIBAVCODEC_BUILD >= 4754)
		avcodec::avcodec_close( m_stream->codec );
#else
 		avcodec::avcodec_close( &m_stream->codec );
#endif
		m_stream = NULL;
	}

	if( m_fctx )
	{
		avcodec::av_close_input_file( m_fctx );
		m_fctx = NULL;
	}
}

void FFMpeg_Helper::RegisterProtocols()
{
	static bool Done = false;
	if( Done )
		return;
	Done = true;

	avcodec::av_register_all();
	avcodec::register_protocol( &RageProtocol );
}

int FFMpeg_Helper::GetRawFrameNumber()
{
#if (LIBAVCODEC_BUILD >= 4754)
	return m_stream->codec->frame_number;
#else
	return m_stream->codec.frame_number;
#endif
}

/* Read until we get a frame, EOF or error.  Return -1 on error, 0 on EOF, 1 if we have a frame. */
int FFMpeg_Helper::GetFrame()
{
	while( 1 )
	{
		int ret = DecodePacket();
		if( ret == 1 )
			break;
		if( ret == -1 )
			return -1;
		if( ret == 0 && eof > 0 )
			return 0; /* eof */

		ASSERT( ret == 0 );
		ret = ReadPacket();
		if( ret < 0 )
			return ret; /* error */
	}

	++FrameNumber;

	if( FrameNumber == 1 )
	{
		/* Some videos start with a timestamp other than 0.  I think this is used
		 * when audio starts before the video.  We don't want to honor that, since
		 * the DShow renderer doesn't and we don't want to break sync compatibility.
		 *
		 * Look at the second frame.  (If we have B-frames, the first frame will be an
		 * I-frame with the timestamp of the next P-frame, not its own timestamp, and we
		 * want to ignore that and look at the next B-frame.) */
		const float expect = LastFrameDelay;
		const float actual = CurrentTimestamp;
		if( actual - expect > 0 )
		{
			LOG->Trace("Expect %f, got %f -> %f", expect, actual, actual - expect );
			TimestampOffset = actual - expect;
		}
	}

	return 1;
}

float FFMpeg_Helper::GetTimestamp() const
{
	/* The first frame always has a timestamp of 0. */
	if( FrameNumber == 0 )
		return 0;

	return CurrentTimestamp - TimestampOffset;
}

/* Read a packet.  Return -1 on error, 0 on EOF, 1 on OK. */
int FFMpeg_Helper::ReadPacket()
{
	if( eof > 0 )
		return 0;

	while( 1 )
	{
		//CHECKPOINT;
		if( current_packet_offset != -1 )
		{
			current_packet_offset = -1;
			avcodec::av_free_packet( &pkt );
		}

		int ret = avcodec::av_read_frame( m_fctx, &pkt );
		/* XXX: why is avformat returning AVERROR_NOMEM on EOF? */
		if( ret < 0 )
		{
			/* EOF. */
			eof = 1;
			pkt.size = 0;
			
			return 0;
		}

		if( pkt.stream_index == m_stream->index )
		{
			current_packet_offset = 0;
			return 1;
		}

		/* It's not for the video stream; ignore it. */
		avcodec::av_free_packet( &pkt );
	}
}


void FFMpeg_Helper::RenderFrame(RageSurface *img, int tex_fmt)
{
	avcodec::AVPicture pict;
	pict.data[0] = (unsigned char *)img->pixels;
	pict.linesize[0] = img->pitch;

#if (LIBAVCODEC_BUILD >= 4754)
	img_convert__(&pict, AVPixelFormats[tex_fmt].pf,
			(avcodec::AVPicture *) &this->frame, this->m_stream->codec->pix_fmt, 
			this->m_stream->codec->width, this->m_stream->codec->height);
#else
	img_convert__(&pict, AVPixelFormats[tex_fmt].pf,
			(avcodec::AVPicture *) &this->frame, this->m_stream->codec.pix_fmt, 
			this->m_stream->codec.width, this->m_stream->codec.height);
#endif
}

/* Decode data from the current packet.  Return -1 on error, 0 if the packet is finished,
 * and 1 if we have a frame (we may have more data in the packet). */
int FFMpeg_Helper::DecodePacket()
{
	if( eof == 0 && current_packet_offset == -1 )
		return 0; /* no packet */

	while( eof == 1 || (eof == 0 && current_packet_offset < pkt.size) )
	{
		if ( GetNextTimestamp )
		{
			if (pkt.dts != int64_t(AV_NOPTS_VALUE))
#if (LIBAVCODEC_BUILD >= 4754)
				pts = (float)pkt.dts * m_stream->time_base.num / m_stream->time_base.den;
#else
 				pts = (float)pkt.dts / AV_TIME_BASE;
#endif
			else
				pts = -1;
			GetNextTimestamp = false;
		}

		/* If we have no data on the first frame, just return EOF; passing an empty packet
		 * to avcodec_decode_video in this case is crashing it.  However, passing an empty
		 * packet is normal with B-frames, to flush.  This may be unnecessary in newer
		 * versions of avcodec, but I'm waiting until a new stable release to upgrade. */
		if( pkt.size == 0 && FrameNumber == -1 )
			return 0; /* eof */

		int got_frame;
		//CHECKPOINT;
		/* Hack: we need to send size = 0 to flush frames at the end, but we have
		 * to give it a buffer to read from since it tries to read anyway. */
		static uint8_t dummy[FF_INPUT_BUFFER_PADDING_SIZE] = { 0 };
		int len = avcodec::avcodec_decode_video(
#if (LIBAVCODEC_BUILD >= 4754)
				m_stream->codec, 
#else
 				&m_stream->codec, 
#endif
				&frame, &got_frame,
				pkt.size? pkt.data:dummy, pkt.size );
		//CHECKPOINT;

		if (len < 0)
		{
			LOG->Warn("avcodec_decode_video: %i", len);
			return -1; // XXX
		}

		current_packet_offset += len;

		if (!got_frame)
		{
			if( eof == 1 )
				eof = 2;
			continue;
		}

		GetNextTimestamp = true;

		if (pts != -1)
		{
			CurrentTimestamp = pts;
		}
		else
		{
			/* If the timestamp is zero, this frame is to be played at the
			 * time of the last frame plus the length of the last frame. */
			CurrentTimestamp += LastFrameDelay;
		}

		/* Length of this frame: */
#if (LIBAVCODEC_BUILD >= 4754)
		LastFrameDelay = (float)m_stream->codec->time_base.num / m_stream->codec->time_base.den;
#else
 		LastFrameDelay = (float)m_stream->codec.frame_rate_base / m_stream->codec.frame_rate;
#endif
		LastFrameDelay += frame.repeat_pict * (LastFrameDelay * 0.5f);

		return 1;
	}

	return 0; /* packet done */
}

