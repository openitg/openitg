#include "global.h"
#include "GameState.h"
#include "MessageManager.h"
#include "LightsDriver_ActorMessage.h"

REGISTER_LIGHTS_DRIVER( ActorMessage );

static const CString OnOrOff( bool bVal )
{
	return bVal ? "On" : "Off";
}

LightsDriver_ActorMessage::LightsDriver_ActorMessage()
{
	m_pLastState = new LightsState;
}

LightsDriver_ActorMessage::~LightsDriver_ActorMessage()
{
	delete m_pLastState;
}

void LightsDriver_ActorMessage::Set( const LightsState *ls )
{
	if( MESSAGEMAN == NULL )
		FAIL_M( "assumed MESSAGEMAN is always non-NULL, it isn't" );

	/* Broadcast cabinet lights */
	FOREACH_CabinetLight( cl )
	{
		/* Only broadcast messages for changed states. */
		bool bNow = ls->m_bCabinetLights[cl];
		bool bLast = m_pLastState->m_bCabinetLights[cl];

		if( bNow == bLast )
			continue;

		CString sMsg = CabinetLightToString(cl) + OnOrOff(bNow);
		MESSAGEMAN->Broadcast( sMsg );
	}

	/* Broadcast game lights. */
	FOREACH_GameController( gc )
	{
		PlayerNumber pn = PlayerNumber(gc);

		FOREACH_GameButton( gb )
		{
			bool bNow = ls->m_bGameButtonLights[gc][gb];
			bool bLast = m_pLastState->m_bGameButtonLights[gc][gb];

			if( bNow == bLast )
				continue;

			const Game* pGame = GAMESTATE->GetCurrentGame();

			const CString sPlayer = PlayerNumberToString(pn);
			const CString sButton = GameButtonToString(pGame, gb);

			const CString sMsg = sPlayer + sButton + OnOrOff(bNow);
			MESSAGEMAN->Broadcast( sMsg );
		}
	}

	/* copy the previous state into the current one for the next update */
	*m_pLastState = *ls;
}

/*
 * (c) 2011 Mark Cannon
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
