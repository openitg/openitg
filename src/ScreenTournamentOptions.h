#ifndef SCREEN_TOURNAMENT_OPTIONS_H
#define SCREEN_TOURNAMENT_OPTIONS_H

#include "TournamentManager.h"
#include "ScreenOptions.h"
#include "PlayerNumber.h"

class ScreenTournamentOptions : public ScreenOptions
{
public:
	ScreenTournamentOptions( CString sName );

	virtual void Init();

        virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
        virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn, const InputEventType type );
	virtual void MenuBack( PlayerNumber pn, const InputEventType type );
/*
	virtual void MenuLeft( PlayerNumber pn, const InputEventType type ) { LOG->Debug( "MenuLeft(%i)", pn+1); }
	virtual void MenuRight( PlayerNumber pn, const InputEventType type ) { LOG->Debug( "MenuRight(%i)", pn+1); }
	virtual void MenuUp( PlayerNumber pn, const InputEventType type ) { LOG->Debug( "MenuUp(%i)", pn+1); }
	virtual void MenuDown( PlayerNumber pn, const InputEventType type ) { LOG->Debug( "MenuDown(%i)", pn+1); }
*/

private:
	CString m_sPlayerName, m_sPlayerScoreName;
	bool RegistrationValid();

	virtual void ImportOptions( int row, const vector<PlayerNumber> &vpns );
	virtual void ExportOptions( int row, const vector<PlayerNumber> &vpns );

	virtual void GoToNextScreen();
	virtual void GoToPrevScreen();
	virtual void ReloadScreen();
};

#endif // SCREEN_TOURNAMENT_OPTIONS_H

