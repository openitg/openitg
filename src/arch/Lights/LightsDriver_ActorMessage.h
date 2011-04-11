/* LightsDriver_ActorMessage: simple lights driver that sends Messages
 * to Actors based on what lights are on or off. This is a more generic
 * implementation of what we used to have in ScreenTestInput. */

#ifndef LIGHTS_DRIVER_ACTORMESSAGE_H
#define LIGHTS_DRIVER_ACTORMESSAGE_H

#include "LightsDriver.h"

class LightsDriver_ActorMessage: public LightsDriver
{
public:
	LightsDriver_ActorMessage();
	virtual ~LightsDriver_ActorMessage();

	virtual void Set( const LightsState *ls );
private:
	LightsState *m_pLastState;
};

#endif // LIGHTS_DRIVER_ACTORMESSAGE_H

/*
 * (c) 2011 Mark Cannon
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
