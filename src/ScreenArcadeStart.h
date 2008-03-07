/* ScreenArcadeStart - performs several start-up checks before loading
 * on an arcade platform. */

#ifndef SCREEN_ARCADE_START_H
#define SCREEN_ARCADE_START_H

#include "ScreenWithMenuElements.h"
#include "BitmapText.h"

class ScreenArcadeStart : public ScreenWithMenuElements
{
public:
	ScreenArcadeStart( CString sName );
	virtual void Init();
	virtual ~ScreenArcadeStart();

	virtual void DrawPrimitives();
	virtual void Update( float fDelta );
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
private:
	void Refresh();
	void LoadHandler();

	BitmapText m_Error;

	bool m_bFatalError;
};

#endif

