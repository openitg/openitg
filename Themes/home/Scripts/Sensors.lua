--[[
OpenITG broadcaster for MK6 sensors, version 0.8
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

This is surprisingly fast. I feel bad for the guy who has to code the XML, though...
Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]


local sensor_names = { "right", "left", "bottom", "top" }

-- Broadcasts messages for individual sensors, e.g.
-- "LeftP1SensorTopOn", "RightP1SensorLeftOff"
local function BroadcastMessagesForSensors( table, sensor, pn, name )
	for i=1,4 do
		if table[i][sensor] then
			MESSAGEMAN:Broadcast( name .. "Sensor" .. sensor_names[i] .. "P" .. pn .. "On" )
		else
			MESSAGEMAN:Broadcast( name .. "Sensor" .. sensor_names[i] .. "P" .. pn .. "Off" )
		end
	end
end

-- Left, Right, Up, and Down, respectively,
-- within the input bit field, per player.
local ivals =
{
	[1] = { 30, 29, 32, 31 },
	[2] = { 14, 13, 16, 15 },
	["Names"] = { "Left", "Up", "Right", "Down" }
}

function BroadcastMK6Messages()
	local sensor_table = MK6_GetSensors()

	for pn=1,2 do
		for i=1,4 do
			BroadcastMessagesForSensors( sensor_table, ivals[pn][i], pn, ivals.Names[i] )
		end
	end
end