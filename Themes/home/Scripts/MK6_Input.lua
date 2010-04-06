--[[
OpenITG broadcaster for MK6 input, version 1.0
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

This is still fast. I feel bad for the guy who has to code the XML, though...
Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]

--[[
Available calls: 
 - BroadcastMK6Buttons() - broadcasts messages for MK6 buttons
 - BroadcastMK6Sensors() - broadcasts messages for floor sensors
 - BroadcastMK6Messages() - broadcasts messages for all inputs

Button command syntax: <button><pn>MessageCommand
  - Button names: "MenuLeft", "MenuRight", "Start", "Select"
Examples: "MenuLeftP1MessageCommand", "StartP2MessageCommand"

Sensor command syntax: <direction><pn><sensor>MessageCommand
  - Sensor names: "Left", "Right", "Top", "Bottom"
  - Arrow names: "Left", "Right", "Up", "Down"
Examples: "LeftP1TopMessageCommand", "UpP2LeftMessageCommand"
--]]

--[[

SENSOR BROADCASTING

--]]

-- name definitions for each sensor, in order of appearance
local sensor_names = { "Right", "Left", "Bottom", "Top" }

-- contains all necessary data to send messages about each sensor
local arrow_maps =
{
	-- the sensor values for each player, relative to "Names"
	[1] = { 30, 29, 32, 31 },
	[2] = { 14, 13, 16, 15 },

	-- names for each button type
	["Names"] = { "Left", "Right", "Up", "Down" }
}

-- Broadcasts messages for individual sensors, e.g. "LeftP1BottomOn"
local function BroadcastMessagesForSensors( input )
	for pn=1,2 do
		for i=1,4 do
			-- the index of the bit that matches this arrow
			local sensor = arrow_maps[pn][i]

			for set=1,4 do
				-- the state of this sensor in sensor array 'i'
				local state = input[set][sensor] and "On" or "Off"

				-- message to send, e.g. "LeftP1BottomOn"
				local message = arrow_maps.Names[i] .. "P" .. pn .. sensor_names[set] .. state

				-- broadcast it!
				MESSAGEMAN:Broadcast( message )
			end
		end
	end
end



--[[

BUTTON BROADCASTING

--]]

-- the buttons on an MK6 that don't use sensor readings.
-- we can use arbitrary sensor sets to detect these.
local button_maps =
{
	[09] = "MenuRightP2",
	[10] = "MenuLeftP2",
	[11] = "SelectP2",
	[12] = "StartP2",

	[18] = "Test",
	[22] = "Service",
	[23] = "Coin",

	[25] = "MenuRightP1",
	[26] = "MenuLeftP1",
	[27] = "SelectP1",
	[28] = "StartP1"
}

-- broadcasts messages for the buttons being pressed
-- that are outlined in the above table. since buttons
-- report for all sensors, use the last set. That's the
-- most recent, and the most accurate.
local function BroadcastMessagesForButtons( input )
	for i=1,32 do
		-- only process if we have a definition
		if button_maps[i] then
			local state = input[4][i] and "On" or "Off"
			MESSAGEMAN:Broadcast( button_maps[i] .. state )
		end
	end
end

function BroadcastMK6Buttons()
	local sensor_table = MK6_GetSensors()
	BroadcastMessagesForButtons( sensor_table )
end

function BroadcastMK6Sensors()
	local sensor_table = MK6_GetSensors()
	BroadcastMessagesForSensors( sensor_table )
end

function BroadcastMK6Messages()
	local sensor_table = MK6_GetSensors()

	-- do this first, because it takes far less time
	BroadcastMessagesForButtons( sensor_table )
	BroadcastMessagesForSensors( sensor_table )
end
