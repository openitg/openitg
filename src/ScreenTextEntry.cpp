#include "global.h"
#include "ScreenTextEntry.h"
#include "PrefsManager.h"
#include "ScreenManager.h"
#include "GameSoundManager.h"
#include "GameConstantsAndTypes.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "FontCharAliases.h"
#include "ScreenDimensions.h"
#include "ActorUtil.h"


static const char* g_szKeys[NUM_KEYBOARD_ROWS][KEYS_PER_ROW] =
{
	{"A","B","C","D","E","F","G","H","I","J","K","L","M"},
	{"N","O","P","Q","R","S","T","U","V","W","X","Y","Z"},
	{"a","b","c","d","e","f","g","h","i","j","k","l","m"},
	{"n","o","p","q","r","s","t","u","v","w","x","y","z"},
	{"0","1","2","3","4","5","6","7","8","9"},
	{"!","@","#","$","%","^","&","(",")","[","]","{","}"},
	{"+","-","=","_",",",".","'","\"",":"},
	{"","","Space","","","Backsp","","","Cancel","","","Done",""},
};

static Preference<bool> g_bAllowOldKeyboardInput( "AllowOldKeyboardInput",	true );

float GetButtonX( int x )
{
	return roundf( SCALE( x, 0, KEYS_PER_ROW-1, SCREEN_LEFT+100, SCREEN_RIGHT-100 ) );
}

float GetButtonY( KeyboardRow r )
{
	return roundf( SCALE( r, 0, NUM_KEYBOARD_ROWS-1, SCREEN_CENTER_Y-30, SCREEN_BOTTOM-80 ) );
}

CString ScreenTextEntry::s_sLastAnswer = "";
bool ScreenTextEntry::s_bCancelledLast = false;

/* XXX: Don't let the user use internal-use codepoints (those
 * that resolve to Unicode codepoints above 0xFFFF); those are
 * subject to change and shouldn't be written to .SMs.
 *
 * Handle UTF-8.  Right now, we need to at least be able to backspace
 * a whole UTF-8 character.  Better would be to operate in longchars.
 */
//REGISTER_SCREEN_CLASS( ScreenTextEntry );

ScreenTextEntry::ScreenTextEntry( 
	CString sClassName, 
	ScreenMessage smSendOnPop,
	CString sQuestion, 
	CString sInitialAnswer, 
	int iMaxInputLength,
	bool(*Validate)(CString sAnswer,CString &sErrorOut), 
	void(*OnOK)(CString sAnswer), 
	void(*OnCancel)(), 
	bool bPassword ) :
	Screen( sClassName )
{
	m_bIsTransparent = true;	// draw screens below us

	m_smSendOnPop = smSendOnPop;
	m_sQuestion = sQuestion;
	m_sAnswer = CStringToWstring( sInitialAnswer );
	m_iMaxInputLength = iMaxInputLength;
	m_pValidate = Validate;
	m_pOnOK = OnOK;
	m_pOnCancel = OnCancel;
	m_bPassword = bPassword;
}

void ScreenTextEntry::Init()
{
	m_Background.Load( THEME->GetPathB(m_sName,"background") );
	m_Background->SetDrawOrder( DRAW_ORDER_BEFORE_EVERYTHING );
	this->AddChild( m_Background );
	m_Background->PlayCommand( "On" );

	m_textQuestion.LoadFromFont( THEME->GetPathF(m_sName,"question") );
	m_textQuestion.SetName( "Question" );
	m_textQuestion.SetText( m_sQuestion );
	SET_XY_AND_ON_COMMAND( m_textQuestion );
	this->AddChild( &m_textQuestion );

	m_sprAnswerBox.Load( THEME->GetPathG(m_sName,"AnswerBox") );
	m_sprAnswerBox->SetName( "AnswerBox" );
	SET_XY_AND_ON_COMMAND( m_sprAnswerBox );
	this->AddChild( m_sprAnswerBox );

	m_textAnswer.LoadFromFont( THEME->GetPathF(m_sName,"answer") );
	m_textAnswer.SetName( "Answer" );
	SET_XY_AND_ON_COMMAND( m_textAnswer );
	UpdateAnswerText();
	this->AddChild( &m_textAnswer );


	m_sprCursor.Load( THEME->GetPathG(m_sName,"cursor") );
	m_sprCursor->SetName( "Cursor" );
	ON_COMMAND( m_sprCursor );
	this->AddChild( m_sprCursor );

	m_iFocusX = 0;
	m_iFocusY = (KeyboardRow)0;

	// load the most derived versions
	this->InitKeyboard();
	this->UpdateKeyboardText();
	
	PositionCursor();

	m_In.Load( THEME->GetPathB(m_sName,"in") );
	m_In.StartTransitioning();
	this->AddChild( &m_In );
	
	m_Out.Load( THEME->GetPathB(m_sName,"out") );
	this->AddChild( &m_Out );
	
	m_Cancel.Load( THEME->GetPathB(m_sName,"cancel") );
	this->AddChild( &m_Cancel );


	m_sndType.Load( THEME->GetPathS(m_sName,"type"), true );
	m_sndBackspace.Load( THEME->GetPathS(m_sName,"backspace"), true );
	m_sndChange.Load( THEME->GetPathS(m_sName,"change"), true );
}

