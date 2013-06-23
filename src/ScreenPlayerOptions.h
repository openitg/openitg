#ifndef SCREENPLAYEROPTIONS_H
#define SCREENPLAYEROPTIONS_H

#include "ScreenOptionsMaster.h"
#include "PlayerOptions.h"

AutoScreenMessage( SM_BackFromPlayerOptions )

class ScreenPlayerOptions : public ScreenOptionsMaster
{
public:
	ScreenPlayerOptions( CString sName );
	virtual void Init();

	virtual void Update( float fDelta );
	virtual void DrawPrimitives();
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

private:
	void GoToNextScreen();
	void GoToPrevScreen();

	void UpdateDisqualified( int row, PlayerNumber pn );

	bool        m_bAcceptedChoices;
	bool        m_bGoToOptions;
	bool        m_bAskOptionsMessage;
	Sprite      m_sprOptionsMessage;

	RageSound	m_CancelAll;
	AutoActor	m_sprCancelAll[NUM_PLAYERS];

	// used to restore options if we back out
	PlayerOptions	m_OriginalOptions[NUM_PLAYERS];
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
