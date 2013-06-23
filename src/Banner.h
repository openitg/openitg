/* Banner - The song's banner displayed in SelectSong. */

#ifndef BANNER_H
#define BANNER_H

#include "Sprite.h"
#include "RageTextureID.h"
class Song;
class Course;
class Character;

class Banner : public Sprite
{
public:
	Banner();
	virtual ~Banner() { }

	virtual bool Load( RageTextureID ID );

	virtual void Update( float fDeltaTime );

	void LoadFromSong( Song* pSong );		// NULL means no song
	void LoadAllMusic();
	void LoadMode();
	void LoadCustom();
	void LoadFromGroup( CString sGroupName );
	void LoadFromCourse( Course* pCourse );
	void LoadCardFromCharacter( Character* pCharacter );
	void LoadIconFromCharacter( Character* pCharacter );
	void LoadTABreakFromCharacter( Character* pCharacter );
	void LoadRoulette();
	void LoadRandom();
	void LoadFallback();

	static void CacheGlobalBanners();

	void SetScrolling( bool bScroll, float Percent = 0);
	bool IsScrolling() const { return m_bScrolling; }
	float ScrollingPercent() const { return m_fPercentScrolling; }

protected:

	bool m_bScrolling;
	float m_fPercentScrolling;
};

#endif

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
