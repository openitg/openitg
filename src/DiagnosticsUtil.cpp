#include "global.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "ProfileManager.h"
#include "SongManager.h"
#include "LuaManager.h"
#include "DiagnosticsUtil.h"
#include "arch/ArchHooks/ArchHooks.h"
#include "UserPackManager.h"	// for USER_PACK_SAVE_PATH

#include "XmlFile.h"
#include "ProductInfo.h"
#include "io/ITGIO.h"
#include "io/USBDevice.h"

#include "iButton.h"

#define PATCH_XML_PATH "Data/patch/patch.xml"
#define STATS_XML_PATH "Data/MachineProfile/Stats.xml"

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

// XXX: we should probably take a parameter for these later on.
// for now, return the only disk space value that matters to us.

CString DiagnosticsUtil::GetDiskSpaceFree()
{
	uint64_t iBytes = HOOKS->GetDiskSpaceFree( USER_PACK_SAVE_PATH );
	return FormatByteValue( iBytes );
}

CString DiagnosticsUtil::GetDiskSpaceTotal()
{
	uint64_t iBytes = HOOKS->GetDiskSpaceTotal( USER_PACK_SAVE_PATH );
	return FormatByteValue( iBytes );
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
	return CString(ProductInfo::getFullVersionString());
}

CString DiagnosticsUtil::GetProductVer()
{
	return CString(ProductInfo::getVersion());
}

namespace
{
	/* this allows us to use the serial numbers on home builds for
	 * debugging information. VersionDate and VersionNumber are extern'd
	 * from verstub. */
	CString GenerateDebugSerial()
	{
		return CString(ProductInfo::getSerial());
	}
}

CString DiagnosticsUtil::GetSerialNumber()
{
	/* Attempt to get a serial number from the dongle */
	CString sSerial = iButton::GetSerialNumber();

	/* If the dongle failed to read, generate a debug serial. */
	if( sSerial.empty() )
		sSerial = GenerateDebugSerial();

	return sSerial;
}

bool DiagnosticsUtil::HubIsConnected()
{
	vector<USBDevice> vDevices;
	GetUSBDeviceList( vDevices );

	for( unsigned i = 0; i < vDevices.size(); i++ )
		if( vDevices[i].IsHub() )
			return true;

	return false;
}

CString g_sInputType = "";

CString DiagnosticsUtil::GetInputType()
{
	return g_sInputType;
}

void DiagnosticsUtil::SetInputType( const CString &sType )
{
	g_sInputType = sType;
}

// LUA bindings for diagnostics functions

#include "LuaFunctions.h"

LuaFunction_NoArgs( GetUptime,			SecondsToHHMMSS( (int)RageTimer::GetTimeSinceStart() ) ); 
LuaFunction_NoArgs( GetNumIOErrors,		ITGIO::m_iInputErrorCount );

// disk space functions
LuaFunction_NoArgs( GetDiskSpaceFree,		DiagnosticsUtil::GetDiskSpaceFree() );
LuaFunction_NoArgs( GetDiskSpaceTotal,		DiagnosticsUtil::GetDiskSpaceTotal() );

// versioning functions
LuaFunction_NoArgs( GetRevision,		DiagnosticsUtil::GetRevision() );
LuaFunction_NoArgs( GetProductName,		DiagnosticsUtil::GetProductName() );
LuaFunction_NoArgs( GetProductVer,		DiagnosticsUtil::GetProductVer() );

// diagnostics enumeration functions
LuaFunction_NoArgs( GetNumCrashLogs,		DiagnosticsUtil::GetNumCrashLogs() );
LuaFunction_NoArgs( GetNumMachineScores,	DiagnosticsUtil::GetNumMachineScores() );
LuaFunction_NoArgs( GetNumMachineEdits, 	DiagnosticsUtil::GetNumMachineEdits() );

// arcade diagnostics
LuaFunction_NoArgs( GetIP,			DiagnosticsUtil::GetIP() );
LuaFunction_NoArgs( GetSerialNumber,		DiagnosticsUtil::GetSerialNumber() );
LuaFunction_NoArgs( HubIsConnected,		DiagnosticsUtil::HubIsConnected() );
LuaFunction_NoArgs( GetInputType,		DiagnosticsUtil::GetInputType() );

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
