/* OptionRow - One line in ScreenOptions. */

#ifndef OptionRow_H
#define OptionRow_H

#include "ActorFrame.h"
#include "BitmapText.h"
#include "OptionsCursor.h"
#include "OptionIcon.h"
#include "ThemeMetric.h"

class OptionRowHandler;

enum SelectType
{
	SELECT_ONE,
	SELECT_MULTIPLE,
	SELECT_NONE,
	NUM_SELECT_TYPES,
	SELECT_INVALID
};
const CString& SelectTypeToString( SelectType pm );
SelectType StringToSelectType( const CString& s );

enum LayoutType
{
	LAYOUT_SHOW_ALL_IN_ROW,
	LAYOUT_SHOW_ONE_IN_ROW,
	NUM_LAYOUT_TYPES,
	LAYOUT_INVALID
};
const CString& LayoutTypeToString( LayoutType pm );
LayoutType StringToLayoutType( const CString& s );

struct OptionRowDefinition
{
	CString name;
	bool bOneChoiceForAllPlayers;
	SelectType selectType;
	LayoutType layoutType;
	vector<CString> choices;
	set<PlayerNumber> m_vEnabledForPlayers;	// only players in this set may change focus to this row
	bool	m_bExportOnChange;
	bool	m_bAllowThemeItems;	// if false, ignores ScreenOptions::THEME_ITEMS

	bool IsEnabledForPlayer( PlayerNumber pn ) const 
	{
		return m_vEnabledForPlayers.find(pn) != m_vEnabledForPlayers.end(); 
	}

	OptionRowDefinition() { Init(); }
	void Init()
	{
		name = "";
		bOneChoiceForAllPlayers = false;
		selectType = SELECT_ONE;
		layoutType = LAYOUT_SHOW_ALL_IN_ROW;
		choices.clear();
		m_vEnabledForPlayers.clear();
		FOREACH_PlayerNumber( pn )
			m_vEnabledForPlayers.insert( pn );
		m_bExportOnChange = false;
		m_bAllowThemeItems = true;
	}

	OptionRowDefinition( const char *n, bool b, const char *c0=NULL, const char *c1=NULL, const char *c2=NULL, const char *c3=NULL, const char *c4=NULL, const char *c5=NULL, const char *c6=NULL, const char *c7=NULL, const char *c8=NULL, const char *c9=NULL, const char *c10=NULL, const char *c11=NULL, const char *c12=NULL, const char *c13=NULL, const char *c14=NULL, const char *c15=NULL, const char *c16=NULL, const char *c17=NULL, const char *c18=NULL, const char *c19=NULL )
	{
		Init();
		name=n;
		bOneChoiceForAllPlayers=b;
		selectType=SELECT_ONE;
		layoutType=LAYOUT_SHOW_ALL_IN_ROW;
#define PUSH( c )	if(c) choices.push_back(c);
		PUSH(c0);PUSH(c1);PUSH(c2);PUSH(c3);PUSH(c4);PUSH(c5);PUSH(c6);PUSH(c7);PUSH(c8);PUSH(c9);PUSH(c10);PUSH(c11);PUSH(c12);PUSH(c13);PUSH(c14);PUSH(c15);PUSH(c16);PUSH(c17);PUSH(c18);PUSH(c19);
#undef PUSH
	}
};

class OptionRow : public ActorFrame
{
public:
	OptionRow();
	~OptionRow();

	void Clear();
	void LoadMetrics( const CString &sType );
	void LoadNormal( const OptionRowDefinition &def, OptionRowHandler *pHand, bool bFirstItemGoesDown );
	void LoadExit();
	void LoadOptionIcon( PlayerNumber pn, const CString &sText );

	CString GetRowTitle() const;

	void ImportOptions( const vector<PlayerNumber> &vpns );
	int ExportOptions( const vector<PlayerNumber> &vpns, bool bRowHasFocus[NUM_PLAYERS] );

	void AfterImportOptions();
	void DetachHandler();

	void PositionUnderlines( PlayerNumber pn );
	void PositionIcons();
	void UpdateText();
	void SetRowFocus( bool bRowHasFocus[NUM_PLAYERS] );
	void UpdateEnabledDisabled();

	int GetOneSelection( PlayerNumber pn, bool bAllowFail=false ) const;
	int GetOneSharedSelection( bool bAllowFail=false ) const;
	void SetOneSelection( PlayerNumber pn, int iChoice );
	void SetOneSharedSelection( int iChoice );

