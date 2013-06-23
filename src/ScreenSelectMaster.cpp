#include "global.h"
#include "ScreenSelectMaster.h"
#include "ScreenManager.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "ThemeManager.h"
#include "GameSoundManager.h"
#include "GameState.h"
#include "AnnouncerManager.h"
#include "GameCommand.h"
#include "ActorUtil.h"
#include "RageLog.h"
#include <set>
#include "Foreach.h"
#include "RageSoundManager.h"

AutoScreenMessage( SM_PlayPostSwitchPage )

CString CURSOR_OFFSET_X_FROM_ICON_NAME( size_t p, size_t part ) { return ssprintf("CursorPart%dP%dOffsetXFromIcon",int(part+1),int(p+1)); }
CString CURSOR_OFFSET_Y_FROM_ICON_NAME( size_t p, size_t part ) { return ssprintf("CursorPart%dP%dOffsetYFromIcon",int(part+1),int(p+1)); }
/* e.g. "OptionOrderLeft=0:1,1:2,2:3,3:4" */
CString OPTION_ORDER_NAME( size_t dir )							{ return "OptionOrder"+MenuDirToString((MenuDir)dir); }

REGISTER_SCREEN_CLASS( ScreenSelectMaster );
ScreenSelectMaster::ScreenSelectMaster( CString sClassName ) : ScreenSelect( sClassName ),
	NUM_ICON_PARTS(m_sName,"NumIconParts"),
	NUM_PREVIEW_PARTS(m_sName,"NumPreviewParts"),
	NUM_CURSOR_PARTS(m_sName,"NumCursorParts"),
	SHARED_PREVIEW_AND_CURSOR(m_sName,"SharedPreviewAndCursor"),
	NUM_CHOICES_ON_PAGE_1(m_sName,"NumChoicesOnPage1"),
	CURSOR_OFFSET_X_FROM_ICON(m_sName,CURSOR_OFFSET_X_FROM_ICON_NAME,NUM_PLAYERS,NUM_CURSOR_PARTS),
	CURSOR_OFFSET_Y_FROM_ICON(m_sName,CURSOR_OFFSET_Y_FROM_ICON_NAME,NUM_PLAYERS,NUM_CURSOR_PARTS),
	OVERRIDE_LOCK_INPUT_SECONDS(m_sName,"OverrideLockInputSeconds"),
	LOCK_INPUT_SECONDS(m_sName,"LockInputSeconds"),
	PRE_SWITCH_PAGE_SECONDS(m_sName,"PreSwitchPageSeconds"),
	POST_SWITCH_PAGE_SECONDS(m_sName,"PostSwitchPageSeconds"),
	OVERRIDE_SLEEP_AFTER_TWEEN_OFF_SECONDS(m_sName,"OverrideSleepAfterTweenOffSeconds"),
	SLEEP_AFTER_TWEEN_OFF_SECONDS(m_sName,"SleepAfterTweenOffSeconds"),
	OPTION_ORDER(m_sName,OPTION_ORDER_NAME,NUM_MENU_DIRS),
	WRAP_CURSOR(m_sName,"WrapCursor"),
	WRAP_SCROLLER(m_sName,"WrapScroller"),
	SCROLLER_FAST_CATCHUP(m_sName,"ScrollerFastCatchup"),
	ALLOW_REPEATING_INPUT(m_sName,"AllowRepeatingInput"),
	SHOW_SCROLLER(m_sName,"ShowScroller"),
	SCROLLER_SECONDS_PER_ITEM(m_sName,"ScrollerSecondsPerItem"),
	SCROLLER_NUM_ITEMS_TO_DRAW(m_sName,"ScrollerNumItemsToDraw"),
	SCROLLER_TRANSFORM(m_sName,"ScrollerTransform"),
	DEFAULT_CHOICE(m_sName,"DefaultChoice")
{
}

