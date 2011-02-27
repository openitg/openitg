function Platform() return "ps2" end
function IsHomeMode() return true end

function SelectButtonAvailable()
	return false
end

function LoadScreenIfControllersPresent( screen )
	if AnyControllersAreConnected() then
		SCREENMAN:SetNewScreen( screen )
	end
end


-- On evaluation screens, at most one locked item (song, course, etc) may become
-- unlocked.
--
-- Note that these need to align with the [Unlocks] metrics set.
local Locks =
{
	{ SongsRequired = 5,	Type = "Song",		Code = 1, Name = "Disconnected Hyper" },
	{ SongsRequired = 15,	Type = "Course",	Code = 2, Name = "Bounce" },
	{ SongsRequired = 25,	Type = "Song",		Code = 3, Name = "Tell" },
	{ SongsRequired = 35,	Type = "Modifier",	Code = 4, Name = "Bumpy" },
	{ SongsRequired = 45,	Type = "Song",		Code = 5, Name = "Anubis" },
	{ SongsRequired = 55,	Type = "Course",	Code = 6, Name = "Outer World" },
	{ SongsRequired = 65,	Type = "Song",		Code = 7, Name = "Bubble Dancer" },
	{ SongsRequired = 75,	Type = "Modifier",	Code = 8, Name = "Beat" },
	{ SongsRequired = 85,	Type = "Song",		Code = 9, Name = "Disconnected Mobius" },
	{ SongsRequired = 95,	Type = "Song",		Code = 10, Name = "Liquid Moon" },
	{ SongsRequired = 105,	Type = "Song",		Code = 11, Name = "Don't Promise Me ~Happiness~" },
	{ SongsRequired = 115,	Type = "Song",		Code = 12, Name = "Funk Factory" },
	{ SongsRequired = 125,	Type = "Song",		Code = 13, Name = "DJ Party" },
	{ SongsRequired = 135,	Type = "Song",		Code = 14, Name = "Tribal Style" },
	{ SongsRequired = 145,	Type = "Song",		Code = 15, Name = "Infection" },
	{ SongsRequired = 155,	Type = "Course",	Code = 16, Name = "The Legend" },
	{ SongsRequired = 165,	Type = "Song",		Code = 17, Name = "Pandemonium" },
	{ SongsRequired = 175,	Type = "Song",		Code = 18, Name = "Incognito" },
	{ SongsRequired = 185,	Type = "Song",		Code = 19, Name = "Xuxa" },
	{ SongsRequired = 195,	Type = "Song",		Code = 20, Name = "Wake Up" },
}

-- Get the lock that we're about to release, but don't actually unlock it.
local function GetUnlock()
	Trace( "GetUnlock()" )
	local Profile = PROFILEMAN:GetMachineProfile()
	local SongsPlayed = Profile:GetTotalNumSongsPlayed()

	for n,Lock in ipairs(Locks) do
		Trace( "Check #" .. n .. ": " .. Lock.Name )

		if Profile:IsCodeUnlocked( Lock.Code ) then 
			Trace( "Already unlocked" )
		elseif SongsPlayed >= Lock.SongsRequired then 
			Trace( "OK" )
			return Lock
		end
	end
end

-- If there's a song to unlock, play a set of commands to allow hooking.
function GetUnlockCommand()
	local Lock = GetUnlock()

	if Lock then
		-- MESSAGEMAN:Broadcast( "Unlock" )
		Trace( "Unlock "..Lock.Code )
		return "playcommand,Unlock" .. Lock.Type .. ";playcommand,Unlock" .. Lock.Name
	else
		return "playcommand,NoUnlock"
	end
end

local OriginalGetEvaluationHelpText = GetEvaluationHelpText
function GetEvaluationHelpText()
	local Text = OriginalGetEvaluationHelpText()
	
	-- If we've unlocked a song, say so in the help text.
	local Lock = GetUnlock()
	if Lock then
		Text = "Unlocked " .. Lock.Name .. "!\n" .. Text;
	end

	return Text
end

-- This is called by FirstUpdateCommand in ScreenEvaluation, to guarantee that it's
-- called *after* the other screen elements have a chance to call GetUnlockCommand()
-- and GetEvaluationHelpText().
function FinalizeUnlock()
	local Lock = GetUnlock()
	if Lock then
		UNLOCKMAN:UnlockCode( Lock.Code )
		UNLOCKMAN:PreferUnlockCode( Lock.Code )
	end
end

function UnlockAll()
	Trace( "Unlocked everything" )
	for n,Lock in ipairs(Locks) do
		UNLOCKMAN:UnlockCode( Lock.Code )
	end
end

function GetInitialDifficulty()
	return PROFILEMAN:GetMachineProfile():GetSaved()["DefaultDifficulty"] or GetDefaultDifficulty()
end