ScreenTextEntry::~ScreenTextEntry()
{
}

void ScreenTextEntry::InitKeyboard()
{
	FOREACH_KeyboardRow( r )
	{
		for( int x=0; x<KEYS_PER_ROW; ++x )
		{
			BitmapText &bt = m_textKeyboardChars[r][x];
			bt.LoadFromFont( THEME->GetPathF(m_sName,"keyboard") );
			bt.SetXY( GetButtonX(x), GetButtonY(r) );
			this->AddChild( &bt );
		}
	}
}

void ScreenTextEntry::UpdateKeyboardText()
{
	FOREACH_KeyboardRow( r )
	{
		for( int x=0; x<KEYS_PER_ROW; ++x )
		{
			const char *s = g_szKeys[r][x];
			BitmapText &bt = m_textKeyboardChars[r][x];
			bt.SetText( s );
		}
	}
}

void ScreenTextEntry::UpdateAnswerText()
{
	CString txt = WStringToCString(m_sAnswer);
	if( m_bPassword )
	{
		int len = txt.length();
		txt = "";
		for( int i=0; i<len; ++i )
			txt += "*";
	}
	FontCharAliases::ReplaceMarkers(txt);
	m_textAnswer.SetText( txt );
}

void ScreenTextEntry::PositionCursor()
{
	BitmapText &bt = m_textKeyboardChars[m_iFocusY][m_iFocusX];
	m_sprCursor->SetXY( bt.GetX(), bt.GetY() );
}

void ScreenTextEntry::Update( float fDeltaTime )
{
	Screen::Update( fDeltaTime );
}

void ScreenTextEntry::DrawPrimitives()
{
	Screen::DrawPrimitives();
}

void ScreenTextEntry::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( m_In.IsTransitioning() || m_Out.IsTransitioning() || m_Cancel.IsTransitioning() )
		return;

	//The user wants to input text traditionally
	if ( g_bAllowOldKeyboardInput.Get() && ( type == IET_FIRST_PRESS ) )
	{
		if ( DeviceI.button == KEY_BACK )
		{
			BackspaceInAnswer();
		}
		else if ( DeviceI.ToChar() >= ' ' ) 
		{
			bool bIsHoldingShift = 
					INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_RSHIFT)) ||
					INPUTFILTER->IsBeingPressed( DeviceInput(DEVICE_KEYBOARD, KEY_LSHIFT));
			if ( bIsHoldingShift )
			{

				char c = (char)toupper( DeviceI.ToChar() );

				switch( c )
				{
				case '`':	c='~';	break;
				case '1':	c='!';	break;
				case '2':	c='@';	break;
				case '3':	c='#';	break;
				case '4':	c='$';	break;
				case '5':	c='%';	break;
				case '6':	c='^';	break;
				case '7':	c='&';	break;
				case '8':	c='*';	break;
				case '9':	c='(';	break;
				case '0':	c=')';	break;
				case '-':	c='_';	break;
				case '=':	c='+';	break;
				case '[':	c='{';	break;
				case ']':	c='}';	break;
				case '\'':	c='"';	break;
				case '\\':	c='|';	break;
				case ';':	c=':';	break;
				case ',':	c='<';	break;
				case '.':	c='>';	break;
				case '/':	c='?';	break;
				}

				AppendToAnswer( ssprintf( "%c", c ) );
			}
			else
				AppendToAnswer( ssprintf( "%c", DeviceI.ToChar() ) );

			//If the user wishes to select text in traditional way, start should finish text entry
			m_iFocusY = KEYBOARD_ROW_SPECIAL;
			m_iFocusX = DONE;

			this->UpdateKeyboardText();
			PositionCursor();
		}
	}

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );
}

