/* ScreenArcadeDiagnostics - Shows System Diagnostics of the machine. */

#ifndef SCREEN_ARCADE_DIAGNOSTICS_H
#define SCREEN_ARCADE_DIAGNOSTICS_H

#include "ScreenWithMenuElements.h"
#include "BitmapText.h"


class ScreenArcadeDiagnostics : public ScreenWithMenuElements
{
public:
	ScreenArcadeDiagnostics( CString sName );
	virtual void Init();
	virtual ~ScreenArcadeDiagnostics();

	virtual void DrawPrimitives();
	virtual void Update( float fDelta );
	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
};

#endif
