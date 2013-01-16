#include "global.h"
#include "RageLog.h"
#include "RageSoundReader_MP3.h"
#include "RageTimer.h"
#include "RageUtil.h"
#include "RageSoundReader_Preload.h"
#include "RageSoundReader_Resample.h"

#include "test_misc.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

void ReadData( SoundReader *snd,
	       int ms,		/* start */
	       char *buf,	/* out */
	       int bytes )
{
	if( ms != -1 )
		snd->SetPosition_Accurate( ms );
	if( snd->GetNumChannels() == 1 )
		bytes /= 2;
	int got = snd->Read( buf, bytes );
	if( got != -1 && snd->GetNumChannels() == 1 )
	{
		int16_t *pTemp = (int16_t *) alloca( got );
		memcpy( pTemp, buf, got );
		for( int i = 0; i < got/2; ++i )
		{
			int16_t *p = (int16_t *) buf;
			p[i*2+0] = pTemp[i];
			p[i*2+1] = pTemp[i];
		}
	}
		
	ASSERT( got == bytes );
}
	       
void find( const char *haystack, int hs, const char *needle, int ns )
{
	for( int i = 0; i <= hs-ns; ++i )
	{
		if( !memcmp(haystack+i, needle, ns) )
		{
			printf("xx %i\n", i);
			return;
		}
	}
}

void dump_bin( const char *fn, const char *buf, int size )
{
	int fd = open( fn, O_WRONLY|O_CREAT|O_TRUNC );
	ASSERT( fd != -1 );
	write( fd, buf, size );
	close( fd );
}

void dump( const char *fn, const int16_t *buf, int samples )
{
	FILE *f = fopen( fn, "w+");
	ASSERT( f );

	int cnt = 0;
	for( int i = 0; i < samples; ++i )
	{
		fprintf( f, "0x%04hx,", buf[i] );
		if( cnt++ == 16 )
		{
			fprintf( f, "\n" );
			cnt = 0;
		}
	}

	fclose( f );
}

void dump( const char *buf, int size )
{
	for( int i = 0; i < size; ++i )
		printf("%-2.2x ", ((unsigned char*)buf)[i]);
	printf("\n\n");
}

void dump( const int16_t *buf, int samples )
{
	for( int i = 0; i < samples; ++i )
		printf( "0x%04hx,", buf[i] );
	printf( "\n" );
}


void compare_buffers( const int16_t *expect, const int16_t *got, int frames,
		int &NumInaccurateSamplesAtStart,
		int &NumInaccurateSamples )
{
	bool InaccurateSinceStart = true;

	NumInaccurateSamplesAtStart = 0;
	NumInaccurateSamples = 0;
		
	for( int i = 0; i < frames; ++i )
	{
		int diff = abs( expect[i] - got[i] );
		if( diff > 200 )
		{
			printf("%i\n", diff);
			++NumInaccurateSamples;
			if( InaccurateSinceStart )
				++NumInaccurateSamplesAtStart;
		}
		else
			InaccurateSinceStart = false;

	}

}

bool compare_buffers( const int16_t *expect, const int16_t *got, int frames, int channels )
{
	/* Compare each channel separately.  Try to figure out if
	 * the data is exactly the same, 
	 * 
	 * 2: either the source or dest data starts out around 0 and converges quickly
	 *    on the other; this happens with resamplers after a seek, since they're missing
	 *    data before the seeked position (could be fixed)
	 *
	 * When seeking, resamplers should ideally seek slightly before the requested position,
	 * read the extra data, feed it to the resampling engine, and discard the extra data.
	 * Otherwise, the resampler is starting with no data, and will give some low samples.
	 * (The amount of incorrect data depends on the window size of the resampler.)
	 * Detect if this happens: determine if the data starts very inaccurately and quickly
	 * converges.
	 *
	 * Determine if data is identical, but offset.
	 * 
	 * */

	

	int NumInaccurateSamples;
	int NumInaccurateSamplesAtStart;
	compare_buffers( expect, got, frames, NumInaccurateSamplesAtStart, NumInaccurateSamples );

	printf("%i/%i off, %i at start\n", NumInaccurateSamples, frames, NumInaccurateSamplesAtStart );
	return NumInaccurateSamples == 0;
}



