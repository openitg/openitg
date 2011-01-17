--[[
Custom preferences rows for StepMania 3.95/OpenITG, v0.9
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

Needs CustomPrefsAPI for pref handling, PrefsRow for row generation
Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]

function CustomPrefsRow( Params, Names, Values, PrefName )
	-- set the PrefName as the metrics Name
	Params = Params or {}
	Params.Name = PrefName

	local amt = table.getn(Names)
	local val = CustomPrefs.Get( PrefName )

	local function Load(self, list, pn)
		for i=1,amt do
			if Values[i] == val then list[i] = true return end
		end
		list[1] = true	-- fall back to first value
	end

	local function Save(self, list, pn)
		for i=1,amt do if list[i] then
			CustomPrefs.Set( PrefName, Values[i] )
			return end
		end
	end

	return CreateOptionRow( Params, Names, Load, Save )
end

-- simply uses tostring() for all the Value names
function CustomPrefsRowNoNames( Params, Values, PrefName )
	local Names = {}
	for i=1,table.getn(Values) do Names[i] = tostring(Values[i]) end

	return CustomPrefsRow( Params, Names, Values, PrefName )
end

function CustomPrefsRowBool( Params, PrefName )
	local Names = { "OFF", "ON" }
	local Values = { false, true }
	return CustomPrefsRow( Params, Names, Values, PrefName )
end

-- the metrics.ini will throw errors for multiple arguments, due to ','.
-- TODO: try escape codes and see if that works in the .ini file
function CustomPrefsRowBoolSimple( PrefName )
	local Params = {}
	return CustomPrefsRowBool( Params, PrefName )
end

function MusicSelectSecondsRow()
	local Values = { 50, 60, 70, 80, 90, 105, 120, 150, 180, -1 }
	local num = table.getn(Values)
	local Names = {}
	for i=1,num do Names[i] = tostring(Values[i]) end
	Names[num] = "UNLIMITED"

	return CustomPrefsRow( Params, Names, Values, "MusicSelectSeconds" )
end

function CreditTypeRow()
	local Values = { "Coin", "Token", "Card" }
	return CustomPrefsRowNoNames( Params, Values, "CreditType" )
end
