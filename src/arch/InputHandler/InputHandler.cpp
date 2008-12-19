#include "global.h"
#include "InputFilter.h"
#include "RageUtil.h"
#include "InputHandler.h"
#include "RageLog.h"
#include "PrefsManager.h" // XXX
#include <map>

/* This code is taken from CNLohr's 3.9 AC build. */
map<int, RageTimer> m_LastHit;

void InputHandler::UpdateTimer()
{
	m_LastUpdate.Touch();
	m_iInputsSinceUpdate = 0;
}

void InputHandler::ButtonPressed( DeviceInput di, bool Down )
{
	if( di.ts.IsZero() )
	{
		di.ts = m_LastUpdate.Half();
		++m_iInputsSinceUpdate;
	}

	// uncomment if you need to check timestamps on input...
	// threaded input should never pass 0.000010 or so.
	/*
	if( di.button != KEY_SPACE )
		LOG->Debug( "%s %s - timestamp, %f", di.toString().c_str(), Down ? "pressed" : "released", di.ts.Ago() );
	*/

	// if it's a release, only allow it after the debounce time passes through
	if( !Down )
	{
		//if ( m_LastHit.find(di.button) == m_LastHit.end() && m_LastHit[di.button].PeekDeltaTime() > PREFSMAN->m_fInputDebounceTime )
		if ( m_LastHit[di.button].PeekDeltaTime() > PREFSMAN->m_fInputDebounceTime )
		{
			INPUTFILTER->ButtonPressed( di, Down );
			m_LastHit[di.button].Touch();
		}
//		else
//		{
//			LOG->Debug("Debounce'd: PeekDeltaTime: %f, m_fInputDebounceTime: %f", m_LastHit[di.button].PeekDeltaTime(), PREFSMAN->m_fInputDebounceTime.Get());
//		}
	}
	else
	{
		INPUTFILTER->ButtonPressed( di, Down );
	}

	if( m_iInputsSinceUpdate >= 50 )
	{
		/*
		 * We havn't received an update in a long time, so warn about it.  We expect to receive
		 * input events before the first UpdateTimer call only on the first update.  Leave
		 * m_iInputsSinceUpdate where it is, so we only warn once.  Only updates that didn't provide
		 * a timestamp are counted; if the driver provides its own timestamps, UpdateTimer is
		 * optional.
		 *
		 * This can also happen if a device sends a lot of inputs at once; for example, a keyboard
		 * driver that sends every key in one frame.  If that's really needed (wasteful), increase
		 * the threshold.
		 */
		LOG->Warn( "InputHandler::ButtonPressed: Driver sent 50 updates without calling UpdateTimer" );
		FAIL_M("Driver sent 50 updates without calling UpdateTimer");	// that wasn't so hard now, was it?
	}
}

/*
 * (c) 2003-2004 Glenn Maynard
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