void ScreenSelectMaster::Init()
{
	ScreenSelect::Init();

	// TODO: Move default choice to ScreenSelect
	int iDefaultChoice = -1;
	for( unsigned c=0; c<m_aGameCommands.size(); c++ )
	{
		const GameCommand& mc = m_aGameCommands[c];
		if( mc.m_sName == (CString) DEFAULT_CHOICE )
		{
			iDefaultChoice = c;
			break;
		}
	}


	FOREACH_PlayerNumber( p )
	{
		m_iChoice[p] = (iDefaultChoice!=-1) ? iDefaultChoice : 0;
		m_bChosen[p] = false;
	}

	// init cursor
	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
		{
			m_sprCursor[i][0].Load( THEME->GetPathG(m_sName,ssprintf("Cursor Part%d",i+1)) );
			m_sprCursor[i][0]->SetName( ssprintf("CursorPart%d",i+1) );
			this->AddChild( m_sprCursor[i][0] );
		}
	}
	else
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
		{
			FOREACH_HumanPlayer( p )
			{
				m_sprCursor[i][p].Load( THEME->GetPathG(m_sName,ssprintf("Cursor Part%d P%d",i+1,p+1)) );
				m_sprCursor[i][p]->SetName( ssprintf("CursorPart%dP%d",i+1,p+1) );
				this->AddChild( m_sprCursor[i][p] );
			}
		}
	}

	// init scroll
	if( SHOW_SCROLLER )
	{
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			m_Scroller[0].Load3( 
				SCROLLER_SECONDS_PER_ITEM, 
				SCROLLER_NUM_ITEMS_TO_DRAW,
				SCROLLER_FAST_CATCHUP,
				SCROLLER_TRANSFORM,
				false );
			m_Scroller[0].SetName( "Scroller" );
			this->AddChild( &m_Scroller[0] );

			for( unsigned c=0; c<m_aGameCommands.size(); c++ )
			{
				const GameCommand& mc = m_aGameCommands[c];

				m_sprScroll[c][0].Load( THEME->GetPathG(m_sName,ssprintf("Scroll Choice%s",mc.m_sName.c_str())) );
				m_sprScroll[c][0]->SetName( ssprintf("Scroll") );
				m_Scroller[0].AddChild( m_sprScroll[c][0] );
			}
		}
		else
		{
			FOREACH_HumanPlayer( p )
			{
				m_Scroller[p].Load3(
					SCROLLER_SECONDS_PER_ITEM, 
					SCROLLER_NUM_ITEMS_TO_DRAW,
					SCROLLER_FAST_CATCHUP,
					SCROLLER_TRANSFORM,
					false );
				m_Scroller[p].SetName( ssprintf("ScrollerP%d",p+1) );
				this->AddChild( &m_Scroller[p] );
				
				for( unsigned c=0; c<m_aGameCommands.size(); c++ )
				{
					const GameCommand& mc = m_aGameCommands[c];

					m_sprScroll[c][p].Load( THEME->GetPathG(m_sName,ssprintf("Scroll Choice%s P%d",mc.m_sName.c_str(),p+1)) );
					m_sprScroll[c][p]->SetName( ssprintf("ScrollP%d",p+1) );
					m_Scroller[p].AddChild( m_sprScroll[c][p] );
				}
			}
		}
	}

	for( unsigned c=0; c<m_aGameCommands.size(); c++ )
	{
		const GameCommand& mc = m_aGameCommands[c];

		// init icon
		for( int i=0; i<NUM_ICON_PARTS; i++ )
		{
			m_sprIcon[i][c].Load( THEME->GetPathG(m_sName,ssprintf("Icon Part%d Choice%s",i+1,mc.m_sName.c_str())) );
			m_sprIcon[i][c]->SetName( ssprintf("IconPart%dChoice%s",i+1,mc.m_sName.c_str()) );
			this->AddChild( m_sprIcon[i][c] );
		}

		// init preview 
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				m_sprPreview[i][c][0].Load( THEME->GetPathG(m_sName,ssprintf("Preview Part%d Choice%s",i+1,mc.m_sName.c_str())) );
				m_sprPreview[i][c][0]->SetName( ssprintf("PreviewPart%d",i+1) );
				this->AddChild( m_sprPreview[i][c][0] );
			}
		}
		else
		{
			FOREACH_HumanPlayer( p )
			{
				for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
				{
					m_sprPreview[i][c][p].Load( THEME->GetPathG(m_sName,ssprintf("Preview Part%d Choice%s P%d",i+1,mc.m_sName.c_str(),p+1)) );
					m_sprPreview[i][c][p]->SetName( ssprintf("PreviewPart%dP%d",i+1,p+1) );
					this->AddChild( m_sprPreview[i][c][p] );
				}
			}
		}
	}


	for( int page=0; page<NUM_PAGES; page++ )
	{
		m_sprMore[page].Load( THEME->GetPathG(m_sName, ssprintf("more page%d",page+1)) );
		m_sprMore[page]->SetName( ssprintf("MorePage%d",page+1) );
		this->AddChild( m_sprMore[page] );

		m_sprExplanation[page].Load( THEME->GetPathG(m_sName, ssprintf("explanation page%d",page+1)) );
		m_sprExplanation[page]->SetName( ssprintf("ExplanationPage%d",page+1) );
		this->AddChild( m_sprExplanation[page] );
	}


	FOREACH_PlayerNumber( p )
	{
		CLAMP( m_iChoice[p], 0, (int)m_aGameCommands.size()-1 );
		m_bChosen[p] = false;
	}
	
	m_soundChange.Load( THEME->GetPathS(m_sName,"change"), true );
	m_soundDifficult.Load( ANNOUNCER->GetPathTo("select difficulty challenge") );
	m_soundStart.Load( THEME->GetPathS(m_sName,"start") );

	// init m_Next order info
	FOREACH_MenuDir( dir )
	{
		/* Reasonable defaults: */
		for( unsigned c = 0; c < m_aGameCommands.size(); ++c )
		{
			int add;
			switch( dir )
			{
			case MENU_DIR_UP:
			case MENU_DIR_LEFT:	add = -1; break;
			default:			add = +1; break;
			}

			m_Next[dir][c] = c + add;
			/* Always wrap around MENU_DIR_AUTO. */
			if( dir == MENU_DIR_AUTO || (bool)WRAP_CURSOR )
				wrap( m_Next[dir][c], m_aGameCommands.size() );
			else
				m_Next[dir][c] = clamp( m_Next[dir][c], 0, (int)m_aGameCommands.size()-1 );
		}

		const CString order = OPTION_ORDER.GetValue( dir );
		vector<CString> parts;
		split( order, ",", parts, true );

		if( parts.size() == 0 )
			continue;

		for( unsigned part = 0; part < parts.size(); ++part )
		{
			unsigned from, to;
			if( sscanf( parts[part], "%u:%u", &from, &to ) != 2 )
			{
				LOG->Warn( "%s::OptionOrder%s parse error", m_sName.c_str(), MenuDirToString(dir).c_str() );
				continue;
			}

			--from;
			--to;

			if( from >= m_aGameCommands.size() ||
				to >= m_aGameCommands.size() )
			{
				LOG->Warn( "%s::OptionOrder%s out of range", m_sName.c_str(), MenuDirToString(dir).c_str() );
				continue;
			}

			m_Next[dir][from] = to;
		}
	}

	this->UpdateSelectableChoices();

	TweenOnScreen();

	m_fLockInputSecs = (bool)OVERRIDE_LOCK_INPUT_SECONDS ? LOCK_INPUT_SECONDS : this->GetTweenTimeLeft();
	if( m_fLockInputSecs == 0 )
		m_fLockInputSecs = 0.0001f;	// always lock for a tiny amount of time so that we throw away any queued inputs during the load.

}

