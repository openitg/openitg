#include "global.h"
#include "OptionsList.h"
#include "GameState.h"
#include "Style.h"
#include "PlayerState.h"

#define LINE(sLineName)				THEME->GetMetric (m_sName,ssprintf("Line%s",sLineName.c_str()))
#define MAX_ITEMS_BEFORE_SPLIT			THEME->GetMetricI(m_sName,"MaxItemsBeforeSplit")
#define ITEMS_SPLIT_WIDTH			THEME->GetMetricF(m_sName,"ItemsSplitWidth")
#define DIRECT_LINES				THEME->GetMetric (m_sName,"DirectLines")
#define TOP_MENUS				THEME->GetMetric (m_sName,"TopMenus")

static const CString RESET_ROW = "ResetOptions";

void OptionListRow::Load( OptionsList *pOptions, const CString &sType )
{
	m_sType = sType;
	m_pOptions = pOptions;
	ITEMS_SPACING_Y	.Load(sType,"ItemsSpacingY");

	m_Text.resize( 1 );
	m_Text[0].LoadFromFont( THEME->GetPathF(sType, "normal") );
	m_Text[0].SetName( "Text" );
	ActorUtil::LoadAllCommands( m_Text[0], sType );

	m_Underlines.resize( 1 );
	m_Underlines[0].Load( THEME->GetPathG(sType, "underline") );
	m_Underlines[0]->SetName( "Underline" );
	ActorUtil::LoadAllCommands( *m_Underlines[0], sType );

	m_Text[0].PlayCommand( "On" );
	m_Underlines[0]->PlayCommand( "On" );
}

// GetTitleForHandler
// can always use the title
// can have special speed row titles, note skin, or any SELECT_ONE

void OptionListRow::SetFromHandler( const OptionRowHandler *cpHandler )
{
	this->FinishTweening();
	this->RemoveAllChildren();

	if( cpHandler == NULL )
		return;

	OptionRowHandler *pHandler = (OptionRowHandler *)cpHandler;

	int iNum = max( m_pOptions->m_RowDefs[pHandler]->choices.size(), m_Text.size() )+1;
	BitmapText defText = m_Text[0];
	m_Text.resize( iNum, defText );
	m_Underlines.resize( iNum, m_Underlines[0] );

	/*
		it ain't over yet

		there's something missing in AutoActor's operator=() taken from
		SM 4.0 that might've been accidentally thrown out.

		TODO: clean up AutoActor so resize() actually works

		--infamouspat
	*/
	for( unsigned i = 0; i < m_pOptions->m_RowDefs[pHandler]->choices.size(); ++i )
	{
		m_Underlines[i].Load( THEME->GetPathG(m_sType, "underline") );
		m_Underlines[i]->SetName( "Underline" );
		ActorUtil::LoadAllCommands( *m_Underlines[i], m_sType );

		// init underlines
		this->AddChild( m_Underlines[i] );

		// init text
		this->AddChild( &m_Text[i] );
	}


	SetTextFromHandler( pHandler );

	const unsigned iCnt = m_pOptions->m_RowDefs[pHandler]->choices.size();
	m_bItemsInTwoRows = (int) iCnt > MAX_ITEMS_BEFORE_SPLIT;
	const float fWidth = ITEMS_SPLIT_WIDTH;
	float fY = 0;
	for( unsigned i = 0; i < iCnt; ++i )
	{
		float fX = 0;
		if( m_bItemsInTwoRows )
		{
			if( (i % 2) == 0 )
				fX = -fWidth/2;
			else
				fX = +fWidth/2;
		}

		// set the Y position of each item in the line
		m_Text[i].SetXY( fX, fY );
		m_Underlines[i]->SetXY( fX, fY );

		if( m_bItemsInTwoRows )
		{
			m_Underlines[i]->PlayCommand( "SetTwoRows" );
		}
		else
		{
			m_Underlines[i]->PlayCommand( "SetOneRow" );
		}
		if( !m_bItemsInTwoRows || (i % 2) == 1 || i+1 == iCnt )
			fY += ITEMS_SPACING_Y;
	}

	int iExit = m_pOptions->m_RowDefs[pHandler]->choices.size();
	m_Text[iExit].SetText( "Exit" ); // XXX localize
	m_Text[iExit].SetXY( 0, fY );
	this->AddChild( &m_Text[iExit] );
}

