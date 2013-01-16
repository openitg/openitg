#include "global.h"
#include "OptionRowHandler.h"
#include "ScreenOptionsMasterPrefs.h"
#include "NoteSkinManager.h"
#include "RageLog.h"
#include "GameState.h"
#include "Steps.h"
#include "Style.h"
#include "song.h"
#include "SongManager.h"
#include "Character.h"
#include "PrefsManager.h"
#include "GameManager.h"
#include "CommonMetrics.h"
#include "ProfileManager.h"

#define ENTRY(s)					THEME->GetMetric ("ScreenOptionsMaster",s)
#define ENTRY_MODE(s,i)				THEME->GetMetric ("ScreenOptionsMaster",ssprintf("%s,%i",(s).c_str(),(i+1)))
#define ENTRY_DEFAULT(s)			THEME->GetMetric ("ScreenOptionsMaster",(s) + "Default")

/*Metrics entry to toggle the display of the song's meter besides the difficulty name in ScreenPlayerOptions -Wanny */
ThemeMetric<bool> HIDE_METER		("ScreenPlayerOptions","HideMeter"); 

static void SelectExactlyOne( int iSelection, vector<bool> &vbSelectedOut )
{
	ASSERT_M( iSelection >= 0  &&  iSelection < (int) vbSelectedOut.size(),
			  ssprintf("%d/%u",iSelection, unsigned(vbSelectedOut.size())) );
	for( int i=0; i<int(vbSelectedOut.size()); i++ )
		vbSelectedOut[i] = i==iSelection;
}

static int GetOneSelection( const vector<bool> &vbSelected )
{
	int iRet = -1;
	for( unsigned i=0; i<vbSelected.size(); i++ )
	{
		if( vbSelected[i] )
		{
			ASSERT( iRet == -1 );	// only one should be selected
			iRet = i;
		}
	}
	ASSERT( iRet != -1 );	// shouldn't call this if not expecting one to be selected
	return iRet;
}

class OptionRowHandlerList : public OptionRowHandler
{
public:
	vector<GameCommand> ListEntries;
	GameCommand Default;
	bool m_bUseModNameForIcon;
	vector<CString> m_vsBroadcastOnExport;