void ScreenSelectMaster::Update( float fDelta )
{
	ScreenSelect::Update( fDelta );
	m_fLockInputSecs = max( 0, m_fLockInputSecs-fDelta );
}

void ScreenSelectMaster::HandleScreenMessage( const ScreenMessage SM )
{
	ScreenSelect::HandleScreenMessage( SM );

	if( SM == SM_PlayPostSwitchPage )
	{
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			{
				m_sprCursor[i][0]->SetXY( GetCursorX((PlayerNumber)0,i), GetCursorY((PlayerNumber)0,i) );
				COMMAND( m_sprCursor[i][0], "PostSwitchPage" );
			}
		}
		else
		{
			for( int i=0; i<NUM_CURSOR_PARTS; i++ )
				FOREACH_HumanPlayer( p )
				{
					m_sprCursor[i][p]->SetXY( GetCursorX(p,i), GetCursorY(p,i) );
					COMMAND( m_sprCursor[i][p], "PostSwitchPage" );
				}
		}

		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
				COMMAND( m_sprPreview[i][m_iChoice[0]][0], "PostSwitchPage" );
		}
		else
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
				FOREACH_HumanPlayer( p )
					COMMAND( m_sprPreview[i][m_iChoice[p]][p], "PostSwitchPage" );
		}
		m_fLockInputSecs = POST_SWITCH_PAGE_SECONDS;
	}
	else if( SM == SM_BeginFadingOut )
	{
		TweenOffScreen();
		/*
			* We start our own tween-out (TweenOffScreen), wait some amount of time, then
			* start the base tween (ScreenWithMenuElements, called from SM_AllDoneChoosing);
			* we move on when that finishes.  This is a pain to tweak, especially now
			* that elements essentially owned by the derived class are starting to tween
			* in the ScreenWithMenuElements tween (underlay, overlay); we have to tweak the
			* duration of the "out" transition to determine how long to wait after fSecs
			* before moving on.
			*
			* Send a command to all children, so we can run overlay and underlay tweens at the
			* same time as the elements controlled by TweenOffScreen.  Run this here, so
			* it affects the result of GetTweenTimeLeft().
			*/
		this->PlayCommand( "TweenOff" );

		float fSecs = 0;
		/* This can be used to allow overlap between the main tween-off and the MenuElements
			* tweenoff. */
		if( OVERRIDE_SLEEP_AFTER_TWEEN_OFF_SECONDS )
			fSecs = SLEEP_AFTER_TWEEN_OFF_SECONDS;
		else
			fSecs = GetTweenTimeLeft();
		fSecs = max( fSecs, 0 );

		SCREENMAN->PostMessageToTopScreen( SM_AllDoneChoosing, fSecs );	// nofify parent that we're finished
		StopTimer();
	}
}

