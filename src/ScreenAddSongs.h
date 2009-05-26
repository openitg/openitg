#ifndef SCREEN_ADD_SONGS_H
#define SCREEN_ADD_SONGS_H

#include "RageFile.h"
#include "Screen.h"
#include "ScreenWithMenuElements.h"
#include "PlayerNumber.h"
#include "song.h"
#include "RageThreads.h"
#include "ScreenMiniMenu.h"
#include "LinkedOptionsMenu.h"

class ScreenAddSongs : public ScreenWithMenuElements
{
public:
	ScreenAddSongs( CString sName );
	~ScreenAddSongs();

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
	void LoadAddedZips();

	CStringArray m_asAddedZips;
	CStringArray m_asAddableZips[NUM_PLAYERS];

	BitmapText m_Disclaimer;

	LinkedOptionsMenu m_AddedZips;
	LinkedOptionsMenu m_USBZips;
	LinkedOptionsMenu m_Exit;
	LinkedOptionsMenu *m_pCurLOM;

	RageThread m_PlayerSongLoadThread;

	bool m_bCardMounted[NUM_PLAYERS];
	PlayerNumber m_CurPlayer;
	bool m_bRestart;
	bool m_bPrompt;
};

#endif