	OptionRowHandlerList() { Init(); }
	virtual void Init()
	{
		OptionRowHandler::Init();
		ListEntries.clear();
		Default.Init();
		m_bUseModNameForIcon = false;
		m_vsBroadcastOnExport.clear();
	}
	virtual void Load( OptionRowDefinition &defOut, CString sParam )
	{
		ASSERT( sParam.size() );

		if( sParam.CompareNoCase("NoteSkins")==0 )		{ FillNoteSkins( defOut, sParam );		return; }
		else if( sParam.CompareNoCase("Steps")==0 )		{ FillSteps( defOut, sParam, false );	return; }
		else if( sParam.CompareNoCase("StepsLocked")==0 )	{ FillSteps( defOut, sParam, true );	return; }
		else if( sParam.CompareNoCase("Characters")==0 )	{ FillCharacters( defOut, sParam );		return; }
		else if( sParam.CompareNoCase("Styles")==0 )		{ FillStyles( defOut, sParam );			return; }
		else if( sParam.CompareNoCase("Groups")==0 )		{ FillGroups( defOut, sParam );			return; }
		else if( sParam.CompareNoCase("Difficulties")==0 )	{ FillDifficulties( defOut, sParam );	return; }
		else if( sParam.CompareNoCase("SongsInCurrentSongGroup")==0 )	{ FillSongsInCurrentSongGroup( defOut, sParam );	return; }

		Init();
		defOut.Init();

		m_bUseModNameForIcon = true;
			
		defOut.name = sParam;

		Default.Load( -1, ParseCommands(ENTRY_DEFAULT(sParam)) );

		/* Parse the basic configuration metric. */
		Commands cmds = ParseCommands( ENTRY(sParam) );
		if( cmds.v.size() < 1 )
			RageException::Throw( "Parse error in ScreenOptionsMaster::%s", sParam.c_str() );

		defOut.bOneChoiceForAllPlayers = false;
		const int NumCols = atoi( cmds.v[0].m_vsArgs[0] );
		for( unsigned i=1; i<cmds.v.size(); i++ )
		{
			const Command &cmd = cmds.v[i];
			CString sName = cmd.GetName();

			if(		 sName == "together" )			defOut.bOneChoiceForAllPlayers = true;
			else if( sName == "selectmultiple" )	defOut.selectType = SELECT_MULTIPLE;
			else if( sName == "selectone" )			defOut.selectType = SELECT_ONE;
			else if( sName == "selectnone" )		defOut.selectType = SELECT_NONE;
			else if( sName == "showoneinrow" )		defOut.layoutType = LAYOUT_SHOW_ONE_IN_ROW;
			else if( sName == "reloadrowmessages" )
			{
				for( unsigned a=1; a<cmd.m_vsArgs.size(); a++ )
					m_vsReloadRowMessages.push_back( cmd.m_vsArgs[a] );
			}
			else if( sName == "enabledforplayers" )
			{
				defOut.m_vEnabledForPlayers.clear();
				for( unsigned a=1; a<cmd.m_vsArgs.size(); a++ )
				{
					CString sArg = cmd.m_vsArgs[a];
					PlayerNumber pn = (PlayerNumber)(atoi(sArg)-1);
					ASSERT( pn >= 0 && pn < NUM_PLAYERS );
					defOut.m_vEnabledForPlayers.insert( pn );
				}
			}
			else if( sName == "exportonchange" )	defOut.m_bExportOnChange = true;
			else if( sName == "broadcastonexport" )
			{
				for( unsigned i=1; i<cmd.m_vsArgs.size(); i++ )
					m_vsBroadcastOnExport.push_back( cmd.m_vsArgs[i] );
			}
			else		RageException::Throw( "Unkown row flag \"%s\"", sName.c_str() );
		}

		for( int col = 0; col < NumCols; ++col )
		{
			GameCommand mc;
			mc.Load( 0, ParseCommands(ENTRY_MODE(sParam, col)) );
			/* If the row has just one entry, use the name of the row as the name of the
			 * entry.  If it has more than one, each one must be specified explicitly. */
			if( mc.m_sName == "" && NumCols == 1 )
				mc.m_sName = sParam;
			if( mc.m_sName == "" )
				RageException::Throw( "List \"%s\", col %i has no name", sParam.c_str(), col );

			if( !mc.IsPlayable() )
			{
				LOG->Trace( "\"%s\" is not playable.", sParam.c_str() );
				continue;
			}

			ListEntries.push_back( mc );

			CString sName = mc.m_sName;
			CString sChoice = mc.m_sName;
			defOut.choices.push_back( sChoice );
		}

		// OpenITG hack: load player-defined speed mods
		if (sParam == "Speed")
		{
			set<CString> additionalSet;
			
			// load anything from the machine profile first
			Profile *pMProf = PROFILEMAN->GetMachineProfile();
			if (pMProf != NULL)
			{
				FOREACH_CONST(CString, pMProf->m_sPlayerAdditionalModifiers, mod)
					additionalSet.insert(*mod);
			}

			// then load anything from the players' profiles
			FOREACH_EnabledPlayer( pn )
			{
				Profile *pProf = PROFILEMAN->GetProfile(pn);
				if (pProf == NULL) continue;
				FOREACH_CONST(CString, pProf->m_sPlayerAdditionalModifiers, mod)
					additionalSet.insert(*mod);
			}
			FOREACHS_CONST( CString, additionalSet, addit_mod )
			{
				Regex mult("^[0-9]{1,2}(\\.[0-9]{1,2})?x$");
				Regex constmod("^C[0-9]{1,4}$");
				Regex mmod("^M[0-9]{1,4}$");
				CString sAdditModName;
				if (mult.Compare(*addit_mod))
				{
					float factor = 1.0f;
					sscanf(*addit_mod, "%fx", &factor);
					sAdditModName = ssprintf("x%.1f", factor);
				}
				else if (constmod.Compare(*addit_mod))
				{
					unsigned bpm = 300;
					sscanf(*addit_mod, "C%u", &bpm);
					sAdditModName = ssprintf("c%u", bpm);
				}
				else if (mmod.Compare(*addit_mod))
				{
					unsigned bpm = 600;
					sscanf(*addit_mod, "M%u", &bpm);
					sAdditModName = ssprintf("m%u", bpm);
				}
				else ASSERT(0); // how'd it get in here in the first place...

				GameCommand mc;
				mc.Load( 0, ParseCommands(CString("mod,")+*addit_mod+";name,"+sAdditModName) );
				if ( !mc.IsPlayable() )
				{
					LOG->Trace( "Additional mod \"%s\" is not playable.", addit_mod->c_str() );
					continue;
				}
				ListEntries.push_back(mc);
				defOut.choices.push_back(mc.m_sName);
			}

		}
	}
	void ImportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, vector<bool> vbSelectedOut[NUM_PLAYERS] ) const
	{
		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			int iFallbackOption = -1;
			bool bUseFallbackOption = true;
			PlayerNumber p = *pn;
			vector<bool> &vbSelOut = vbSelectedOut[p];

			for( unsigned e = 0; e < ListEntries.size(); ++e )
			{
				const GameCommand &mc = ListEntries[e];

				vbSelOut[e] = false;

				if( mc.IsZero() )
				{
					/* The entry has no effect.  This is usually a default "none of the
					 * above" entry.  It will always return true for DescribesCurrentMode().
					 * It's only the selected choice if nothing else matches. */
					if( def.selectType != SELECT_MULTIPLE )
						iFallbackOption = e;
					continue;
				}

				if( def.bOneChoiceForAllPlayers )
				{
					if( mc.DescribesCurrentModeForAllPlayers() )
					{
						bUseFallbackOption = false;
						if( def.selectType != SELECT_MULTIPLE )
							SelectExactlyOne( e, vbSelOut );
						else
							vbSelOut[e] = true;
					}
				}
				else
				{
					if( mc.DescribesCurrentMode( p) )
					{
						bUseFallbackOption = false;
						if( def.selectType != SELECT_MULTIPLE )
							SelectExactlyOne( e, vbSelOut );
						else
							vbSelOut[e] = true;
					}
				}
			}

			if( def.selectType == SELECT_ONE && bUseFallbackOption )
			{
				if( iFallbackOption == -1 )
				{
					CString s = ssprintf("No options in row \"%s\" were selected, and no fallback row found; selected entry 0", m_sName.c_str());
					LOG->Warn( s );
					CHECKPOINT_M( s );
					iFallbackOption = 0;
				}

				SelectExactlyOne( iFallbackOption, vbSelOut );
			}

			VerifySelected( def.selectType, vbSelOut, def.name );
		}
	}

	int ExportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, const vector<bool> vbSelected[NUM_PLAYERS] ) const
	{
		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			const vector<bool> &vbSel = vbSelected[p];
		
			Default.Apply( p );
			for( unsigned i=0; i<vbSel.size(); i++ )
			{
				if( vbSel[i] )
					ListEntries[i].Apply( p );
			}
		}
		FOREACH_CONST( CString, m_vsBroadcastOnExport, s )
			MESSAGEMAN->Broadcast( *s );
		return 0;
	}

	virtual CString GetIconText( const OptionRowDefinition &def, int iFirstSelection ) const
	{
		return m_bUseModNameForIcon ?
			ListEntries[iFirstSelection].m_sModifiers :
			def.choices[iFirstSelection];
	}
	virtual bool HasScreen( int iChoice ) const
	{ 
		const GameCommand &mc = ListEntries[iChoice];
		return !mc.m_sScreen.empty();
	}
	virtual CString GetScreen( int iChoice ) const
        {
                const GameCommand &gc = ListEntries[iChoice];
                return gc.m_sScreen;
        }
	virtual void GetGameCommand( int iChoice, GameCommand &out ) const
	{
		out = ListEntries[iChoice];
	}
	void FillNoteSkins( OptionRowDefinition &defOut, CString sParam )
	{
		Init();
		defOut.Init();

		ASSERT( sParam.size() );
		m_sName = sParam;

		defOut.name = "NoteSkins";
		defOut.bOneChoiceForAllPlayers = false;
		defOut.m_bAllowThemeItems = false;	// we theme the text ourself

		CStringArray arraySkinNames;
		NOTESKIN->GetNoteSkinNames( arraySkinNames );
		for( unsigned skin=0; skin<arraySkinNames.size(); skin++ )
		{
			arraySkinNames[skin].MakeUpper();

			GameCommand mc;
			mc.m_sModifiers = arraySkinNames[skin];
			ListEntries.push_back( mc );
			defOut.choices.push_back( arraySkinNames[skin] );
		}
	}

	void FillSteps( OptionRowDefinition &defOut, CString sParam, bool bLockedTogether )
	{
		Init();
		defOut.Init();

		ASSERT( sParam.size() );
		m_sName = sParam;

		defOut.name = "Steps";
		defOut.bOneChoiceForAllPlayers = bLockedTogether;
		defOut.m_bAllowThemeItems = false;	// we theme the text ourself

		// fill in difficulty names
		if( GAMESTATE->m_bEditing )
		{
			defOut.choices.push_back( "" );
			ListEntries.push_back( GameCommand() );
		}
		else if( GAMESTATE->IsCourseMode() )   // playing a course
		{
			defOut.bOneChoiceForAllPlayers = (bool)PREFSMAN->m_bLockCourseDifficulties;

			vector<Trail*> vTrails;
			GAMESTATE->m_pCurCourse->GetTrails( vTrails, GAMESTATE->GetCurrentStyle()->m_StepsType );
			for( unsigned i=0; i<vTrails.size(); i++ )
			{
				Trail* pTrail = vTrails[i];

				CString s = CourseDifficultyToThemedString( pTrail->m_CourseDifficulty );
				s += ssprintf( " %d", pTrail->GetMeter() );
				defOut.choices.push_back( s );
				GameCommand mc;
				mc.m_pTrail = pTrail;
				ListEntries.push_back( mc );
			}
		}
		else // !GAMESTATE->IsCourseMode(), playing a song
		{
			vector<Steps*> vpSteps;
			Song *pSong = GAMESTATE->m_pCurSong;
			pSong->GetSteps( vpSteps, GAMESTATE->GetCurrentStyle()->m_StepsType );
			StepsUtil::RemoveLockedSteps( pSong, vpSteps );
			StepsUtil::SortNotesArrayByDifficulty( vpSteps );
			for( unsigned i=0; i<vpSteps.size(); i++ )
			{
				Steps* pSteps = vpSteps[i];

				CString s;
				if( pSteps->GetDifficulty() == DIFFICULTY_EDIT )
					s = pSteps->GetDescription();
				else
					s = DifficultyToThemedString( pSteps->GetDifficulty() );
				if (!HIDE_METER) // Condition to display the meter or not -Wanny
				{
					s += ssprintf( " %d", pSteps->GetMeter() );
				}
				defOut.choices.push_back( s );
				GameCommand mc;
				mc.m_pSteps = pSteps;
				mc.m_dc = pSteps->GetDifficulty();
				ListEntries.push_back( mc );
			}
		}
	}

	void FillCharacters( OptionRowDefinition &defOut, CString sParam )
	{
		Init();
		defOut.Init();

		ASSERT( sParam.size() );
		m_sName = sParam;

		defOut.bOneChoiceForAllPlayers = false;
		defOut.m_bAllowThemeItems = false;
		defOut.name = "Characters";
		Default.m_pCharacter = GAMESTATE->GetDefaultCharacter();

		{
			defOut.choices.push_back( "Off" );
			GameCommand mc;
			mc.m_pCharacter = NULL;
			ListEntries.push_back( mc );
		}

		vector<Character*> apCharacters;
		GAMESTATE->GetCharacters( apCharacters );
		for( unsigned i=0; i<apCharacters.size(); i++ )
		{
			Character* pCharacter = apCharacters[i];
			CString s = pCharacter->m_sName;
			s.MakeUpper();

			defOut.choices.push_back( s ); 
			GameCommand mc;
			mc.m_pCharacter = pCharacter;
			ListEntries.push_back( mc );
		}
	}

	void FillStyles( OptionRowDefinition &defOut, CString sParam )
	{
		Init();
		defOut.Init();

		ASSERT( sParam.size() );
		m_sName = sParam;

		defOut.bOneChoiceForAllPlayers = true;
		defOut.name = "Style";
		defOut.m_bAllowThemeItems = false;	// we theme the text ourself

		vector<const Style*> vStyles;
		GAMEMAN->GetStylesForGame( GAMESTATE->m_pCurGame, vStyles );
		ASSERT( vStyles.size() );
		FOREACH_CONST( const Style*, vStyles, s )
		{
			defOut.choices.push_back( GAMEMAN->StyleToThemedString(*s) ); 
			GameCommand mc;
			mc.m_pStyle = *s;
			ListEntries.push_back( mc );
		}

		Default.m_pStyle = vStyles[0];
	}

	void FillGroups( OptionRowDefinition &defOut, CString sParam )
	{
		Init();
		defOut.Init();

		ASSERT( sParam.size() );
		m_sName = sParam;

		defOut.bOneChoiceForAllPlayers = true;
		defOut.m_bAllowThemeItems = false;	// we theme the text ourself
		defOut.name = "Group";
		Default.m_sSongGroup = GROUP_ALL_MUSIC;

		vector<CString> vGroups;
		SONGMAN->GetGroupNames( vGroups );
		ASSERT( vGroups.size() );

		{
			defOut.choices.push_back( "AllGroups" );
			GameCommand mc;
			mc.m_sSongGroup = GROUP_ALL_MUSIC;
			ListEntries.push_back( mc );
		}

		FOREACH_CONST( CString, vGroups, g )
		{
			defOut.choices.push_back( *g ); 
			GameCommand mc;
			mc.m_sSongGroup = *g;
			ListEntries.push_back( mc );
		}
	}

	void FillDifficulties( OptionRowDefinition &defOut, CString sParam )
	{
		Init();
		defOut.Init();

		ASSERT( sParam.size() );
		m_sName = sParam;

		defOut.bOneChoiceForAllPlayers = true;
		defOut.name = "Difficulty";
		Default.m_dc = DIFFICULTY_INVALID;
		defOut.m_bAllowThemeItems = false;	// we theme the text ourself

		{
			defOut.choices.push_back( "AllDifficulties" );
			GameCommand mc;
			mc.m_dc = DIFFICULTY_INVALID;
			ListEntries.push_back( mc );
		}

		FOREACH_CONST( Difficulty, DIFFICULTIES_TO_SHOW.GetValue(), d )
		{
			CString s = DifficultyToThemedString( *d );

			defOut.choices.push_back( s ); 
			GameCommand mc;
			mc.m_dc = *d;
			ListEntries.push_back( mc );
		}
	}

	void FillSongsInCurrentSongGroup( OptionRowDefinition &defOut, CString sParam )
	{
		Init();
		defOut.Init();

		ASSERT( sParam.size() );
		m_sName = sParam;

		vector<Song*> vpSongs;
		SONGMAN->GetSongs( vpSongs, GAMESTATE->m_sPreferredSongGroup );

		if( GAMESTATE->m_pCurSong == NULL )
			GAMESTATE->m_pCurSong.Set( vpSongs[0] );

		defOut.name = "SongsInCurrentSongGroup";
		defOut.bOneChoiceForAllPlayers = true;
		defOut.layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		defOut.m_bExportOnChange = true;

		FOREACH_CONST( Song*, vpSongs, p )
		{
			defOut.choices.push_back( (*p)->GetTranslitFullTitle() ); 
			GameCommand mc;
			mc.m_pSong = *p;
			ListEntries.push_back( mc );
		}
	}
};

