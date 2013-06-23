/* ScreenTitleMenu - The main title screen and menu. */

#ifndef SCREEN_TITLE_MENU_H
#define SCREEN_TITLE_MENU_H

#include "Screen.h"
#include "Transition.h"
#include "Sprite.h"
#include "BitmapText.h"
#include "RageSound.h"
#include "RageTimer.h"
#include "ScreenSelectMaster.h"

class ScreenTitleMenu : public ScreenSelectMaster
{
public:
	ScreenTitleMenu( CString sName );
	virtual void Init();
	virtual ~ScreenTitleMenu();

	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );

	virtual void HandleMessage( const CString& sMessage );

private:
	/* this is a workaround for a rather nasty bug... */
	bool			m_bIgnoreCoinChange;

	AutoActor		m_sprLogo;
	BitmapText		m_textVersion;
	BitmapText		m_textSongs;
	BitmapText		m_textMaxStages;
	BitmapText		m_textLifeDifficulty;
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