int ScreenSelectMaster::GetSelectionIndex( PlayerNumber pn )
{
	return m_iChoice[pn];
}

void ScreenSelectMaster::UpdateSelectableChoices()
{
	for( unsigned c=0; c<m_aGameCommands.size(); c++ )
	{
		for( int i=0; i<NUM_ICON_PARTS; i++ )
			COMMAND( m_sprIcon[i][c], m_aGameCommands[c].IsPlayable()? "Enabled":"Disabled" );
		
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			if( m_sprScroll[c][0].IsLoaded() )
				m_sprScroll[c][0]->PlayCommand( m_aGameCommands[c].IsPlayable()? "Enabled":"Disabled" );
		}
		else
		{
			FOREACH_HumanPlayer( p )
				if( m_sprScroll[c][p].IsLoaded() )
					m_sprScroll[c][p]->PlayCommand( m_aGameCommands[c].IsPlayable()? "Enabled":"Disabled" );
		}
	}

	/*
	 * If no options are playable at all, just wait.  Some external
	 * stimulus may make options available (such as coin insertion).
	 *
	 * If any options are playable, make sure one is selected.
	 */
	FOREACH_HumanPlayer( p )
		if( !m_aGameCommands[m_iChoice[p]].IsPlayable() )
			Move( p, MENU_DIR_AUTO );
}

bool ScreenSelectMaster::AnyOptionsArePlayable() const
{
	for( unsigned i = 0; i < m_aGameCommands.size(); ++i )
		if( m_aGameCommands[i].IsPlayable() )
			return true;

	return false;
}

bool ScreenSelectMaster::Move( PlayerNumber pn, MenuDir dir )
{
	if( !AnyOptionsArePlayable() )
		return false;

	int iSwitchToIndex = m_iChoice[pn];
	set<int> seen;
try_again:
	iSwitchToIndex = m_Next[dir][iSwitchToIndex];
	if( iSwitchToIndex == -1 )
		return false; // can't go that way
	if( seen.find(iSwitchToIndex) != seen.end() )
		return false; // went full circle and none found
	seen.insert( iSwitchToIndex );

	if( !m_aGameCommands[iSwitchToIndex].IsPlayable() )
		goto try_again;

	return ChangeSelection( pn, dir, iSwitchToIndex );
}

void ScreenSelectMaster::MenuLeft( PlayerNumber pn, const InputEventType type )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( type == IET_RELEASE )
		return;
	if( !ALLOW_REPEATING_INPUT && type != IET_FIRST_PRESS )
		return;
	if( Move(pn, MENU_DIR_LEFT) )
	{
		m_soundChange.Play();
		MESSAGEMAN->Broadcast( (Message)(MESSAGE_MENU_LEFT_P1+pn) );
	}
}

void ScreenSelectMaster::MenuRight( PlayerNumber pn, const InputEventType type )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( type == IET_RELEASE )
		return;
	if( !ALLOW_REPEATING_INPUT && type != IET_FIRST_PRESS )
		return;
	if( Move(pn, MENU_DIR_RIGHT) )
	{
		m_soundChange.Play();
		MESSAGEMAN->Broadcast( (Message)(MESSAGE_MENU_RIGHT_P1+pn) );
	}
}

