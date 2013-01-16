/* TODO: figure out why one of the pages displays a lot in a row. */

#include "global.h"
#include "ScreenBookkeeping.h"
#include "RageLog.h"
#include "Bookkeeper.h"
#include "ScreenDimensions.h"


REGISTER_SCREEN_CLASS( ScreenBookkeeping );
ScreenBookkeeping::ScreenBookkeeping( CString sClassName ) : ScreenWithMenuElements( sClassName )
{
	LOG->Trace( "ScreenBookkeeping::ScreenBookkeeping()" );
}

void ScreenBookkeeping::Init()
{
	ScreenWithMenuElements::Init();

	m_textAllTime.LoadFromFont( THEME->GetPathF(m_sName,"AllTime") );
	m_textAllTime.SetName( "AllTime" );
	SET_XY_AND_ON_COMMAND( m_textAllTime );
	this->AddChild( &m_textAllTime );

	m_textTitle.LoadFromFont( THEME->GetPathF(m_sName,"title") );
	m_textTitle.SetName( "Title" );
	SET_XY_AND_ON_COMMAND( m_textTitle );
	this->AddChild( &m_textTitle );

	for( int i=0; i<NUM_BOOKKEEPING_COLS; i++ )
	{
		m_textData[i].LoadFromFont( THEME->GetPathF(m_sName,"data") );
		m_textData[i].SetName( "Data" );
		SET_XY_AND_ON_COMMAND( m_textData[i] );
		float fX = SCALE( i, 0.f, NUM_BOOKKEEPING_COLS-1, SCREEN_LEFT+50, SCREEN_RIGHT-160 );
		m_textData[i].SetX( fX );
		this->AddChild( &m_textData[i] );
	}

	ChangeView( (View)0 );

	this->SortByDrawOrder();
}

ScreenBookkeeping::~ScreenBookkeeping()
{
	LOG->Trace( "ScreenBookkeeping::~ScreenBookkeeping()" );
}

void ScreenBookkeeping::Update( float fDelta )
{
	ChangeView( m_View );	// refresh so that counts change in real-time

	ScreenWithMenuElements::Update( fDelta );
}

void ScreenBookkeeping::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	if( type != IET_FIRST_PRESS && type != IET_SLOW_REPEAT )
		return;	// ignore

	Screen::Input( DeviceI, type, GameI, MenuI, StyleI );	// default handler
}

void ScreenBookkeeping::HandleScreenMessage( const ScreenMessage SM )
{
	switch( SM )
	{
	case SM_GoToNextScreen:
	case SM_GoToPrevScreen:
		SCREENMAN->SetNewScreen( "ScreenOptionsMenu" );
		break;
	}
}

void ScreenBookkeeping::MenuLeft( PlayerNumber pn )
{
	m_View = (View)(m_View-1);
	int iView = (int)m_View;
	CLAMP( iView, 0, NUM_VIEWS-1 );
	m_View = (View)iView;
	ChangeView( m_View );
}

void ScreenBookkeeping::MenuRight( PlayerNumber pn )
{
	m_View = (View)(m_View+1);
	int iView = (int)m_View;
	CLAMP( iView, 0, NUM_VIEWS-1 );
	m_View = (View)iView;
	ChangeView( m_View );
}

void ScreenBookkeeping::MenuStart( PlayerNumber pn )
{
	if( !IsTransitioning() )
	{
		SCREENMAN->PlayStartSound();
		StartTransitioning( SM_GoToNextScreen );		
	}
}

void ScreenBookkeeping::MenuBack( PlayerNumber pn )
{
	if(!IsTransitioning())
	{
		SCREENMAN->PlayStartSound();
		StartTransitioning( SM_GoToPrevScreen );		
	}
}

void ScreenBookkeeping::MenuCoin( PlayerNumber pn )
{
	ChangeView( m_View );

	Screen::MenuCoin( pn );
}

