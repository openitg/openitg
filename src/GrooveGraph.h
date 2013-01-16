/* GrooveGraph - The song's GrooveGraph displayed in SelectSong. */

#ifndef GROOVE_GRAPH_H
#define GROOVE_GRAPH_H

#include "ActorFrame.h"
#include "Sprite.h"
#include "GameConstantsAndTypes.h"
#include "Difficulty.h"
class Song;


class GrooveGraph : public ActorFrame
{
public:
	GrooveGraph();

	void SetFromSong( Song* pSong );	// NULL means no Song

	void TweenOnScreen();
	void TweenOffScreen();

protected:

	class Mountain : public Actor
	{
	public:
		Mountain();

		virtual void Update( float fDeltaTime );
		virtual void DrawPrimitives();

		void SetValues( float fNewValues[] );

		float m_fPercentTowardNew;
		float m_fValuesNew[NUM_DIFFICULTIES];
		float m_fValuesOld[NUM_DIFFICULTIES];
	};

	Sprite m_sprBase;
	Mountain m_Mountains[NUM_RADAR_CATEGORIES];
};

#endif

/*
 * (c) 2003-2004 Chris Danford
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
