#ifndef SCREEN_EDIT_MENU_H
#define SCREEN_EDIT_MENU_H

#include "ScreenWithMenuElements.h"
#include "EditMenu.h"
#include "BitmapText.h"

class ScreenEditMenu : public ScreenWithMenuElements
{
public:
	ScreenEditMenu( CString sName );
	virtual void Init();

	virtual void HandleScreenMessage( const ScreenMessage SM );

private:

	void MenuUp( PlayerNumber pn );
	void MenuDown( PlayerNumber pn );
	void MenuLeft( PlayerNumber pn, const InputEventType type );
	void MenuRight( PlayerNumber pn, const InputEventType type );
	void MenuBack( PlayerNumber pn );
	void MenuStart( PlayerNumber pn );

	void RefreshExplanationText();
	void RefreshNumStepsLoadedFromProfile();
public:
	EditMenu		m_Selector;
private:
	BitmapText		m_textExplanation;
	BitmapText		m_textNumStepsLoadedFromProfile;
};

#endif

/*
 * (c) 2002-2004 Chris Danford
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
