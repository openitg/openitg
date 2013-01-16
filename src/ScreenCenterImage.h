/* ScreenCenterImage - Allows the user to adjust screen centering. */

#ifndef SCREEN_CENTER_IMAGE_H
#define SCREEN_CENTER_IMAGE_H

#include "ScreenWithMenuElements.h"
#include "BitmapText.h"


class ScreenCenterImage : public ScreenWithMenuElements
{
public:
	ScreenCenterImage( CString sName );
	void Init();
	virtual ~ScreenCenterImage();

	virtual void Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI );
	virtual void Update( float fDeltaTime );
	virtual void HandleScreenMessage( const ScreenMessage SM );

private:
	BitmapText	m_textInstructions;

	enum Axis { AXIS_TRANS_X, AXIS_TRANS_Y, AXIS_SCALE_X, AXIS_SCALE_Y, NUM_AXES };
	float m_fScale[NUM_AXES];

	void Move( Axis axis, float fDeltaTime );
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