bool test_read( SoundReader *snd, const char *expected_data, int bytes )
{
	bytes = (bytes/4) * 4;
	char buf[bytes];
	int got = snd->Read( buf, bytes );
	ASSERT( got == bytes );

	//compare_buffers( (const int16_t *) expected_data,
	//		 (const int16_t *) buf,
	//		 bytes/2,
	//		 2 );
	
	if( !memcmp(expected_data, buf, bytes) )
		return true;

	dump( (const int16_t *) expected_data, min(100, bytes/2) );
	dump( (const int16_t *) buf, min(100, bytes/2) );
	return false;
}


bool must_be_eof( SoundReader *snd )
{
	char buf[4];
	int got = snd->Read( buf, 4 );
	return got == 0;
}

const int FILTER_NONE			= 0;
const int FILTER_PRELOAD		= 1 << 0;
const int FILTER_RESAMPLE_FAST		= 1 << 1;


SoundReader *ApplyFilters( SoundReader *s, int filters )
{
	if( filters & FILTER_PRELOAD )
	{
		RageSoundReader_Preload *r = new RageSoundReader_Preload();
		if( r->Open( s ) )
			s = r;
		else
		{
			printf( "Didn't preload\n" );
			delete r;
		}
	}
	
	if( filters & FILTER_RESAMPLE_FAST )
	{
		RageSoundReader_Resample *r = RageSoundReader_Resample::MakeResampler( RageSoundReader_Resample::RESAMP_FAST );
		r->Open( s );
		r->SetSampleRate( 10000 );

		s = r;
	}

	return s;
}

bool CheckSetPositionAccurate( SoundReader *snd )
{
	const int one_second=snd->GetSampleRate()*2*2;
	char data[one_second];

	snd->SetPosition_Accurate( 100 );
	ReadData( snd, 100, data, one_second/10 );

	snd->SetPosition_Accurate( 10000 );
	snd->SetPosition_Accurate( 100 );
	if( !test_read( snd, data, one_second/10 ) )
	{
		LOG->Warn("Fail: rewind didn't work");
		return false;
	}

	return true;
}

int FramesOfSilence( const int16_t *data, int frames )
{
	int SilentFrames = 0;
	while( SilentFrames < frames )
	{
		for( int c = 0; c < 2; ++c )
			if( *data++ )
				return SilentFrames;
		++SilentFrames;
	}
	return SilentFrames;
}

/* The number of frames we compare against expected data: */
const int TestDataSize = 2;

/* Find "haystack" in "needle".  Start looking at "expect" and move outward; find
 * the closest. */
void *xmemsearch( const void *haystack_, size_t haystacklen,
		const void *needle_, size_t needlelen,
		int expect )
{
	if( !needlelen )
		return (void *) haystack_;

	const char *haystack = (const char *) haystack_;
	const char *haystack_end = haystack+haystacklen;
	const char *needle = (const char *) needle_;

	int out_len = 0;
	while(1)
	{
		const char *hay_early = haystack + expect - out_len;
		const char *hay_late = haystack + expect + out_len;
		if( hay_early >= haystack && hay_early+needlelen < haystack_end )
		{
			if( !memcmp( hay_early, needle, needlelen ) )
				return (void *) hay_early;
		}

		if( hay_late >= haystack && hay_late+needlelen < haystack_end )
		{
			if( !memcmp( hay_late, needle, needlelen ) )
				return (void *) hay_late;

		}

		if( hay_early < haystack && hay_late + needlelen >= haystack_end )
			break;

		++out_len;
	}

	return NULL;
}

struct TestFile
{
	const char *fn;

	/* The number of silent frames we expect: */
	int SilentFrames;

	/* The first two frames (four samples): */
	int16_t initial[TestDataSize*2];

	/* Frames of data half a second in: */
	int16_t later[TestDataSize*2];
};
const int channels = 2;

