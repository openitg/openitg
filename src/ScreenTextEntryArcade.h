/* ScreenTextEntryArcade - a more arcade-friendly text entry method */

#ifndef SCREEN_TEXT_ENTRY_ARCADE_H
#define SCREEN_TEXT_ENTRY_ARCADE_H

#include "ScreenTextEntry.h"
#include "BitmapText.h"
#include "HelpText.h"

enum ArcadeKeyboardRow
{
	AC_KEYBOARD_ROW_A-M,
	AC_KEYBOARD_ROW_N-Z,
	AC_KEYBOARD_ROW_NUM,
	NUM_AC_KEYBOARD_ROWS
};
#define FOREACH_ArcadeKeyboardRow( i ) FOREACH_ENUM( ArcadeKeyboardRow, NUM_AC_KEYBOARD_ROWS, i )

const int KEYS_PER_ROW = 13;

class ScreenTextEntryArcade : public ScreenTextEntry
{
public:
	ScreenTextEntryArcade(
		CString sName,
		ScreenMessage smSendOnPop,
		CString sQuestion,
		CString sInitialAnswer,
		int iMaxInputLength,
		bool(*Validate)(CString sAnswer,CString &sErrorOut) = NULL,
		void(*OnOK)(CString sAnswer) = NULL,
		void(*OnCancel)() = NULL,
		bool bPassword = false );
	~ScreenTextEntryArcade();

	virtual void Init();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );

private:
	virtual void InitKeyboard();

	void UpdateKeyboardText();

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuLeft( PlayerNumber pn, const InputEventType type );
	virtual void MenuRight( PlayerNumber pn, const InputEventType type );

	virtual void LoadHelpText();

	// used to switch letter case and numbers to symbols
	bool		m_bShifted;

	// m_iFocusX is already in ScreenTextEntry - this is for clarity
	int			m_iFocusX;
	ArcadeKeyboardRow	m_iFocusY;	

	HelpDisplay	*m_textHelp;
	BitmapText	m_textKeyboardChars[NUM_AC_KEYBOARD_ROWS][KEYS_PER_ROW];
};

#endif // SCREEN_TEXT_ENTRY_ARCADE_H
