#include "global.h"
#include "Banner.h"
#include "PrefsManager.h"
#include "SongManager.h"
#include "song.h"
#include "RageTextureManager.h"
#include "Character.h"

ThemeMetric<bool> SCROLL_RANDOM		("Banner","ScrollRandom");
ThemeMetric<bool> SCROLL_ROULETTE		("Banner","ScrollRoulette");

Banner::Banner()
{
	m_bScrolling = false;
	m_fPercentScrolling = 0;
}

void Banner::CacheGlobalBanners()
{
	if( PREFSMAN->m_BannerCache == PrefsManager::BNCACHE_OFF )
		return;

	TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Banner","all music")) );
	TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Common","fallback banner")) );
	TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Banner","roulette")) );
	TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Banner","random")) );
	TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Banner","Mode")) );
	TEXTUREMAN->CacheTexture( SongBannerTexture(THEME->GetPathG("Banner","custom")) );
}

bool Banner::Load( RageTextureID ID )
{
	if( ID.filename == "" )
		ID = THEME->GetPathG("Common","fallback banner");

	ID = SongBannerTexture(ID);

	m_fPercentScrolling = 0;
	m_bScrolling = false;

	TEXTUREMAN->DisableOddDimensionWarning();
	TEXTUREMAN->VolatileTexture( ID );
	bool ret = Sprite::Load( ID );
	TEXTUREMAN->EnableOddDimensionWarning();

	return ret;
};

void Banner::Update( float fDeltaTime )
{
	Sprite::Update( fDeltaTime );

	if( m_bScrolling )
	{
		m_fPercentScrolling += fDeltaTime/2; 
		m_fPercentScrolling -= (int)m_fPercentScrolling;

		const RectF *pTextureRect = m_pTexture->GetTextureCoordRect(0);
 
		float fTexCoords[8] = 
		{
			0+m_fPercentScrolling, pTextureRect->top,		// top left
			0+m_fPercentScrolling, pTextureRect->bottom,	// bottom left
			1+m_fPercentScrolling, pTextureRect->bottom,	// bottom right
			1+m_fPercentScrolling, pTextureRect->top,		// top right
		};
		Sprite::SetCustomTextureCoords( fTexCoords );
	}
}

void Banner::SetScrolling( bool bScroll, float Percent)
{
	m_bScrolling = bScroll;
	m_fPercentScrolling = Percent;

	/* Set up the texture coord rects for the current state. */
	Update(0);
}

void Banner::LoadFromSong( Song* pSong )		
{
	if( pSong == NULL ) // NULL means no song
		LoadFallback();
	else if( pSong->IsCustomSong() )
		LoadCustom();
	else if( pSong->HasBanner() )
		Load( pSong->GetBannerPath() );
	else
		LoadFallback();

	m_bScrolling = false;
}

void Banner::LoadAllMusic()
{
	Load( THEME->GetPathG("Banner","All") );
	m_bScrolling = false;
}

void Banner::LoadMode()
{
	Load( THEME->GetPathG("Banner","Mode") );
	m_bScrolling = false;
}

void Banner::LoadCustom()
{
	Load( THEME->GetPathG("Banner","custom") );
	m_bScrolling = false;
}

void Banner::LoadFromGroup( CString sGroupName )
{
	CString sGroupBannerPath = SONGMAN->GetGroupBannerPath( sGroupName );
	if( sGroupBannerPath != "" )	Load( sGroupBannerPath );
	else							LoadFallback();
	m_bScrolling = false;
}

void Banner::LoadFromCourse( Course* pCourse )		// NULL means no course
{
	if( pCourse == NULL )						LoadFallback();
	else if( pCourse->m_sBannerPath != "" )		Load( pCourse->m_sBannerPath );
	else										LoadFallback();

	m_bScrolling = false;
}

void Banner::LoadCardFromCharacter( Character* pCharacter )	
{
	ASSERT( pCharacter );

	if( pCharacter->GetCardPath() != "" )		Load( pCharacter->GetCardPath() );
	else										LoadFallback();

	m_bScrolling = false;
}

void Banner::LoadIconFromCharacter( Character* pCharacter )	
{
	ASSERT( pCharacter );

	if( pCharacter->GetIconPath() != "" )		Load( pCharacter->GetIconPath() );
	else if( pCharacter->GetCardPath() != "" )	Load( pCharacter->GetCardPath() );
	else										LoadFallback();

	m_bScrolling = false;
}

void Banner::LoadTABreakFromCharacter( Character* pCharacter )
{
	if( pCharacter == NULL )
	{
		Load( THEME->GetPathG("Common","fallback takingabreak") );
	}
	else 
	{
		Load( pCharacter->GetTakingABreakPath() );
		m_bScrolling = false;
	}
}

void Banner::LoadFallback()
{
	Load( THEME->GetPathG("Common","fallback banner") );
}

void Banner::LoadRoulette()
{
	Load( THEME->GetPathG("Banner","roulette") );
	m_bScrolling = (bool)SCROLL_ROULETTE;
}

void Banner::LoadRandom()
{
	Load( THEME->GetPathG("Banner","random") );
	m_bScrolling = (bool)SCROLL_RANDOM;
}


/*
 * (c) 2001-2004 Chris Danford
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
