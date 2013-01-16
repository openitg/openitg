#include "global.h"
#include "RageLog.h"
#include "RageFile.h"
#include "RageUtil.h"
#include "RageTimer.h"
#include "RageThreads.h"
#include "timer/Timer.h"
#include <ctime>
#if defined(_WINDOWS)
#include "windows.h"
#endif
#include <map>

RageLog* LOG;		// global and accessable from anywhere in the program

/* We have a couple log types and a couple logs.
 *
 * Traces are for very verbose debug information.  Use them as much as you want.
 *
 * Warnings are for things that shouldn't happen, but can be dealt with.  (If
 * they can't be dealt with, use RageException::Throw, which will also send a
 * warning.)
 *
 * Info is for important bits of information.  These should be used selectively.
 * Try to keep Info text dense; lots of information is fine, but try to keep
 * it to a reasonable total length.
 *
 * log.txt receives all logs.  This file can get rather large.
 *
 * info.txt receives warnings and infos.  This file should be fairly small; small
 * enough to be mailed without having to be edited or zipped, and small enough
 * to be very easily read.
 *
 */

/* Map data names to logged data.
 *
 * This lets us keep individual bits of changing information to be present
 * in crash logs.  For example, we want to know which file was being processed
 * if we crash during a texture load.  However, we don't want to log every
 * load, since there are a huge number, even for log.txt.  We only want to
 * know the current one, if any.
 *
 * So, when a texture begins loading, we do:
 * LOG->MapLog("TextureManager::Load", "Loading foo.png");
 * and when it finishes loading without crashing,
 * LOG->UnmapLog("TextureManager::Load");
 *
 * Each time a mapped log changes, we update a block of static text to be put
 * in info.txt, so we see "Loading foo.png".
 *
 * The identifier is never displayed, so we can use a simple local object to
 * map/unmap, using any mechanism to generate unique IDs. */
map<CString, CString> LogMaps;

/* Time Map - remember the start time of various things so we can tell how long
 * they took for profiling purposes.
 *
 * Note that the timer implementation is inspired by:
 * http://www.songho.ca/misc/timer/timer.html
 */
map<CString, ProfilingTimer*> TimeMap;

#ifdef ITG_ARCADE
#define LOG_PATH	"Data/log.txt"
#define INFO_PATH	"Data/info.txt"
#else
#define LOG_PATH	"log.txt"
#define INFO_PATH	"info.txt"
#endif

static RageFile *g_fileLog, *g_fileInfo;

/* Mutex writes to the files.  Writing to files is not thread-aware, and this is the
 * only place we write to the same file from multiple threads. */
RageMutex *g_Mutex;

/* staticlog gets info.txt
 * crashlog gets log.txt */
enum {
	/* If this is set, the message will also be written to info.txt and
	 * will be flagged "important" when sent to HOOKS->Log. (info and warnings) */
	WRITE_TO_INFO = 0x01,
	
	/* Whether this line should be loud when written to log.txt (warnings). */
	WRITE_LOUD = 0x02,

	/* Display to output regardless of log settings */
	WRITE_TO_STDOUT = 0x04
};

RageLog::RageLog()
{
	g_fileLog = new RageFile;
	g_fileInfo = new RageFile;
	
	g_Mutex = new RageMutex("Log");

	m_bLogToDisk = false;
	m_bInfoToDisk = false;
	m_bFlush = false;
	m_bShowLogOutput = false;

	// delete old log files
	remove( LOG_PATH );
	remove( INFO_PATH );
}

RageLog::~RageLog()
{
	/* Add the mapped log data to info.txt. */
	const CString AdditionalLog = GetAdditionalLog();
	vector<CString> AdditionalLogLines;
	split( AdditionalLog, "\n", AdditionalLogLines );
	for( unsigned i = 0; i < AdditionalLogLines.size(); ++i )
	{
		TrimLeft( AdditionalLogLines[i] );
		TrimRight( AdditionalLogLines[i] );
		this->Info( "%s", AdditionalLogLines[i].c_str() );
	}

	Flush();
	SetShowLogOutput( false );
	g_fileLog->Close();
	g_fileInfo->Close();

	delete g_Mutex;
	g_Mutex = NULL;

	delete g_fileLog;
	g_fileLog = NULL;
	delete g_fileInfo;
	g_fileInfo = NULL;
}

void RageLog::SetLogToDisk( bool b )
{
	if( m_bLogToDisk == b )
		return;

	m_bLogToDisk = b;

	if( !m_bLogToDisk )
	{
		if( g_fileLog->IsOpen() )
			g_fileLog->Close();
		return;
	}

	if( !g_fileLog->Open( LOG_PATH, RageFile::WRITE|RageFile::STREAMED ) )
		fprintf( stderr, "Couldn't open %s: %s\n", LOG_PATH, g_fileLog->GetError().c_str() );
}

