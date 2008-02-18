#include "global.h"
#include "LuaManager.h"
#include "LuaFunctions.h"
#include "LuaReference.h"
#include "RageUtil.h"
#include "RageLog.h"
#include "RageFile.h"
#include "RageThreads.h"
#include "arch/Dialog/Dialog.h"
#include "Foreach.h"

//All for the updates....Wow
#include "ProductInfo.h"
#include "ezsockets.h"
#include "RageFileManager.h"
#include "PrefsManager.h"
#include "MemoryCardManager.h"
#include "ProfileManager.h"
#include "Foreach.h" // I almost forgot!
#include "ScreenManager.h"

#include "SongManager.h" // To check the Edit Count on the machine [ScreenArcadeDiagnostics]

#include "XmlFile.h" // I need this, for checking the Revision [ScreenArcadeDiagnostics]

// If you need this, uncomment it. - Vyhd
#if 0
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h> // Hayy, now I can get an IP! [ScreenArcadeDiagnostics]

#include <csetjmp>
#include <cassert>
#endif

LuaManager *LUA = NULL;
static LuaFunctionList *g_LuaFunctions = NULL;

#if defined(_MSC_VER) && !defined(_XBOX)
	#pragma comment(lib, "lua-5.0/lib/LibLua.lib")
	#pragma comment(lib, "lua-5.0/lib/LibLuaLib.lib")
#elif defined(_XBOX)
	#pragma comment(lib, "lua-5.0/lib/LibLuaXbox.lib")
	#pragma comment(lib, "lua-5.0/lib/LibLuaLibXbox.lib")
#endif
#if defined(_MSC_VER) || defined (_XBOX)
	/* "interaction between '_setjmp' and C++ object destruction is non-portable"
	 * We don't care; we'll throw a fatal exception immediately anyway. */
	#pragma warning (disable : 4611)
#endif
#if defined (DARWIN)
	extern void NORETURN longjmp(jmp_buf env, int val);
#endif



struct ChunkReaderData
{
	const CString *buf;
	bool done;
	ChunkReaderData() { buf = NULL; done = false; }
};

const char *ChunkReaderString( lua_State *L, void *ptr, size_t *size )
{
	ChunkReaderData *data = (ChunkReaderData *) ptr;
	if( data->done )
		return NULL;

	data->done = true;

	*size = data->buf->size();
	const char *ret = data->buf->data();
	
	return ret;
}

void LuaManager::SetGlobal( const CString &sName, int val )
{
	Lua *L = LUA->Get();
	LuaHelpers::PushStack( val, L );
	lua_setglobal( L, sName );
	LUA->Release(L);
}

void LuaManager::SetGlobal( const CString &sName, bool val )
{
	Lua *L = LUA->Get();
	LuaHelpers::PushStack( val, L );
	lua_setglobal( L, sName );
	LUA->Release(L);
}

void LuaManager::SetGlobal( const CString &sName, const CString &val )
{
	Lua *L = LUA->Get();
	LuaHelpers::PushStack( val, L );
	lua_setglobal( L, sName );
	LUA->Release(L);
}

void LuaManager::UnsetGlobal( const CString &sName )
{
	Lua *L = LUA->Get();
	lua_pushnil( L );
	lua_setglobal( L, sName );
	LUA->Release(L);
}


void LuaHelpers::Push( const bool &Object, lua_State *L ) { lua_pushboolean( L, Object ); }
void LuaHelpers::Push( const float &Object, lua_State *L ) { lua_pushnumber( L, Object ); }
void LuaHelpers::Push( const int &Object, lua_State *L ) { lua_pushnumber( L, Object ); }
void LuaHelpers::Push( const CString &Object, lua_State *L ) { lua_pushstring( L, Object ); }

