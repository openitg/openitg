#include "global.h"
#include "ScreenTextEntryArcade.h"

static char* g_szKeys[NUM_AC_KEYBOARD_ROWS][KEYS_PER_ROW] =
{
	{"a","b","c","d","e","f","g","h","i","j","k","l","m"},
	{"n","o","p","q","r","s","t","u","v","w","x","y","z"},
	{"1","2","3","4","5","6","7","8","9","0"}
};

CString ScreenTextEntryArcade::s_sLastAnswer = "";
bool ScreenTextEntryArcade::s_bCancelledLast = false;

ScreenTextEntryArcade::ScreenTextEntryArcade( CString sClassName, ScreenMessage smSendOnPop, CString sQuestion, CString sInitialAnswer,	int iMaxInputLength,
	bool(*Validate)(CString sAnswer, CString &sErrorOut), void(*OnOK)(CString sAnswer), void(*OnCancel)(), bool bPassword ) :
	ScreenTextEntry( sClassName, smSendOnPop, sQuestion, sInitialAnswer, iMaxInputLength, Validate, OnOK, OnCancel, bPassword )
{
}

float GetButtonX( int x )
{
	return roundf( SCALE(x, 0, KEYS_PER_ROW-1, SCREEN_LEFT+100, SCREEN_RIGHT-100) );
}

float GetButtonY( ArcadeKeyboardRow r )
{
	return roundf( SCALE(r, 0, NUM_AC_KEYBOARD_ROWS-1, SCREEN_CENTER_Y-30, SCREEN_BOTTOM-80) );
}

void ScreenTextEntryArcade::Init()
{
	ScreenTextEntry::Init();

	InitKeyboard();
}

void ScreenTextEntryArcade::InitKeyboard()
{
	FOREACH_ArcadeKeyboardRow( row )
	{
		for( int key = 0; key < KEYS_PER_ROW; key++ )
		{
			BitmapText &bt = m_textKeyboardChars[row][key];
			bt.LoadFromFont( THEME->GetPathF(m_sName, "keyboard") );
			bt.SetXY( GetButtonX(key), GetButtonY(row) );
			this->AddChild( &bt );
		}
	}
}

void ScreenTextEntryArcade::UpdateKeyboardText()
{
	FOREACH_ArcadeKeyboardRow( row )
	{
		for( int key = 0; key < KEYS_PER_ROW; key++ )
		{
			char *s = g_szKeys[row][key];
			BitmapText &bt = m_textKeyboardChars[row][key];
			bt.SetText( s );
		}
	}
}

void ScreenTextEntryArcade::ToggleShift()
{
	CString sBuffer;

	for( int row = 0; row < AC_KEYBOARD_ROW_NUM; row++ )
	{
		for( int key = 0; key < KEYS_PER_ROW; key++ )
		{
			if( g_szKeys[row][key] == "" )
				continue;

			sBuffer = g_szKeys[row][key];
			g_szKeys[row][key] = m_bShifted ? sBuffer.MakeLower() : sBuffer.MakeUpper();
		}
	}

	if( m_iFocusY == AC_KEYBOARD_ROW_NUM )
	{
		switch( m_iFocusX )
		{
			case "1": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "1" : "!"; break;
			case "2": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "2" : "?"; break;
			case "3": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "3" : "."; break;
			case "4": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "4" : ","; break;
			case "5": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "5" : "_"; break;
			case "6": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "6" : "*"; break;
			case "7": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "7" : "("; break;
			case "8": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "8" : ")"; break;
			case "9": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "9" : "<"; break;
			case "0": g_szKeys[m_iFocusY][m_iFocusX] = m_bShifted ? "0" : ">"; break;
		}
	}

	// toggle
	m_bShifted = !m_bShifted;
}

