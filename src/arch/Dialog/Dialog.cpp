#include "global.h"
#include "Dialog.h"
#include "DialogDriver.h"
#include "PrefsManager.h"
#include "RageUtil.h"
#include "RageLog.h"

/* the old identifier should maintain backwards compatibility. */
static Preference<CString> g_sIgnoredDialogs( "IgnoredMessageWindows", "" );


static DialogDriver *g_pImpl = NULL;
static DialogDriver_Null g_NullDriver;
static bool g_bWindowed = true;		// Start out true so that we'll show errors before DISPLAY is init'd.
static bool g_bIsShowingDialog = false;

void Dialog::Init()
{
	if( g_pImpl != NULL )
		return;

	g_pImpl = DialogDriver::Create();

	/* DialogDriver_Null should have worked, at least. */
	ASSERT( g_pImpl != NULL );
}

void Dialog::Shutdown()
{
	delete g_pImpl;
	g_pImpl = NULL;
}

bool Dialog::IsShowingDialog()
{
	return g_bIsShowingDialog;
}

static bool MessageIsIgnored( CString sID )
{
	vector<CString> asList;
	split( g_sIgnoredDialogs, ",", asList );
	for( unsigned i = 0; i < asList.size(); ++i )
		if( !sID.CompareNoCase(asList[i]) )
			return true;
	return false;
}

void Dialog::IgnoreMessage( CString sID )
{
	/* We can't ignore messages before PREFSMAN is around. */
	if( PREFSMAN == NULL )
	{
		if( sID != "" && LOG )
			LOG->Warn( "Dialog: message \"%s\" set ID too early for ignorable messages", sID.c_str() );
			
		return;
	}

	if( sID == "" )
		return;

	if( MessageIsIgnored(sID) )
		return;

	vector<CString> asList;
	split( g_sIgnoredDialogs, ",", asList );
	asList.push_back( sID );
	g_sIgnoredDialogs.Set( join(",",asList) );
	PREFSMAN->SaveGlobalPrefsToDisk();
}

void Dialog::Error( CString sMessage, CString sID )
{
	Dialog::Init();

	if( LOG )
		LOG->Trace( "Dialog: \"%s\" [%s]", sMessage.c_str(), sID.c_str() );

	if( sID != "" && MessageIsIgnored(sID) )
		return;

	g_bIsShowingDialog = true;
	
	g_pImpl->Error( sMessage, sID );
	
	g_bIsShowingDialog = false;
}

void Dialog::SetWindowed( bool bWindowed )
{
	g_bWindowed = bWindowed;
}

void Dialog::OK( CString sMessage, CString sID )
{
	Dialog::Init();

	if( LOG )
		LOG->Debug( "Dialog: \"%s\" [%s]", sMessage.c_str(), sID.c_str() );

	if( sID != "" && MessageIsIgnored(sID) )
		return;

	g_bIsShowingDialog = true;
	
	// only show Dialog if windowed
	if( !g_bWindowed )
		g_NullDriver.OK( sMessage, sID );
	else
		g_pImpl->OK( sMessage, sID );	// call derived version
	
	g_bIsShowingDialog = false;
}

Dialog::Result Dialog::AbortRetryIgnore( CString sMessage, CString sID )
{
	Dialog::Init();

	if( LOG )
		LOG->Trace( "Dialog: \"%s\" [%s]", sMessage.c_str(), sID.c_str() );

	if( sID != "" && MessageIsIgnored(sID) )
		return g_NullDriver.AbortRetryIgnore( sMessage, sID );

	g_bIsShowingDialog = true;
	
	// only show Dialog if windowed
	Dialog::Result ret;
	if( !g_bWindowed )
		ret = g_NullDriver.AbortRetryIgnore( sMessage, sID );
	else
		ret = g_pImpl->AbortRetryIgnore( sMessage, sID );	// call derived version
	
	g_bIsShowingDialog = false;

	return ret;
}

Dialog::Result Dialog::AbortRetry( CString sMessage, CString sID )
{
	Dialog::Init();

	if( LOG )
		LOG->Trace( "Dialog: \"%s\" [%s]", sMessage.c_str(), sID.c_str() );

	if( sID != "" && MessageIsIgnored(sID) )
		return g_NullDriver.AbortRetry( sMessage, sID );

	g_bIsShowingDialog = true;

	// only show Dialog if windowed
	Dialog::Result ret;
	if( !g_bWindowed )
		ret = g_NullDriver.AbortRetry( sMessage, sID );
	else
		ret = g_pImpl->AbortRetry( sMessage, sID );	// call derived version
	
	g_bIsShowingDialog = false;

	return ret;
}

/*
 * (c) 2003-2004 Glenn Maynard, Chris Danford
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
