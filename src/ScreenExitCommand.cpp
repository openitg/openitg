#include "global.h"
#include "ScreenExitCommand.h"
#include "StepMania.h"

#ifdef WIN32
#include <windows.h>
#include <shellapi.h>
#endif

#define EXEC_PATH		THEME->GetMetric(m_sName, "ExecPath")
#define EXEC_PARAMS		THEME->GetMetric(m_sName, "ExecParams")

/* This screen used to wait for sounds to stop.  However, implementing GetPlayingSounds()
 * is annoying, because sounds might be deleted at any time; they aren't ours to have
 * references to.  Also, it's better to quit on command instead of waiting several seconds
 * for a sound to stop. */
REGISTER_SCREEN_CLASS( ScreenExitCommand );
ScreenExitCommand::ScreenExitCommand( CString sName ): Screen( sName )
{
	m_Exited = false;

	#ifdef WIN32
		ShellExecute(NULL, NULL, EXEC_PATH, EXEC_PARAMS, NULL, SW_SHOWNORMAL);
	#endif

	#ifdef Linux
		//sucky compared to the windows code :/
		system(EXEC_PATH EXEC_PARAMS);
	#endif


	ExitGame();

	/* It'd be better for any previous screen playing music to fade it out as it fades
	 * out the screen.  XXX: Check to see if it's fading out; if it'll stop playing in
	 * reasonable time, let it. */
//	SOUND->StopMusic();
}

void ScreenExitCommand::Update( float fDelta )
{
#if 0
	if( m_Exited )
		return;

	/* Grab the list of playing sounds, and see if it's empty. */
	const set<RageSound *> &PlayingSounds = SOUNDMAN->GetPlayingSounds();
	bool DoQuit = PlayingSounds.empty();

	/* As a safety precaution, don't wait indefinitely, in case some sound was
	 * inadvertently set to play too long. */
	if( !DoQuit && m_ShutdownTimer.PeekDeltaTime() > 3 )
	{
		DoQuit = true;
		CString warn = ssprintf("ScreenExitCommand: %i sound%s failed to finish playing quickly: ",
			(int) PlayingSounds.size(), (PlayingSounds.size()==1?"":"s") );
		for( set<RageSound *>::const_iterator i = PlayingSounds.begin();
			i != PlayingSounds.end(); ++i )
		{
			warn += (*i)->GetLoadedFilePath() + "; ";
		}
			
		LOG->Warn("%s", warn.c_str() );
	}

	if( DoQuit )
	{
		m_Exited = true;
		LOG->Trace("ScreenExitCommand: shutting down");
		ExitGame();
	}
#endif
}

/*
 * (c) 2003-2004 Glenn Maynard.  (c) 2008 Rob Campbell.
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
