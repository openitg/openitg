#ifndef SCREEN_ARCADE_PATCH_H
#define SCREEN_ARCADE_PATCH_H

#include "RageThreads.h"
#include "RageFile.h"
#include "ScreenWithMenuElements.h"
#include "PlayerNumber.h"
#include "BitmapText.h"

class ScreenArcadePatch : public ScreenWithMenuElements
{
public:
	ScreenArcadePatch( CString sName );
	~ScreenArcadePatch();

	virtual void Init();

	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void Update(float f);
	virtual void DrawPrimitives();
	
	virtual void HandleScreenMessage( const ScreenMessage SM );
	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
private:
	/* Main patch-checking function */
	void CheckForPatches();


	/* find a card that can be used, load from it */
	void FindCard();
	bool LoadFromCard();	

	/* add patches to the member vector to check */
	bool AddPatches( CString sPattern );

	/* load and verify the patch passed to LoadPatch */
	bool LoadPatch( CString sPath );
	bool FinalizePatch();

	bool m_bReboot;

	/* All found patches */
	CStringArray m_vsPatches;
	
	PlayerNumber m_Player;
	CString m_sCardDir;
	CString m_sPatchPath;
	CString m_sSuccessMessage;

	BitmapText m_Status;
	BitmapText m_Patch;
};

#endif
