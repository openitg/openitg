#include "global.h"

/*
 * Styles define a set of columns for each player, and information about those
 * columns, like what Instruments are used play those columns and what track 
 * to use to populate the column's notes.
 * A "track" is the term used to descibe a particular vertical sting of note 
 * in NoteData.
 * A "column" is the term used to describe the vertical string of notes that 
 * a player sees on the screen while they're playing.  Column notes are 
 * picked from a track, but columns and tracks don't have a 1-to-1 
 * correspondance.  For example, dance-versus has 8 columns but only 4 tracks
 * because two players place from the same set of 4 tracks.
 */

#include "Style.h"
#include "Game.h"
#include "GameState.h"
#include "NoteData.h"
#include <float.h>


void Style::GetTransformedNoteDataForStyle( PlayerNumber pn, const NoteData& original, NoteData& noteDataOut ) const
{
	int iNewToOriginalTrack[MAX_COLS_PER_PLAYER];
	for( int col=0; col<m_iColsPerPlayer; col++ )
	{
		ColumnInfo colInfo = m_ColumnInfo[pn][col];
		int originalTrack = colInfo.track;
		
		iNewToOriginalTrack[col] = originalTrack;
	}
	
	noteDataOut.LoadTransformed( original, m_iColsPerPlayer, iNewToOriginalTrack );
}

GameInput Style::StyleInputToGameInput( const StyleInput& StyleI ) const
{
	ASSERT_M( StyleI.player < NUM_PLAYERS, ssprintf("P%i", StyleI.player) );
	ASSERT_M( StyleI.col < MAX_COLS_PER_PLAYER, ssprintf("C%i", StyleI.col) );

	bool bUsingOneSide = this->m_StyleType != ONE_PLAYER_TWO_SIDES;

	FOREACH_GameController(gc)
	{
		if( bUsingOneSide && gc != (int) StyleI.player )
			continue;

		for( GameButton gb = GAME_BUTTON_NEXT; gb < m_pGame->m_iButtonsPerController && m_iInputColumn[gc][gb-GAME_BUTTON_NEXT] != END_MAPPING; gb=(GameButton)(gb+1) )
			if( m_iInputColumn[gc][gb-GAME_BUTTON_NEXT] == StyleI.col )
				return GameInput( gc, gb );
	}

	FAIL_M( ssprintf("Unknown StyleInput %i,%i", StyleI.player, StyleI.col) );
};

StyleInput Style::GameInputToStyleInput( const GameInput &GameI ) const
{
	StyleInput SI;

	if( GameI.button < GAME_BUTTON_NEXT )
		return SI;	// Return invalid.

	int iColumnIndex = GameI.button - GAME_BUTTON_NEXT;

	if( m_iInputColumn[GameI.controller][iColumnIndex] == NO_MAPPING )
		return SI;	// Return invalid.

	for( int i = 0; i <= iColumnIndex; ++i )
		if( m_iInputColumn[GameI.controller][i] == END_MAPPING )
			return SI;	// Return invalid.

	SI = StyleInput( ControllerToPlayerNumber(GameI.controller), m_iInputColumn[GameI.controller][iColumnIndex] );

	return SI;
}


PlayerNumber Style::ControllerToPlayerNumber( GameController controller ) const
{
	switch( m_StyleType )
	{
	case ONE_PLAYER_ONE_SIDE:
	case TWO_PLAYERS_TWO_SIDES:
		return (PlayerNumber)controller;
	case ONE_PLAYER_TWO_SIDES:
		return GAMESTATE->m_MasterPlayerNumber;
	default:
		ASSERT(0);
		return PLAYER_INVALID;
	}
}

bool Style::MatchesStepsType( StepsType type ) const
{
	if(type == m_StepsType) return true;

	return false;
}

void Style::GetMinAndMaxColX( PlayerNumber pn, float& fMixXOut, float& fMaxXOut ) const
{
	ASSERT( pn != PLAYER_INVALID );

	fMixXOut = FLT_MAX;
	fMaxXOut = FLT_MIN;
	for( int i=0; i<m_iColsPerPlayer; i++ )
	{
		fMixXOut = min( fMixXOut, m_ColumnInfo[pn][i].fXOffset );
		fMaxXOut = max( fMaxXOut, m_ColumnInfo[pn][i].fXOffset );
	}
}

/*
 * (c) 2001-2002 Chris Danford
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
