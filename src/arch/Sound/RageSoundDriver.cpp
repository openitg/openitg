#include "global.h"
#include "RageSoundDriver.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "Foreach.h"
#include "arch/arch_default.h"

DriverList RageSoundDriver::m_pDriverList;

RageSoundDriver *RageSoundDriver::Create( const CString &sDrivers )
{
	vector<CString> DriversToTry;
	split( sDrivers.empty()? DEFAULT_SOUND_DRIVER_LIST:sDrivers, ",", DriversToTry, true );
	
	FOREACH_CONST( CString, DriversToTry, Driver )
	{
		RageDriver *pDriver = m_pDriverList.Create( *Driver );
		if( pDriver == NULL )
		{
			LOG->Warn( "Unknown sound driver: %s", Driver->c_str() );
			continue;
		}

		RageSoundDriver *pRet = dynamic_cast<RageSoundDriver *>( pDriver );
		ASSERT( pRet != NULL );

		const CString sError = pRet->Init();
		if( sError.empty() )
		{
			LOG->Info( "Sound driver: %s", Driver->c_str() );
			return pRet;
		}
		LOG->Info( "Couldn't load driver %s: %s", Driver->c_str(), sError.c_str() );
		SAFE_DELETE( pRet );
	}

	return NULL;
}

/*
 * (c) 2002-2006 Glenn Maynard, Steve Checkoway
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
