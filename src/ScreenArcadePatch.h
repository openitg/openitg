#ifndef SCREEN_ARCADE_PATCH_H
#define SCREEN_ARCADE_PATCH_H

#include "ScreenWithMenuElements.h"
#include "BitmapText.h"
#include "RageThreads.h"

enum PatchState
{
	PATCH_NONE,
	PATCH_CHECKING,
	PATCH_INSTALLED,
	PATCH_ERROR,
	NUM_PATCH_STATES
};

class RageFileDriverZip;

class ScreenArcadePatch: public ScreenWithMenuElements
{
public:
	ScreenArcadePatch( CString sName );
	~ScreenArcadePatch();

	virtual void Init();
	virtual void Update( float fDeltaTime );
	virtual void HandleScreenMessage( const ScreenMessage SM );

	virtual void MenuStart( PlayerNumber pn );
	virtual void MenuBack( PlayerNumber pn )	{ MenuStart(pn); }
private:
	/* current state of the patch being checked */
	PatchState m_State, m_LastUpdatedState;

	/* thread that does the checking */
	RageThread m_Thread;
	static int PatchThread_Start( void *p )
	{
		((ScreenArcadePatch *)p)->PatchMain();
		return 0;
	}

	/* main function for patch checking */
	void PatchMain();

	/* secondary functions, more encapsulated than the last ones... */
	bool HasPatch( PlayerNumber pn, const CStringArray &vsPatterns );
	bool VerifyPatch( const CString &sPath, const CStringArray &vsKeyPaths );
	bool GetXMLData( RageFileDriverZip *pZip, CString &sGame, CString &sMessage, int &iRevision );

	CStringArray m_vsPatches;

	CString m_sProfileDir;

	BitmapText m_StateText;
	BitmapText m_PatchText;
};

#endif // SCREEN_ARCADE_PATCH_H

/*
 * (c) 2008-2009 BoXoRRoXoRs
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
