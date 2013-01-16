#include "global.h"
#include "InputFilter.h"
#include "Preference.h"
#include "RageLog.h"
#include "RageInput.h"
#include "RageThreads.h"

InputFilter*	INPUTFILTER = NULL;	// global and accessable from anywhere in our program

static const float TIME_BEFORE_SLOW_REPEATS = 0.25f;
static const float TIME_BEFORE_FAST_REPEATS = 1.5f;

static const float SLOW_REPEATS_PER_SEC = 8;
static const float FAST_REPEATS_PER_SEC = 8;

static float g_fTimeBeforeSlow, g_fTimeBeforeFast, g_fTimeBetweenSlow, g_fTimeBetweenFast;

/*
 * Some input devices require debouncing.  Do this on both press and release.  After
 * reporting a change in state, don't report another for the debounce period.  If a
 * button is reported pressed, report it.  If the button is immediately reported
 * released, wait a period before reporting it; if the button is repressed during
 * that time, the release is never reported.
 *
 * The detail is important: if a button is pressed for 1ms and released, we must
 * always report it, even if the debounce period is 10ms, since it might be a coin
 * counter with a very short signal.  The only time we discard events is if a button
 * is pressed, released and then pressed again quickly.
 *
 * This delay in events is ordinarily not noticable, because we report initial
 * presses and releases immediately.  However, if a real press is ever delayed,
 * this won't cause timing problems, because the event timestamp is preserved.
 */
static Preference<float> g_fInputDebounceTime( "InputDebounceTime", 0 );

ButtonState::ButtonState() :
	m_BeingHeldTime(RageZeroTimer),
	m_LastReportTime(RageZeroTimer)
{
	m_BeingHeld = false;
	m_LastReportedHeld = false;
	m_fSecsHeld = 0;
}

InputFilter::InputFilter()
{
	queuemutex = new RageMutex("InputFilter");

	Reset();
	ResetRepeatRate();
}

InputFilter::~InputFilter()
{
	delete queuemutex;
}

void InputFilter::Reset()
{
	for( int i=0; i<NUM_INPUT_DEVICES; i++ )
		ResetDevice( InputDevice(i) );
}

void InputFilter::SetRepeatRate( float fSlowDelay, float fSlowRate, float fFastDelay, float fFastRate )
{
	g_fTimeBeforeSlow = fSlowDelay;
	g_fTimeBeforeFast = fFastDelay;
	g_fTimeBetweenSlow = 1/fSlowRate;
	g_fTimeBetweenFast = 1/fFastRate;
}

void InputFilter::ResetRepeatRate()
{
	SetRepeatRate( TIME_BEFORE_SLOW_REPEATS, SLOW_REPEATS_PER_SEC, TIME_BEFORE_FAST_REPEATS, FAST_REPEATS_PER_SEC );
}

void InputFilter::ButtonPressed( DeviceInput di, bool Down )
{
	LockMut(*queuemutex);

	if( di.ts.IsZero() )
		LOG->Warn( "InputFilter::ButtonPressed: zero timestamp is invalid" );

	if( di.device >= NUM_INPUT_DEVICES )
	{
		LOG->Warn( "Invalid device %i,%i", di.device, NUM_INPUT_DEVICES );
		return;
	}
	if( di.button >= GetNumDeviceButtons(di.device) )
	{
		LOG->Warn( "Invalid button %i,%i", di.button, GetNumDeviceButtons(di.device) );
		return;
	}

	ButtonState &bs = m_ButtonState[di.device][di.button];

	RageTimer now;
	CheckButtonChange( bs, di, now );

	if( bs.m_BeingHeld != Down )
	{
		bs.m_BeingHeld = Down;
		bs.m_BeingHeldTime = di.ts;
	}

	/* Try to report presses immediately. */
	CheckButtonChange( bs, di, now );
}