bool LuaHelpers::FromStack( bool &Object, int iOffset, lua_State *L ) { Object = !!lua_toboolean( L, iOffset ); return true; }
bool LuaHelpers::FromStack( float &Object, int iOffset, lua_State *L ) { Object = (float)lua_tonumber( L, iOffset ); return true; }
bool LuaHelpers::FromStack( int &Object, int iOffset, lua_State *L ) { Object = (int) lua_tonumber( L, iOffset ); return true; }
bool LuaHelpers::FromStack( CString &Object, int iOffset, lua_State *L )
{
	const char *pStr = lua_tostring( L, iOffset );
	if( pStr != NULL )
		Object = pStr;

	return pStr != NULL;
}

void LuaHelpers::CreateTableFromArrayB( Lua *L, const vector<bool> &aIn )
{
	lua_newtable( L );
	for( unsigned i = 0; i < aIn.size(); ++i )
	{
		lua_pushboolean( L, aIn[i] );
		lua_rawseti( L, -2, i+1 );
	}
}

void LuaHelpers::ReadArrayFromTableB( Lua *L, vector<bool> &aOut )
{
	luaL_checktype( L, -1, LUA_TTABLE );

	for( unsigned i = 0; i < aOut.size(); ++i )
	{
		lua_rawgeti( L, -1, i+1 );
		bool bOn = !!lua_toboolean( L, -1 );
		aOut[i] = bOn;
		lua_pop( L, 1 );
	}
}


static int LuaPanic( lua_State *L )
{
	CString sErr;
	LuaHelpers::PopStack( sErr, L );

	RageException::Throw( "%s", sErr.c_str() );
}



// Actor registration
static vector<RegisterWithLuaFn>	*g_vRegisterActorTypes = NULL;

void LuaManager::Register( RegisterWithLuaFn pfn )
{
	if( g_vRegisterActorTypes == NULL )
		g_vRegisterActorTypes = new vector<RegisterWithLuaFn>;

	g_vRegisterActorTypes->push_back( pfn );
}





LuaManager::LuaManager()
{
	LUA = this;	// so that LUA is available when we call the Register functions

	L = NULL;
	m_pLock = new RageMutex( "Lua" );

	ResetState();
}

LuaManager::~LuaManager()
{
	lua_close( L );
	delete m_pLock;
}

Lua *LuaManager::Get()
{
	m_pLock->Lock();
	return L;
}

void LuaManager::Release( Lua *&p )
{
	ASSERT( p == L );
	m_pLock->Unlock();
	p = NULL;
}


void LuaManager::RegisterTypes()
{
	for( const LuaFunctionList *p = g_LuaFunctions; p; p=p->next )
		lua_register( L, p->name, p->func );
	
	if( g_vRegisterActorTypes )
	{
		for( unsigned i=0; i<g_vRegisterActorTypes->size(); i++ )
		{
			RegisterWithLuaFn fn = (*g_vRegisterActorTypes)[i];
			fn( L );
		}
	}
}

void LuaManager::ResetState()
{
	m_pLock->Lock();

	if( L != NULL )
	{
		LuaReference::BeforeResetAll();

		lua_close( L );
	}

	L = lua_open();
	ASSERT( L );

	lua_atpanic( L, LuaPanic );
	
	luaopen_base( L );
	luaopen_math( L );
	luaopen_string( L );
	luaopen_table( L );
	lua_settop(L, 0); // luaopen_* pushes stuff onto the stack that we don't need

	RegisterTypes();

	LuaReference::AfterResetAll();

	m_pLock->Unlock();
}

void LuaHelpers::PrepareExpression( CString &sInOut )
{
	// HACK: Many metrics have "//" comments that Lua fails to parse.
	// Replace them with Lua-style comments.
	sInOut.Replace( "//", "--" );
	
	// comment out HTML style color values
	sInOut.Replace( "#", "--" );
	
	// Remove leading +, eg. "+50"; Lua doesn't handle that.
	if( sInOut.size() >= 1 && sInOut[0] == '+' )
		sInOut.erase( 0, 1 );
}

