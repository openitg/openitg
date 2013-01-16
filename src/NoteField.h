/* NoteField - An Actor that renders a NoteData. */

#ifndef NOTE_FIELD_H
#define NOTE_FIELD_H

#include "Sprite.h"
#include "ActorFrame.h"
#include "BitmapText.h"
#include "PrefsManager.h"
#include "BitmapText.h"
#include "Quad.h"
#include "NoteData.h"
#include "NoteDisplay.h"
#include "ReceptorArrowRow.h"
#include "GhostArrowRow.h"

class NoteField : public ActorFrame
{
public:
	NoteField();
	~NoteField();
	virtual void Update( float fDeltaTime );
	virtual void DrawPrimitives();
	
	virtual void Init( const PlayerState* pPlayerState, float fYReverseOffsetPixels );
	virtual void Load( 
		const NoteData* pNoteData, 
		int iStartDrawingPixel, 
		int iEndDrawingPixel );
	virtual void Unload();

	int	m_iBeginMarker, m_iEndMarker;	// only used with MODE_EDIT

	void FadeToFail();
	void CacheNoteSkin( const CString &sNoteSkin );

	void Step( int iCol, TapNoteScore score );
	void SetPressed( int iCol );
	void DidTapNote( int iCol, TapNoteScore score, bool bBright );
	void DidHoldNote( int iCol, HoldNoteScore score, bool bBright );

	void RefreshBeatToNoteSkin();

protected:
	void CacheAllUsedNoteSkins();

	bool IsOnScreen( float fBeat, int iFirstPixelToDraw, int iLastPixelToDraw );

	void DrawBeatBar( const float fBeat );
	void DrawMarkerBar( int fBeat );
	void DrawAreaHighlight( int iStartBeat, int iEndBeat );
	void DrawBPMText( const float fBeat, const float fBPM );
	void DrawFreezeText( const float fBeat, const float fBPM );
	void DrawBGChangeText( const float fBeat, const CString sNewBGName );
	float GetWidth();

	const NoteData *m_pNoteData;

	float	m_fPercentFadeToFail;	// -1 of not fading to fail

	const PlayerState*	m_pPlayerState;
	int				m_iStartDrawingPixel;	// this should be a negative number
	int				m_iEndDrawingPixel;	// this should be a positive number
	float			m_fYReverseOffsetPixels;

	// color arrows
	struct NoteDisplayCols
	{
		NoteDisplay		*display;
		ReceptorArrowRow	m_ReceptorArrowRow;
		GhostArrowRow	m_GhostArrowRow;
		NoteDisplayCols( int iNumCols ) { display = new NoteDisplay[iNumCols]; }
		~NoteDisplayCols() { delete [] display; }
	};

	/* All loaded note displays, mapped by their name. */
	map<CString, NoteDisplayCols *> m_NoteDisplays;

	int m_LastSeenBeatToNoteSkinRev;

	/* Map of beat->NoteDisplayCols.  This is updated whenever GAMESTATE-> changes. */
	typedef map<float, NoteDisplayCols *> NDMap;
	void SearchForBeat( NDMap::iterator &cur, NDMap::iterator &next, float Beat );
	NoteDisplayCols *SearchForBeat( float Beat );
	NoteDisplayCols *SearchForSongBeat();

	NDMap m_BeatToNoteDisplays;

	NoteDisplayCols *LastDisplay;

	// used in MODE_EDIT
	Sprite			m_sprBars;	// 4 frames: Measure, 4th, 8th, 16th
	BitmapText		m_textMeasureNumber;
	Quad			m_rectMarkerBar;
	Quad			m_rectAreaHighlight;
};

#endif

/*
 * (c) 2001-2004 Chris Danford
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