bool RunTests( SoundReader *snd, const TestFile &tf )
{
	const char *fn = tf.fn;
	{
		float len = snd->GetLength();
//		printf("%f\n", len);
		if( len < 1000.0f )
		{
			LOG->Warn( "Test file %s is too short", fn );
			return false;
		}
	}

	/* Read the first second of the file.  Do this without calling any
	 * seek functions. */
	const int one_second_frames = snd->GetSampleRate();
	const int one_second=one_second_frames*sizeof(int16_t)*2;
	int16_t sdata[one_second_frames*2];
	char *data = (char *) sdata;
	memset( data, 0x42, one_second );
	ReadData( snd, -1, data, one_second );

	{
		/* Find out how many frames of silence we have. */
		int SilentFrames = FramesOfSilence( sdata, one_second_frames );
		
		const int16_t *InitialData = sdata + SilentFrames*2;
		const int InitialDataSize = one_second_frames - SilentFrames;

		if( InitialDataSize < (int) sizeof(tf.initial) )
		{
			LOG->Warn( "Not enough (%i<%i) data to check after %i frames of silence", InitialDataSize, sizeof(tf.initial), SilentFrames );
			return false;
		}
		
		bool Failed = false;
		if( SilentFrames != tf.SilentFrames  )
		{
			LOG->Trace( "Expected %i silence, got %i (%i too high)", tf.SilentFrames, SilentFrames, SilentFrames-tf.SilentFrames );
			Failed = true;
		}
		
		bool Identical = !memcmp( InitialData, tf.initial, sizeof(tf.initial) );
		if( !Identical )
		{
			LOG->Trace("Expected data:");
			dump( tf.initial, ARRAYSIZE(tf.initial) );
			LOG->Trace(" ");
			Failed = true;
		}
			
		if( Failed )
		{
			LOG->Trace("Got data:");
			dump( InitialData, min( 16, 2*InitialDataSize ) );
		}

		const int LaterOffsetFrames = one_second_frames/2; /* half second */
		const int LaterOffsetSamples = LaterOffsetFrames * channels;
		const int16_t *LaterData = sdata + LaterOffsetSamples;
		Identical = !memcmp( LaterData, tf.later, sizeof(tf.later) );
		if( !Identical )
		{
			LOG->Trace("Expected half second data:");
                        dump( tf.later, ARRAYSIZE(tf.later) );
			LOG->Trace("Got half second data:");
			dump( LaterData, 16 );
			Failed = true;

			/* See if we can find the half second data. */
			int16_t *p = (int16_t *) xmemsearch( sdata, one_second, tf.later, sizeof(tf.later), LaterOffsetSamples*sizeof(int16_t) );
			if( p )
			{
				int SamplesOff = p-sdata;
				int FramesOff = SamplesOff/2;
				LOG->Trace("Found half second data at frame %i (wanted %i), ahead by %i samples",
						FramesOff, LaterOffsetFrames, FramesOff-LaterOffsetFrames );
			}
//			else
//				dump( "foo", sdata, one_second/sizeof(int16_t) );

		}

		if( Failed )
			return false;
	}

	/* Make sure we're getting non-null data. */
	{
		bool all_null=true;
		bool all_42=true;

		for( int i = 0; i < one_second; ++i )
		{
			if( data[i] != 0 )
				all_null=false;
			if( data[i] != 0x42 )
				all_42=false;

		}
		
		if( all_null || all_42 )
		{
			LOG->Warn( "'%s': sanity check failed (%i %i)", fn, all_null, all_42 );
			return false;
		}
	}

	/* Read to EOF, discarding the data. */
	while(1)
	{
		char buf[4096];
		int got = snd->Read( buf, sizeof(buf) );
		if( got < int(sizeof(buf)) )
			break;
	}
	
	/* Now, make sure reading after an EOF returns another EOF. */
	if( !must_be_eof(snd) )
	{
		LOG->Warn("Fail: Reading past EOF didn't EOF");
		return false;
	}

	if( !must_be_eof(snd) )
	{
		LOG->Warn("Fail: Reading past EOF twice didn't EOF");
		return false;
	}
	


	/* SetPosition_Accurate(0) must always reset properly. */
	snd->SetPosition_Accurate( 0 );
	if( !test_read( snd, data, one_second ) )
	{
		LOG->Warn("Fail: SetPosition_Accurate(0) didn't work");
		return false;
	}

	/* SetPosition_Fast(0) must always reset properly.   */
	snd->SetPosition_Fast( 0 );
	if( !test_read( snd, data, one_second ) )
	{
		LOG->Warn("Fail: SetPosition_Fast(0) didn't work");
		return false;
	}

	/* Seek to 1ms and make sure it gives us the correct data. */
	snd->SetPosition_Accurate( 1 );
	if( !test_read( snd, data + one_second * 1/1000, one_second * 1/1000 ) )
	{
		LOG->Warn("Fail: SetPosition_Accurate(1) didn't work");
		return false;
	}

	/* Seek to 500ms and make sure it gives us the correct data. */
	snd->SetPosition_Accurate( 500 );
	if( !test_read( snd, data+one_second * 500/1000, one_second * 500/1000 ) )
	{
		LOG->Warn("Fail: seek(500) didn't work");
		return false;
	}

	/* Make sure seeking past end of file returns 0. */
	int ret2 = snd->SetPosition_Fast( 10000000 );
	if( ret2 != 0 )
	{
		LOG->Warn( "Fail: SetPosition_Fast(1000000) returned %i instead of 0", ret2 );
		return false;
	}

	/* Make sure that reading after a seek past EOF returns EOF. */
	if( !must_be_eof(snd) )
	{
		LOG->Warn("Fail: SetPosition_Fast EOF didn't EOF");
		return false;
	}

	ret2 = snd->SetPosition_Accurate( 10000000 );
	if( ret2 != 0 )
	{
		LOG->Warn( "Fail: SetPosition_Accurate(1000000) returned %i instead of 0", ret2 );
		return false;
	}

	if( !must_be_eof(snd) )
	{
		LOG->Warn("Fail: SetPosition_Fast EOF didn't EOF");
		return false;
	}
	
	/* Seek to a spot that lies exactly on a TOC entry, to check the
	 * case that SetPosition_hard doesn't actually have to do anything. */
	return true;
}


