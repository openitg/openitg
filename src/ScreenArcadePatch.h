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
	/* starts the patch loading thread when called */
	static int PatchThread_Start( void *p ) { ((ScreenArcadePatch *) p)->CheckForPatches(); return 0; }


	/* Main patch-checking function */
	void CheckForPatches();


	/* find a card that can be used, load from it */
	void FindCard();
	bool LoadFromCard();	

	/* add patches to the member vector to check */
	bool AddPatches( CString sPattern );

	/* load and verify the patch passed to LoadPatch */
	bool LoadPatch( CString sPath, bool bOnCard );
	bool FinalizePatch();

	bool m_bReboot;
	bool m_bExit;

	/* All found patches */
	CStringArray m_vsPatches;
	
	PlayerNumber m_Player;
	CString m_sCardDir;
	CString m_sPatchPath;
	CString m_sSuccessMessage;

	BitmapText m_Status;
	BitmapText m_Patch;

	/* The thread that handles the loading/verification, and its shutdown */
	RageThread m_PatchThread;
};

#endif

/*
 * Copyright (c) 2008 BoXoRRoXoRs
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