void ScreenSelectMaster::MenuUp( PlayerNumber pn, const InputEventType type )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( type == IET_RELEASE )
		return;
	if( !ALLOW_REPEATING_INPUT && type != IET_FIRST_PRESS )
		return;
	if( Move(pn, MENU_DIR_UP) )
	{
		m_soundChange.Play();
		MESSAGEMAN->Broadcast( (Message)(MESSAGE_MENU_UP_P1+pn) );
	}
}

void ScreenSelectMaster::MenuDown( PlayerNumber pn, const InputEventType type )
{
	if( m_fLockInputSecs > 0 || m_bChosen[pn] )
		return;
	if( type == IET_RELEASE )
		return;
	if( !ALLOW_REPEATING_INPUT && type != IET_FIRST_PRESS )
		return;
	if( Move(pn, MENU_DIR_DOWN) )
	{
		m_soundChange.Play();
		MESSAGEMAN->Broadcast( (Message)(MESSAGE_MENU_DOWN_P1+pn) );
	}
}

bool ScreenSelectMaster::ChangePage( int iNewChoice )
{
	Page newPage = GetPage(iNewChoice);

	// If anyone has already chosen, don't allow changing of pages
	FOREACH_PlayerNumber( p )
		if( GAMESTATE->IsHumanPlayer(p) && m_bChosen[p] )
			return false;

	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			COMMAND( m_sprCursor[i][0], "PreSwitchPage" );
	}
	else
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			FOREACH_PlayerNumber( p )
				if( GAMESTATE->IsHumanPlayer(p) )
					COMMAND( m_sprCursor[i][p], "PreSwitchPage" );
	}

	const CString sIconAndExplanationCommand = ssprintf( "SwitchToPage%d", newPage+1 );

	for( unsigned c=0; c<m_aGameCommands.size(); c++ )
		for( int i=0; i<NUM_ICON_PARTS; i++ )
			COMMAND( m_sprIcon[i][c], sIconAndExplanationCommand );

	if( SHARED_PREVIEW_AND_CURSOR )
	{
		int iChoice = m_iChoice[GAMESTATE->m_MasterPlayerNumber];
		for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			COMMAND( m_sprPreview[i][iChoice][0], "PreSwitchPage" );
	}
	else
	{
		for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			FOREACH_HumanPlayer( p )
			{
				int iChoice = m_iChoice[p];
				COMMAND( m_sprPreview[i][iChoice][p], "PreSwitchPage" );
			}
	}

	for( int page=0; page<NUM_PAGES; page++ )
	{
		COMMAND( m_sprExplanation[page], sIconAndExplanationCommand );
		COMMAND( m_sprMore[page], sIconAndExplanationCommand );
	}



	if( newPage == PAGE_2 )
	{
		// XXX: only play this once (I thought we already did that?)
		// DDR plays it on every change to page 2.  -Chris
		/* That sounds ugly if you go back and forth quickly. -g */
		// DDR locks input while it's scrolling.  Should we do the same? -Chris
		m_soundDifficult.Stop();
		m_soundDifficult.PlayRandom();
	}

	// change both players
	FOREACH_PlayerNumber( p )
		m_iChoice[p] = iNewChoice;

	m_fLockInputSecs = PRE_SWITCH_PAGE_SECONDS;
	this->PostScreenMessage( SM_PlayPostSwitchPage, PRE_SWITCH_PAGE_SECONDS );
	return true;
}

