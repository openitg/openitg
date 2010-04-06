#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "ProfileManager.h"
#include "SongManager.h"
#include "LuaManager.h"
#include "DiagnosticsUtil.h"
#include "arch/ArchHooks/ArchHooks.h"

#include "XmlFile.h"
#include "ProductInfo.h"
#include "io/ITGIO.h"
#include "io/USBDevice.h"

#include "ibutton/ibutton.h"

#define PATCH_XML_PATH "Data/patch/patch.xml"
#define STATS_XML_PATH "Data/MachineProfile/Stats.xml"

// these should be defined in verstub.cpp across all platforms
extern const bool VersionSVN;
extern const char *const VersionDate;
extern unsigned long VersionNumber;

/* /stats/ is mounted to Data for arcade builds, so this should be okay. */
int DiagnosticsUtil::GetNumCrashLogs()
{
	CStringArray aLogs;
	GetDirListing( "Data/crashinfo-*.txt" , aLogs );
	return aLogs.size();
}

int DiagnosticsUtil::GetNumMachineEdits()
{
	CStringArray aEdits;
	CString sDir = PROFILEMAN->GetProfileDir(PROFILE_SLOT_MACHINE) + EDIT_SUBDIR;
	GetDirListing( sDir , aEdits );
	return aEdits.size();
}

CString DiagnosticsUtil::GetIP()
{
	CString sAddress, sNetmask, sError;

	if( HOOKS->GetNetworkAddress(sAddress, sNetmask, sError) )
		return ssprintf( "%s, Netmask: %s", sAddress.c_str(), sNetmask.c_str() );
	else
		return sError;
}

int DiagnosticsUtil::GetRevision()
{
	// default value if a patch value can't be found/loaded
	int iRevision = 1;

	// Create the XML Handler, and clear it, for practice.
	XNode *xml = new XNode;
	xml->Clear();
	xml->m_sName = "patch";

	// if the file is readable and has the proper node, save its value
	if( !IsAFile(PATCH_XML_PATH) )
		LOG->Warn( "GetRevision(): There is no patch file (patch.xml)" );
	else if( !xml->LoadFromFile(PATCH_XML_PATH) )
		LOG->Warn( "GetRevision(): Could not load from patch.xml" );
	else if( !xml->GetChild("Revision") )
		LOG->Warn( "GetRevision(): Revision node missing! (patch.xml)" );
	else
		iRevision = atoi( xml->GetChild("Revision")->m_sValue );

	SAFE_DELETE( xml );

	return iRevision;
}

int DiagnosticsUtil::GetNumMachineScores()
{
	// Create the XML Handler and clear it, for practice
	XNode *xml = new XNode;
	xml->Clear();
	
	// Check for the file existing
	if( !IsAFile(STATS_XML_PATH) )
	{
		LOG->Warn( "There is no Stats.xml file!" );
		SAFE_DELETE( xml ); 
		return 0;
	}
	
	// Make sure you can read it
	if( !xml->LoadFromFile(STATS_XML_PATH) )
	{
		LOG->Trace( "Stats.xml unloadable!" );
		SAFE_DELETE( xml ); 
		return 0;
	}
	
	const XNode *pData = xml->GetChild( "SongScores" );
	
	if( pData == NULL )
	{
		LOG->Warn( "Error loading scores: <SongScores> node missing" );
		SAFE_DELETE( xml ); 
		return 0;
	}
	
	unsigned int iScoreCount = 0;
	
	// Named here, for LoadFromFile() renames it to "Stats"
	xml->m_sName = "SongScores";
	
	// For each pData Child, or the Child in SongScores...
	FOREACH_CONST_Child( pData , p )
		iScoreCount++;

	SAFE_DELETE( xml ); 

	return iScoreCount;
}

CString DiagnosticsUtil::GetProductName()
{
	if( VersionSVN )
		return CString(PRODUCT_NAME_VER) + " " + ssprintf( "r%lu", VersionNumber);

	return CString(PRODUCT_NAME_VER);
}

