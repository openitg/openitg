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

function GetCustomPref( name )
	-- we should never request a pref that isn't here
	if not PrefsTable[name] then return nil end

	return PrefsTable[name]
end

-- TODO: conserve disk saves by saving once, after
-- setting all preferences. How can we do that?
function SetCustomPref( name, value )
	PrefsTable[name] = value
	PROFILEMAN:SaveMachineProfile()
end

-- this is used to initialize a custom preference,
-- but won't clobber pre-set values.
local function InitCustomPref( name, value )
	if PrefsTable[name] then return end
	SetCustomPref( name, value )
end

-- This is the official list of custom mods.
-- Anything not here will be ignored.
-- TODO: change into a table and init dynamically
InitCustomPref( "MusicSelectSeconds", 50 )
InitCustomPref( "CreditType", "Coin" )
InitCustomPref( "UseOldLogo", false )