class OptionRowHandlerLua : public OptionRowHandler
{
public:
	LuaExpression *m_pLuaTable;

	OptionRowHandlerLua() { m_pLuaTable = new LuaExpression; Init(); }
	virtual ~OptionRowHandlerLua() { delete m_pLuaTable; }
	void Init()
	{
		OptionRowHandler::Init();
		m_pLuaTable->Unset();
	}
	virtual void Load( OptionRowDefinition &defOut, CString sLuaFunction )
	{
		ASSERT( sLuaFunction.size() );

		Init();
		defOut.Init();

		m_sName = sLuaFunction;
		defOut.m_bAllowThemeItems = false;	// Lua options are always dynamic and can theme themselves.

		Lua *L = LUA->Get();

		/* Run the Lua expression.  It should return a table. */
		m_pLuaTable->SetFromExpression( sLuaFunction );

		if( m_pLuaTable->GetLuaType() != LUA_TTABLE )
			RageException::Throw( "Result of \"%s\" is not a table", sLuaFunction.c_str() );

		m_pLuaTable->PushSelf( L );

		lua_pushstring( L, "Name" );
		lua_gettable( L, -2 );
		const char *pStr = lua_tostring( L, -1 );
		if( pStr == NULL )
			RageException::Throw( "\"%s\" \"Name\" entry is not a string", sLuaFunction.c_str() );
		defOut.name = pStr;
		lua_pop( L, 1 );


		lua_pushstring( L, "OneChoiceForAllPlayers" );
		lua_gettable( L, -2 );
		defOut.bOneChoiceForAllPlayers = !!lua_toboolean( L, -1 );
		lua_pop( L, 1 );


		lua_pushstring( L, "ExportOnChange" );
		lua_gettable( L, -2 );
		defOut.m_bExportOnChange = !!lua_toboolean( L, -1 );
		lua_pop( L, 1 );


		lua_pushstring( L, "LayoutType" );
		lua_gettable( L, -2 );
		pStr = lua_tostring( L, -1 );
		if( pStr == NULL )
			RageException::Throw( "\"%s\" \"LayoutType\" entry is not a string", sLuaFunction.c_str() );
		defOut.layoutType = StringToLayoutType( pStr );
		ASSERT( defOut.layoutType != LAYOUT_INVALID );
		lua_pop( L, 1 );


		lua_pushstring( L, "SelectType" );
		lua_gettable( L, -2 );
		pStr = lua_tostring( L, -1 );
		if( pStr == NULL )
			RageException::Throw( "\"%s\" \"SelectType\" entry is not a string", sLuaFunction.c_str() );
		defOut.selectType = StringToSelectType( pStr );
		ASSERT( defOut.selectType != SELECT_INVALID );
		lua_pop( L, 1 );


		/* Iterate over the "Choices" table. */
		lua_pushstring( L, "Choices" );
		lua_gettable( L, -2 );
		if( !lua_istable( L, -1 ) )
			RageException::Throw( "\"%s\" \"Choices\" is not a table", sLuaFunction.c_str() );

		lua_pushnil( L );
		while( lua_next(L, -2) != 0 )
		{
			/* `key' is at index -2 and `value' at index -1 */
			const char *pValue = lua_tostring( L, -1 );
			if( pValue == NULL )
				RageException::Throw( "\"%s\" Column entry is not a string", sLuaFunction.c_str() );
//				LOG->Trace( "'%s'", pValue);

			defOut.choices.push_back( pValue );

			lua_pop( L, 1 );  /* removes `value'; keeps `key' for next iteration */
		}

		lua_pop( L, 1 ); /* pop choices table */


		/* Iterate over the "EnabledForPlayers" table. */
		lua_pushstring( L, "EnabledForPlayers" );
		lua_gettable( L, -2 );
		if( !lua_isnil( L, -1 ) )
		{
			if( !lua_istable( L, -1 ) )
				RageException::Throw( "\"%s\" \"EnabledForPlayers\" is not a table", sLuaFunction.c_str() );

			defOut.m_vEnabledForPlayers.clear();	// and fill in with supplied PlayerNumbers below

			lua_pushnil( L );
			while( lua_next(L, -2) != 0 )
			{
				/* `key' is at index -2 and `value' at index -1 */
				PlayerNumber pn = (PlayerNumber)luaL_checkint( L, -1 );

				defOut.m_vEnabledForPlayers.insert( pn );

				lua_pop( L, 1 );  /* removes `value'; keeps `key' for next iteration */
			}
		}
		lua_pop( L, 1 ); /* pop EnabledForPlayers table */

		
		/* Iterate over the "ReloadRowMessages" table. */
		lua_pushstring( L, "ReloadRowMessages" );
		lua_gettable( L, -2 );
		if( !lua_isnil( L, -1 ) )
		{
			if( !lua_istable( L, -1 ) )
				RageException::Throw( "\"%s\" \"ReloadRowMessages\" is not a table", sLuaFunction.c_str() );

			lua_pushnil( L );
			while( lua_next(L, -2) != 0 )
			{
				/* `key' is at index -2 and `value' at index -1 */
				const char *pValue = lua_tostring( L, -1 );
				if( pValue == NULL )
					RageException::Throw( "\"%s\" Column entry is not a string", sLuaFunction.c_str() );
				LOG->Trace( "Found ReloadRowMessage '%s'", pValue);

				m_vsReloadRowMessages.push_back( pValue );

				lua_pop( L, 1 );  /* removes `value'; keeps `key' for next iteration */
			}
		}
		lua_pop( L, 1 ); /* pop ReloadRowMessages table */

		
		/* Look for "ExportOnChange" value. */
		lua_pushstring( L, "ExportOnChange" );
		lua_gettable( L, -2 );
		if( !lua_isnil( L, -1 ) )
		{
			defOut.m_bExportOnChange = !!MyLua_checkboolean( L, -1 );
		}
		lua_pop( L, 1 ); /* pop ExportOnChange value */


		lua_pop( L, 1 ); /* pop main table */

		//ASSERT_M( lua_gettop(L) == 0, ssprintf("assertion 'lua_gettop(L) == 0' failed for \"%s\"", sLuaFunction.c_str()) );
		if( lua_gettop(L) != 0 )
		{
			LOG->Warn( "OptionsRowHandlerLua Warning: LUA stack not empty before release" );
			while( lua_gettop(L) != 0 )
				lua_pop( L, 1 );
		}

		LUA->Release(L);
	}
	virtual void Reload( OptionRowDefinition &defOut )
	{
		Lua *L = LUA->Get();

		/* Run the Lua expression.  It should return a table. */
		m_pLuaTable->SetFromExpression( m_sName );

		if( m_pLuaTable->GetLuaType() != LUA_TTABLE )
			RageException::Throw( "Result of \"%s\" is not a table", m_sName.c_str() );

		m_pLuaTable->PushSelf( L );


		/* Iterate over the "EnabledForPlayers" table. */
		lua_pushstring( L, "EnabledForPlayers" );
		lua_gettable( L, -2 );
		if( !lua_isnil( L, -1 ) )
		{
			if( !lua_istable( L, -1 ) )
				RageException::Throw( "\"%s\" \"EnabledForPlayers\" is not a table", m_sName.c_str() );

			defOut.m_vEnabledForPlayers.clear();	// and fill in with supplied PlayerNumbers below

			lua_pushnil( L );
			while( lua_next(L, -2) != 0 )
			{
				/* `key' is at index -2 and `value' at index -1 */
				PlayerNumber pn = (PlayerNumber)luaL_checkint( L, -1 );

				defOut.m_vEnabledForPlayers.insert( pn );

				lua_pop( L, 1 );  /* removes `value'; keeps `key' for next iteration */
			}
		}
		lua_pop( L, 1 ); /* pop EnabledForPlayers table */


		lua_pop( L, 1 ); /* pop main table */

		// HACK: we're getting a mysterious stack object and I'm not sure where.
		// Just remove it manually for now - I'll get a proper fix in later.
		if( lua_gettop(L) == 1 )
			lua_pop( L, 1 );
		else
			ASSERT( lua_gettop(L) == 0 );

		LUA->Release(L);
	}
	virtual void ImportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, vector<bool> vbSelectedOut[NUM_PLAYERS] ) const
	{
		Lua *L = LUA->Get();

		ASSERT( lua_gettop(L) == 0 );

		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			vector<bool> &vbSelOut = vbSelectedOut[p];

			/* Evaluate the LoadSelections(self,array,pn) function, where array is a table
			* representing vbSelectedOut. */

			/* All selections default to false. */
			for( unsigned i = 0; i < vbSelOut.size(); ++i )
				vbSelOut[i] = false;

			/* Create the vbSelectedOut table. */
			LuaHelpers::CreateTableFromArrayB( L, vbSelOut );
			ASSERT( lua_gettop(L) == 1 ); /* vbSelectedOut table */

			/* Get the function to call from m_LuaTable. */
			m_pLuaTable->PushSelf( L );
			ASSERT( lua_istable( L, -1 ) );

			lua_pushstring( L, "LoadSelections" );
			lua_gettable( L, -2 );
			if( !lua_isfunction( L, -1 ) )
				RageException::Throw( "\"%s\" \"LoadSelections\" entry is not a function", def.name.c_str() );

			/* Argument 1 (self): */
			m_pLuaTable->PushSelf( L );

			/* Argument 2 (vbSelectedOut): */
			lua_pushvalue( L, 1 );

			/* Argument 3 (pn): */
			LuaHelpers::PushStack( (int) p, L );

			ASSERT( lua_gettop(L) == 6 ); /* vbSelectedOut, m_iLuaTable, function, self, arg, arg */

			lua_call( L, 3, 0 ); // call function with 3 arguments and 0 results
			ASSERT( lua_gettop(L) == 2 );

			lua_pop( L, 1 ); /* pop option table */

			LuaHelpers::ReadArrayFromTableB( L, vbSelOut );
			
			lua_pop( L, 1 ); /* pop vbSelectedOut table */

			ASSERT( lua_gettop(L) == 0 );
		}

		LUA->Release(L);
	}
    virtual int ExportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, const vector<bool> vbSelected[NUM_PLAYERS] ) const
	{
		Lua *L = LUA->Get();

		ASSERT( lua_gettop(L) == 0 );

		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			const vector<bool> &vbSel = vbSelected[p];

			/* Evaluate SaveSelections(self,array,pn) function, where array is a table
			 * representing vbSelectedOut. */

			vector<bool> vbSelectedCopy = vbSel;

			/* Create the vbSelectedOut table. */
			LuaHelpers::CreateTableFromArrayB( L, vbSelectedCopy );
			ASSERT( lua_gettop(L) == 1 ); /* vbSelectedOut table */

			/* Get the function to call. */
			m_pLuaTable->PushSelf( L );
			ASSERT( lua_istable( L, -1 ) );

			lua_pushstring( L, "SaveSelections" );
			lua_gettable( L, -2 );
			if( !lua_isfunction( L, -1 ) )
				RageException::Throw( "\"%s\" \"SaveSelections\" entry is not a function", def.name.c_str() );

			/* Argument 1 (self): */
			m_pLuaTable->PushSelf( L );

			/* Argument 2 (vbSelectedOut): */
			lua_pushvalue( L, 1 );

			/* Argument 3 (pn): */
			LuaHelpers::PushStack( (int) p, L );

			ASSERT( lua_gettop(L) == 6 ); /* vbSelectedOut, m_iLuaTable, function, self, arg, arg */

			lua_call( L, 3, 0 ); // call function with 3 arguments and 0 results
			ASSERT( lua_gettop(L) == 2 );

			lua_pop( L, 1 ); /* pop option table */
			lua_pop( L, 1 ); /* pop vbSelected table */

			ASSERT( lua_gettop(L) == 0 );
		}

		LUA->Release(L);

		// XXX: allow specifying the mask
		return 0;
	}
};

