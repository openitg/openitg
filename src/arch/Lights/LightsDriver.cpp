#include "global.h"
#include "LightsDriver.h"
#include "RageLog.h"
#include "Foreach.h"
#include "arch/arch_default.h"

#include "arch/Lights/LightsDriver_Dynamic.h"
#include "arch/Lights/LightsDriver_Dynamic_Unix.h"

/* We explicitly load these drivers. */
#include "arch/Lights/LightsDriver_ActorMessage.h"
#include "arch/Lights/LightsDriver_External.h"
#include "arch/Lights/LightsDriver_SystemMessage.h"

DriverList LightsDriver::m_pDriverList;

#include "RageFileManager.h"
#include "RageUtil.h"

#if defined(_WINDOWS)
#define LIB_EXTENSION ".dll"
#elif defined(LINUX)
#define LIB_EXTENSION ".so"
#endif

const CString GetModuleDir()
{
	static CString sModulePath;

	if( !sModulePath.empty() )
		return sModulePath;

	vector<RageFileManager::DriverLocation> mountpoints;
	FILEMAN->GetLoadedDrivers( mountpoints );

	for( unsigned i = 0; i < mountpoints.size(); ++i )
	{
		RageFileManager::DriverLocation *l = &mountpoints[i];

		if( l->Type != "dir" )
			continue;

		if( l->MountPoint == "/" )
			sModulePath = l->Root + "/Data/Modules/";
		else if( l->MountPoint == "/Data" )
			sModulePath = l->Root + "/Modules/";

		if( !sModulePath.empty() )
			break;
	}

	// extraneous dots will screw up the system-level paths.
	CollapsePath(sModulePath);

	LOG->Debug( "GetModuleDir(): returning %s", sModulePath.c_str() );
	return sModulePath;
}

/* XXX: can we find a better place for this? */
static LightsDriver *TryModuleDriver( const CString &sName )
{
	/* e.g. "stdout" looks for "LightsDriver_Stdout.so" */
	CString sDriverName = "LightsDriver_" + sName + LIB_EXTENSION;

	CString sRagePath = "Data/Modules/" + sDriverName;
	CString sRealPath = GetModuleDir() + sDriverName;

	if( !IsAFile(sRagePath) )
		return NULL;

#if defined(LINUX)
	LightsDriver_Dynamic *ret = new LightsDriver_Dynamic_Unix( sRealPath );
#else
	LightsDriver_Dynamic *ret = NULL;
	// TODO: LightsDriver_Dynamic_Win32
	return NULL;
#endif

	ret->Load();

	/* if this module couldn't actually load, don't return it. */
	if( !ret->IsLoaded() )
		SAFE_DELETE( ret );

	return ret;
}

void LightsDriver::Create( const CString &sDrivers, vector<LightsDriver *> &Add )
{
	LOG->Trace( "Initializing lights drivers: %s", sDrivers.c_str() );

	vector<CString> asDriversToTry;
	split( sDrivers, ",", asDriversToTry, true );
	
	FOREACH_CONST( CString, asDriversToTry, Driver )
	{
		RageDriver *pRet = m_pDriverList.Create( *Driver );

		if( pRet == NULL )
		{
			/* no built-in driver was found. See if any modules match this name. */
			pRet = TryModuleDriver( *Driver );

			if( pRet == NULL )
			{
				LOG->Trace( "Unknown lights driver: %s", Driver->c_str() );
				continue;
			}
		}

		LightsDriver *pDriver = dynamic_cast<LightsDriver *>( pRet );
		ASSERT( pDriver != NULL );

		LOG->Info( "Lights driver: %s", Driver->c_str() );
		Add.push_back( pDriver );
	}

	/* always add these drivers. */
	Add.push_back( new LightsDriver_SystemMessage );
	Add.push_back( new LightsDriver_ActorMessage );
	Add.push_back( new LightsDriver_External );
}

/*
 * (c) 2002-2005 Glenn Maynard
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
