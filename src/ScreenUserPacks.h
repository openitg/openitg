#ifndef SCREEN_USER_PACKS_H
#define SCREEN_USER_PACKS_H

#include "RageThreads.h"
#include "RageTimer.h"
#include "RageFile.h"
#include "ScreenMiniMenu.h"
#include "ScreenWithMenuElements.h"
#include "LinkedOptionsMenu.h"
#include "song.h"
#include "PlayerNumber.h"

class ScreenUserPacks : public ScreenWithMenuElements
{
public:
	ScreenUserPacks( CString sName );
	~ScreenUserPacks();

	virtual void Init();

	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void Update(float f);
	virtual void DrawPrimitives();
	
	virtual void HandleScreenMessage( const ScreenMessage SM );
	//virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
	virtual void HandleMessage( const CString &sMessage );

	void StartSongThread();
	bool m_bStopThread;
private:
	void ReloadZips();

	CStringArray m_asAddedZips;
	CStringArray m_asAddableZips[NUM_PLAYERS];

	BitmapText m_Disclaimer;

	LinkedOptionsMenu m_AddedZips;
	LinkedOptionsMenu m_USBZips;
	LinkedOptionsMenu m_Exit;
	LinkedOptionsMenu *m_pCurLOM;

	RageThread m_PlayerSongLoadThread;

	RageSound m_SoundDelete;
	RageSound m_SoundTransferDone;

	bool m_bCardMounted[NUM_PLAYERS];
	PlayerNumber m_CurPlayer;
	bool m_bRestart;
	bool m_bPrompt;
};

#endif /* SCREEN_USER_PACKS_H */

/*
 * (c) 2009 "infamouspat"
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