class OptionRowHandlerConfig : public OptionRowHandler
{
public:
	const ConfOption *opt;

	OptionRowHandlerConfig() { Init(); }
	void Init()
	{
		OptionRowHandler::Init();
		opt = NULL;
	}
	virtual void Load( OptionRowDefinition &defOut, CString sParam )
	{
		ASSERT( sParam.size() );

		Init();
		defOut.Init();

		/* Configuration values are never per-player. */
		defOut.bOneChoiceForAllPlayers = true;

		ConfOption *pConfOption = ConfOption::Find( sParam );
		if( pConfOption == NULL )
			RageException::Throw( "Invalid Conf type \"%s\"", sParam.c_str() );

   		pConfOption->UpdateAvailableOptions();

		opt = pConfOption;
		opt->MakeOptionsList( defOut.choices );

		defOut.name = opt->name;
	}
	virtual void ImportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, vector<bool> vbSelectedOut[NUM_PLAYERS] ) const
	{
		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			vector<bool> &vbSelOut = vbSelectedOut[p];

			int iSelection = opt->Get();
			SelectExactlyOne( iSelection, vbSelOut );
		}
	}
	virtual int ExportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, const vector<bool> vbSelected[NUM_PLAYERS] ) const
	{
		bool bChanged = false;

		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			const vector<bool> &vbSel = vbSelected[p];

			int sel = GetOneSelection(vbSel);

			/* Get the original choice. */
			int Original = opt->Get();

			/* Apply. */
			opt->Put( sel );

			/* Get the new choice. */
			int New = opt->Get();

			/* If it didn't change, don't return any side-effects. */
			if( Original != New )
				bChanged = true;
		}

		return bChanged ? opt->GetEffects() : 0;
	}
};