void ScreenBookkeeping::ChangeView( View newView )
{
	m_View = newView;


	{
		CString s;
		s += "All-time Total: ";
		s += ssprintf( "%i (%i/%i)\n",
			BOOKKEEPER->GetCoinsTotal(false) + BOOKKEEPER->GetCoinsTotal(true),
			BOOKKEEPER->GetCoinsTotal(false), BOOKKEEPER->GetCoinsTotal(true) );

		m_textAllTime.SetText( s );
	}

	switch( m_View )
	{
	case VIEW_LAST_DAYS:
	case VIEW_LAST_DAYS_SVC:
		{
			CString sTitleText = ssprintf("Coin Data of Last %d Days", NUM_LAST_DAYS);
			sTitleText += (m_View == VIEW_LAST_DAYS) ? " (machine)" : " (service)";
			m_textTitle.SetText( sTitleText );

			int coins[NUM_LAST_DAYS];
			BOOKKEEPER->GetCoinsLastDays( coins, (m_View == VIEW_LAST_DAYS_SVC) );
			int iTotalLast = 0;
			
			CString sTitle, sData;
			for( int i=0; i<NUM_LAST_DAYS; i++ )
			{
				sTitle += LastDayToDisplayString(i) + "\n";
				sData += ssprintf("%-4d", coins[i]) + "\n";
				iTotalLast += coins[i];
			}

			sTitle += "Total\n";
			sData += ssprintf("%-4i\n", iTotalLast );
			
			m_textData[0].SetText( "" );
			m_textData[1].SetHorizAlign( Actor::align_left );
			m_textData[1].SetText( sTitle );
			m_textData[2].SetText( "" );
			m_textData[3].SetHorizAlign( Actor::align_right );
			m_textData[3].SetText( sData );
		}
		break;
	case VIEW_LAST_WEEKS:
	case VIEW_LAST_WEEKS_SVC:
		{
			CString sTitleText = ssprintf("Coin Data of Last %d Weeks", NUM_LAST_WEEKS);
			sTitleText += (m_View == VIEW_LAST_WEEKS) ? " (machine)" : " (service)";
			m_textTitle.SetText( sTitleText );

			int coins[NUM_LAST_WEEKS];
			BOOKKEEPER->GetCoinsLastWeeks( coins, (m_View == VIEW_LAST_WEEKS_SVC) );

			CString sTitle, sData;
			for( int col=0; col<4; col++ )
			{
				CString sTemp;
				for( int row=0; row<52/4; row++ )
				{
					int week = row*4+col;
					sTemp += LastWeekToDisplayString(week) + ssprintf(": %d",coins[week]) + "\n";
				}

				m_textData[col].SetHorizAlign( Actor::align_left );
				m_textData[col].SetText( sTemp );
			}
		}
		break;
	case VIEW_DAY_OF_WEEK:
	case VIEW_DAY_OF_WEEK_SVC:
		{
			CString sTitleText = "Coin Data by Day of Week";
			sTitleText += (m_View == VIEW_DAY_OF_WEEK) ? " (machine)" : " (service)";
			m_textTitle.SetText( sTitleText );

			int coins[DAYS_IN_WEEK];
			BOOKKEEPER->GetCoinsByDayOfWeek( coins, (m_View == VIEW_DAY_OF_WEEK_SVC) );

			CString sTitle, sData;
			for( int i=0; i<DAYS_IN_WEEK; i++ )
			{
				sTitle += DayOfWeekToString(i) + "\n";
				sData += ssprintf("%-4d",coins[i]) + "\n";
			}
			
			m_textData[0].SetText( "" );
			m_textData[1].SetHorizAlign( Actor::align_left );
			m_textData[1].SetText( sTitle );
			m_textData[2].SetText( "" );
			m_textData[3].SetHorizAlign( Actor::align_right );
			m_textData[3].SetText( sData );
		}
		break;
	case VIEW_HOUR_OF_DAY:
	case VIEW_HOUR_OF_DAY_SVC:
		{
			CString sTitleText = "Coin Data by Hour of Day";
			sTitleText += (m_View == VIEW_HOUR_OF_DAY) ? " (machine)" : " (service)";
			m_textTitle.SetText( sTitleText );

			int coins[HOURS_IN_DAY];
			BOOKKEEPER->GetCoinsByHour( coins, (m_View == VIEW_HOUR_OF_DAY_SVC) );

			CString sTitle1, sData1;
			for( int i=0; i<HOURS_IN_DAY/2; i++ )
			{
				sTitle1 += HourInDayToDisplayString(i) + "\n";
				sData1 += ssprintf("%-4d",coins[i]) + "\n";
			}
			
			CString sTitle2, sData2;
			for( int i=(HOURS_IN_DAY/2); i<HOURS_IN_DAY; i++ )
			{
				sTitle2 += HourInDayToDisplayString(i) + "\n";
				sData2 += ssprintf("%-4d",coins[i]) + "\n";
			}
			
			m_textData[0].SetHorizAlign( Actor::align_left );
			m_textData[0].SetText( sTitle1 );
			m_textData[1].SetHorizAlign( Actor::align_right );
			m_textData[1].SetText( sData1 );
			m_textData[2].SetHorizAlign( Actor::align_left );
			m_textData[2].SetText( sTitle2 );
			m_textData[3].SetHorizAlign( Actor::align_right );
			m_textData[3].SetText( sData2 );
		}
		break;
	default:
		ASSERT(0);
	}
}

/*
 * (c) 2003-2004 Chris Danford
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
