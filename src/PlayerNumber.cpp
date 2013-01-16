#include "global.h"
#include "GameState.h"
#include "LuaManager.h"

static const CString PlayerNumberNames[] =
{
        "P1",
        "P2",
};
XToString( PlayerNumber, 2 );

PlayerNumber GetNextHumanPlayer( PlayerNumber pn )
{
	for( PlayerNumber p=(PlayerNumber)(pn+1); p<NUM_PLAYERS; p=(PlayerNumber)(p+1) )
	{
		if( GAMESTATE->IsHumanPlayer(p) )
			return p;
	}
	return PLAYER_INVALID;
}

PlayerNumber GetNextEnabledPlayer( PlayerNumber pn )
{
	for( PlayerNumber p=(PlayerNumber)(pn+1); p<NUM_PLAYERS; p=(PlayerNumber)(p+1) )
	{
		if( GAMESTATE->IsPlayerEnabled(p) )
			return p;
	}
	return PLAYER_INVALID;
}

PlayerNumber GetNextCpuPlayer( PlayerNumber pn )
{
	for( PlayerNumber p=(PlayerNumber)(pn+1); p<NUM_PLAYERS; p=(PlayerNumber)(p+1) )
	{
		if( GAMESTATE->IsCpuPlayer(p) )
			return p;
	}
	return PLAYER_INVALID;
}

PlayerNumber GetNextPotentialCpuPlayer( PlayerNumber pn )
{
	for( PlayerNumber p=(PlayerNumber)(pn+1); p<NUM_PLAYERS; p=(PlayerNumber)(p+1) )
	{
		if( !GAMESTATE->IsHumanPlayer(p) )
			return p;
	}
	return PLAYER_INVALID;
}

void LuaPlayerNumber(lua_State* L)
{
	FOREACH_PlayerNumber( pn )
		LUA->SetGlobal( ssprintf("PLAYER_%d",pn+1), pn );
	LUA->SetGlobal( "NUM_PLAYERS", NUM_PLAYERS );
}
REGISTER_WITH_LUA_FUNCTION( LuaPlayerNumber );


/*
 * (c) 2001-2004 Chris Danford, Chris Gomez
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
