#include "global.h"
#include "RageFile.h"
#include "RageLog.h"
#include "RageUtil.h"
#include "AnnouncerManager.h"

AnnouncerManager*	ANNOUNCER = NULL;	// global object accessable from anywhere in the program


const CString EMPTY_ANNOUNCER_NAME = "Empty";
const CString ANNOUNCERS_DIR  = "Announcers/";

AnnouncerManager::AnnouncerManager()
{
	LOG->Trace("AnnouncerManager::AnnouncerManager()");
}

void AnnouncerManager::GetAnnouncerNames( CStringArray& AddTo )
{
	GetDirListing( ANNOUNCERS_DIR+"*", AddTo, true );
	
	// don't load directories matching EMPTY_ANNOUNCER_NAME
	for( int i=AddTo.size()-1; i>=0; i-- )
		if( !stricmp( AddTo[i], EMPTY_ANNOUNCER_NAME ) )
			AddTo.erase(AddTo.begin()+i, AddTo.begin()+i+1 );
}

bool AnnouncerManager::DoesAnnouncerExist( CString sAnnouncerName )
{
	if( sAnnouncerName == "" )
		return true;

	CStringArray asAnnouncerNames;
	GetAnnouncerNames( asAnnouncerNames );
	for( unsigned i=0; i<asAnnouncerNames.size(); i++ )
		if( 0==stricmp(sAnnouncerName, asAnnouncerNames[i]) )
			return true;
	return false;
}

CString AnnouncerManager::GetAnnouncerDirFromName( CString sAnnouncerName )
{
	return ANNOUNCERS_DIR + sAnnouncerName + "/";
}

void AnnouncerManager::SwitchAnnouncer( CString sNewAnnouncerName )
{
	if( !DoesAnnouncerExist(sNewAnnouncerName) )
		m_sCurAnnouncerName = "";
	else
		m_sCurAnnouncerName = sNewAnnouncerName;
}

/* Aliases for announcer paths.  This is for compatibility, so we don't force
 * announcer changes along with everything else.  We could use it to support
 * DWI announcers transparently, too. */
static const char *aliases[][2] = {
	/* ScreenSelectDifficulty compatibility: */
	{ "ScreenSelectDifficulty comment beginner", "select difficulty comment beginner" },
	{ "ScreenSelectDifficulty comment easy", "select difficulty comment easy" },
	{ "ScreenSelectDifficulty comment medium", "select difficulty comment medium" },
	{ "ScreenSelectDifficulty comment hard", "select difficulty comment hard" },
	{ "ScreenSelectDifficulty comment oni", "select difficulty comment oni" },
	{ "ScreenSelectDifficulty comment nonstop", "select difficulty comment nonstop" },
	{ "ScreenSelectDifficulty comment endless", "select difficulty comment endless" },
	{ "ScreenSelectDifficulty intro", "select difficulty intro" },

	/* ScreenSelectStyle compatibility: */
	{ "ScreenSelectStyle intro", "select style intro" },
	{ "ScreenSelectStyle comment single", "select style comment single" },
	{ "ScreenSelectStyle comment double", "select style comment double" },
	{ "ScreenSelectStyle comment solo", "select style comment solo" },
	{ "ScreenSelectStyle comment versus", "select style comment versus" },

	{ NULL, NULL }
};

/* Find an announcer directory with sounds in it.  First search sFolderName,
 * then all aliases above.  Ignore directories that are empty, since we might
 * have "select difficulty intro" with sounds and an empty "ScreenSelectDifficulty
 * intro". */
CString AnnouncerManager::GetPathTo( CString sAnnouncerName, CString sFolderName )
{
	if(sAnnouncerName == "")
		return ""; /* announcer disabled */

	const CString AnnouncerPath = GetAnnouncerDirFromName(sAnnouncerName);

	if( !DirectoryIsEmpty(AnnouncerPath+sFolderName+"/") )
		return AnnouncerPath+sFolderName+"/";

	/* Search for the announcer folder in the list of aliases. */
	int i;
	for(i = 0; aliases[i][0] != NULL; ++i)
	{
		if(sFolderName.CompareNoCase(aliases[i][0]))
			continue; /* no match */

		if( !DirectoryIsEmpty(AnnouncerPath+aliases[i][1]+"/") )
			return AnnouncerPath+aliases[i][1]+"/";
	}

	/* No announcer directory matched.  In debug, create the directory by
	 * its preferred name. */
#ifdef DEBUG
	LOG->Trace( "The announcer in '%s' is missing the folder '%s'.",
		AnnouncerPath.c_str(), sFolderName.c_str() );
//	MessageBeep( MB_OK );
	RageFile temp;
	temp.Open( AnnouncerPath+sFolderName + "/announcer files go here.txt", RageFile::WRITE );
#endif
	
	return "";
}

CString AnnouncerManager::GetPathTo( CString sFolderName )
{
	return GetPathTo(m_sCurAnnouncerName, sFolderName);
}

bool AnnouncerManager::HasSoundsFor( CString sFolderName )
{
	return !DirectoryIsEmpty( GetPathTo(sFolderName) );
}

void AnnouncerManager::NextAnnouncer()
{
	CStringArray as;
	GetAnnouncerNames( as );
	if( as.size()==0 )
		return;

	if( m_sCurAnnouncerName == "" )
		SwitchAnnouncer( as[0] );
	else
	{
		unsigned i;
		for( i=0; i<as.size(); i++ )
			if( as[i].CompareNoCase(m_sCurAnnouncerName)==0 )
				break;
		if( i==as.size()-1 )
			SwitchAnnouncer( "" );
		else
		{
			int iNewIndex = (i+1)%as.size();
			SwitchAnnouncer( as[iNewIndex] );
		}
	}
}

/*
 * (c) 2001-2004 Chris Danford
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