void RageLog::SetInfoToDisk( bool b )
{
	if( m_bInfoToDisk == b )
		return;

	m_bInfoToDisk = b;

	if( !m_bInfoToDisk )
	{
		if( g_fileInfo->IsOpen() )
			g_fileInfo->Close();
		return;
	}

	if( !g_fileInfo->Open( INFO_PATH, RageFile::WRITE|RageFile::STREAMED ) )
		fprintf( stderr, "Couldn't open %s: %s\n", INFO_PATH, g_fileInfo->GetError().c_str() );
}

void RageLog::SetFlushing( bool b )
{
	m_bFlush = b;
}

/* Enable or disable display of output to stdout, or a console window in Windows. */
void RageLog::SetShowLogOutput( bool show )
{
	m_bShowLogOutput = show;

#if defined(WIN32) && !defined(_XBOX)
	if( m_bShowLogOutput )
	{
		// create a new console window and attach standard handles
		AllocConsole();
		freopen("CONOUT$","wb",stdout);
		freopen("CONOUT$","wb",stderr);
	}
	else
	{
		FreeConsole();
	}
#endif
}

void RageLog::Trace( const char *fmt, ...)
{
    va_list	va;
    va_start(va, fmt);
    CString sBuff = vssprintf( fmt, va );
    va_end(va);

	Write(0, sBuff);
}

/* Use this for more important information; it'll always be included
 * in crash dumps. */
void RageLog::Info( const char *fmt, ...)
{
    va_list	va;
    va_start(va, fmt);
    CString sBuff = vssprintf( fmt, va );
    va_end(va);

	Write(WRITE_TO_INFO, sBuff);
}

void RageLog::Warn( const char *fmt, ...)
{
    va_list	va;
    va_start(va, fmt);
    CString sBuff = vssprintf( fmt, va );
    va_end(va);

	Write( WRITE_TO_INFO | WRITE_LOUD, sBuff );
}

/* Acts like Trace(), but displays even with ShowLogOutput off. */
void RageLog::Debug( const char *fmt, ...)
{
    va_list	va;
    va_start(va, fmt);
    CString sBuff = vssprintf( fmt, va );
    va_end(va);

	Write( WRITE_TO_STDOUT, sBuff );
}

void RageLog::Write( int where, const CString &line )
{
	LockMut( *g_Mutex );

	vector<CString> lines;
	split( line, "\n", lines, false );
	if( m_bLogToDisk && g_fileLog->IsOpen() && (where & WRITE_LOUD) )
		g_fileLog->PutLine( "/////////////////////////////////////////" );
	if( where & WRITE_LOUD )
		printf( "/////////////////////////////////////////\n" );

	CString sTimestamp = SecondsToMMSSMsMsMs(RageTimer::GetTimeSinceStart()) + ": ";
	CString sWarning;
	if( where & WRITE_LOUD )
		sWarning = "WARNING: ";

	for( unsigned i = 0; i < lines.size(); ++i )
	{
		CString &str = lines[i];

		if( sWarning.size() )
			str.insert( 0, sWarning );

		if( m_bShowLogOutput || where != 0 )
			printf("%s\n", str.c_str() );
		if( where & WRITE_TO_INFO )
			AddToInfo( str );
		if( m_bLogToDisk && where&WRITE_TO_INFO && g_fileInfo->IsOpen() )
			g_fileInfo->PutLine( str );

		/* Add a timestamp to log.txt and RecentLogs, but not the rest of info.txt
		 * and stdout. */
		str.insert( 0, sTimestamp );

		AddToRecentLogs( str );
		
		if( m_bLogToDisk && g_fileLog->IsOpen() )
			g_fileLog->PutLine( str );
	}

	if( m_bLogToDisk && g_fileLog->IsOpen() && (where & WRITE_LOUD) )
		g_fileLog->PutLine( "/////////////////////////////////////////" );
	if( where & WRITE_LOUD )
		printf( "/////////////////////////////////////////\n" );

	if( m_bFlush || (where & WRITE_TO_INFO) )
		Flush();
}


void RageLog::Flush()
{
	g_fileLog->Flush();
	g_fileInfo->Flush();
}


/* These Get* functions are designed to be called from crash conditions.  We
 * store the text in a static buffer, which we can always access, and we double-
 * check that it's null terminated when we return them.  Finally, multi-line
 * outputs can specify the string used to delineate lines (\n, \r or \r\n).  That
 * way, crash handlers can simply write() the buffer to a file without having
 * to convert line endings, which is tedious when you can't malloc(). */

#if defined(_WIN32)
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

