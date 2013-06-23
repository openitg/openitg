/* XXX: this is going to take a lot of work to convert over to the
 * SM4 RageDriver structure. We'll do that later. For now, we'll
 * just stuff arch.cpp's MakeRageMovieTexture here... -- Vyhd */

#include "global.h"
#include "MovieTexture.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "MovieTexture_Null.h"
#include "Preference.h"
#include "RageFile.h"

#include "Selector_MovieTexture.h"

static Preference<CString> g_sMovieDrivers( "MovieDrivers", "" ); // "" = DEFAULT_MOVIE_DRIVER_LIST

static void DumpAVIDebugInfo(CString);

#include "arch/arch_default.h"

/* Try drivers in order of preference until we find one that works. */
RageMovieTexture *RageMovieTexture::Create( RageTextureID ID )
{
	DumpAVIDebugInfo( ID.filename );

	CString drivers = g_sMovieDrivers;
	if( drivers.empty() )	
		drivers = DEFAULT_MOVIE_DRIVER_LIST;

	CStringArray DriversToTry;
	split(drivers, ",", DriversToTry, true);
	ASSERT(DriversToTry.size() != 0);

	CString Driver;
	RageMovieTexture *ret = NULL;

	for( unsigned i=0; ret==NULL && i<DriversToTry.size(); ++i )
	{
		Driver = DriversToTry[i];
		LOG->Trace("Initializing driver: %s", Driver.c_str());
#ifdef USE_MOVIE_TEXTURE_DSHOW
		if( !Driver.CompareNoCase("DShow") ) ret = new MovieTexture_DShow(ID);
#endif
#ifdef USE_MOVIE_TEXTURE_FFMPEG
		if( !Driver.CompareNoCase("FFMpeg") ) ret = new MovieTexture_FFMpeg(ID);
#endif
#ifdef USE_MOVIE_TEXTURE_NULL
		if( !Driver.CompareNoCase("Null") ) ret = new MovieTexture_Null(ID);
#endif
		if( ret == NULL )
		{
			LOG->Warn( "Unknown movie driver name: %s", Driver.c_str() );
			continue;
		}

		CString sError = ret->Init();
		if( sError != "" )
		{
			LOG->Info( "Couldn't load driver %s: %s", Driver.c_str(), sError.c_str() );
			SAFE_DELETE( ret );
		}
	}
	if (!ret)
		RageException::Throw("Couldn't create a movie texture");

	LOG->Trace("Created movie texture \"%s\" with driver \"%s\"",
		ID.filename.c_str(), Driver.c_str() );
	return ret;
}

// Helper for MakeRageMovieTexture()
static void DumpAVIDebugInfo( CString fn )
{
	CString type, handler;
	if( !RageMovieTexture::GetFourCC( fn, handler, type ) )
		return;

	LOG->Trace("Movie %s has handler '%s', type '%s'", fn.c_str(), handler.c_str(), type.c_str());
}

void ForceToAscii( CString &str )
{
	for( unsigned i=0; i<str.size(); ++i )
		if( str[i] < 0x20 || str[i] > 0x7E )
			str[i] = '?';
}

bool RageMovieTexture::GetFourCC( CString fn, CString &handler, CString &type )
{
	CString ignore, ext;
	splitpath( fn, ignore, ignore, ext);
	if( !ext.CompareNoCase(".mpg") ||
		!ext.CompareNoCase(".mpeg") ||
		!ext.CompareNoCase(".mpv") ||
		!ext.CompareNoCase(".mpe") )
	{
		handler = type = "MPEG";
		return true;
	}

	//Not very pretty but should do all the same error checking without iostream
#define HANDLE_ERROR(x) { \
		LOG->Warn( "Error reading %s: %s", fn.c_str(), x ); \
		handler = type = ""; \
		return false; \
	}

	RageFile file;
	if( !file.Open(fn) )
		HANDLE_ERROR("Could not open file.");
	if( !file.Seek(0x70) )
		HANDLE_ERROR("Could not seek.");
	type = "    ";
	if( file.Read((char *)type.c_str(), 4) != 4 )
		HANDLE_ERROR("Could not read.");
	ForceToAscii( type );
	
	if( file.Seek(0xBC) != 0xBC )
		HANDLE_ERROR("Could not seek.");
	handler = "    ";
	if( file.Read((char *)handler.c_str(), 4) != 4 )
		HANDLE_ERROR("Could not read.");
	ForceToAscii( handler );

	return true;
#undef HANDLE_ERROR
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