bool ScreenSelectMaster::ChangeSelection( PlayerNumber pn, MenuDir dir, int iNewChoice )
{
	if( iNewChoice == m_iChoice[pn] )
		return false; // already there

	if( GetPage(m_iChoice[pn]) != GetPage(iNewChoice) )
		return ChangePage( iNewChoice );

	FOREACH_PlayerNumber( p )
	{
		const int iOldChoice = m_iChoice[p];

		/* Set the new m_iChoice even for disabled players, since a player might
		 * join on a SHARED_PREVIEW_AND_CURSOR after the cursor has been moved. */
		m_iChoice[p] = iNewChoice;

		if( p!=pn )
			continue;	// skip

		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				COMMAND( m_sprPreview[i][iOldChoice][0], "LoseFocus" );
				COMMAND( m_sprPreview[i][iNewChoice][0], "GainFocus" );
			}
		}
		else
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				COMMAND( m_sprPreview[i][iOldChoice][p], "LoseFocus" );
				COMMAND( m_sprPreview[i][iNewChoice][p], "GainFocus" );
			}
		}

		{
			/* XXX: If !SharedPreviewAndCursor, this is incorrect.  (Nothing uses
			 * both icon focus and !SharedPreviewAndCursor right now.) */
			for( int i=0; i<NUM_ICON_PARTS; i++ )
			{
				COMMAND( m_sprIcon[i][iOldChoice], "LoseFocus" );
				COMMAND( m_sprIcon[i][iNewChoice], "GainFocus" );
			}
		}

		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			{
				COMMAND( m_sprCursor[i][0], "Change" );
				m_sprCursor[i][0]->SetXY( GetCursorX((PlayerNumber)0,i), GetCursorY((PlayerNumber)0,i) );
			}
		}
		else
		{
			for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			{
				COMMAND( m_sprCursor[i][p], "Change" );
				m_sprCursor[i][p]->SetXY( GetCursorX(p,i), GetCursorY(p,i) );
			}
		}

		if( SHOW_SCROLLER )
		{
			if( WRAP_SCROLLER )
			{
				// HACK: We can't tell from the option orders whether or not we wrapped.
				// For now, assume that the order is increasing left to right.
				int iPressedDir = (dir == MENU_DIR_LEFT) ? -1 : +1;
				int iActualDir = (iOldChoice < iNewChoice) ? +1 : -1;

				if( iPressedDir != iActualDir )	// wrapped
				{
					ActorScroller &scroller = SHARED_PREVIEW_AND_CURSOR ? m_Scroller[0] : m_Scroller[p];
					float fItem = scroller.GetCurrentItem();
					int iNumChoices = m_aGameCommands.size();
					fItem += iActualDir * iNumChoices;
					scroller.SetCurrentAndDestinationItem( fItem );
				}
			}

			if( SHARED_PREVIEW_AND_CURSOR )
				m_Scroller[0].SetDestinationItem( (float)iNewChoice );
			else
				m_Scroller[p].SetDestinationItem( (float)iNewChoice );

			if( SHARED_PREVIEW_AND_CURSOR )
				for( unsigned c=0; c<m_aGameCommands.size(); c++ )
					COMMAND( *m_sprScroll[c][0], int(c) == m_iChoice[0]? "GainFocus":"LoseFocus" );
			else
				for( unsigned c=0; c<m_aGameCommands.size(); c++ )
					COMMAND( *m_sprScroll[c][p], int(c) == m_iChoice[p]? "GainFocus":"LoseFocus" );
		}
	}


	return true;
}

ScreenSelectMaster::Page ScreenSelectMaster::GetPage( int iChoiceIndex ) const
{
	return iChoiceIndex < NUM_CHOICES_ON_PAGE_1? PAGE_1:PAGE_2;
}

ScreenSelectMaster::Page ScreenSelectMaster::GetCurrentPage() const
{
	// Both players are guaranteed to be on the same page.
	return GetPage( m_iChoice[GAMESTATE->m_MasterPlayerNumber] );
}


float ScreenSelectMaster::DoMenuStart( PlayerNumber pn )
{
	if( m_bChosen[pn] )
		return 0;

	bool bAnyChosen = false;
	FOREACH_PlayerNumber( p )
		bAnyChosen |= m_bChosen[p];

	m_bChosen[pn] = true;

	MESSAGEMAN->Broadcast( (Message)(MESSAGE_MADE_CHOICE_P1+pn) );

	bool bIsFirstToChoose = bAnyChosen;

	float fSecs = 0;

	if( bIsFirstToChoose )
	{
		for( int page=0; page<NUM_PAGES; page++ )
		{
			OFF_COMMAND( m_sprMore[page] );
			fSecs = max( fSecs, m_sprMore[page]->GetTweenTimeLeft() );
		}

		int iIndex = SHARED_PREVIEW_AND_CURSOR ? 0 : pn;

		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
		{
			COMMAND( m_sprCursor[i][pn], "Choose");
			fSecs = max( fSecs, m_sprCursor[i][iIndex]->GetTweenTimeLeft() );
		}
	}

	return fSecs;
}

