--[[
Custom preferences API for StepMania 3.95/OpenITG, v0.6
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

A giant kludge saved into the MachineProfile. Whooo 3.95~
Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]

-- only run this once ProfileManager has been instantiated and registered
if not PROFILEMAN then return end

-- set up the global table if it does not yet exist
local MachineTable = PROFILEMAN:GetMachineProfile():GetSaved()
if not MachineTable.CustomPrefs then MachineTable.CustomPrefs = {} end

-- the global table that holds all preference data
local PrefsTable = MachineTable.CustomPrefs

-- Set up our public namespace and associated functions
CustomPrefs =
{
	NeedToSaveMachineProfile = false,

	Get = function(name)
		return PrefsTable[name]
	end,

	-- don't save the machine profile when initializing prefs
	Set = function(name, value)
		PrefsTable[name] = value
		NeedToSaveMachineProfile = true
	end,

	SaveMachineProfileIfNeeded = function()
		Debug( "Called SaveMachineProfileIfNeeded" )
		if NeedToSaveMachineProfile == true then
			Debug( "Needed to save profile, saving now" )
			SCREENMAN:OverlayMessage( "Saving profile..." )
			PROFILEMAN:SaveMachineProfile()
			SCREENMAN:HideOverlayMessage()
			NeedToSaveMachineProfile = false
		end
	end,
}

-- initialize a custom preference without clobbering any existing values
local function InitCustomPref( name, value )
	if PrefsTable[name] == nil then
		CustomPrefs.Set( name, value, true )
	end
end

-- This is the list of custom mods, settable from "OpenITG Options".
local Prefs =
{
	-- Amount of seconds to be used on the Select Music screen
	{ "MusicSelectSeconds",	50 },

	-- Type of credit used, which adjusts attract sequence text
	{ "CreditType",		"Coin" },

	-- Use the ITG2 logo instead of the OpenITG logo
	{ "UseOldLogo",		false },
}

--[[
Initialize the list of preferences
--]]

for num,tbl in ipairs(Prefs) do
	InitCustomPref( tbl[1], tbl[2] )
end

-- OPTIMIZATION: don't save default values. We'll re-create them if need be,
-- but saving to the disk is an expensive operation on ITG cabinets.
CustomPrefs.NeedToSaveMachineProfile = false
