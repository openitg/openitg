#ifndef SCREEN_ADD_SONGS_H
#define SCREEN_ADD_SONGS_H

#include "RageFile.h"
#include "Screen.h"
#include "ScreenWithMenuElements.h"
#include "PlayerNumber.h"
#include "song.h"
#include "RageThreads.h"

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
	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );

	void StartSongThread();
	bool m_bStopThread;

private:
	void LoadAddedZips();

	map<CString,CString> m_mAddedZips;
	BitmapText m_AddedZipList;

	CStringArray m_asAddableZips[NUM_PLAYERS];
	BitmapText m_AddableZipSelection;

	BitmapText m_Disclaimer;

	bool m_bRestart;

	bool m_bCardMounted[NUM_PLAYERS];
	RageThread m_PlayerSongLoadThread;
	PlayerNumber m_CurrentPlayer;
	bool m_bPrompt;

};

#endif
