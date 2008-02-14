#ifndef SCREEN_ARCADE_PATCH_H
#define SCREEN_ARCADE_PATCH_H

#include "ScreenWithMenuElements.h"
#include "BitmapText.h"
#include "RageFile.h"
#include "RageThreads.h"

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
	

	bool bChecking;
	bool bScanned;
	BitmapText m_Status;
	CString m_sSuccessMsg;
	
private:
	int CommitPatch();
	bool CheckCards();
	bool MountCards();
	bool ScanPatch();
	bool CopyPatch();
	bool UnmountCards();
	bool CheckSignature();
	bool CheckXml();
	bool CopyPatchContents();
	//void *UpdatePatchCopyProgress(float fPercent);

	BitmapText m_Patch;
	PlayerNumber pn;
	
	CString Type;
	CString Root;

	RageFile fPatch;
	
	CString sFullDir;
	CString sDir;
	CStringArray aPatches;
	CString sFile;
};

#endif