bool test_file( const TestFile &tf, int filters )
{
	const char *fn = tf.fn;
			
	LOG->Trace("Testing: %s", fn );
	CString error;
	SoundReader *s = SoundReader_FileReader::OpenFile( fn, error );
	s = ApplyFilters( s, filters );
	
	if( s == NULL )
	{
		LOG->Trace( "File '%s' failed to open: %s", fn, error.c_str() );
		return false;
	}
	//SoundReader *snd = s;
	SoundReader *snd = s->Copy();
	delete s;

	bool ret = RunTests( snd, tf );

	delete snd;

	/* Check SetPosition_Accurate consistency:
	 * 
	 * Reopen the file from scratch, seek to 100ms, read some data, do some
	 * operations that would result in the internal TOC being filled (seek
	 * to the end), then re-read the data at 100ms and make sure it's the same. */
	
	snd = SoundReader_FileReader::OpenFile( fn, error );
	snd = ApplyFilters( snd, filters );
	
	if( snd == NULL )
	{
		LOG->Trace( "File '%s' failed to open: %s", fn, error.c_str() );
		return false;
	}

	if( !CheckSetPositionAccurate( snd ) )
		ret = false;

	delete snd;
	return ret;
}

int main( int argc, char *argv[] )
{
	test_handle_args( argc, argv );
	test_init();

	TestFile files[] = {
		/* These are all the same data, but they're encoded with different amounts of lossage, so the
		 * values are all similar but, unlike the header tests below, not identical. */
/*		{ "test PCM 44100 stereo.wav",	0, {0xfb6b,0x0076,0xf82b,0x0028}, {0xf598,0xf630,0xf2bd,0xf11d} },
		{ "test ADPCM 44100 stereo.wav",0, {0xfb6b,0x0076,0xf82b,0x0028}, {0xf6d7,0xf54f,0xf300,0xf19e} },
//		{ "test ADPCM 22050 mono.wav",	1, {0xfc2a,0xfc2a,0xf4a8,0xf4a8}, {0xf329,0xf329,0xf1c2,0xf1c2} },
		{ "test PCM8bit 44100 stereo.wav",	0, {0xfb7b,0x0080,0xf878,0x0080}, {0xf575,0xf676,0xf272,0xf171} },
		// XXX: add MP3-in-WAV test
		{ "test OGG 44100 stereo.ogg",	0, {0xfb90,0x00b6,0xf84c,0x00ec}, {0xf794,0xf6c4,0xf34e,0xf2cd} },
		{ "test MP3 first frame corrupt.mp3", 2343, {0x0001,0x0000,0x0001,0x0000}, {0xe12f,0xfe36,0xf337,0x0778} },
		{ "test BASS first frame corrupt.wav", 2343, {0x0001,0x0000,0x0001,0x0000}, {0xe12f,0xfe36,0xf337,0x0778} },
*/
		/* "BASS" is the results of decoding each MP3 with BASS's "writewav" program, in
		 * order to test compatibility with DWI. */

		/* The following all contain the same data; they simply have different tags and headers. */
		{ "test MP3 44100 stereo CBR.mp3", 592, {0x0000,0x0001,0x0000,0x0001}, {0xef22,0x0cd6,0xee84,0x0bb6} },
		{ "test MP3 44100 stereo CBR (ID3V1).mp3", 592, {0x0000,0x0001,0x0000,0x0001}, {0xef22,0x0cd6,0xee84,0x0bb6} },
		{ "test MP3 44100 stereo CBR (ID3V2).mp3", 592, {0x0000,0x0001,0x0000,0x0001}, {0xef22,0x0cd6,0xee84,0x0bb6} },
		{ "test MP3 44100 stereo CBR (INFO, LAME).mp3", 1744, {0x0000,0x0001,0x0000,0x0001}, {0xfc71,0x23ee,0xfb74,0x2141} },

		{ "test BASS 44100 stereo CBR.wav", 592, {0x0000,0x0001,0x0000,0x0001}, {0xef22,0x0cd6,0xee84,0x0bb6} },
		{ "test BASS 44100 stereo CBR (ID3V1).wav", 592, {0x0000,0x0001,0x0000,0x0001}, {0xef22,0x0cd6,0xee84,0x0bb6} },
		{ "test BASS 44100 stereo CBR (ID3V2).wav", 592, {0x0000,0x0001,0x0000,0x0001}, {0xef22,0x0cd6,0xee84,0x0bb6} },
		{ "test BASS 44100 stereo CBR (INFO, LAME).wav", 1744, {0x0000,0x0001,0x0000,0x0001}, {0xfc71,0x23ee,0xfb74,0x2141} },

		/* The following all contain the same data; they simply have different tags and headers. */
		{ "test MP3 44100 stereo VBR (INFO, LAME).mp3", 1774, {0xffff,0x0000,0xffff,0x0000}, {0xfe43,0x262a,0xfd0c,0x22bc} },
		{ "test MP3 44100 stereo VBR (XING, LAME).mp3", 622, {0xffff,0x0000,0xffff,0x0000}, {0xef6c,0x0cb8,0xef10,0x0bd2} },
		{ "test MP3 44100 stereo VBR (XING, LAME, ID3V1, ID3V2).mp3", 622, {0xffff,0x0000,0xffff,0x0000}, {0xef6c,0x0cb8,0xef10,0x0bd2} },

		{ "test BASS 44100 stereo VBR (INFO, LAME).wav", 1774, {0xffff,0x0000,0xffff,0x0000}, {0xfe43,0x262a,0xfd0c,0x22bc} },
		{ "test BASS 44100 stereo VBR (XING, LAME).wav", 622, {0xffff,0x0000,0xffff,0x0000}, {0xef6c,0x0cb8,0xef10,0x0bd2} },
		{ "test BASS 44100 stereo VBR (XING, LAME, ID3V1, ID3V2).wav", 622, {0xffff,0x0000,0xffff,0x0000}, {0xef6c,0x0cb8,0xef10,0x0bd2} },
		{ NULL,							0, {0,0,0,0}, {0,0,0,0} }
	};
	
	for( int i = 0; files[i].fn; ++i )
	{
		if( !test_file( files[i], 0 ) )
			LOG->Trace(" ");
	}

	test_deinit();
	exit(0);
}