void OptionListRow::SetTextFromHandler( const OptionRowHandler *cpHandler )
{
	ASSERT( cpHandler );

	OptionRowHandler *pHandler = (OptionRowHandler *)cpHandler;

	for( unsigned i = 0; i < m_pOptions->m_RowDefs[pHandler]->choices.size(); ++i )
	{
		// init text
		//CString sText = pHandler->GetThemedItemText( i );
		// XXX: localize somehow --infamouspat
		CString sText = m_pOptions->m_RowDefs[pHandler]->choices[i];

		CString sDest = pHandler->GetScreen( i );
		if( m_pOptions->m_setDirectRows.find(sDest) != m_pOptions->m_setDirectRows.end() && sDest.size() )
		{
			OptionRowHandler *pTarget = m_pOptions->m_Rows[sDest];
			if( m_pOptions->m_RowDefs[pTarget]->selectType == SELECT_ONE )
			{
				int iSelection = m_pOptions->GetOneSelection(sDest);
				//sText += ": " + pTarget->GetThemedItemText( iSelection );
				sText += ": " + m_pOptions->m_RowDefs[pTarget]->choices[iSelection];
			}
		}

		m_Text[i].SetText( sText );
	}
}

void OptionListRow::SetUnderlines( const vector<bool> &aSelections, const OptionRowHandler *cpHandler )
{
	OptionRowHandler *pHandler = (OptionRowHandler *)cpHandler;
	for( unsigned i = 0; i < aSelections.size(); ++i )
	{
		//AutoActor pActor = m_Underlines[i];

		bool bSelected = aSelections[i];
		CString sDest = pHandler->GetScreen( i );
		if( sDest.size() )
		{
			/* This is a submenu.  Underline the row if its options have been changed
			 * from the default. */
			OptionRowHandler *pTarget = m_pOptions->m_Rows[sDest];
			if( m_pOptions->m_RowDefs[pTarget]->selectType == SELECT_ONE )
			{
				int iSelection = m_pOptions->GetOneSelection(sDest);
				//const OptionRowHandler *pHandler = m_pOptions->m_Rows.find(sDest)->second;
				//int iDefault = pHandler->GetDefaultOption();
				// XXX: fix this plz
				int iDefault = 0;
				if( iDefault != -1 && iSelection != iDefault )
					bSelected |= true;
			}
			else if( m_pOptions->m_RowDefs[pTarget]->selectType == SELECT_MULTIPLE )
			{
				const vector<bool> &bTargetSelections = m_pOptions->m_bSelections.find(sDest)->second;
				for( unsigned i=0; i<bTargetSelections.size(); i++ )
				{
					if( bTargetSelections[i] )
						bSelected = true;
				}
			}
		}

		//pActor->PlayCommand( bSelected?"Show":"Hide" );
		m_Underlines[i]->PlayCommand( bSelected?"Show":"Hide" );
	}
}

void OptionListRow::PositionCursor( Actor *pCursor, int iSelection )
{
	float fX = m_Text[iSelection].GetDestX();
	float fY = m_Text[iSelection].GetDestY();
	if( m_bItemsInTwoRows )
		pCursor->PlayCommand( "PositionTwoRows" );
	else
		pCursor->PlayCommand( "PositionOneRow" );
	pCursor->SetXY( fX, fY );
}

OptionsList::OptionsList()
{
	m_iCurrentRow = 0;
	m_pLinked = NULL;
}

OptionsList::~OptionsList()
{
	FOREACHM( CString, OptionRowHandler *, m_Rows, hand ) {
		delete hand->second;
	}
}