void ScreenTextEntry::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_DoneClosingWipingLeft:
		break;
	case SM_DoneClosingWipingRight:
		break;
	case SM_DoneOpeningWipingLeft:
		break;
	case SM_DoneOpeningWipingRight:
		SCREENMAN->PopTopScreen( m_smSendOnPop );
		break;
	}
}

void ScreenTextEntry::MoveX( int iDir )
{
	CString sKey;
	do
	{
		m_iFocusX += iDir;
		wrap( m_iFocusX, KEYS_PER_ROW );

		sKey = g_szKeys[m_iFocusY][m_iFocusX]; 
	}
	while( sKey == "" );

	m_sndChange.Play();
	PositionCursor();
}

void ScreenTextEntry::MoveY( int iDir )
{
	CString sKey;
	do
	{
		m_iFocusY = (KeyboardRow)(m_iFocusY + iDir);
		wrap( m_iFocusY, NUM_KEYBOARD_ROWS );

		// HACK: Round to nearest option so that we always stop 
		// on KEYBOARD_ROW_SPECIAL.
		if( m_iFocusY == KEYBOARD_ROW_SPECIAL )
		{
			for( int i=0; true; i++ )
			{
				sKey = g_szKeys[m_iFocusY][m_iFocusX]; 
				if( sKey != "" )
					break;

				// UGLY: Probe one space to the left before looking to the right
				m_iFocusX += (i==0) ? -1 : +1;
				wrap( m_iFocusX, KEYS_PER_ROW );
			}
		}

		sKey = g_szKeys[m_iFocusY][m_iFocusX]; 
	}
	while( sKey == "" );

	m_sndChange.Play();
	PositionCursor();
}

void ScreenTextEntry::AppendToAnswer( CString s )
{
	wstring sNewAnswer = m_sAnswer+CStringToWstring(s);
	if( (int)sNewAnswer.length() > m_iMaxInputLength )
	{
		SCREENMAN->PlayInvalidSound();
		return;
	}

	m_sAnswer = sNewAnswer;
	m_sndType.Play();
	UpdateAnswerText();

	this->UpdateKeyboardText();
}

void ScreenTextEntry::BackspaceInAnswer()
{
	if( m_sAnswer.empty() )
	{
		SCREENMAN->PlayInvalidSound();
		return;
	}
	m_sAnswer.erase( m_sAnswer.end()-1 );
	m_sndBackspace.Play();
	UpdateAnswerText();
}

void ScreenTextEntry::MenuStart( PlayerNumber pn )
{
	if( m_iFocusY == KEYBOARD_ROW_SPECIAL )
	{
		switch( m_iFocusX )
		{
		case SPACEBAR:
			AppendToAnswer( " " );
			break;
		case BACKSPACE:
			BackspaceInAnswer();
			break;
		case CANCEL:
			End( true );
			break;
		case DONE:
			End( false );
			break;
		default:
			break;
		}
	}
	else
	{
		AppendToAnswer( g_szKeys[m_iFocusY][m_iFocusX] );
	}
}

void ScreenTextEntry::End( bool bCancelled )
{
	if( bCancelled )
	{
		if( m_pOnCancel ) 
			m_pOnCancel();
		
		m_Cancel.StartTransitioning( SM_DoneOpeningWipingRight );
	}
	else
	{
		CString sAnswer = WStringToCString(m_sAnswer);
		CString sError;
		if ( m_pValidate != NULL )
		{
			bool bValidAnswer = m_pValidate( sAnswer, sError );
			if( !bValidAnswer )
			{
				SCREENMAN->Prompt( SM_None, sError );
				return;	// don't end this screen.
			}
		}

		if( m_pOnOK )
		{
			CString ret = WStringToCString(m_sAnswer);
			FontCharAliases::ReplaceMarkers(ret);
			m_pOnOK( ret );
		}

		m_Out.StartTransitioning( SM_DoneOpeningWipingRight );
		SCREENMAN->PlayStartSound();
	}

	m_Background->PlayCommand("Off");

	OFF_COMMAND( m_textQuestion );
	OFF_COMMAND( m_sprAnswerBox );
	OFF_COMMAND( m_textAnswer );
	OFF_COMMAND( m_sprCursor );

	s_bCancelledLast = bCancelled;
	s_sLastAnswer = bCancelled ? CString("") : WStringToCString(m_sAnswer);
}

void ScreenTextEntry::MenuBack( PlayerNumber pn )
{
	End( true );
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