class OptionRowHandlerStepsType : public OptionRowHandler
{
public:
	BroadcastOnChange<StepsType> *m_pstToFill;
	vector<StepsType> m_vStepsTypesToShow;

	OptionRowHandlerStepsType() { Init(); }
	void Init()
	{
		OptionRowHandler::Init();
		m_pstToFill = NULL;
		m_vStepsTypesToShow.clear();
	}

	virtual void Load( OptionRowDefinition &defOut, CString sParam )
	{
		ASSERT( sParam.size() );

		Init();
		defOut.Init();

		if( sParam == "EditStepsType" )
		{
			m_pstToFill = &GAMESTATE->m_stEdit;
		}
		else if( sParam == "EditSourceStepsType" )
		{
			m_pstToFill = &GAMESTATE->m_stEditSource;
			m_vsReloadRowMessages.push_back( MessageToString(MESSAGE_CURRENT_STEPS_P1_CHANGED) );
			m_vsReloadRowMessages.push_back( MessageToString(MESSAGE_EDIT_STEPS_TYPE_CHANGED) );
			if( GAMESTATE->m_pCurSteps[0].Get() != NULL )
				defOut.m_vEnabledForPlayers.clear();	// hide row
		}
		else
		{
			RageException::Throw( "invalid StepsType param \"%s\"", sParam.c_str() );
		}

		m_sName = sParam;
		defOut.name = sParam;
		defOut.bOneChoiceForAllPlayers = true;
		defOut.layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		defOut.m_bExportOnChange = true;
		defOut.m_bAllowThemeItems = false;	// we theme the text ourself

		// calculate which StepsTypes to show
		m_vStepsTypesToShow = STEPS_TYPES_TO_SHOW.GetValue();

		FOREACH_CONST( StepsType, m_vStepsTypesToShow, st )
		{
			CString s = GAMEMAN->StepsTypeToThemedString( *st );
			defOut.choices.push_back( s );
		}

		if( *m_pstToFill == STEPS_TYPE_INVALID )
			m_pstToFill->Set( m_vStepsTypesToShow[0] );
	}
	virtual void ImportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, vector<bool> vbSelectedOut[NUM_PLAYERS] ) const
	{
		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			vector<bool> &vbSelOut = vbSelectedOut[p];

			if( GAMESTATE->m_pCurSteps[0] )
			{
				StepsType st = GAMESTATE->m_pCurSteps[0]->m_StepsType;
				vector<StepsType>::const_iterator iter = find( m_vStepsTypesToShow.begin(), m_vStepsTypesToShow.end(), st );
				if( iter != m_vStepsTypesToShow.end() )
				{
					unsigned i = iter - m_vStepsTypesToShow.begin();
					vbSelOut[i] = true;
					continue;	// done with this player
				}
			}
			vbSelOut[0] = true;
		}
	}
	virtual int ExportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, const vector<bool> vbSelected[NUM_PLAYERS] ) const
	{
		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			const vector<bool> &vbSel = vbSelected[p];

			int index = GetOneSelection( vbSel );
			m_pstToFill->Set( m_vStepsTypesToShow[index] );
		}

		return 0;
	}
};