void OptionsList::Load( CString sType, PlayerNumber pn )
{
	TOP_MENU.Load( sType, "TopMenu" );

	m_pn = pn;
	m_bStartIsDown = false;

	m_Cursor.Load( THEME->GetPathG(sType, "cursor") );
	m_Cursor->SetName( "Cursor" );
	ActorUtil::LoadAllCommands( *m_Cursor, sType );
	this->AddChild( m_Cursor );

	vector<CString> asDirectLines;
	split( DIRECT_LINES, ",", asDirectLines, true );
	FOREACH( CString, asDirectLines, s )
		m_setDirectRows.insert( *s );

	vector<CString> setToLoad;
	split( TOP_MENUS, ",", setToLoad );
	m_setTopMenus.insert( setToLoad.begin(), setToLoad.end() );

	m_soundLeft.Load( THEME->GetPathS("OptionsList","left") );
	m_soundRight.Load( THEME->GetPathS("OptionsList","right") );
	m_soundOpened.Load( THEME->GetPathS("OptionsList","opened") );
	m_soundClosed.Load( THEME->GetPathS("OptionsList","closed") );
	m_soundPush.Load( THEME->GetPathS("OptionsList","push") );
	m_soundPop.Load( THEME->GetPathS("OptionsList","pop") );
	m_soundStart.Load( THEME->GetPathS("OptionsList","start") );

	while( !setToLoad.empty() )
	{
		CString sLineName = *setToLoad.begin();
		setToLoad.erase( setToLoad.begin() );

		if( m_Rows.find(sLineName) != m_Rows.end() )
		{
			continue;
		}

		OptionRowDefinition *ord = new OptionRowDefinition;
		CString sRowCommands = LINE(sLineName);
		Commands cmds;
		ParseCommands( sRowCommands, cmds );

		// XXX this might not work
		OptionRowHandler *pHand = OptionRowHandlerUtil::Make( cmds.v[0], *ord );
		if( pHand == NULL )
			RageException::Throw( "Invalid OptionRowHandler '%s' in %s::Line%s", cmds.v[0].GetOriginalCommandString().c_str(), m_sName.c_str(), sLineName.c_str() );
		m_Rows[sLineName] = pHand;
		
		m_RowDefs[pHand] = ord;
		m_asLoadedRows.push_back( sLineName );
		ImportRow( sLineName );

		for( size_t i = 0; i < ord->choices.size(); ++i )
		{
			CString sScreen = pHand->GetScreen(i);
			if( !sScreen.empty() )
				setToLoad.push_back( sScreen );
		}
	}

	for( int i = 0; i < 2; ++i )
	{
		m_Row[i].SetName( "OptionsList" );
		m_Row[i].Load( this, "OptionsList" );
		ActorUtil::LoadAllCommands( m_Row[i], sType );
		this->AddChild( &m_Row[i] );
	}

	this->PlayCommand( "On" );
	this->PlayCommand( "TweenOff" );
	this->FinishTweening();
}

void OptionsList::Reset()
{
	/* Import options. */
	FOREACHM( CString, OptionRowHandler *, m_Rows, hand )
	{
		CString sLineName = hand->first;
		ImportRow( sLineName );
	}
}

void OptionsList::Open()
{	
	m_soundOpened.Play();
	this->PlayCommand( "Reset" );

	/* Push the initial menu. */
	ASSERT( m_asMenuStack.size() == 0 );
	Push( TOP_MENU );

	this->FinishTweening();
	m_Row[!m_iCurrentRow].SetFromHandler( NULL );
	this->PlayCommand( "TweenOn" );
}

void OptionsList::Close()
{
	m_bStartIsDown = false;
	m_asMenuStack.clear();
	m_soundClosed.Play();
	this->PlayCommand( "TweenOff" );
}

CString OptionsList::GetCurrentRow() const
{
	ASSERT( !m_asMenuStack.empty() ); // called while the menu was closed
	return m_asMenuStack.back();
}

const OptionRowHandler *OptionsList::GetCurrentHandler()
{
	CString sCurrentRow = GetCurrentRow();
	return m_Rows[sCurrentRow];
}

int OptionsList::GetOneSelection( CString sRow, bool bAllowFail ) const
{
	map<CString, vector<bool> >::const_iterator it = m_bSelections.find(sRow);
	ASSERT_M( it != m_bSelections.end(), sRow );
	const vector<bool> &bSelections = it->second;
	for( unsigned i=0; i<bSelections.size(); i++ )
	{
		if( bSelections[i] )
			return i;
	}

	ASSERT( bAllowFail );	// shouldn't call this if not expecting one to be selected
	return -1;
}

