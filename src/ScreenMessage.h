/* ScreenMessage - Definition of common ScreenMessages. */

#ifndef SCREENMESSAGE_H
#define SCREENMESSAGE_H

#include <map>

enum ScreenMessage {
	SM_None = 0,
	SM_DoneClosingWipingLeft,
	SM_DoneClosingWipingRight,
	SM_DoneOpeningWipingLeft,
	SM_DoneOpeningWipingRight,
	SM_MenuTimer,
	SM_DoneFadingIn,
	SM_BeginFadingOut,
	SM_GoToNextScreen,
	SM_GoToPrevScreen,
	SM_GainFocus,
	SM_LoseFocus,
	SM_StopMusic,
	SM_Pause,
	SM_Success,
	SM_Failure,

	// messages sent by Combo
	SM_PlayToasty,
	SM_100Combo,
	SM_200Combo,
	SM_300Combo,
	SM_400Combo,
	SM_500Combo,
	SM_600Combo,
	SM_700Combo,
	SM_800Combo,
	SM_900Combo,
	SM_1000Combo,
	SM_ComboStopped,
	SM_ComboContinuing,
	SM_MissComboAborted,

	SM_BattleTrickLevel1,
	SM_BattleTrickLevel2,
	SM_BattleTrickLevel3,
	SM_BattleDamageLevel1,
	SM_BattleDamageLevel2,
	SM_BattleDamageLevel3,

	SM_User
};

//AutoScreenMessageHandler Class
class ASMHClass
{
public:
	ScreenMessage ToMessageNumber( const CString & Name );
	CString	NumberToString( ScreenMessage SM );
	void LogMessageNumbers();
private:
	map < CString, ScreenMessage > *m_pScreenMessages;
	ScreenMessage m_iCurScreenMessage;
};

extern ASMHClass AutoScreenMessageHandler;

// Automatically generate a unique ScreenMessage value
#define AutoScreenMessage( x ) \
	const ScreenMessage x = AutoScreenMessageHandler.ToMessageNumber( #x );


#endif

/*
 * (c) 2003-2005 Chris Danford, Charles Lohr
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