	int GetChoiceInRowWithFocus( PlayerNumber pn ) const
	{
		if( m_RowDef.bOneChoiceForAllPlayers )
			pn = PLAYER_1;
		if( m_RowDef.choices.empty() )
			return -1;
		int iChoice = m_iChoiceInRowWithFocus[pn];
		return iChoice; 
	}
	void SetChoiceInRowWithFocus( PlayerNumber pn, int iChoice )
	{
		if( m_RowDef.bOneChoiceForAllPlayers )
			pn = PLAYER_1;
		ASSERT(iChoice >= 0 && iChoice < (int)m_RowDef.choices.size());
		m_iChoiceInRowWithFocus[pn] = iChoice;
	}
	bool GetSelected( PlayerNumber pn, int iChoice ) const
	{
		if( m_RowDef.bOneChoiceForAllPlayers )
			pn = PLAYER_1;
		return m_vbSelected[pn][iChoice];
	}
	void SetSelected( PlayerNumber pn, int iChoice, bool b )
	{
		if( m_RowDef.bOneChoiceForAllPlayers )
			pn = PLAYER_1;
		m_vbSelected[pn][iChoice] = b;
	}

	enum RowType
	{
		ROW_NORMAL,
		ROW_EXIT
	};
	const OptionRowDefinition &GetRowDef() const { return m_RowDef; }
	OptionRowDefinition &GetRowDef() { return m_RowDef; }
	RowType GetRowType() const { return m_RowType; }
	OptionRowHandler *GetHandler() { return m_pHand; }

	BitmapText &GetTextItemForRow( PlayerNumber pn, int iChoiceOnRow );
	void GetWidthXY( PlayerNumber pn, int iChoiceOnRow, int &iWidthOut, int &iXOut, int &iYOut );

	void SetRowY( float fRowY )				{ m_fY = fRowY; }
	float GetRowY()							{ return m_fY; }
	void SetRowHidden( bool bRowHidden )	{ m_bHidden = bRowHidden; }
	bool GetRowHidden()						{ return m_bHidden; }
	unsigned GetTextItemsSize() { return m_textItems.size(); }
	bool GetFirstItemGoesDown() { return m_bFirstItemGoesDown; }

	void PrepareItemText( CString &s ) const;

	void SetExitText( CString sExitText );

	void Reload();

	//
	// Messages
	//
	virtual void HandleMessage( const CString& sMessage );

protected:
	CString 				m_sType;
	OptionRowDefinition		m_RowDef;
	RowType					m_RowType;
	OptionRowHandler*		m_pHand;
	ActorFrame				m_Frame;
	vector<BitmapText *>	m_textItems;				// size depends on m_bRowIsLong and which players are joined
	vector<OptionsCursor *>	m_Underline[NUM_PLAYERS];	// size depends on m_bRowIsLong and which players are joined
	Sprite					m_sprBullet;
	BitmapText				m_textTitle;
	OptionIcon				m_OptionIcons[NUM_PLAYERS];
	bool					m_bFirstItemGoesDown;
	bool					m_bRowHasFocus[NUM_PLAYERS];

	int m_iChoiceInRowWithFocus[NUM_PLAYERS];	// this choice has input focus
	// Only one will true at a time if m_RowDef.bMultiSelect
	vector<bool> m_vbSelected[NUM_PLAYERS];	// size = m_RowDef.choices.size()

	float m_fY;
	bool m_bHidden; // currently off screen

	
	// metrics
	ThemeMetric<float>				ARROWS_X;
	ThemeMetric<float>				LABELS_X;
	ThemeMetric<apActorCommands>	LABELS_ON_COMMAND;
	ThemeMetric<float>				ITEMS_START_X;
	ThemeMetric<float>				ITEMS_END_X;
	ThemeMetric<float>				ITEMS_GAP_X;
	ThemeMetric1D<float>			ITEMS_LONG_ROW_X;
	ThemeMetric<float>				ITEMS_LONG_ROW_SHARED_X;
	ThemeMetric<apActorCommands>	ITEMS_ON_COMMAND;
	ThemeMetric1D<float>			ICONS_X;
	ThemeMetric<apActorCommands>	ICONS_ON_COMMAND;
	ThemeMetric<RageColor>			COLOR_SELECTED;
	ThemeMetric<RageColor>			COLOR_NOT_SELECTED;
	ThemeMetric<RageColor>			COLOR_DISABLED;
	ThemeMetric<bool>				CAPITALIZE_ALL_OPTION_NAMES;
	ThemeMetric<float>				TWEEN_SECONDS;
	ThemeMetric<bool>				THEME_ITEMS;
	ThemeMetric<bool>				THEME_TITLES;
	ThemeMetric<bool>				SHOW_BPM_IN_SPEED_TITLE;
};

#endif

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
