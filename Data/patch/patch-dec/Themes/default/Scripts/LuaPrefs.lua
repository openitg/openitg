--[[
LUA-based OptionRows for OpenITG ("ScreenOpenITGOptions")
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]

-- "0" is used as a sentinel for "unlimited" for most prefs here.

-- Rounds up or down to the nearest interval of 'factor'
local function round( num, factor )
	return math.floor((num+(factor/2))*(1/factor))*factor
end

-- Special options line for SoundVolumeAttract that
-- clamps to the maximum value of SoundVolume.
function SoundVolumeAttractOptions()
	-- round this value to the nearest 0.1, so we don't bork due to
	-- floating-point errors. this will cause inaccuracies with the
	-- attract settings, but they won't be significantly far off and
	-- we can at least ensure all the float comparisons line up.
	local soundvol = round(PREFSMAN:GetPreference("SoundVolume"), 0.1)

	-- 0 - 100% in 10% increments, plus 0% itself
	local num_options = 11

	-- return in decimal format and offset by 1, so the
	-- one-indexed list corresponds to a zero-indexed value
	local function IndexToVolume(i)
		return (i-1)*0.1
	end

	-- scale to SoundVolume and round it
	local function IndexToScaledVolume(i)
		return round( IndexToVolume(i)*soundvol, 0.01)
	end

	local Names = { }
	Names[1] = "SILENT"
	-- print in percent format
	for i = 2,num_options do
		Names[i] = math.floor(IndexToVolume(i)*100) .. "%"
	end

	local function Load(self, list, pn)
		local attractvol = round(PREFSMAN:GetPreference("SoundVolumeAttract"), 0.01)
		for i = 1,num_options do
			if attractvol == IndexToScaledVolume(i) then list[i] = true return end
		end
		list[6] = true	-- 50% default
	end

	local function Save(self, list, pn)
		for i = 1,num_options do
			if list[i] then
				PREFSMAN:SetPreference( "SoundVolumeAttract", IndexToScaledVolume(i) )
				return
			end
		end
	end

	local Params = { Name = "SoundVolumeAttract" }

	return CreateOptionRow( Params, Names, Load, Save )
end

function CustomMaxSecondsOptions()
	-- start with 2:00, go to 15:00 + UNLIMITED
	local Values = {}
	for i = 1,14 do Values[i] = 60*(i+1) end
	Values[15] = 0

	local Names = {}
	for i = 1,14 do Names[i] = SecondsToMMSS(Values[i]) end
	Names[15] = "UNLIMITED"

	local Params = { Name = "CustomMaxSeconds" }
	return CreatePrefsRow( Params, Names, Values, "CustomMaxSeconds" )
end

function CustomsLoadMaxOptions()
	-- start with 10, go to 100
	local Values = {}
	for i = 1,10 do Values[i] = 10*i end
	Values[11] = 0

	local Names = {}
	for i = 1,10 do Names[i] = Values[i] end
	Names[11] = "UNLIMITED"

	local Params = { Name = "CustomsLoadMax" }
	return CreatePrefsRow( Params, Names, Values, "CustomsLoadMax" )
end

function CustomsLoadTimeoutOptions()
	-- start with 1, go to 15
	local Values = {}
	for i = 1,15 do Values[i] = i end
	Values[16] = 0

	local Names = {}
	for i = 1,15 do Names[i] = Values[i] end
	Values[16] = "UNLIMITED"

	local Params = { Name = "CustomsLoadTimeout" }
	return CreatePrefsRow( Params, Names, Values, "CustomsLoadTimeout" )
end

function CustomMaxSizeOptions()
	local Values = { 3, 4, 5, 6, 7, 8, 9, 10, 0 }

	local Names = {}
	for i = 1,8 do Names[i] = Values[i] .. " MB" end
	Names[9] = "UNLIMITED"

	local Params = { Name = "CustomMaxSizeMB" }

	return CreatePrefsRow( Params, Names, Values, "CustomMaxSizeMB" )
end

function CustomMaxStepSizeOptions()
	-- offset by 30 KB, increments of 15 KB up to 150 KB and unlimited
	local Names = {}
	for i=1,9 do Names[i] = (30+((i-1)*15)) .. "KB" end
	Names[10] = "UNLIMITED"

	local Params = { Name = "CustomMaxStepSizeKB" }

	return CreatePrefsRowRange( Params, Names, "CustomMaxStepSizeKB", 30, 15 )
end

-- Base function for the below derivations
local function SongLengthOptions( listname, prefname, offset, num, delta )
	-- This is designed as a zero-index system, so offset it as needed.
	
	-- build the values table - use num+1 for 'unlimited'
	local Values = {}
	for i = 1,num do Values[i] = offset+((i-1)*delta) end
	Values[num+1] = 100000;
				
	-- build the names table
	local Names = {}
	for i = 1,num do Names[i] = SecondsToMMSS(Values[i]) end
	Names[num+1] = "UNLIMITED"

	local Params = { Name = listname }

	return CreatePrefsRow( Params, Names, Values, prefname )
end

-- Long version, lowest: 2:15 (135), highest: 05:00 (300), 11+1 options, 15-second delta
function LongVersionOptions()
	return SongLengthOptions( "LongVersion", "LongVerSongSeconds", 135, 12, 15 )
end

-- Marathon version, lowest: 5:00 (300), highest: 10:00 (600), 10+1 options, 30-second delta
function MarathonVersionOptions()
	return SongLengthOptions( "MarathonVersion", "MarathonVerSongSeconds", 300, 11, 30 )
end
