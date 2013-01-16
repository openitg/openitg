/* ScreenSelectGroup - Set the current song group from a list. Deprecated. Replaced by ScreenSelectMaster. */

#ifndef SCREEN_SELECT_GROUP_H
#define SCREEN_SELECT_GROUP_H

#include "ScreenWithMenuElements.h"
#include "Sprite.h"
#include "RandomSample.h"
#include "FadingBanner.h"
#include "MusicList.h"
#include "GroupList.h"


class ScreenSelectGroup : public ScreenWithMenuElements
{
public:
	ScreenSelectGroup( CString sName );
	virtual void Init();

	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	void AfterChange();

	void MenuLeft( PlayerNumber pn );
	void MenuRight( PlayerNumber pn );
	void MenuUp( PlayerNumber pn );
	void MenuDown( PlayerNumber pn );
	void MenuStart( PlayerNumber pn );
	void MenuBack( PlayerNumber pn );

	void TweenOffScreen();
	void TweenOnScreen();

private:

	Sprite			m_sprExplanation;
	Sprite			m_sprFrame;
	FadingBanner	m_Banner;
	BitmapText		m_textNumber;
	Sprite			m_sprContents;
	
	MusicList		m_MusicList;
	GroupList		m_GroupList;

	RandomSample m_soundChange;

	bool m_bChosen;
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