void OptionsList::PositionCursor()
{
	m_Row[m_iCurrentRow].PositionCursor( m_Cursor, m_iMenuStackSelection );
}

/* Toggle to the next menu.  This is used to switch quickly through option submenus,
 * to choose many options or to find the one you're looking for.  For that goal,
 * it's not helpful to switch only through the options listed in the current parent
 * menu; always toggle through the whole set.  Skip menus that only contain other
 * menus. */
void OptionsList::SwitchMenu( int iDir )
{
	/* Consider the menu as a list, where the main menu is the first item and the
	 * submenus follow.  This allows consistent navigation; moving right from the
	 * main menu walks through the menus, moving left goes back as far as the main
	 * menu.  Don't loop, so it's harder to lose track of menus. */
	CString sTopRow = m_asMenuStack.front();
	const OptionRowHandler *cpHandler = m_Rows[sTopRow];
	OptionRowHandler *pHandler = (OptionRowHandler *)cpHandler;
	int iCurrentRow = 0;
	if( m_asMenuStack.size() == 1 )
		iCurrentRow = -1;
	else
		iCurrentRow = FindScreenInHandler( m_RowDefs[pHandler], pHandler, m_asMenuStack.back() );

	iCurrentRow += iDir;
	if( iCurrentRow >= 0 )
	{
		if( iCurrentRow >= (int) m_RowDefs[pHandler]->choices.size() )
			return;
		CString sDest = pHandler->GetScreen( iCurrentRow );
		if( sDest.empty() )
			return;

		if( m_asMenuStack.size() == 1 )
			m_asMenuStack.push_back( sDest );
		else
			m_asMenuStack.back() = sDest;
	}
	else
	{
		if( m_asMenuStack.size() == 1 )
			return;

		m_asMenuStack.pop_back();
	}

	SetDefaultCurrentRow();
	SwitchToCurrentRow();
	TweenOnCurrentRow( iDir > 0 );
}

void OptionsList::MoveItem( const CString &sRowName, int iMove )
{
}

CString InputTypeToString(InputEventType type)
{
	switch (type)
	{
		case IET_FIRST_PRESS: return "IET_FIRST_PRESS";
		case IET_RELEASE: return "IET_RELEASE";
		case IET_SLOW_REPEAT: return "IET_SLOW_REPEAT";
		case IET_FAST_REPEAT: return "IET_FAST_REPEAT";
		default: return "What?";
	}
}

