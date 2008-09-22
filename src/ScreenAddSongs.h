#ifndef SCREEN_ADD_SONGS_H
#define SCREEN_ADD_SONGS_H

#include "RageFile.h"
#include "Screen.h"
#include "ScreenWithMenuElements.h"
#include "PlayerNumber.h"
#include "song.h"

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

private:
	CStringArray m_vsLoadedSongs;
	vector<Song *> m_vLoadedSongs;
	BitmapText m_AddedSongList;

	CStringArray m_asAddableSongs[NUM_PLAYERS];
	BitmapText m_AddableSongSelection;

	bool m_bRefreshSongMan;

	bool m_bCardMounted[NUM_PLAYERS];
};

#endif