void InputFilter::CheckButtonChange( ButtonState &bs, DeviceInput di, const RageTimer &now )
{
	if( bs.m_BeingHeld == bs.m_LastReportedHeld )
		return;

	/* If the last IET_FIRST_PRESS or IET_RELEASE event was sent too recently,
	 * wait a while before sending it. */
	if( now - bs.m_LastReportTime < g_fInputDebounceTime )
		return;

	bs.m_LastReportTime = now;
	bs.m_LastReportedHeld = bs.m_BeingHeld;
	bs.m_fSecsHeld = 0;
	bs.m_LastInputTime = bs.m_BeingHeldTime;

	di.ts = bs.m_BeingHeldTime;
	if( !bs.m_LastReportedHeld )
		di.level = 0;

	queue.push_back( InputEvent(di, bs.m_LastReportedHeld?IET_FIRST_PRESS:IET_RELEASE) );
}

void InputFilter::SetButtonComment( DeviceInput di, const CString &sComment )
{
	LockMut(*queuemutex);
	ButtonState &bs = m_ButtonState[di.device][di.button];
	bs.m_sComment = sComment;
}

/* Release all buttons on the given device. */
void InputFilter::ResetDevice( InputDevice device )
{
	RageTimer now;
	for( int button = 0; button < GetNumDeviceButtons(device); ++button )
		ButtonPressed( DeviceInput(device, button, -1, now), false );
}

void InputFilter::Update(float fDeltaTime)
{
	RageTimer now;

	INPUTMAN->Update( fDeltaTime );

	/* Make sure that nothing gets inserted while we do this, to prevent
	 * things like "key pressed, key release, key repeat". */
	LockMut(*queuemutex);

	// Don't reconstruct "di" inside the loop.  This line alone is 
	// taking 4% of the CPU on a P3-666.
	DeviceInput di( (InputDevice)0,0,1.0f,now);

	FOREACH_InputDevice( d )
	{
		di.device = d;

		for( int b=0; b < GetNumDeviceButtons(d); b++ )	// foreach button
		{
			ButtonState &bs = m_ButtonState[d][b];
			di.button = b;

			CheckButtonChange( bs, di, now );

			/* Generate IET_LEVEL_CHANGED events. */
		/* EXPERIMENTAL: this doesn't seem to be used at all, so why bother?
		 * Let's see if any weird bugs pop up as a result.
		 *
			 if( bs.m_LastLevel != bs.m_Level )
			{
				queue.push_back( InputEvent(di,IET_LEVEL_CHANGED) );
				bs.m_LastLevel = bs.m_Level;
			}

		 */

			/* Generate IET_FAST_REPEAT and IET_SLOW_REPEAT events. */
			if( !bs.m_LastReportedHeld )
				continue;

			const float fOldHoldTime = bs.m_fSecsHeld;
			bs.m_fSecsHeld += fDeltaTime;
			const float fNewHoldTime = bs.m_fSecsHeld;
			
			if( fNewHoldTime <= g_fTimeBeforeSlow )
				continue;

			float fTimeBetweenRepeats;
			InputEventType iet;
			if( fOldHoldTime > g_fTimeBeforeSlow )
			{
				if( fOldHoldTime > g_fTimeBeforeFast )
				{
					fTimeBetweenRepeats = g_fTimeBetweenFast;
					iet = IET_FAST_REPEAT;
				}
				else
				{
					fTimeBetweenRepeats = g_fTimeBetweenSlow;
					iet = IET_SLOW_REPEAT;
				}
				if( int(fOldHoldTime/fTimeBetweenRepeats) != int(fNewHoldTime/fTimeBetweenRepeats) )
				{
					queue.push_back( InputEvent(di,iet) );
				}
			}
		}
	}

}

bool InputFilter::IsBeingPressed( DeviceInput di )
{
	return m_ButtonState[di.device][di.button].m_LastReportedHeld;
}

float InputFilter::GetSecsHeld( DeviceInput di )
{
	return m_ButtonState[di.device][di.button].m_fSecsHeld;
}

CString InputFilter::GetButtonComment( DeviceInput di ) const
{
	LockMut(*queuemutex);
	return m_ButtonState[di.device][di.button].m_sComment;
}

void InputFilter::ResetKeyRepeat( DeviceInput di )
{
	m_ButtonState[di.device][di.button].m_fSecsHeld = 0;
}

void InputFilter::GetInputEvents( InputEventArray &array )
{
	LockMut(*queuemutex);
	array = queue;
	queue.clear();
}

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