class OptionRowHandlerSteps : public OptionRowHandler
{
public:
	BroadcastOnChangePtr<Steps> *m_ppStepsToFill;
	BroadcastOnChange<Difficulty> *m_pDifficultyToFill;
	const BroadcastOnChange<StepsType> *m_pst;
	vector<Steps*> m_vSteps;
	vector<Difficulty> m_vDifficulties;

	OptionRowHandlerSteps() { Init(); }
	void Init()
	{
		OptionRowHandler::Init();
		m_ppStepsToFill = NULL;
		m_pDifficultyToFill = NULL;
		m_vSteps.clear();
		m_vDifficulties.clear();
	}

	virtual void Load( OptionRowDefinition &defOut, CString sParam )
	{
		ASSERT( sParam.size() );

		Init();
		defOut.Init();

		if( sParam == "EditSteps" )
		{
			m_ppStepsToFill = &GAMESTATE->m_pCurSteps[0];
			m_pDifficultyToFill = &GAMESTATE->m_PreferredDifficulty[0];
			m_pst = &GAMESTATE->m_stEdit;
			m_vsReloadRowMessages.push_back( MessageToString(MESSAGE_EDIT_STEPS_TYPE_CHANGED) );
		}
		else if( sParam == "EditSourceSteps" )
		{
			m_ppStepsToFill = &GAMESTATE->m_pEditSourceSteps;
			m_pst = &GAMESTATE->m_stEditSource;
			m_vsReloadRowMessages.push_back( MessageToString(MESSAGE_EDIT_SOURCE_STEPS_TYPE_CHANGED) );
			if( GAMESTATE->m_pCurSteps[0].Get() != NULL )
				defOut.m_vEnabledForPlayers.clear();	// hide row
		}
		else
		{
			RageException::Throw( "invalid StepsType param \"%s\"", sParam.c_str() );
		}
		
		m_sName = sParam;
		defOut.name = sParam;
		defOut.bOneChoiceForAllPlayers = true;
		defOut.layoutType = LAYOUT_SHOW_ONE_IN_ROW;
		defOut.m_bExportOnChange = true;
		defOut.m_bAllowThemeItems = false;	// we theme the text ourself
		m_vsReloadRowMessages.push_back( MessageToString(MESSAGE_CURRENT_SONG_CHANGED) );

		if( GAMESTATE->m_pCurSong )
		{
			FOREACH_Difficulty( dc )
			{
				if( dc == DIFFICULTY_EDIT )
					continue;
				m_vDifficulties.push_back( dc );
				Steps* pSteps = GAMESTATE->m_pCurSong->GetStepsByDifficulty( *m_pst, dc );
				m_vSteps.push_back( pSteps );
			}
			GAMESTATE->m_pCurSong->GetSteps( m_vSteps, *m_pst, DIFFICULTY_EDIT );
			m_vDifficulties.resize( m_vSteps.size(), DIFFICULTY_EDIT );

			if( m_sName == "EditSteps" )
			{
				m_vSteps.push_back( NULL );
				m_vDifficulties.push_back( DIFFICULTY_EDIT );
			}

			for( unsigned i=0; i<m_vSteps.size(); i++ )
			{
				Steps* pSteps = m_vSteps[i];
				Difficulty dc = m_vDifficulties[i];

				CString s;
				if( dc == DIFFICULTY_EDIT )
				{
					if( pSteps )
						s = pSteps->GetDescription();
					else
						s = "NewEdit";
				}
				else
				{
					s = DifficultyToThemedString( dc );
				}
				defOut.choices.push_back( s );
			}
		}
		else
		{
			m_vDifficulties.push_back( DIFFICULTY_EDIT );
			m_vSteps.push_back( NULL );
			defOut.choices.push_back( "none" );
		}

		if( m_pDifficultyToFill )
			m_pDifficultyToFill->Set( m_vDifficulties[0] );
		m_ppStepsToFill->Set( m_vSteps[0] );
	}
	virtual void ImportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, vector<bool> vbSelectedOut[NUM_PLAYERS] ) const
	{
		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			vector<bool> &vbSelOut = vbSelectedOut[p];

			ASSERT( m_vSteps.size() == vbSelOut.size() );

			// look for matching steps
			vector<Steps*>::const_iterator iter = find( m_vSteps.begin(), m_vSteps.end(), m_ppStepsToFill->Get() );
			if( iter != m_vSteps.end() )
			{
				unsigned i = iter - m_vSteps.begin();
				vbSelOut[i] = true;
				return;
			}
			// look for matching difficulty
			if( m_pDifficultyToFill )
			{
				FOREACH_CONST( Difficulty, m_vDifficulties, d )
				{
					unsigned i = d - m_vDifficulties.begin();
					if( *d == GAMESTATE->m_PreferredDifficulty[0] )
					{
						vbSelOut[i] = true;
						vector<PlayerNumber> v;
						v.push_back( p );
						ExportOption( def, v, vbSelectedOut );	// current steps changed
						continue;
					}
				}
			}
			// default to 1st
			vbSelOut[0] = true;
		}
	}
	virtual int ExportOption( const OptionRowDefinition &def, const vector<PlayerNumber> &vpns, const vector<bool> vbSelected[NUM_PLAYERS] ) const
	{
		FOREACH_CONST( PlayerNumber, vpns, pn )
		{
			PlayerNumber p = *pn;
			const vector<bool> &vbSel = vbSelected[p];

			int index = GetOneSelection( vbSel );
			Difficulty dc = m_vDifficulties[index];
			Steps *pSteps = m_vSteps[index];
			if( m_pDifficultyToFill )
				m_pDifficultyToFill->Set( dc );
			m_ppStepsToFill->Set( pSteps );
		}

		return 0;
	}
};

///////////////////////////////////////////////////////////////////////////////////

OptionRowHandler* OptionRowHandlerUtil::Make( const Command &command, OptionRowDefinition &defOut )
{
	OptionRowHandler* pHand = NULL;

	BeginHandleArgs;

	const CString &name = command.GetName();

#define MAKE( type )	{ type *p = new type; p->Load( defOut, sArg(1) ); pHand = p; }

	if(		 name == "list" )			MAKE( OptionRowHandlerList )
	else if( name == "lua" )			MAKE( OptionRowHandlerLua )
	else if( name == "conf" )			MAKE( OptionRowHandlerConfig )
	else if( name == "stepstype" )		MAKE( OptionRowHandlerStepsType )
	else if( name == "steps" )			MAKE( OptionRowHandlerSteps )

	EndHandleArgs;

	return pHand;
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