void OptionsList::Input( const DeviceInput& DeviceI, const InputEventType type, const GameInput &GameI, const MenuInput &MenuI, const StyleInput &StyleI )
{
	PlayerNumber pn = GAMESTATE->GetCurrentStyle()->ControllerToPlayerNumber( GameI.controller );

	CString msg;
	//if( m_Codes.InputMessage( GameI, pn, msg) )
		//this->HandleMessage( msg );

	const OptionRowHandler *cpHandler = GetCurrentHandler();
	OptionRowHandler *pHandler = (OptionRowHandler *)cpHandler;

	/* XXX: if there were a good reason to switch to InputEventPlus, this is it */
	// would Start ever be down in ITG? --infamouspat
	if( m_bStartIsDown )
	{
		if( MenuI.button == MENU_BUTTON_LEFT || MenuI.button == MENU_BUTTON_RIGHT )
		{
			if( type != IET_FIRST_PRESS )
				return;

			m_bAcceptStartRelease = false;

			const CString &sCurrentRow = m_asMenuStack.back();
			vector<bool> &bSelections = m_bSelections[sCurrentRow];
			if( m_iMenuStackSelection == (int)bSelections.size() )
				return;

			CString sDest = pHandler->GetScreen( m_iMenuStackSelection );
			if( m_setDirectRows.find(sDest) != m_setDirectRows.end() && sDest.size() )
			{
				OptionRowHandler *pTarget = m_Rows[sDest];
				vector<bool> &bTargetSelections = m_bSelections[sDest];

				if( m_RowDefs[pTarget]->selectType == SELECT_ONE )
				{
					int iSelection = GetOneSelection(sDest);
					int iDir = (MenuI.button == MENU_BUTTON_RIGHT? +1:-1);
					iSelection += iDir;
					CString sDir = (MenuI.button == MENU_BUTTON_RIGHT? "Right":"Left");
					wrap( iSelection, bTargetSelections.size() );
					SelectItem( sDest, iSelection );
					if ( iDir == 1 )
						m_soundRight.Play();
					else
						m_soundLeft.Play();

					CString msg(ssprintf("OptionsListQuickChange%sP%d",sDir.c_str(),pn+1));
					
					//msg.SetParam( "Player", pn );
					//msg.SetParam( "Direction", iDir );
					MESSAGEMAN->Broadcast( msg );
				}
			}
			return;
		}
	}

	if( MenuI.button == MENU_BUTTON_LEFT )
	{
		if( type == IET_RELEASE )
			return;

		--m_iMenuStackSelection;
		wrap( m_iMenuStackSelection, m_RowDefs[pHandler]->choices.size()+1 ); // +1 for exit row
		m_soundLeft.Play();
		//m_soundOpened.Play();
		PositionCursor();

		CString msg(ssprintf("OptionsListLeftP%d",pn+1));
		//msg.SetParam( "Player", pn );
		MESSAGEMAN->Broadcast( msg );
		return;
	}
	else if( MenuI.button == MENU_BUTTON_RIGHT )
	{
		if( type == IET_RELEASE )
			return;

		++m_iMenuStackSelection;
		wrap( m_iMenuStackSelection, m_RowDefs[pHandler]->choices.size()+1 ); // +1 for exit row
		m_soundRight.Play();
		//m_soundOpened.Play();
		PositionCursor();

		CString msg(ssprintf("OptionsListRightP%d",pn+1));
		//msg.SetParam( "Player", pn );
		MESSAGEMAN->Broadcast( msg );
		return;
	}
	else if( MenuI.button == MENU_BUTTON_START )
	{
		if( type == IET_FIRST_PRESS )
		{
			m_bStartIsDown = true;
			m_bAcceptStartRelease = true;
			return;
		}
		if( type == IET_RELEASE )
		{
			if( m_bAcceptStartRelease )
				Start();
			m_bStartIsDown = false;
		}

		return;
	}
	else if( MenuI.button == MENU_BUTTON_SELECT )
	{
		//if( type != IET_FIRST_PRESS || type != IET_RELEASE )
			//return;
		if( type == IET_RELEASE )
		{
			Pop();
			return;
		}
		return;
	}
}

void OptionsList::SwitchToCurrentRow()
{
	m_iCurrentRow = !m_iCurrentRow;

	/* Set up the new row. */
	
	m_Row[m_iCurrentRow].SetFromHandler( GetCurrentHandler() );
	m_Row[m_iCurrentRow].SetUnderlines( m_bSelections[m_asMenuStack.back()], GetCurrentHandler() );
	PositionCursor();

	//Message msg("OptionsMenuChanged");
	//msg.SetParam( "Player", m_pn );
	//msg.SetParam( "Menu", m_asMenuStack.back() );
	//MESSAGEMAN->Broadcast( msg );
}

/* After setting up a new row, tween it on. */
void OptionsList::TweenOnCurrentRow( bool bForward )
{
	OptionListRow &OldRow = m_Row[!m_iCurrentRow];
	OptionListRow &NewRow = m_Row[m_iCurrentRow];

	/* Tween out the old row. */
	if( bForward )
		OldRow.PlayCommand( "TweenOutForward" );
	else
		OldRow.PlayCommand( "TweenOutBackward" );

	/* Tween in the old row. */
	if( bForward )
		NewRow.PlayCommand( "TweenInForward" );
	else
		NewRow.PlayCommand( "TweenInBackward" );
}

