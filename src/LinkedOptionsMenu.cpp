#include "global.h"
#include "ScreenManager.h"
#include "ActorUtil.h"
#include "LinkedOptionsMenu.h"

#ifdef _MSC_VER
/* It's going to be a pain to fix these. Disable for now. */
#pragma warning( disable : 4018 )	// signed/unsigned mismatch
#pragma warning( disable : 4244 )	// conversion, possible loss of data
#endif

void LinkedOptionsMenu::Load( LinkedOptionsMenu *prev, LinkedOptionsMenu *next )
{
	m_sprLineHighlight.Load( THEME->GetPathG(m_sName, "line highlight") );

	m_Frame.SetName( "Frame" );
	this->AddChild( &m_Frame );

	m_SoundMoveRow.Load( THEME->GetPathS( m_sName, "move row" ) );
	m_SoundSwitchMenu.Load( THEME->GetPathS( m_sName, "switch menu" ) );

	m_FramePage.Load( THEME->GetPathG( m_sName, "page" ) );
	m_Frame.AddChild( m_FramePage );
	
	m_Cursor.SetName( "Cursor" );
	m_Cursor.Load( m_sName, OptionsCursor::underline );
	m_Cursor.Set(PLAYER_1);
	m_Frame.AddChild( &m_Cursor );
	ON_COMMAND( m_Cursor );

	m_sprIndicatorUp.SetName("IndicatorUp");
	m_sprIndicatorDown.SetName("IndicatorDown");
	m_sprIndicatorUp.Load( THEME->GetPathG( m_sName, "indicator up" ) );
	m_sprIndicatorDown.Load( THEME->GetPathG( m_sName, "indicator down" ) );
	m_Frame.AddChild( &m_sprIndicatorUp );
	m_Frame.AddChild( &m_sprIndicatorDown );
	ON_COMMAND( m_sprIndicatorUp );
	ON_COMMAND( m_sprIndicatorDown );
	m_bIndTweenedOn[0] = false; m_bIndTweenedOn[1] = false;
	//m_sprIndicatorUp.PlayCommand( "TweenOff" );
	//m_sprIndicatorDown.PlayCommand( "TweenOff" );

	m_PrevMenu = prev;
	m_NextMenu = next;

	m_bFocus = false;
	m_iCurrentSelection = -1;
	m_iCurPage = 0;

	ROW_OFFSET_X.Load( m_sName, "RowOffsetX" );
	ROW_OFFSET_Y.Load( m_sName, "RowOffsetY" );
	ROW_SPACING_Y.Load( m_sName, "RowSpacingY" );
	CURSOR_OFFSET_X.Load( m_sName, "CursorOffsetX" );
	MENU_WRAPPING.Load( m_sName, "MenuWrapping" );
	ROWS_PER_PAGE.Load( m_sName, "RowsPerPage" );
	ASSERT(ROWS_PER_PAGE > 0);

	ON_COMMAND( m_Frame );

	ClearChoices();
}

void LinkedOptionsMenu::ClearChoices() 
{
	m_Cursor.PlayCommand( "TweenOff" );
	m_sprIndicatorUp.PlayCommand("TweenOff");
	m_sprIndicatorDown.PlayCommand("TweenOff");
	m_bIndTweenedOn[0] = false; m_bIndTweenedOn[1] = false;
	// XXX memleak much?
	for( unsigned i = 0; i < m_Rows.size(); i++ )
	{
		m_Rows[i]->PlayCommand( "Off" );
	}
	m_Rows.clear();
}

void LinkedOptionsMenu::Focus()
{
	m_bFocus = true;
	this->PlayCommand( "Focus" );
	m_Cursor.PlayCommand( "TweenOn" );
}

void LinkedOptionsMenu::Unfocus()
{
	m_bFocus = false;
	m_Cursor.PlayCommand( "TweenOff" );
	this->PlayCommand( "Unfocus" );
}