bool LuaHelpers::RunScriptFile( const CString &sFile )
{
	RageFile f;
	if( !f.Open( sFile ) )
	{
		CString sError = ssprintf( "Couldn't open Lua script \"%s\": %s", sFile.c_str(), f.GetError().c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}

	CString sScript;
	if( f.Read( sScript ) == -1 )
	{
		CString sError = ssprintf( "Error reading Lua script \"%s\": %s", sFile.c_str(), f.GetError().c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}

	Lua *L = LUA->Get();

	CString sError;
	if( !LuaHelpers::RunScript( L, sScript, sFile, sError, 0 ) )
	{
		LUA->Release(L);
		sError = ssprintf( "Lua runtime error: %s", sError.c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}
	LUA->Release(L);

	return true;
}

bool LuaHelpers::RunScript( Lua *L, const CString &sScript, const CString &sName, CString &sError, int iReturnValues )
{
	// load string
	{
		ChunkReaderData data;
		data.buf = &sScript;
		int ret = lua_load( L, ChunkReaderString, &data, sName );

		if( ret )
		{
			LuaHelpers::PopStack( sError, L );
			return false;
		}
	}

	// evaluate
	{
		int ret = lua_pcall( L, 0, iReturnValues, 0 );
		if( ret )
		{
			LuaHelpers::PopStack( sError, L );
			return false;
		}
	}

	return true;
}


bool LuaHelpers::RunScript( Lua *L, const CString &sExpression, const CString &sName, int iReturnValues )
{
	CString sError;
	if( !LuaHelpers::RunScript( L, sExpression, sName.size()? sName:CString("in"), sError, iReturnValues ) )
	{
		sError = ssprintf( "Lua runtime error parsing \"%s\": %s", sName.size()? sName.c_str():sExpression.c_str(), sError.c_str() );
		Dialog::OK( sError, "LUA_ERROR" );
		return false;
	}

	return true;
}

bool LuaHelpers::RunExpressionB( const CString &str )
{
	Lua *L = LUA->Get();

	if( !LuaHelpers::RunScript(L, "return " + str, "", 1) )
	{
		LUA->Release(L);
		return false;
	}

	/* Don't accept a function as a return value. */
	if( lua_isfunction( L, -1 ) )
		RageException::Throw( "result is a function; did you forget \"()\"?" );

	bool result = !!lua_toboolean( L, -1 );
	lua_pop( L, 1 );
	LUA->Release(L);

	return result;
}

float LuaHelpers::RunExpressionF( const CString &str )
{
	Lua *L = LUA->Get();
	if( !LuaHelpers::RunScript(L, "return " + str, "", 1) )
	{
		LUA->Release(L);
		return 0;
	}

	/* Don't accept a function as a return value. */
	if( lua_isfunction( L, -1 ) )
		RageException::Throw( "result is a function; did you forget \"()\"?" );

	float result = (float) lua_tonumber( L, -1 );
	lua_pop( L, 1 );

	LUA->Release(L);
	return result;
}

int LuaHelpers::RunExpressionI( const CString &str )
{
	return (int) LuaHelpers::RunExpressionF(str);
}

bool LuaHelpers::RunExpressionS( const CString &str, CString &sOut )
{
	Lua *L = LUA->Get();
	if( !LuaHelpers::RunScript(L, "return " + str, "", 1) )
	{
		LUA->Release(L);
		return false;
	}

	/* Don't accept a function as a return value. */
	if( lua_isfunction( L, -1 ) )
		RageException::Throw( "result is a function; did you forget \"()\"?" );

	sOut = lua_tostring( L, -1 );
	lua_pop( L, 1 );

	LUA->Release(L);
	return true;
}

bool LuaHelpers::RunAtExpressionS( CString &sStr )
{
	if( sStr.size() == 0 || sStr[0] != '@' )
		return false;

	/* Erase "@". */
	sStr.erase( 0, 1 );

	CString sOut;
	LuaHelpers::RunExpressionS( sStr, sOut );
	sStr = sOut;
	return true;
}

/* Like luaL_typerror, but without the special case for argument 1 being "self"
 * in method calls, so we give a correct error message after we remove self. */
int LuaHelpers::TypeError( Lua *L, int iArgNo, const char *szName )
{
	lua_Debug debug;
	lua_getstack( L, 0, &debug );
	lua_getinfo( L, "n", &debug );
	return luaL_error( L, "bad argument #%d to \"%s\" (%s expected, got %s)",
		iArgNo, debug.name? debug.name:"(unknown)", szName, lua_typename(L, lua_type(L, iArgNo)) );
}


LuaFunctionList::LuaFunctionList( CString name_, lua_CFunction func_ )
{
	name = name_;
	func = func_;
	next = g_LuaFunctions;
	g_LuaFunctions = this;
}


static bool Trace( const CString &sString )
{
	LOG->Trace( "%s", sString.c_str() );
	return true;
}

// Holy fuck this is useful. Found in ScreenPackages, Props to Mr. X?
// -- Matt1360
static size_t FindEndOfHeaders( const CString &buf )
{
	size_t iPos1 = buf.find( "\n\n" );
	size_t iPos2 = buf.find( "\r\n\r\n" );
	LOG->Trace("end: %u, %u", unsigned(iPos1), unsigned(iPos2));
	if( iPos1 != string::npos && (iPos2 == string::npos || iPos2 > iPos1) )
		return iPos1 + 2;
	else if( iPos2 != string::npos && (iPos1 == string::npos || iPos1 > iPos2) )
		return iPos2 + 4;
	else
		return string::npos;
}
// If you need this, we can uncomment it. I don't think it's being used, though.
// - Vyhd
// Progress Indicator for loading custom songs. -- Matt1360
/*
void UpdateLoadingProgress( float fProgress )
{
	CString sText = ssprintf( "Please wait ... Copying Patch: \n%u%%\n\n\n"
		"Removing the USB drive will cancel this Patch,\n"
		"May corrupt USB drive if removed while mounted", 
		(int)(fProgress*100) );

	SCREENMAN->OverlayMessage( sText );
	SCREENMAN->Draw();
}
*/

#if 0
// Get Newest Revision
// This is the ugliest shit (And iUpdate()) that I've ever fucking written
// -- Matt1360
static int newRevision() {

// 	FILE * newRevF;
// 	char newRev [32];
	
// 	system ( "wget http://www.jeffrey1790.com/machine/currentrev -O /tmp/currentrev" );
// 	newRevF = fopen ( "/tmp/currentrev" , "r" );
// 	if ( newRevF != NULL ) {
// 		fgets ( newRev , 32 , newRevF );
// 		fclose ( newRevF );
// 		system ( "rm /tmp/currentrev" );
// 	}
	
// 	return atoi ( newRev );

	// My attempt at an efficient Machine Update
	// -- Matt1360
	#ifdef WITHOUT_NETWORKING
	return 0;	// In the theme, returning 0 will make it be dead connect.
	#endif // Does this work?

	EzSockets	m_wSocket;

	m_wSocket.close();
	m_wSocket.create();

	m_wSocket.blocking = true;

	CString Server = "jeffrey1790.com";	
	int Port = 80;

	if( !m_wSocket.connect( Server , (short) Port ) )
	{
		return 0;	// Eventually, make it return 0 if it fails, this way
				// In the theme we can make it not show anything
				// Except "Update Check Failed ... Check Network Connection"
				// Or some bullshit.
				// -- Matt1360
	}

	//Produce HTTP header
	CString Header="";

	Header = "GET http://www.jeffrey1790.com/machine/currentrev HTTP/1.0\r\n";
	Header+= "Host: " + Server + "\r\n";
	Header+= "Connection: closed\r\n\r\n";
	
	m_wSocket.blocking = false;

	m_wSocket.SendData( Header.c_str(), Header.length() );

	CString m_sBUFFER = "";
	int m_iBytesGot = 0;

	while(1)
	{
		char Buffer[1024];
		int iSize = m_wSocket.pReadData( Buffer );

		if( iSize <= 0 )
			break;

		m_sBUFFER.append( Buffer, iSize );
		m_iBytesGot += iSize;	// Use something like this for logging		
	}

	int i = m_sBUFFER.find ( "text/plain" );
	int j = m_sBUFFER.GetLength();

	int iRevision = atoi( m_sBUFFER.substr( i + 14 , j ).c_str() );

	return iRevision;
}

// Install Latest Update
static bool iUpdate() {

	// First, we need a clean working directory.
	system ( "rm -R /patches/ *" );
	
	int newRev = newRevision();
	
	if ( newRev > REVISION ) {
		char tempvar [512];
		sprintf(tempvar, "wget http://www.jeffrey1790.com/machine/r%u.dxl -O /patches/revision.dxl", newRev );
		system(tempvar);

		FILE * exist;
		exist = fopen("/patches/revision.dxl","r"); //Hacky hacky
		if (exist != NULL) {
			fclose ( exist );
			//system("cd /patches/;unzip revision.dxl;rm revision.dxl;/bin/sh setup.sh");
			//debug (1 line):
			system("cd /patches/;unzip -o revision.dxl;rm revision.dxl");
			system ( "chmod +x /patches/ *" );
			return true;
		}
	} else {
		return false;
	}*/

	// First, we remove the current patches that were left behind.
	// Also, we kill it after to be safe.

	#ifdef WITHOUT_NETWORKING
		PREFSMAN->m_bUsbUpdate == true; // If there's no connection, we can't be oline.
	#endif

	bool bStyle = PREFSMAN->m_bUsbUpdate;

	if ( bStyle == true ) // Updating with USB Manually, no connection.
	{
		/* Steps to do this Shit...
		 * Mount each card
		 * Search each card for the update file
		 * Something like "DXL r##.dxl"
		 * Compare that with current revision!
		 * Compare it for encryption
		 * RSA check the last 128 bytes
		 * Copy, Extract, Text the encryption on patch.zip
		 * Place a shell script
		 * Reboot
		 */
		
		// We need this, so it calls on the foreach for the sDir. -- Matt1360
		int iPn = 1;

		// First, we check each players' Memory Card.
		FOREACH_PlayerNumber( pn ) {
			if ( !MEMCARDMAN->MountCard( pn ) ) {
				iPn++;
				break;
			}
			
			// Make sure the card is mounted, and ready.
			if( MEMCARDMAN->GetCardState(pn) != MEMORY_CARD_STATE_READY ) {
				iPn++;
				break;
			}
			
			// So now we've established, a card is mounted...Lets get the directory.
			CString sDir = ssprintf( "/@mc%i/" , iPn );
			
			// Lets find all the .dxl files.
			CStringArray aFiles;
			GetDirListing( sDir+"*.dxl" , aFiles , false , false );
			
			// Just call the first one. That's all we need. We can't update multiples. -- Matt1360
			if ( aFiles.size() == 0 ) {
				LOG->Warn ( "No DXL File(s) Found!!" );
				iPn++;
				break;
			}
			
			CString sFullDir = sDir + aFiles[0];
			LOG->Warn ( "Patch Found: %s at %s" , aFiles[0].c_str() , sDir.c_str() );

			CString sText = "";
			// Next copy the file Using that Pointer Function...			
			CString out = "Patches/" + aFiles[0];
			CustomCopy( sFullDir , out , &UpdateLoadingProgress );
			SCREENMAN->HideSystemMessage();

			MEMCARDMAN->UnmountCard( pn );
			// Don't put a iPn++ here, because we found a patch. We only want one...
			// TODO: Check that this works with 2 players... Check that it works with only Player 2.
		}

	} else { // Updating online.

		int iThisRevision = REVISION;
		int iNewRevision = newRevision();

		if ( !iNewRevision >= iThisRevision )	// If the new revision isn't greater than ours...
			return false; 			// Then we're up to date...

		EzSockets	m_wSocket;
		RageFile	m_fNewFile;

		m_wSocket.close();
		m_wSocket.create();

		m_wSocket.blocking = true;

		CString Server = "jeffrey1790.com";	
		int Port = 80;

		if( !m_wSocket.connect( Server , (short) Port ) ) // If it can't connect
			return false;				  // Then we're in crap. "Up to Date"

		//Produce HTTP header
		CString Header="";
		// UGLY: Clean this up...Seriously one variable is good enough.
		CString sUpdateDir = "http://www.jeffrey1790.com/machine/";
		CString sUpdateFile = ssprintf( "r%i.dxl" , iNewRevision );
		CString sUpdateAddr = ssprintf( "http://www.jeffrey1790.com/machine/%s" , sUpdateFile.c_str() );

		Header = "GET " + sUpdateAddr + " HTTP/1.0\r\n";
		Header+= "Host: " + Server + "\r\n";
		Header+= "Connection: closed\r\n\r\n";

		m_wSocket.blocking = false;

		m_wSocket.SendData( Header.c_str(), Header.length() );

		// First thing's first... Major cleanup.
		// I'm going to use a vector array (ooh fuckin' fancy) to hold every file in Patches/
		// From there they will be for looped and FILEMAN->Remove'd
		// -- Matt1360

		// Better Idea. Use the startup script to clean it out.
		// This way, there are -no- crashes. It's impossible, nearly.
		// The basic workaround: "rm -R /root/stepmania/Patches/*"
		// Possibly add -f to the command to force it.
		// Also, there will surely be a requirement for chmodding the directory.
		// -- Matt1360

// 		CStringArray aFiles; // Fix: Faster with an array.
// 		GetDirListing( "Patches/" , aFiles, false, false );
// 		for ( unsigned i = 0; aFiles.size(); i++ ) {
// 			if( !aFiles.size() ) // If there is no size?
// 				break;
// 			FILEMAN->Remove ( "Patches/" + aFiles[i] );
// 			LOG->Warn( "Killed: %s" , aFiles[i].c_str() );
// 		}
// 		LOG->Warn ("KILLED ALL FILES");

		if ( !m_fNewFile.Open ( "Patches/" + sUpdateFile , RageFile::WRITE | RageFile::STREAMED ) )
			return false;	// Something wrong if we hit this. Bad disk?

		CString m_sBUFFER = "";

		while(1)
		{
			char Buffer[1024];
			int iSize = m_wSocket.pReadData( Buffer );
	
			if( iSize <= 0 )
				break;

			m_sBUFFER.append( Buffer, iSize );
// 			m_iBytesGot += iSize;	// Use something like this for logging
		}

		// Kill the fucking header. Using that ScreenPackages function.
		// GOD this is a lifesaver!
		// -- Matt1360
		size_t iHeaderEnd = FindEndOfHeaders( m_sBUFFER );
		if( iHeaderEnd == m_sBUFFER.npos )
			return false;
		m_sBUFFER.erase( 0, iHeaderEnd );

		m_fNewFile.Write( m_sBUFFER );
		
		return true;
	}

	// Here, we will take care of the file after it's been loaded to the Patches Directory.
	// REMEMBER: ShutdownGame() when complete! In StepMania.h/cpp
	// ExitGame() May be safer? Research! All in StepMania.h/cpp
	
	// Make it false, just in case. You never know what might not fall through.
	// -- Matt1360
	return false;
	
}
#endif

// This is how I chose to find the Crash Log size.
// -- Matt1360
static int GetNumCrashLogs()
{
	// Give ourselves a variable.
	CStringArray aLogs;
	
	// Get them all.
	GetDirListing( "/stats/crashlog-*.txt" , aLogs );
	
	return aLogs.size();
}

static int GetNumMachineEdits()
{
	CStringArray aEdits;
	CString sDir = PROFILEMAN->GetProfileDir(PROFILE_SLOT_MACHINE) + EDIT_SUBDIR;
	
	GetDirListing( sDir , aEdits );
	
	return aEdits.size();
}

static int GetIP()
{
	return 0;
}

static int GetRevision()
{
	// Create the XML Handler, and clear it, for practice.
	XNode *xml = new XNode;
	xml->Clear();
	xml->m_sName = "patch";
	
	// Check for the file existing
	if( !IsAFile( "/Data/patch/patch.xml" ) )
	{
		LOG->Trace( "There is no patch file (patch.xml)" );
		return 1;
	}
	
	// Make sure you can read it
	if( !xml->LoadFromFile( "/Data/patch/patch.xml" ) )
	{
		LOG->Trace( "patch.xml unloadable" );
		return 1;
	}
	
	// Check the node <Revision>x</Revision>
	if( !xml->GetChild( "Revision" ) )
	{
		LOG->Trace( "Revision node missing! (patch.xml)" );
		return 1;
	}
	
	// Return as an integer
	return atoi( xml->GetChildValue( "Revision" ) );
}

static int GetNumMachineScores()
{
	// Create the XML Handler and clear it, for practice
	XNode *xml = new XNode;
	xml->Clear();
	
	// Check for the file existing
	if( !IsAFile( "/Data/MachineProfile/Stats.xml" ) )
	{
		LOG->Trace( "There is no stats file!" );
		return 1;
	}
	
	// Make sure you can read it
	if( !xml->LoadFromFile( "/Data/MachineProfile/Stats.xml" ) )
	{
		LOG->Trace( "Stats.xml unloadable!" );
		return 1;
	}
	
	const XNode *pData = xml->GetChild( "SongScores" );
	
	if( pData == NULL )
	{
		LOG->Warn( "Error loading scores: <SongScores> node missing" );
		return 0;
	}
	
	int iScoreCount = 0;
	
	// Named here, for LoadFromFile() renames it to "Stats"
	xml->m_sName = "SongScores";
	
	// For each pData Child, or the Child in SongScores...
	FOREACH_CONST_Child( pData , p )
		iScoreCount++;
	
	return iScoreCount;
}

LuaFunction( Trace, Trace(SArg(1)) ); // Log Traces

/*
 * [ScreenArcadeDiagnostics]
 *
 * All ITG2AC Functions here
 * Mostly...Implemented
 *
 * Work Log!
 *
 * Work started 2/9/08, after 10 PM - 2:30 AM
 *  ProductName, Revision, Uptime, Crashlogs,
 *  Machine edits, done!
 *
 * Work, 2/10/08 7 PM - 9:30 PM
 *  Did work on GetNumMachineScores() ... That sucked
 *  Somewhat complete...Can't do IO Errors, an ITG-IO
 *  exclusive, it seems.
 *
 * Total Hours: ~6
 */
LuaFunction( GetProductName	, CString( PRODUCT_NAME ) ); // Return the product's name from ProductInfo.h [ScreenArcadeDiagnostics]
LuaFunction( GetRevision	, GetRevision() ); // Return current Revision ( ProductInfo.h ) [ScreenArcadeDiagnostics]
LuaFunction( GetUptime		, SecondsToMMSSMsMsMs( RageTimer::GetTimeSinceStart() ) ); // Uptime calling [ScreenArcadeDiagnostics]
LuaFunction( GetIP		, GetIP() ); // Calling the IP [ScreenArcadeDiagnostics]
LuaFunction( GetNumCrashLogs	, GetNumCrashLogs() ); // Count the crashlogs [ScreenArcadeDiagnostics]
LuaFunction( GetNumMachineEdits	, GetNumMachineEdits() ); // Count the machine edits [ScreenArcadeDiagnostics]
LuaFunction( GetNumIOErrors	, 0 ); // Call the number of I/O Errors [ScreenArcadeDiagnostics]
LuaFunction( GetNumMachineScores, GetNumMachineScores() ); // Call the machine score count [ScreenArcadeDiagnostics]

/*
 * [ScreenArcadePatch]
 *
 * Work Started 2/11/08 9:30 PM
 *
 */

/*
 *
 * LuaFunction( gRev, REVISION );		// Returns Revision (ProductInfo.h)
 * LuaFunction( newRev , newRevision() );	// Returns Newest Revision
 * LuaFunction( iUpdate , iUpdate() );	// Initiates an update...Checks Revision First.
 * LuaFunction( uptime , SecondsToMMSSMsMsMs(RageTimer::GetTimeSinceStart()) );		// Returns the current Uptime
 *
 */

/*
 * (c) 2004 Glenn Maynard
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
