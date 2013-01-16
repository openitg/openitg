/* GhostArrow - Trail a note leaves when it is destroyed. */

#ifndef GHOSTARROW_H
#define GHOSTARROW_H

#include "AutoActor.h"
#include "GameConstantsAndTypes.h"
#include "PlayerNumber.h"
#include "ActorFrame.h"
#include "ActorCommands.h"

class GhostArrow : public ActorFrame
{
public:
	GhostArrow();

	void Load( const CString &sButton, const CString &sFile );

	void StepTap( TapNoteScore tns );
	void StepHold( HoldNoteScore hns );

protected:
	AutoActor	m_sprTap[NUM_TAP_NOTE_SCORES];
	AutoActor	m_sprHold[NUM_HOLD_NOTE_SCORES];
	apActorCommands m_acTap[NUM_TAP_NOTE_SCORES];
	apActorCommands m_acHold[NUM_HOLD_NOTE_SCORES];
};


#endif

/*
 * (c) 2001-2004 Ben Nordstrom, Chris Danford
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
