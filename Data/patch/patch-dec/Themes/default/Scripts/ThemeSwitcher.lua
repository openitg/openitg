--[[
Lua Theme Switcher, OpenITG beta 1, version 1.5
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]

local function IsBlacklisted( name )
	-- never display fallback folders
	if string.find( name, "fallback" ) then return true end

	-- never display dot directories (e.g. ".svn", ".nano")
	if string.sub( name, 1, 1 ) == "." then return true end

	-- never display the themes in this list
	local BlacklistedThemes = { "ps2", "ps2onpc" }
	for i=1,table.getn(BlacklistedThemes) do
		if name == BlacklistedThemes[i] then return true end
	end

	return false
end

local function GetThemesFiltered()
	local ret = {}
	local themes = THEME:GetThemeNames()

	-- lowercase all themes, so we can guarantee case matching.
	for i=1,table.getn(themes) do
		if not IsBlacklisted(themes[i]) then
			Debug( "Adding theme: " .. themes[i] )
			ret[table.getn(ret)+1] = string.lower(themes[i])
		else
			Debug( "Threw out theme: " .. themes[i] )
		end
	end

	return ret
end

-- changed to use an argument for the next screen to go to
function ThemeSwitcher( next_screen )
	-- default to options menu unless otherwise set
	next_screen = next_screen or "ScreenOptionsMenu"

	local Names = GetThemesFiltered()
	local amt = table.getn(Names)

	local function Load(self, list, pn)
		local theme = THEME:GetCurThemeName()
		for i=1,amt do
			if string.lower(Names[i]) == theme then list[i] = true return end
		end

		-- fall back to Default if we have no matches
		for i=1,amt do
			if string.lower(Names[i]) == "default" then list[i] = true return end
		end

		-- fall back to the first option, so we don't crash
		list[1] = true
	end

	local function Save(self, list, pn)
		for i=1,amt do if list[i] then
			local command = "theme," .. Names[i]
			command = command .. ";screen," .. next_screen
			GAMESTATE:DelayedGameCommand( command ) end
		end
	end

	-- grab the pre-used metrics from the built-in language INI.
	local Params = { Name = "Theme" }

	return CreateOptionRow( Params, Names, Load, Save )
end