void OptionsList::ImportRow( CString sRow )
{
	vector<bool> aSelections[NUM_PLAYERS];
	vector<PlayerNumber> vpns;
	vpns.push_back( m_pn );
	OptionRowHandler *pHandler = m_Rows[sRow];
	aSelections[ m_pn ].resize( m_RowDefs[pHandler]->choices.size() );
	pHandler->ImportOption( *(m_RowDefs[pHandler]), vpns, aSelections );
	m_bSelections[sRow] = aSelections[ m_pn ];

	if( m_setTopMenus.find(sRow) != m_setTopMenus.end() )
		fill( m_bSelections[sRow].begin(), m_bSelections[sRow].end(), false );
}

void OptionsList::ExportRow( CString sRow )
{
	if( m_setTopMenus.find(sRow) != m_setTopMenus.end() )
		return;

	vector<bool> aSelections[NUM_PLAYERS];
	aSelections[m_pn] = m_bSelections[sRow];

	vector<PlayerNumber> vpns;
	vpns.push_back( m_pn );

	m_Rows[sRow]->ExportOption( *(m_RowDefs[m_Rows[sRow]]), vpns, aSelections );
}

void OptionsList::SetDefaultCurrentRow()
{
	/* If all items on the row just point to other menus, default to 0. */
	m_iMenuStackSelection = 0;

	ASSERT_M( m_asMenuStack.size() > 0, "Dumbass, you don't even have any row options available." );
	const CString &sCurrentRow = m_asMenuStack.back();
	//OptionRowHandler *pHandler = m_Rows.find(sCurrentRow)->second;
	ASSERT( m_Rows.find(sCurrentRow) != m_Rows.end() );
	OptionRowHandler *pHandler = m_Rows[sCurrentRow];
	ASSERT( pHandler );
	if( m_RowDefs[pHandler]->selectType == SELECT_ONE )
	{
		/* One item is selected, so position the cursor on it. */
		m_iMenuStackSelection = GetOneSelection( sCurrentRow, true );
		if( m_iMenuStackSelection == -1 )
			m_iMenuStackSelection = 0;
	}
}

int OptionsList::FindScreenInHandler( OptionRowDefinition *pDef, const OptionRowHandler *pHandler, CString sScreen )
{
	for( size_t i = 0; i < pDef->choices.size(); ++i )
	{
		if( pHandler->GetScreen(i) == sScreen )
			return i;
	}
	return -1;
}

/*
int OptionsList::FindScreenInHandler( const OptionRowHandler *pHandler, CString sScreen )
{
	for( size_t i = 0; i < m_RowDefs[pHandler]->choices.size(); ++i )
	{
		if( pHandler->GetScreen(i) == sScreen )
			return i;
	}
	return -1;
}
*/

void OptionsList::Pop()
{
	if( m_asMenuStack.size() == 1 )
	{
		Close();
		return;
	}

	m_soundPop.Play();
	CString sLastMenu = m_asMenuStack.back();

	m_asMenuStack.pop_back();

	/* Choose the default option. */
	SetDefaultCurrentRow();

	/* If the old menu exists as a target from the new menu, switch to it. */
	const OptionRowHandler *cpHandler = GetCurrentHandler();
	OptionRowHandler *pHandler = (OptionRowHandler *)cpHandler;
	int iIndex = FindScreenInHandler( m_RowDefs[pHandler], cpHandler, sLastMenu );
	if( iIndex != -1 )
		m_iMenuStackSelection = iIndex;

	SwitchToCurrentRow();
	TweenOnCurrentRow( false );
}

void OptionsList::Push( CString sDest )
{
	m_asMenuStack.push_back( sDest );
	m_soundPush.Play();
	SetDefaultCurrentRow();
	SwitchToCurrentRow();
}

void OptionsList::SelectItem( const CString &sRowName, int iMenuItem )
{
	OptionRowHandler *pHandler = m_Rows[sRowName];
	vector<bool> &bSelections = m_bSelections[sRowName];

	if( m_RowDefs[pHandler]->selectType == SELECT_MULTIPLE )
	{
		bool bSelected = !bSelections[iMenuItem];
		bSelections[iMenuItem] = bSelected;

		//if( bSelected )
		//	m_SoundToggleOn.Play();
		//else
		//	m_SoundToggleOff.Play();
	}
	else	// data.selectType != SELECT_MULTIPLE
	{
		fill( bSelections.begin(), bSelections.end(), false ); 
		bSelections[iMenuItem] = true;
	}

	SelectionsChanged( sRowName );
	UpdateMenuFromSelections();
}

