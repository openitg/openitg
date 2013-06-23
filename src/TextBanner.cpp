#include "global.h"
#include "TextBanner.h"
#include "RageUtil.h"
#include "song.h"
#include "PrefsManager.h"
#include "ThemeManager.h"
#include "SongManager.h"
#include "RageTextureManager.h"
#include "ActorUtil.h"
#include "ThemeMetric.h"
#include "Command.h"


void TextBanner::Load( CString sType )
{
	m_bInitted = true;

	m_textTitle.SetName( "Title" );
	m_textTitle.LoadFromFont( THEME->GetPathF(sType,"text") );
	this->AddChild( &m_textTitle );

	m_textSubTitle.SetName( "Subtitle" );
	m_textSubTitle.LoadFromFont( THEME->GetPathF(sType,"text") );
	this->AddChild( &m_textSubTitle );

	m_textArtist.SetName( "Artist" );
	m_textArtist.LoadFromFont( THEME->GetPathF(sType,"text") );
	this->AddChild( &m_textArtist );

	ARTIST_PREPEND_STRING			.Load(sType,"ArtistPrependString");
	TWO_LINES_TITLE_COMMAND			.Load(sType,"TwoLinesTitleCommand");
	TWO_LINES_SUBTITLE_COMMAND		.Load(sType,"TwoLinesSubtitleCommand");
	TWO_LINES_ARTIST_COMMAND		.Load(sType,"TwoLinesArtistCommand");
	THREE_LINES_TITLE_COMMAND		.Load(sType,"ThreeLinesTitleCommand");
	THREE_LINES_SUBTITLE_COMMAND	.Load(sType,"ThreeLinesSubtitleCommand");
	THREE_LINES_ARTIST_COMMAND		.Load(sType,"ThreeLinesArtistCommand");

	ActorUtil::SetXYAndOnCommand( m_textTitle, sType );
	ActorUtil::SetXYAndOnCommand( m_textSubTitle, sType );
	ActorUtil::SetXYAndOnCommand( m_textArtist, sType );
}

TextBanner::TextBanner()
{
	m_bInitted = false;
}


void TextBanner::LoadFromString( 
	const CString &sDisplayTitle, const CString &sTranslitTitle, 
	const CString &sDisplaySubTitle, const CString &sTranslitSubTitle, 
	const CString &sDisplayArtist, const CString &sTranslitArtist )
{
	ASSERT( m_bInitted );

	m_textTitle.SetText( sDisplayTitle, sTranslitTitle );
	m_textSubTitle.SetText( sDisplaySubTitle, sTranslitSubTitle );
	m_textArtist.SetText( sDisplayArtist, sTranslitArtist );

	bool bTwoLines = sDisplaySubTitle.size() == 0;

	if( bTwoLines )
	{
		m_textTitle.RunCommands( TWO_LINES_TITLE_COMMAND, this );
		m_textSubTitle.RunCommands( TWO_LINES_SUBTITLE_COMMAND, this );
		m_textArtist.RunCommands( TWO_LINES_ARTIST_COMMAND, this );
	}
	else
	{
		m_textTitle.RunCommands( THREE_LINES_TITLE_COMMAND, this );
		m_textSubTitle.RunCommands( THREE_LINES_SUBTITLE_COMMAND, this );
		m_textArtist.RunCommands( THREE_LINES_ARTIST_COMMAND, this );
	}
}

void TextBanner::LoadFromSong( const Song* pSong )
{
	ASSERT( m_bInitted );

	CString sDisplayTitle		= pSong ? pSong->GetDisplayMainTitle() : CString("");
	CString sTranslitTitle		= pSong ? pSong->GetTranslitMainTitle() : CString("");
	CString sDisplaySubTitle	= pSong ? pSong->GetDisplaySubTitle() : CString("");
	CString sTranslitSubTitle	= pSong ? pSong->GetTranslitSubTitle() : CString("");
	CString sDisplayArtist		= pSong ? (CString)ARTIST_PREPEND_STRING + pSong->GetDisplayArtist() : CString("");
	CString sTranslitArtist		= pSong ? (CString)ARTIST_PREPEND_STRING + pSong->GetTranslitArtist() : CString("");

	LoadFromString( 
		sDisplayTitle, sTranslitTitle, 
		sDisplaySubTitle, sTranslitSubTitle, 
		sDisplayArtist, sTranslitArtist );
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
