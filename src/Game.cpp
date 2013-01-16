#include "global.h"
#include "Game.h"
#include "Style.h"
#include "GameState.h"
#include "PrefsManager.h"

int Game::GetNumGameplayButtons() const
{
	return m_iButtonsPerController - GAME_BUTTON_NEXT;
}

GameButton Game::ButtonNameToIndex( const CString &sButtonName ) const
{
	for( int i=0; i<m_iButtonsPerController; i++ ) 
		if( stricmp(m_szButtonNames[i], sButtonName) == 0 )
			return GameButton(i);

	return GAME_BUTTON_INVALID;
}

MenuInput Game::GameInputToMenuInput( GameInput GameI ) const
{
	PlayerNumber pn = (PlayerNumber) GameI.controller;
	if( GAMESTATE->GetCurrentStyle() )
		pn = GAMESTATE->GetCurrentStyle()->ControllerToPlayerNumber(GameI.controller);

	FOREACH_MenuButton(i)
		if( m_DedicatedMenuButton[i] == GameI.button )
			return MenuInput( pn, i );

	if( !PREFSMAN->m_bOnlyDedicatedMenuButtons )
	{
		FOREACH_MenuButton(i)
			if( m_SecondaryMenuButton[i] == GameI.button )
				return MenuInput( pn, i );
	}

	return MenuInput();	// invalid GameInput
}

void Game::MenuInputToGameInput( MenuInput MenuI, GameInput GameIout[4] ) const
{
	ASSERT( MenuI.IsValid() );

	GameIout[0].MakeInvalid();	// initialize
	GameIout[1].MakeInvalid();	
	GameIout[2].MakeInvalid();	
	GameIout[3].MakeInvalid();	

	GameController controller[2];
	StyleType type = TWO_PLAYERS_TWO_SIDES;
	if( GAMESTATE->GetCurrentStyle() )
		type = GAMESTATE->GetCurrentStyle()->m_StyleType;

	int iNumSidesUsing = 1;
	switch( type )
	{
	case ONE_PLAYER_ONE_SIDE:
	case TWO_PLAYERS_TWO_SIDES:
		controller[0] = (GameController)MenuI.player;
		iNumSidesUsing = 1;
		break;
	case ONE_PLAYER_TWO_SIDES:
		controller[0] = GAME_CONTROLLER_1;
		controller[1] = GAME_CONTROLLER_2;
		iNumSidesUsing = 2;
		break;
	default:
		ASSERT(0);	// invalid m_StyleType
	};

	GameButton button[2] = { m_DedicatedMenuButton[MenuI.button], m_SecondaryMenuButton[MenuI.button] };
	int iNumButtonsUsing = PREFSMAN->m_bOnlyDedicatedMenuButtons ? 1 : 2;

	for( int i=0; i<iNumSidesUsing; i++ )
	{
		for( int j=0; j<iNumButtonsUsing; j++ )
		{
			GameIout[i*2+j].controller = controller[i];
			GameIout[i*2+j].button = button[j];
		}
	}
}

CString Game::ColToButtonName( int col ) const
{
	const Style *pStyle = GAMESTATE->GetCurrentStyle();
	const char *pzColumnName = pStyle->m_ColumnInfo[PLAYER_1][col].pzName;
	if( pzColumnName != NULL )
		return pzColumnName;

	StyleInput SI( PLAYER_1, col );
	GameInput GI = pStyle->StyleInputToGameInput( SI );

	return m_szButtonNames[GI.button];
}

TapNoteScore Game::MapTapNoteScore( TapNoteScore tns ) const
{
	switch( tns )
	{
	case TNS_MARVELOUS: return	m_mapMarvelousTo;
	case TNS_PERFECT: return m_mapPerfectTo;
	case TNS_GREAT: return m_mapGreatTo;
	case TNS_GOOD: return m_mapGoodTo;
	case TNS_BOO: return m_mapBooTo;
	default: return tns;
	}
}

// lua start
#include "LuaBinding.h"

template<class T>
class LunaGame: public Luna<T>
{
public:
	LunaGame() { LUA->Register( Register ); }

	static int GetName( T *p, lua_State *L )	{ lua_pushstring( L, p->m_szName ); return 1; }

	static void Register( lua_State *L )
	{
		ADD_METHOD( GetName )
		Luna<T>::Register( L );
	}
};

LUA_REGISTER_CLASS( Game )

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
