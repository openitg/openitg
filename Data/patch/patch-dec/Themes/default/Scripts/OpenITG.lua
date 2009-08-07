--[[
Miscellaneous scripts for OpenITG
Licensed under Creative Commons Attribution-Share Alike 3.0 Unported
(http://creativecommons.org/licenses/by-sa/3.0/)

Written by Mark Cannon ("Vyhd") for OpenITG (http://www.boxorroxors.net/)
All I ask is that you keep this notice intact and don't redistribute in bytecode.
--]]

-- Hide the timer if "MenuTimer" is disabled
function HideTimer()
	local enabled = PREFSMAN:GetPreference("MenuTimer")
	if enabled then return "0" else return "1" end
end

function GetCustomSongCancelText()
	-- Set up two initial line spaces.
	local ret = "\n\n"

	if SelectButtonAvailable() then
		ret = ret .. "Pressing &SELECT will cancel this selection."
		return ret
	else
		ret = ret .. "Pressing &MENULEFT; + &MENURIGHT; will cancel this selection."
		return ret
	end

	return "???"
end

-- Determine whether or not to play Ahead/Behind on scores
-- in gameplay screens. Scores are only compared if there
-- are two players: other criteria can be defined here.
function CompareScores()
	-- This is hardcoded...it's just re-enforced to save time.
        for pn = PLAYER_1,NUM_PLAYERS-1 do
		if not GAMESTATE:IsPlayerEnabled(pn) then return "0" end
	end

	-- Only compare during Event Mode
	if not GAMESTATE:IsEventMode() then return "0" end

	-- only compare if both players have the same steps
	if GAMESTATE:IsCourseMode() then
		local TrailP1 = GAMESTATE:GetCurrentTrail( PLAYER_1 );
		local TrailP2 = GAMESTATE:GetCurrentTrail( PLAYER_2 );
		if TrailP1 ~= TrailP2 then return "0" end
	else
		local StepsP1 = GAMESTATE:GetCurrentSteps( PLAYER_1 );
		local StepsP2 = GAMESTATE:GetCurrentSteps( PLAYER_2 );
		if StepsP1 ~= StepsP2 then return "0" end
	end

	return "1"
end
