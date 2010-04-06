#include "global.h"
#include "InputFilter.h"
#include "RageUtil.h"
#include "InputHandler.h"
#include "RageLog.h"
#include "Foreach.h"
#include "arch/arch_default.h"
#include "InputHandler_MonkeyKeyboard.h"

Preference<bool> g_bDebugInputDrivers( "DebugInputDrivers", false );

#define UPDATE_WARN_LIMIT 100

InputHandler::InputHandler()
{
	m_iInputsSinceUpdate = 0;
	m_DebugTimer.m_bAutoReport = g_bDebugInputDrivers;
}

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

	INPUTFILTER->ButtonPressed( di, Down );

	if( m_iInputsSinceUpdate >= UPDATE_WARN_LIMIT )
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
		LOG->Warn( "InputHandler::ButtonPressed: Driver sent %d updates without calling UpdateTimer", UPDATE_WARN_LIMIT );
	}
}

DriverList InputHandler::m_pDriverList;

void InputHandler::Create( const CString &sDrivers, vector<InputHandler*> &Add )
{
	const CString drivers = sDrivers.empty() ? CString(DEFAULT_INPUT_DRIVER_LIST) : sDrivers;
	vector<CString> DriversToTry;
	split( drivers, ",", DriversToTry, true );

	FOREACH_CONST( CString, DriversToTry, s )
	{
		RageDriver *pDriver = m_pDriverList.Create( *s );
		if( pDriver == NULL )
		{
			LOG->Warn( "Unknown input handler name: %s", s->c_str() );
			continue;
		}

		InputHandler *ret = dynamic_cast<InputHandler*>( pDriver );
		DEBUG_ASSERT( ret );
		Add.push_back( ret );
	}

	// always add
	Add.push_back( new InputHandler_MonkeyKeyboard );
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