void ScreenSelectMaster::MenuStart( PlayerNumber pn )
{
	// If the player isn't already joined, try to join them.
	// Allow a player to join even if input is locked or someone has already already chosen.
	MenuInput MenuI;
	MenuI.player = pn;
	MenuI.button = MENU_BUTTON_START;
	Screen::JoinInput( MenuI );

	if( m_fLockInputSecs > 0 )
		return;
	if( m_bChosen[pn] )
		return;

	GameCommand &mc = m_aGameCommands[m_iChoice[pn]];

	/* If no options are playable, then we're just waiting for one to become available.
	 * If any options are playable, then the selection must be playable. */
	if( !AnyOptionsArePlayable() )
		return;
		
	SOUND->PlayOnceFromDir( ANNOUNCER->GetPathTo(ssprintf("%s comment %s",m_sName.c_str(), mc.m_sName.c_str())) );
	
	/* Play a copy of the sound, so it'll finish playing even if we leave the screen immediately. */
	if( mc.m_sSoundPath.empty() )
		SOUNDMAN->PlayCopyOfSound( m_soundStart );

	if( mc.m_sScreen.empty() )
	{
		mc.ApplyToAllPlayers();
		return;
	}

	float fSecs = 0;
	bool bAllDone = true;
	if( (bool)SHARED_PREVIEW_AND_CURSOR || GetCurrentPage() == PAGE_2 )
	{
		/* Only one player has to pick.  Choose this for all the other players, too. */
		FOREACH_PlayerNumber( p )
		{
			ASSERT( !m_bChosen[p] );
			fSecs = max( fSecs, DoMenuStart(p) );	// no harm in calling this for an unjoined player
		}
	}
	else
	{
		fSecs = max( fSecs, DoMenuStart(pn) );
		// check to see if everyone has chosen
		FOREACH_HumanPlayer( p )
			bAllDone &= m_bChosen[p];
	}

	if( bAllDone )
		this->PostScreenMessage( SM_BeginFadingOut, fSecs );// tell our owner it's time to move on
}

/*
 * We want all items to always run OnCommand and either GainFocus or LoseFocus on
 * tween-in.  If we only run OnCommand, then it has to contain a copy of either
 * GainFocus or LoseFocus, which implies that the default setting is hard-coded in
 * the theme.  Always run both.
 *
 * However, the actual tween-in is OnCommand; we don't always want to actually run
 * through the Gain/LoseFocus tweens during initial tween-in.  So, we run the focus
 * command first, do a FinishTweening to pop it in place, and then run OnCommand.
 * This means that the focus command should be position neutral; eg. only use "addx",
 * not "x".
 */
void ScreenSelectMaster::TweenOnScreen() 
{
	for( unsigned c=0; c<m_aGameCommands.size(); c++ )
	{
		for( int i=0; i<NUM_ICON_PARTS; i++ )
		{
			COMMAND( m_sprIcon[i][c], (int(c) == m_iChoice[0])? "GainFocus":"LoseFocus" );
			m_sprIcon[i][c]->FinishTweening();
			SET_XY_AND_ON_COMMAND( m_sprIcon[i][c] );
		}

		if( SHARED_PREVIEW_AND_CURSOR )
		{
			int p=0;
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				COMMAND( m_sprPreview[i][c][p], (int(c) == m_iChoice[p])? "GainFocus":"LoseFocus" );
				m_sprPreview[i][c][p]->FinishTweening();
				SET_XY_AND_ON_COMMAND( m_sprPreview[i][c][p] );
			}
		}
		else
		{
			FOREACH_HumanPlayer( p )
				for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
				{
					COMMAND( m_sprPreview[i][c][p], int(c) == m_iChoice[p]? "GainFocus":"LoseFocus" );
					m_sprPreview[i][c][p]->FinishTweening();
					SET_XY_AND_ON_COMMAND( m_sprPreview[i][c][p] );
				}
		}
	}

	// Need to SetXY of Cursor after Icons since it depends on the Icons' positions.
	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
		{
			m_sprCursor[i][0]->SetXY( GetCursorX((PlayerNumber)0,i), GetCursorY((PlayerNumber)0,i) );
			ON_COMMAND( m_sprCursor[i][0] );
		}
	}
	else
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			FOREACH_HumanPlayer( p )
			{
				m_sprCursor[i][p]->SetXY( GetCursorX(p,i), GetCursorY(p,i) );
				ON_COMMAND( m_sprCursor[i][p] );
			}
	}

	if( SHOW_SCROLLER )
	{
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			m_Scroller[0].SetCurrentAndDestinationItem( (float)m_iChoice[0] );
			SET_XY_AND_ON_COMMAND( m_Scroller[0] );
			for( unsigned c=0; c<m_aGameCommands.size(); c++ )
				COMMAND( *m_sprScroll[c][0], int(c) == m_iChoice[0]? "GainFocus":"LoseFocus" );
		}
		else
			FOREACH_HumanPlayer( p )
			{
				m_Scroller[p].SetCurrentAndDestinationItem( (float)m_iChoice[p] );
				SET_XY_AND_ON_COMMAND( m_Scroller[p] );
				for( unsigned c=0; c<m_aGameCommands.size(); c++ )
					COMMAND( *m_sprScroll[c][p], int(c) == m_iChoice[p]? "GainFocus":"LoseFocus" );
			}
	}

	//We have to move page two's explanation and more off the screen
	//so it doesn't just sit there on page one.  (Thanks Zhek)
	
	for (int page=0;page<NUM_PAGES;page++)
	{
		m_sprMore[page]->SetXY(999,999);
		m_sprExplanation[page]->SetXY(999,999);
	}

	SET_XY_AND_ON_COMMAND( m_sprExplanation[GetCurrentPage()] );
	SET_XY_AND_ON_COMMAND( m_sprMore[GetCurrentPage()] );

	this->SortByDrawOrder();
}