void LinkedOptionsMenu::SetChoices( const CStringArray &asChoices )
{
	ClearChoices();

	for( unsigned i = 0; i < asChoices.size(); i++ )
	{
		CString ch = asChoices[i];
		BitmapText *bt = new BitmapText;
		m_Rows.push_back( bt );
		bt->LoadFromFont( THEME->GetPathF( m_sName, "menutext" ) );
		bt->SetName( "Row" );
		bt->SetXY( ROW_OFFSET_X, ROW_OFFSET_Y + (ROW_SPACING_Y * (float)(i % ROWS_PER_PAGE)) );
		bt->SetText( ch );
		m_Frame.AddChild( bt );
		ON_COMMAND( *bt );
	}

	// show first page of choices
	for( unsigned i = 0; i < asChoices.size() && i < (unsigned)ROWS_PER_PAGE; i++ )
	{
		m_Rows[i]->PlayCommand("TweenOn");
	}
	m_iCurPage = 0;
	if ( asChoices.size() > (unsigned)ROWS_PER_PAGE )
	{
		m_sprIndicatorDown.PlayCommand("TweenOn");
		m_bIndTweenedOn[1] = true;
	}

	if ( m_Rows.size() > 0 )
	{
		if ( m_bFocus ) m_Cursor.PlayCommand( "TweenOn" );
		m_Cursor.SetXY( CURSOR_OFFSET_X + m_Rows[0]->GetX(), m_Rows[0]->GetY() );
		m_Cursor.SetBarWidth( floor(m_Rows[0]->GetZoomedWidth()) );
		m_iCurrentSelection = 0;
	}
	else
	{
		m_iCurrentSelection = -1;
		m_Cursor.PlayCommand( "TweenOff" );
		if ( m_bFocus )
			SCREENMAN->PostMessageToTopScreen( m_smChangeMenu, 0.0f );
	}
}

CString LinkedOptionsMenu::GetCurrentSelection()
{
	ASSERT( m_iCurrentSelection >= 0 );
	ASSERT( (unsigned)m_iCurrentSelection < m_Rows.size() );
	return m_Rows[m_iCurrentSelection]->GetText();
}

LinkedInputResponseType LinkedOptionsMenu::MoveRow(int iDirection)
{
	ASSERT( m_bFocus );
	ASSERT( iDirection != 0 );
	iDirection = iDirection / abs(iDirection);
	int iNewSelection = m_iCurrentSelection + iDirection;
	if (iNewSelection > 0 && (unsigned)iNewSelection >= m_Rows.size()) // user chose beyond last option
	{
		if ( GetNextMenu() != this && MENU_WRAPPING )
		{
			m_SoundSwitchMenu.Play();
			return LIRT_FORWARDED_NEXT;
		}
		else
			return LIRT_STOP;
	}
	else if (iNewSelection < 0) // user chose before first option
	{
		if ( GetPrevMenu() != this && MENU_WRAPPING )
		{
			m_SoundSwitchMenu.Play();
			return LIRT_FORWARDED_PREV;
		}
		else
			return LIRT_STOP;
	}
	else // user chose somewhere in the middle
	{
		SetChoiceIndex( iNewSelection );
		m_SoundMoveRow.Play();
		return LIRT_ACCEPTED;
	}
}

void LinkedOptionsMenu::SetChoiceIndex( int iNewSelection )
{
	ASSERT( iNewSelection >= 0 );
	ASSERT( (unsigned)iNewSelection < m_Rows.size() );
	int iSavedSelection = m_iCurrentSelection;
	m_iCurrentSelection = iNewSelection;
	if (iNewSelection / ROWS_PER_PAGE != m_iCurPage) // new page
	{
		SetPage( iNewSelection / ROWS_PER_PAGE );
	}
	m_Cursor.PlayCommand( ssprintf("Move%s", iSavedSelection < m_iCurrentSelection ? "Up" : "Down") );
	m_Cursor.SetBarWidth( floor(m_Rows[iNewSelection]->GetZoomedWidth()) );
	m_Cursor.SetY(m_Rows[iNewSelection]->GetY());
}

void LinkedOptionsMenu::SetPage(int iPage)
{
	int iSavedPage = m_iCurPage;
	for( unsigned i = m_iCurPage*ROWS_PER_PAGE; i < (unsigned)(m_iCurPage+1)*ROWS_PER_PAGE && i < m_Rows.size(); i++ )
	{
		m_Rows[i]->PlayCommand("TweenOff");
	}
	m_iCurPage = iPage;
	for( unsigned i = m_iCurPage*ROWS_PER_PAGE; i < (unsigned)(m_iCurPage+1)*ROWS_PER_PAGE && i < m_Rows.size(); i++ )
	{
		m_Rows[i]->PlayCommand("TweenOn");
	}
	if (iSavedPage < m_iCurPage) // went down a page
	{
		m_sprIndicatorUp.PlayCommand("TweenOn");
		m_bIndTweenedOn[0] = true;
		if ( (unsigned)m_iCurPage+1 == m_Rows.size()/ROWS_PER_PAGE + ( (m_Rows.size()%ROWS_PER_PAGE > 0) ? 1 : 0 ) ) // on last page
		{
			m_sprIndicatorDown.PlayCommand("TweenOff");
			m_bIndTweenedOn[1] = false;
		}
		else
			m_sprIndicatorDown.PlayCommand(m_bIndTweenedOn[1] ? "PageChange" : "TweenOn");
	}
	else if (iSavedPage > m_iCurPage) // up a page
	{
		m_sprIndicatorDown.PlayCommand("TweenOn");
		m_bIndTweenedOn[1] = true;
		if ( m_iCurPage == 0 )
		{
			m_sprIndicatorUp.PlayCommand("TweenOff");
			m_bIndTweenedOn[0] = false;
		}
		else
			m_sprIndicatorUp.PlayCommand(m_bIndTweenedOn[0] ? "PageChange" : "TweenOn");
	}
}

