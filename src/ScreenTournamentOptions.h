#ifndef SCREEN_TOURNAMENT_OPTIONS_H
#define SCREEN_TOURNAMENT_OPTIONS_H

#include "ScreenOptions.h"
#include "PlayerNumber.h"

class ScreenTournamentOptions : public ScreenOptions
{
public:
	ScreenTournamentOptions( CString sName );

	virtual void Init();

        virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
        virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
private:
	virtual void ImportOptions( int row, const vector<PlayerNumber> &vpns );
	virtual void ExportOptions( int row, const vector<PlayerNumber> &vpns );

	virtual void GoToNextScreen();
	virtual void GoToPrevScreen();
};

#endif // SCREEN_TOURNAMENT_OPTIONS_H