void ScreenSelectMaster::TweenOffScreen()
{
	if( SHARED_PREVIEW_AND_CURSOR )
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			OFF_COMMAND( m_sprCursor[i][0] );
	}
	else
	{
		for( int i=0; i<NUM_CURSOR_PARTS; i++ )
			FOREACH_HumanPlayer( p )
				OFF_COMMAND( m_sprCursor[i][p] );
	}

	for( unsigned c=0; c<m_aGameCommands.size(); c++ )
	{
		if( GetPage(c) != GetCurrentPage() )
			continue;	// skip

		bool SelectedByEitherPlayer = false;
		if( SHARED_PREVIEW_AND_CURSOR )
		{
			if( m_iChoice[0] == (int)c )
				SelectedByEitherPlayer = true;
		}
		else
			FOREACH_HumanPlayer( p )
				if( m_iChoice[p] == (int)c )
					SelectedByEitherPlayer = true;

		for( int i=0; i<NUM_ICON_PARTS; i++ )
		{
			OFF_COMMAND( m_sprIcon[i][c] );
			COMMAND( m_sprIcon[i][c], SelectedByEitherPlayer? "OffFocused":"OffUnfocused" );
		}


		if( SHARED_PREVIEW_AND_CURSOR )
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
			{
				OFF_COMMAND( m_sprPreview[i][c][0] );
				COMMAND( m_sprPreview[i][c][0], SelectedByEitherPlayer? "OffFocused":"OffUnfocused" );
			}
		}
		else
		{
			for( int i=0; i<NUM_PREVIEW_PARTS; i++ )
				FOREACH_HumanPlayer( p )
				{
					OFF_COMMAND( m_sprPreview[i][c][p] );
					COMMAND( m_sprPreview[i][c][0], SelectedByEitherPlayer? "OffFocused":"OffUnfocused" );
				}
		}
	}

	if( SHOW_SCROLLER )
	{
		if( SHARED_PREVIEW_AND_CURSOR )
			OFF_COMMAND( m_Scroller[0] );
		else
			FOREACH_PlayerNumber( p )
				OFF_COMMAND( m_Scroller[p] );
	}

	OFF_COMMAND( m_sprExplanation[GetCurrentPage()] );
	OFF_COMMAND( m_sprMore[GetCurrentPage()] );
}


float ScreenSelectMaster::GetCursorX( PlayerNumber pn, int iPartIndex )
{
	return m_sprIcon[0][m_iChoice[pn]]->GetX() + CURSOR_OFFSET_X_FROM_ICON.GetValue(pn, iPartIndex);
}

float ScreenSelectMaster::GetCursorY( PlayerNumber pn, int iPartIndex )
{
	return m_sprIcon[0][m_iChoice[pn]]->GetY() + CURSOR_OFFSET_Y_FROM_ICON.GetValue(pn, iPartIndex);
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
