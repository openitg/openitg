-- We want to display different help text depending on whether any high scores were made.
function GetScreenNameEntryTraditionalHelpText()
	local ret

	if GAMESTATE:AnyPlayerHasRankingFeats() then 
		ret = THEME:GetMetric("ScreenNameEntryTraditional","HelpTextHasHighScores")

		if SelectButtonAvailable() then
			ret = ret .. "::" .. THEME:GetMetric( "ScreenNameEntryTraditional", "HelpTextHasHighScoresSelectAvailableText" )
		end
	else
		ret = THEME:GetMetric("ScreenNameEntryTraditional","HelpTextNoHighScores")
	end

	return ret
end

function GetScreenSelectMusicHelpText()
-- function SetScreenSelectMusicHelpText( HelpText )
	local text = {}
	local ret = THEME:GetMetric( "ScreenSelectMusic", "HelpTextNormal" )


	if  SelectButtonAvailable() then
		-- Show the help text if it's available.  This should match ScreenSelectMusic::SelectMenuAvailable.
		if DifficultyChangingIsAvailable() or ModeMenuAvailable() then
			ret = ret .. "::" .. THEME:GetMetric( "ScreenSelectMusic", "SelectButtonAvailableHelpTextAppend" )
		end
	else
		if DifficultyChangingIsAvailable() then
			ret = ret .. "::" .. THEME:GetMetric( "ScreenSelectMusic", "DifficultyChangingAvailableHelpTextAppend" )
		end

		if ModeMenuAvailable() then
			ret = ret .. "::" .. THEME:GetMetric( "ScreenSelectMusic", "SortMenuAvailableHelpTextAppend" )
		end
	end

	if GAMESTATE:GetEnv("Workout") and IsHomeMode() then
		ret = ret .. "::" .. THEME:GetMetric( "ScreenSelectMusic", "WorkoutHelpTextAppend" )
	end

	return ret
end

-- function GetEvaluationHelpText() return THEME:GetMetric( "ScreenEvaluation", "HelpTextNormal" ) end
function GetEvaluationHelpText()
	local ret = THEME:GetMetric( "ScreenEvaluation", "HelpTextNormal" )

	if PROFILEMAN:IsPersistentProfile(PLAYER_1) or PROFILEMAN:IsPersistentProfile(PLAYER_2) then
		if SelectButtonAvailable() then
			ret = ret .. "::" .. THEME:GetMetric( "ScreenEvaluation", "TakeScreenshotHelpTextAppend" )
		else
			ret = ret .. "::" .. THEME:GetMetric( "ScreenEvaluation", "TakeScreenshotNoSelectHelpTextAppend" )
		end
	end

	return ret
end

function GetScreenPlayerOptionsHelpText()
	local ret = THEME:GetMetric( "ScreenPlayerOptions", "HelpTextNormal" )
	if SelectButtonAvailable() then
		ret = ret .. "::" .. THEME:GetMetric( "ScreenPlayerOptions", "SelectAvailableHelpTextAppend" )
	end
	return ret
end
	
function GetScreenSetTimeHelpText()
	if SelectButtonAvailable() then
		return THEME:GetMetric( "ScreenSetTime", "HelpTextSelectButtonAvailable" )
	else
		return THEME:GetMetric( "ScreenSetTime", "HelpTextSelectButtonUnavailable" )
	end
end