static char staticlog[1024*32]="";
static unsigned staticlog_size = 0;
void RageLog::AddToInfo( const CString &str )
{
	static bool limit_reached = false;
	if( limit_reached )
		return;
	
	unsigned len = str.size() + strlen(NEWLINE);
	if( staticlog_size + len > sizeof(staticlog) )
	{
		const CString txt( NEWLINE "Staticlog limit reached" NEWLINE );
		
		const unsigned pos = min( staticlog_size, sizeof(staticlog) - txt.size() );
		memcpy( staticlog+pos, txt.data(), txt.size() );
		limit_reached = true;
		return;
	}

	memcpy( staticlog+staticlog_size, str.data(), str.size() );
	staticlog_size += str.size();
	memcpy( staticlog+staticlog_size, NEWLINE, strlen(NEWLINE) );
	staticlog_size += strlen(NEWLINE);
}

const char *RageLog::GetInfo()
{
	staticlog[ sizeof(staticlog)-1 ] = 0;
	return staticlog;
}

static const int BACKLOG_LINES = 10;
static char backlog[BACKLOG_LINES][1024];
static int backlog_start=0, backlog_cnt=0;
void RageLog::AddToRecentLogs( const CString &str )
{
	unsigned len = str.size();
	if(len > sizeof(backlog[backlog_start])-1)
		len = sizeof(backlog[backlog_start])-1;

	strncpy(backlog[backlog_start], str, len);
	backlog[backlog_start] [ len ] = 0;

	backlog_start++;
	if(backlog_start > backlog_cnt)
		backlog_cnt=backlog_start;
	backlog_start %= BACKLOG_LINES;
}

const char *RageLog::GetRecentLog( int n )
{
	if( n >= BACKLOG_LINES || n >= backlog_cnt )
		return NULL;

	if( backlog_cnt == BACKLOG_LINES )
	{
		n += backlog_start;
		n %= BACKLOG_LINES;
	}
	/* Make sure it's terminated: */
	backlog[n][ sizeof(backlog[n])-1 ] = 0;

	return backlog[n];
}


static char g_AdditionalLogStr[10240] = "";
static int g_AdditionalLogSize = 0;

void RageLog::UpdateMappedLog()
{
	CString str;
	for(map<CString, CString>::const_iterator i = LogMaps.begin(); i != LogMaps.end(); ++i)
		str += ssprintf("%s" NEWLINE, i->second.c_str());

	g_AdditionalLogSize = min( sizeof(g_AdditionalLogStr), str.size()+1 );
	memcpy( g_AdditionalLogStr, str.c_str(), g_AdditionalLogSize );
	g_AdditionalLogStr[ sizeof(g_AdditionalLogStr)-1 ] = 0;
}

const char *RageLog::GetAdditionalLog()
{
	int size = min( g_AdditionalLogSize, (int) sizeof(g_AdditionalLogStr)-1 );
	g_AdditionalLogStr[size] = 0;
	return g_AdditionalLogStr;
}

void RageLog::MapLog(const CString &key, const char *fmt, ...)
{
	CString s;

	va_list	va;
    va_start(va, fmt);
	s += vssprintf( fmt, va );
    va_end(va);

	LogMaps[key] = s;
	UpdateMappedLog();
}

void RageLog::UnmapLog(const CString &key)
{
	LogMaps.erase(key);
	UpdateMappedLog();
}

void RageLog::ProfileStart(const CString &name, const char *fmt, ...)
{
	CString s;
	va_list	va;
	va_start(va, fmt);
	s += vssprintf( fmt, va );
	va_end(va);

	TimeMap[name] = new ProfilingTimer();
	TimeMap[name]->Start();

	if ( LOG != NULL )
		LOG->Debug(s);
	else
		fprintf( stderr, s );
}

void RageLog::ProfileStop( const CString &name, const char *fmt, ...)
{
	CString s;
	va_list	va;
	va_start(va, fmt);
	s += vssprintf( fmt, va );
	va_end(va);


	map<CString, ProfilingTimer*>::iterator i = TimeMap.find(name);
	if ( i == TimeMap.end() ) {
		// category not found, error
		LOG->Warn( "Called ProfileStop() with unknown category %s", name.c_str() );
		return;
	}

	ProfilingTimer *t = i->second;

	t->Stop();
	double TimeSinceInNS = t->GetElapsedSoFarInS();

	if( LOG != NULL )
		LOG->Debug( "[%6.4f] %s", TimeSinceInNS, s.c_str() );
	else
		fprintf( stderr, "[%6.4f] %s", TimeSinceInNS, s.c_str() );
	SAFE_DELETE(t);
}

void ShowWarning( const char *file, int line, const char *message )
{
	if( LOG != NULL )
		LOG->Warn( "%s:%i: %s", file, line, message );
	else
		fprintf( stderr, "%s:%i: %s", file, line, message );
}

/*
 * Copyright (c) 2001-2004 Chris Danford, Glenn Maynard
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
