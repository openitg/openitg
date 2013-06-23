/* ScreenTextEntry - Displays a text entry box over the top of another screen. */

#ifndef SCREEN_TEXT_ENTRY_H
#define SCREEN_TEXT_ENTRY_H

#include "Screen.h"
#include "BitmapText.h"
#include "Transition.h"
#include "Quad.h"
#include "RandomSample.h"
#include "BGAnimation.h"
#include "EnumHelper.h"

enum KeyboardRow
{
	R1, R2, R3, R4, R5, R6, R7,
	KEYBOARD_ROW_SPECIAL,
	NUM_KEYBOARD_ROWS
};
#define FOREACH_KeyboardRow( i ) FOREACH_ENUM( KeyboardRow, NUM_KEYBOARD_ROWS, i )
const int KEYS_PER_ROW = 13;
enum KeyboardRowSpecialKey
{
	SPACEBAR=2, BACKSPACE=5, CANCEL=8, DONE=11
};

class ScreenTextEntry : public Screen
{
public:
	ScreenTextEntry( 
		CString sName, 
		ScreenMessage smSendOnPop,
		CString sQuestion, 
		CString sInitialAnswer, 
		int iMaxInputLength,
		bool(*Validate)(CString sAnswer,CString &sErrorOut) = NULL, 
		void(*OnOK)(CString sAnswer) = NULL, 
		void(*OnCanel)() = NULL, 
		bool bPassword = false );
	~ScreenTextEntry();
	virtual void Init();

	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	static CString s_sLastAnswer;
	static bool s_bCancelledLast;

protected:
	virtual void InitKeyboard();

	virtual void MoveX( int iDir );
	virtual void MoveY( int iDir );
	
	virtual void AppendToAnswer( CString s );
	virtual void BackspaceInAnswer();

	virtual void End( bool bCancelled );

	virtual void MenuLeft( PlayerNumber pn )	{ MoveX(-1); }
	virtual void MenuRight( PlayerNumber pn )	{ MoveX(+1); }
	virtual void MenuUp( PlayerNumber pn )		{ MoveY(-1); }
	virtual void MenuDown( PlayerNumber pn )	{ MoveY(+1); }
	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );

	virtual void UpdateKeyboardText();
	virtual void UpdateAnswerText();

	ScreenMessage	m_smSendOnPop;
	CString			m_sQuestion;
	int				m_iMaxInputLength;
	bool            m_bPassword;
	AutoActor		m_Background;
	BitmapText		m_textQuestion;
	AutoActor		m_sprAnswerBox;
	wstring			m_sAnswer;
	BitmapText		m_textAnswer;
	bool(*m_pValidate)( CString sAnswer, CString &sErrorOut );
	void(*m_pOnOK)( CString sAnswer );
	void(*m_pOnCancel)();
	
	AutoActor		m_sprCursor;

	int				m_iFocusX;
	KeyboardRow		m_iFocusY;
	
	void PositionCursor();

	BitmapText		m_textKeyboardChars[NUM_KEYBOARD_ROWS][KEYS_PER_ROW];

	Transition		m_In;
	Transition		m_Out;
	Transition		m_Cancel;

	RageSound	m_sndType;
	RageSound	m_sndBackspace;
	RageSound	m_sndChange;
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
