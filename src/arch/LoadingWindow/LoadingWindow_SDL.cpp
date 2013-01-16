#include "global.h"

#include "SDL_utils.h"
#include "RageSurface_Load_XPM.h"
#include "SDL_video.h"
#include "LoadingWindow_SDL.h"
#include "loading.xpm"
#include "StepMania.xpm" /* icon */
#include "RageSurface.h"
#include "RageSurfaceUtils.h"
#include "RageSurfaceUtils_Zoom.h"

REGISTER_LOADING_WINDOW( SDL );

LoadingWindow_SDL::LoadingWindow_SDL()
{
	/* There's no consistent way to hint SDL that we want a centered
	 * window.  X11 way: */
#if defined(unix)
	static char center[]="SDL_VIDEO_CENTERED=1";
	putenv( center );
#endif

    /* Initialize the SDL library */
    if( SDL_InitSubSystem(SDL_INIT_VIDEO) < 0 )
        RageException::Throw( "Couldn't initialize SDL: %s", SDL_GetError() );

	/* Set window title and icon */
	SDL_WM_SetCaption("Loading StepMania", "");

	CString error;
	RageSurface *srf = RageSurface_Load_XPM( icon, error );

	uint32_t color;
	if( srf->fmt.MapRGBA( 0xFF, 0, 0xFF, 0xFF, color ) )
		srf->format->palette->colors[ color ].a = 0;

#if !defined(DARWIN)
	/* Windows icons are 32x32 and SDL can't resize them for us, which
	 * causes mask corruption.  (Actually, the above icon *is* 32x32;
	 * this is here just in case it changes.) */
	RageSurfaceUtils::Zoom( srf, 32, 32 );

	{
		SDL_Surface *pSDLSurface = SDLSurfaceFromRageSurface( srf );

		SDL_SetAlpha( pSDLSurface, SDL_SRCALPHA, SDL_ALPHA_OPAQUE );
		SDL_WM_SetIcon( pSDLSurface, NULL ); /* derive from alpha */
		SDL_FreeSurface( pSDLSurface );
	}

	delete srf;
#endif


	/* Load the BMP - we need its dimensions */
    srf = RageSurface_Load_XPM( loading, error );
    if( srf == NULL ) // XXX SDL_GetError
        RageException::Throw( "Couldn't load loading.bmp: %s",SDL_GetError() );


    /* Initialize the window */
    loading_screen = SDL_SetVideoMode( srf->w, srf->h, 16, SDL_SWSURFACE|SDL_ANYFORMAT|SDL_NOFRAME );
    if( loading_screen == NULL )
        RageException::Throw( "Couldn't initialize loading window: %s", SDL_GetError() );

	{
		SDL_Surface *pSDLSurface = SDLSurfaceFromRageSurface( srf );
		SDL_BlitSurface( pSDLSurface, NULL, loading_screen, NULL );
		SDL_FreeSurface( pSDLSurface );
	}

    delete srf;

	SDL_UpdateRect( loading_screen, 0,0,0,0 );
}

LoadingWindow_SDL::~LoadingWindow_SDL()
{
	SDL_QuitSubSystem( SDL_INIT_VIDEO );
}

void LoadingWindow_SDL::Paint()
{
	SDL_UpdateRect(loading_screen, 0,0,0,0);
}

/*
 * (c) 2001-2004 Chris Danford, Glenn Maynard
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
