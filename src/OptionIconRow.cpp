#include "global.h"
#include "OptionIconRow.h"
#include "ThemeManager.h"
#include "GameState.h"
#include "PlayerState.h"


#define SPACING_X	THEME->GetMetricF("OptionIconRow","SpacingX")
#define SPACING_Y	THEME->GetMetricF("OptionIconRow","SpacingY")


OptionIconRow::OptionIconRow()
{
	m_PlayerNumber = NUM_PLAYERS;
	for( unsigned i=0; i<NUM_OPTION_COLS; i++ )
	{
		m_OptionIcon[i].SetXY( i*SPACING_X, i*SPACING_Y );
		this->AddChild( &m_OptionIcon[i] );
	}
}


struct OptionColumnEntry
{
	char szString[30];
	int iSlotIndex;
};

const OptionColumnEntry g_OptionColumnEntries[] =
{
	{"Boost",		0},
	{"Brake",		0},
	{"Wave",		0},
	{"Expand",		0},
	{"Boomerang",	0},
	{"Drunk",		1},
	{"Dizzy",		1},
	{"Mini",		1},
	{"Flip",		1},
	{"Tornado",		1},
	{"Hidden",		2},
	{"Sudden",		2},
	{"Stealth",		2},
	{"Blink",		2},
	{"RandomVanish", 2},
	{"Mirror",		3},
	{"Left",		3},
	{"Right",		3},
	{"Shuffle",		3},
	{"SuperShuffle",3},
	{"Little",		4},
	{"NoHolds",		4},
	{"Dark",		4},
	{"Blind",		4},
	{"Reverse",		5},
	{"Split",		5},
	{"Alternate",	5},
	{"Cross",		5},
	{"Centered",	5},
	{"Incoming",	6},
	{"Space",		6},
	{"Hallway",		6},
	{"Distant",		6},
};

int OptionToPreferredColumn( CString sOptionText )
{
	/* Speedups always go in column 0. digit ... x */
	if(sOptionText.length() > 1 &&
		isdigit(sOptionText[0])    &&
		tolower(sOptionText[sOptionText.length()-1]) == 'x') {
			return 0;
	}

	for( unsigned i=0; i<ARRAYLEN(g_OptionColumnEntries); i++ )
		if( g_OptionColumnEntries[i].szString == sOptionText )
			return g_OptionColumnEntries[i].iSlotIndex;

	/* This warns about C1234 and noteskins. */
//	LOG->Warn("Unknown option: '%s'", sOptionText.c_str() );
	return 0;
}

void OptionIconRow::Load( PlayerNumber pn )
{
	m_PlayerNumber = pn;
	for( unsigned i=0; i<NUM_OPTION_COLS; i++ )
		m_OptionIcon[i].Load( "OptionIconRow" );		
}

void OptionIconRow::Refresh()
{
	ASSERT( m_PlayerNumber != NUM_PLAYERS );
	
	// init

	CString sOptions = GAMESTATE->m_pPlayerState[m_PlayerNumber]->m_PlayerOptions.GetString();
	CStringArray asOptions;
	split( sOptions, ", ", asOptions, true );


	CString asTabs[NUM_OPTION_COLS-1];	// fill these with what will be displayed on the tabs
	
	// for each option, look for the best column to place it in
	for( unsigned i=0; i<asOptions.size(); i++ )
	{
		CString sOption = asOptions[i];
		int iPerferredCol = OptionToPreferredColumn( sOption );

		if( iPerferredCol == -1 )
			continue;	// skip

		// search for a vacant spot
		for( unsigned i=iPerferredCol; i<NUM_OPTION_COLS-1; i++ )
		{
			if( asTabs[i] != "" )
				continue;
			else
			{
				asTabs[i] = sOption;
				break;
			}
		}
	}

	for( unsigned i=0; i<NUM_OPTION_COLS; i++ )
		if( i == 0 )
			m_OptionIcon[i].Set( m_PlayerNumber, "", true );
		else
			m_OptionIcon[i].Set( m_PlayerNumber, asTabs[i-1], false );		
}

void OptionIconRow::DrawPrimitives()
{
	ActorFrame::DrawPrimitives();
}

/*
 * (c) 2002-2004 Chris Danford
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