const CString& DiagnosticsUtil::GetSerialNumber()
{
	static CString g_SerialNum;

	if ( !g_SerialNum.empty() )
		return g_SerialNum;

/* Try to grab a serial number from a dongle;
 * otherwise generate a fake one. */
	g_SerialNum = iButton::GetSerialNumber();

	if( g_SerialNum.empty() )
		g_SerialNum = GenerateDebugSerial();

	return g_SerialNum;
}

/* this allows us to use the serial numbers on builds for
 * more helpful debugging information. PRODUCT_BUILD_DATE
 * is defined in ProductInfo.h */
CString DiagnosticsUtil::GenerateDebugSerial()
{
	char system, type;

// set the compilation OS
#if defined(WIN32)
	system = 'W'; /* Windows */
#elif defined(LINUX)
	if( VersionSVN )
		system = 'S'; /*nix, with SVN */
	else
		system = 'L'; /*nix, no SVN */
#elif defined(DARWIN)
	system = 'M'; /* Mac OS */
#else
	system = 'U'; /* unknown */
#endif

// set the compilation arcade type
#ifdef ITG_ARCADE
	type = 'A';
#else
	type = 'P';
#endif

	// if SVN, display the version regularly: "OITG-W-20090409-600-P"
	// if no SVN, display the version in hex: "OITG-W-20090409-08A-P"
	if( VersionSVN )
		return ssprintf( "OITG-%c-%s-%03lu-%c", system, VersionDate, VersionNumber, type );
	else
		return ssprintf( "OITG-%c-%s-%03lX-%c", system, VersionDate, VersionNumber, type );
}

bool DiagnosticsUtil::HubIsConnected()
{
	vector<USBDevice> vDevices;
	GetUSBDeviceList( vDevices );

	/* Hub can't be connected if there are no devices. */
	if( vDevices.size() == 0 )
		return false;

	for( unsigned i = 0; i < vDevices.size(); i++ )
		if( vDevices[i].IsHub() )
			return true;

	return false;
}

CString m_sInputType = "";

CString DiagnosticsUtil::GetInputType()
{
	return m_sInputType;
}

void DiagnosticsUtil::SetInputType( CString sType )
{
	m_sInputType = sType;
}

void SetProgramGlobals( lua_State* L )
{
	LUA->SetGlobal( "OPENITG", true );
	LUA->SetGlobal( "OPENITG_VERSION", PRODUCT_TOKEN );
}

// LUA bindings for diagnostics functions

#include "LuaFunctions.h"

LuaFunction_NoArgs( GetUptime,			SecondsToHHMMSS( (int)RageTimer::GetTimeSinceStart() ) ); 
LuaFunction_NoArgs( GetNumIOErrors,		ITGIO::m_iInputErrorCount );

// product name function
LuaFunction_NoArgs( GetProductName,		DiagnosticsUtil::GetProductName() );

// diagnostics enumeration functions
LuaFunction_NoArgs( GetNumCrashLogs,		DiagnosticsUtil::GetNumCrashLogs() );
LuaFunction_NoArgs( GetNumMachineScores,	DiagnosticsUtil::GetNumMachineScores() );
LuaFunction_NoArgs( GetNumMachineEdits, 	DiagnosticsUtil::GetNumMachineEdits() );
LuaFunction_NoArgs( GetRevision,		DiagnosticsUtil::GetRevision() );

// arcade diagnostics
LuaFunction_NoArgs( GetIP,			DiagnosticsUtil::GetIP() );
LuaFunction_NoArgs( GetSerialNumber,		DiagnosticsUtil::GetSerialNumber() );
LuaFunction_NoArgs( HubIsConnected,		DiagnosticsUtil::HubIsConnected() );
LuaFunction_NoArgs( GetInputType,		DiagnosticsUtil::GetInputType() );

// set OPENITG LUA variables from here
REGISTER_WITH_LUA_FUNCTION( SetProgramGlobals );
/*
 * (c) 2008 BoXoRRoXoRs
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
