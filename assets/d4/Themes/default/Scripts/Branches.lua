function ScreenTitleBranch()
	if GAMESTATE:GetCoinMode() == COIN_MODE_HOME then return "ScreenTitleMenu" end
	if GAMESTATE:IsEventMode() then return "ScreenEventMenu" end
	return "ScreenTitleJoin"
end

function EvaluationNextScreen()
	Trace( "GetGameplayNextScreen: " )
	Trace( " AllFailed = "..tostring(AllFailed()) )
	Trace( " IsEventMode = "..tostring(GAMESTATE:IsEventMode()) )
	Trace( " IsFinalStage = "..tostring(IsFinalStage()) )

	if GAMESTATE:IsEventMode() then return SongSelectionScreen() end
	if AllFailed() or IsFinalStage() then return "ScreenNameEntryTraditional" end
	return SongSelectionScreen();
end

function GetGameplayNextScreen()
	Trace( "GetGameplayNextScreen: " )
	Trace( " AllFailed = "..tostring(AllFailed()) )
	Trace( " IsEventMode = "..tostring(GAMESTATE:IsEventMode()) )
	Trace( " IsSyncDataChanged = "..tostring(GAMESTATE:IsSyncDataChanged()) )

	if GAMESTATE:IsSyncDataChanged() then 
		return "ScreenSaveSync"
	end
		
	-- Never show evaluation for training.
	if GAMESTATE:GetCurrentSong():GetSongDir() == "Songs/In The Groove/Training1/" then 
		if GAMESTATE:IsEventMode() then 
			return SongSelectionScreen()
		else
			return EvaluationNextScreen()
		end
	elseif AllFailed() and not GAMESTATE:IsCourseMode() then 
		if GAMESTATE:IsEventMode() then 
			return SelectEvaluationScreen()
		else
			return "ScreenNameEntryTraditional"
		end
	else 
		return SelectEvaluationScreen() 
	end
	
	return "GetGameplayNextScreen: YOU SHOULD NEVER GET HERE"
end

function SelectEndingScreen()
	if GAMESTATE:GetEnv("ForceGoodEnding") == "1" or GetBestFinalGrade() <= GRADE_TIER05 then return "ScreenEndingGood" end
	return "ScreenEndingNormal"
end

function ScreenAfterGameplayWorkout()
	if GAMESTATE:GetPlayMode() == PLAY_MODE_NONSTOP then return "ScreenEvaluationNonstopWorkout" end
	if GAMESTATE:GetPlayMode() == PLAY_MODE_ENDLESS then return "ScreenEvaluationNonstopWorkout" end
	return "ScreenEvaluationStageWorkout"
end

function GetScreenEvaluationNonstopWorkoutNextScreen()
	if GAMESTATE:GetPlayMode() == PLAY_MODE_ENDLESS then return "ScreenWorkoutMenu" end
	return "ScreenSelectMusicCourse"
end

function GetGameplayScreen()
	local Song = GAMESTATE:GetCurrentSong();
	if Song and Song:GetSongDir() == "Songs/In The Groove/Training1/" then
		return "ScreenGameplayTraining"
	end

	return "ScreenGameplay"
end

function SongSelectionScreen()
	local s = "ScreenSelectMusic";
	if GAMESTATE:IsCourseMode() then s = s.."Course" end
	return s
end

function ScreenSelectMusicPrevScreen()
	if GAMESTATE:GetEnv("Workout") then return "ScreenWorkoutMenu" end
	return ScreenTitleBranch()
end

function OptionsMenuAvailable()
	if GAMESTATE:IsExtraStage() or GAMESTATE:IsExtraStage2() then return false end
	if GAMESTATE:GetPlayMode()==PLAY_MODE_ONI then return false end
	return true
end

function GetSetTimeNextScreen()
	-- This is called only when we move to the next screen, so we only mark the time set
	-- when the screen is cleared.  That way, if the game is started by the operator and
	-- powered down before setting the screen, we still go to ScreenSetTime on the next boot.
	PROFILEMAN:GetMachineProfile():GetSaved().TimeIsSet = true
	PROFILEMAN:SaveMachineProfile()

	return "ScreenOptionsMenu"
end

function GetDiagnosticsScreen()
	return "ScreenOptionsMenu"
end

function GetUpdateScreen()
	return "ScreenOptionsMenu"
end