LinkedOptionsMenu* LinkedOptionsMenu::GetNextMenu()
{
	if ( m_NextMenu && m_NextMenu->GetChoiceCount() > 0 )
		return m_NextMenu;

	LinkedOptionsMenu *pRet = NULL;

	if ( m_NextMenu )
	{
		pRet = m_NextMenu;
		while ( pRet->m_NextMenu )
		{
			if ( pRet->m_NextMenu->GetChoiceCount() > 0 )
				return pRet->m_NextMenu;

			pRet = pRet->m_NextMenu;
		}
	}
	if ( m_PrevMenu )
	{
		pRet = m_PrevMenu;
		while ( pRet->m_PrevMenu )
			pRet = pRet->m_PrevMenu;

		if ( pRet->GetChoiceCount() > 0 )
			return pRet;

		while ( pRet->m_NextMenu )
		{
			if (pRet->m_NextMenu->GetChoiceCount() > 0)
				return pRet->m_NextMenu;
			pRet = pRet->m_NextMenu;
		}
	}
	return this;
}

LinkedOptionsMenu* LinkedOptionsMenu::GetPrevMenu()
{
	if ( m_PrevMenu && m_PrevMenu->GetChoiceCount() > 0 )
		return m_PrevMenu;

	LinkedOptionsMenu *pRet = NULL;

	if ( m_PrevMenu )
	{
		pRet = m_PrevMenu;
		while ( pRet->m_PrevMenu )
		{
			if ( pRet->m_PrevMenu->GetChoiceCount() > 0 )
				return pRet->m_PrevMenu;

			pRet = pRet->m_PrevMenu;
		}
	}
	if ( m_NextMenu )
	{
		pRet = m_NextMenu;
		while ( pRet->m_NextMenu )
			pRet = pRet->m_NextMenu;

		if ( pRet->GetChoiceCount() > 0 )
			return pRet;

		while ( pRet->m_PrevMenu )
		{
			if (pRet->m_PrevMenu->GetChoiceCount() > 0)
				return pRet->m_PrevMenu;
		}
	}
	return this;
}

LinkedOptionsMenu* LinkedOptionsMenu::SwitchToNextMenu()
{
	LinkedOptionsMenu *pRet = GetNextMenu();
	Unfocus();
	pRet->Focus();
	m_SoundSwitchMenu.Play();
	return pRet;
}

LinkedOptionsMenu* LinkedOptionsMenu::SwitchToPrevMenu()
{
	LinkedOptionsMenu *pRet = GetPrevMenu();
	Unfocus();
	pRet->Focus();
	m_SoundSwitchMenu.Play();
	return pRet;
}

// TODO: cleanup
LinkedInputResponseType LinkedOptionsMenu::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI, LinkedOptionsMenu *&pFocusedMenu )
{
	LinkedInputResponseType resp;

	if ( MenuI.button == MENU_BUTTON_LEFT )
	{
		pFocusedMenu = this;
		resp = MoveRow( -1 );
		if ( resp == LIRT_FORWARDED_PREV )
		{
			Unfocus();
			LinkedOptionsMenu *pMenu = GetPrevMenu();
			pMenu->Focus();
			pMenu->SetChoiceIndex( pMenu->GetChoiceCount()-1 );
			pFocusedMenu = pMenu;
		}
		return resp;
	}
	else if ( MenuI.button == MENU_BUTTON_RIGHT )
	{
		pFocusedMenu = this;
		resp = MoveRow( 1 );
		if ( resp == LIRT_FORWARDED_NEXT )
		{
			Unfocus();
			LinkedOptionsMenu *pMenu = GetNextMenu();
			pMenu->Focus();
			pMenu->SetChoiceIndex( 0 );
			pFocusedMenu = pMenu;
		}
		return resp;
	}
	else if ( MenuI.button == MENU_BUTTON_SELECT )
	{
		Unfocus();
		LinkedOptionsMenu *pMenu = GetNextMenu();
		pMenu->Focus();
		pFocusedMenu = pMenu;
		if ( pMenu == this )
			return LIRT_STOP;
		else
		{
			m_SoundSwitchMenu.Play();
			return LIRT_FORWARDED_NEXT;
		}
	}
	return LIRT_INVALID;
}

/*
 * (c) 2009 "infamouspat"
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
