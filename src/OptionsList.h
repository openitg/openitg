#ifndef SCREEN_OPTIONS_LIST_H
#define SCREEN_OPTIONS_LIST_H

#include "ScreenWithMenuElements.h"
#include "RageSound.h"
#include "Steps.h"
#include "Trail.h"
#include "OptionRowHandler.h"
#include "BitmapText.h"
#include "OptionsCursor.h"
#include "ThemeMetric.h"
#include "RageSound.h"
#include "Sprite.h"

class OptionsList;
class OptionListRow: public ActorFrame
{
public:
	void Load( OptionsList *pOptions, const CString &sType );
	void SetFromHandler( const OptionRowHandler *pHandler );
	void SetTextFromHandler( const OptionRowHandler *pHandler );
	void SetUnderlines( const vector<bool> &aSelections, const OptionRowHandler *pHandler );

	void PositionCursor( Actor *pCursor, int iSelection );

	void Start();

private:
	OptionsList *m_pOptions;

	CString m_sType;

	vector<BitmapText> m_Text;
	// underline for each ("self or child has selection")
	vector<AutoActor> m_Underlines;

	bool	m_bItemsInTwoRows;

	ThemeMetric<float>	ITEMS_SPACING_Y;
};

class OptionsList: public ActorFrame
{
public:
	friend class OptionListRow;

	OptionsList();
	~OptionsList();

	void Load( CString sType, PlayerNumber pn );
	void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	void Reset();

	void Link( OptionsList *pLink ) { m_pLinked = pLink; }

	void Open();	/* Show the top-level menu. */
	void Close();	/* Close all menus (for menu timer). */
	bool Start();	/* return true if the last menu was popped in response to this press */

	bool IsOpened() const { return m_asMenuStack.size() > 0; }

private:
	ThemeMetric<CString> TOP_MENU;

	void SelectItem( const CString &sRowName, int iMenuItem );
	void MoveItem( const CString &sRowName, int iMove );
	void SwitchMenu( int iDir );
	void PositionCursor();
	void SelectionsChanged(  const CString &sRowName );
	void UpdateMenuFromSelections();
	CString GetCurrentRow() const;
	const OptionRowHandler *GetCurrentHandler();
	int GetOneSelection( CString sRow, bool bAllowFail=false ) const;
	void SwitchToCurrentRow();

	void TweenOnCurrentRow( bool bForward );
	void SetDefaultCurrentRow();
	void Push( CString sDest );
	void Pop();
	void ImportRow( CString sRow );
	void ExportRow( CString sRow );

	static int FindScreenInHandler( OptionRowDefinition *ordef, const OptionRowHandler *pHandler, CString sScreen );

	//InputQueueCodeSet	m_Codes;

	OptionsList		*m_pLinked;

	bool			m_bStartIsDown;
	bool			m_bAcceptStartRelease;

	vector<CString> m_asLoadedRows;
	map<CString, OptionRowHandler *> m_Rows;
	map<OptionRowHandler *, OptionRowDefinition *> m_RowDefs;
	map<CString, vector<bool> > m_bSelections;
	set<CString> m_setDirectRows;
	set<CString> m_setTopMenus; /* list of top-level menus, pointing to submenus */

	PlayerNumber m_pn;
	AutoActor m_Cursor;
	OptionListRow m_Row[2];
	int m_iCurrentRow;

	vector<CString> m_asMenuStack;
	int m_iMenuStackSelection;

	RageSound m_soundLeft;
	RageSound m_soundRight;
	RageSound m_soundOpened;
	RageSound m_soundClosed;
	RageSound m_soundPush;
	RageSound m_soundPop;
	RageSound m_soundStart;

};


#endif

/*
 * Copyright (c) 2006 Glenn Maynard
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