void OptionsList::SelectionsChanged( const CString &sRowName )
{
	OptionRowHandler *pHandler = m_Rows[sRowName];
	vector<bool> &bSelections = m_bSelections[sRowName];

	if( m_RowDefs[pHandler]->bOneChoiceForAllPlayers && m_pLinked != NULL )
	{
		vector<bool> &bLinkedSelections = m_pLinked->m_bSelections[sRowName];
		bLinkedSelections = bSelections;

		if( m_pLinked->IsOpened() )
			m_pLinked->UpdateMenuFromSelections();

		m_pLinked->ExportRow( sRowName );
	}

	ExportRow( sRowName );
}

void OptionsList::UpdateMenuFromSelections()
{
	const vector<bool> &bCurrentSelections = m_bSelections.find(GetCurrentRow())->second;
	m_Row[m_iCurrentRow].SetUnderlines( bCurrentSelections, GetCurrentHandler() );
	m_Row[m_iCurrentRow].SetTextFromHandler( GetCurrentHandler() );
}

bool OptionsList::Start()
{
	const OptionRowHandler *cpHandler = GetCurrentHandler();
	OptionRowHandler *pHandler = (OptionRowHandler *)cpHandler;
	const CString &sCurrentRow = m_asMenuStack.back();
	vector<bool> &bSelections = m_bSelections[sCurrentRow];
	m_soundStart.Play();
	if( m_iMenuStackSelection == (int)bSelections.size() )
	{
		Pop();

		CString msg = ssprintf("OptionsListPopP%d",m_pn+1);
		//msg.SetParam( "Player", m_pn );
		MESSAGEMAN->Broadcast( msg );

		return m_asMenuStack.empty();
	}

	{
		CString sIconText;
		GameCommand gc;
		pHandler->GetGameCommand(m_iMenuStackSelection, gc);
		//pHandler->GetIconTextAndGameCommand( m_iMenuStackSelection, sIconText, gc );
		
		const OptionRowDefinition *pDef = (const OptionRowDefinition *)m_RowDefs[pHandler];
		//pHandler->GetIconText( *(m_RowDefs[pHandler]), sIconText, m_iMenuStackSelection );
		sIconText = pHandler->GetIconText( *pDef, m_iMenuStackSelection );
		if( gc.m_sName == RESET_ROW )
		{
			//GAMESTATE->m_pPlayerState[m_pn]->ResetToDefaultPlayerOptions( ModsLevel_Preferred );
			GAMESTATE->m_pPlayerState[m_pn]->Reset();
			
			//GAMESTATE->ResetToDefaultSongOptions( ModsLevel_Preferred );
			GAMESTATE->ResetCurrentOptions();

			/* Import options. */
			FOREACHM( CString, OptionRowHandler *, m_Rows, hand )
			{
				ImportRow( hand->first );
				SelectionsChanged( hand->first );
			}

			UpdateMenuFromSelections();

			CString msg(ssprintf("OptionsListResetP%d",m_pn+1));
			//msg.SetParam( "Player", m_pn );
			MESSAGEMAN->Broadcast( msg );

			return false;
		}
	}

	CString sDest = pHandler->GetScreen( m_iMenuStackSelection );
	if( sDest.size() )
	{
		Push( sDest );
		TweenOnCurrentRow( true );

		CString msg(ssprintf("OptionsListPushP%d",m_pn+1));
		//msg.SetParam( "Player", m_pn );
		MESSAGEMAN->Broadcast( msg );

		return false;
	}

	SelectItem( GetCurrentRow(), m_iMenuStackSelection );

	/* Move to the exit row if we made a single selection. */
	if( m_RowDefs[pHandler]->selectType == SELECT_ONE )
	{
		m_iMenuStackSelection = (int)bSelections.size();
		PositionCursor();
	}

	CString msg(ssprintf("OptionsListStartP%d",m_pn+1));
	//msg.SetParam( "Player", m_pn );
	MESSAGEMAN->Broadcast( msg );

	return false;
}

/*
 * Copyright (c) 2006 Glenn Maynard
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

