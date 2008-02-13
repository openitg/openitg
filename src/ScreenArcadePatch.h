#ifndef SCREEN_ARCADE_PATCH_H
#define SCREEN_ARCADE_PATCH_H

#include "ScreenWithMenuElements.h"
#include "BitmapText.h"
#include "RageFile.h"

class ScreenArcadePatch : public ScreenWithMenuElements
{
public:
	ScreenArcadePatch( CString sName );

	virtual void Init();

	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void Update(float f);
	virtual void DrawPrimitives();
	
	virtual void HandleScreenMessage( const ScreenMessage SM );
	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn );
	
	virtual bool CommitPatch();
	virtual bool CheckCards();
	virtual bool MountCards();
	virtual bool ScanPatch();
	virtual bool CopyPatch();
	virtual bool UnmountCards();
	virtual bool CheckSignature();
	virtual bool CheckXml();

private:
	BitmapText m_Status;
	BitmapText m_Patch;
	PlayerNumber pn;
	bool bChecking;
	bool bScanned;
	
	CString Type;
	CString Root;
	
	RageFile fPatch;
	
	CString sFullDir;
	CString sDir;
	CStringArray aPatches;
	CString sFile;
};

#endif
