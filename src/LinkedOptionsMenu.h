/* LinkedOptionsMenu - dirt simple single-column options menu for ScreenAddSongs */
#ifndef LINKED_OPTIONS_MENU_H
#define LINKED_OPTIONS_MENU_H
#include "global.h"
#include "AutoActor.h"
#include "ActorFrame.h"
#include "OptionsCursor.h"
#include "RageInputDevice.h"
#include "GameInput.h"
#include "InputFilter.h"
#include "StyleInput.h"
#include "MenuInput.h"
#include "BitmapText.h"
#include "ThemeMetric.h"
#include "ScreenMessage.h"

enum LinkedInputResponseType
{
	LIRT_ACCEPTED = 0,
	LIRT_FORWARDED_PREV, // goes to next menu
	LIRT_FORWARDED_NEXT, // goes back to previous menu
	LIRT_STOP, // returned when no cursor change was made (next or previous menu is NULL)
	LIRT_CHOOSE, // user pressed START, choice is returned in Input()
	NUM_LinkedInputResponseType,
	LIRT_INVALID
};

// TODO: make the LOM Frame be the container for all the menus rather than
//     work with it all in Screen::Input()
class LinkedOptionsMenu: public ActorFrame
{
public:
	void Load( LinkedOptionsMenu *prevMenu, LinkedOptionsMenu *nextMenu );
	LinkedInputResponseType Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI, LinkedOptionsMenu *&pFocusedMenu );
	void SetChoices( const CStringArray &asChoices );
	void SetChoiceIndex( int iChoice );
	void SetMenuChangeScreenMessage( const ScreenMessage SM ) { m_smChangeMenu = SM; }
	void ClearChoices();
	void ControlCursor( int iRowStart );
	void Focus();
	void Unfocus();
	int GetChoiceCount() { return m_Rows.size(); }
	CString GetCurrentSelection();
	LinkedInputResponseType MoveRow(int iDirection);
	void SetPage(int iPage);
	LinkedOptionsMenu *GetNextMenu();
	LinkedOptionsMenu *GetPrevMenu();
	LinkedOptionsMenu *SwitchToNextMenu();
	LinkedOptionsMenu *SwitchToPrevMenu();

private:
	vector<BitmapText*>				m_Rows;
	OptionsCursor					m_Cursor;
	ActorFrame						m_Frame;
	AutoActor						m_FramePage;
	Sprite							m_sprLineHighlight;

	Sprite							m_sprIndicatorUp;
	Sprite							m_sprIndicatorDown;
	bool							m_bIndTweenedOn[2];

	bool							m_bFocus;

	int								m_iCurrentSelection;
	int								m_iCurPage;

	ScreenMessage					m_smChangeMenu;

	ThemeMetric<float>				ROW_SPACING_Y;
	ThemeMetric<float>				ROW_OFFSET_Y;
	ThemeMetric<float>				ROW_OFFSET_X;
	ThemeMetric<float>				CURSOR_OFFSET_X;
	ThemeMetric<bool>				MENU_WRAPPING;
	ThemeMetric<int>				ROWS_PER_PAGE;

public:
	LinkedOptionsMenu*				m_PrevMenu;
	LinkedOptionsMenu*				m_NextMenu;
};


#endif /* LINKED_OPTIONS_MENU_H */
/*
 * (c) 2009 Pat McIlroy ("infamouspat")
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
