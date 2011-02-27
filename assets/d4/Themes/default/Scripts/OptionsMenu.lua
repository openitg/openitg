function OptionsRowWeight()
	local function IndexToPounds(i)
		return i*5
	end
	
	local function AllChoices()
		local ret = { }
		for i = 1,100 do ret[i] = IndexToPounds(i).." Lbs" end
		return ret
	end

	local t = 
	{
		-- Name is used to retrieve the header and explanation text.
		Name = "Weight",
		LayoutType = "ShowOneInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = false,
		Choices = AllChoices(),
		LoadSelections = function(self, list, pn)
			local val = PROFILEMAN:GetProfile(pn):GetWeightPounds()
			if val <= 0 then val = 100 end
			for i = 1,table.getn(self.Choices) do
				if val == IndexToPounds(i) then
					list[i] = true
					return
				end
			end
			list[20] = true -- 100 lbs
		end,
		SaveSelections = function(self, list, pn)
			for i = 1,table.getn(self.Choices) do
				if list[i] then
					PROFILEMAN:GetProfile(pn):SetWeightPounds( IndexToPounds(i) )
					return
				end
			end
		end,
	}	
	setmetatable( t, t )
	return t
end

function OptionsRowGoalCalories()
	local function IndexToCalories(i)
		return i*10+20
	end
	
	local function AllChoices()
		local ret = { }
		for i = 1,98 do ret[i] = IndexToCalories(i).." cals" end
		return ret
	end

	local t = 
	{
		-- Name is used to retrieve the header and explanation text.
		Name = "GoalCalories",
		LayoutType = "ShowOneInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = AllChoices(),
		EnabledForPlayers = GetPlayersWithGoalType( GOAL_CALORIES ),
		ReloadRowMessages = { "GoalTypeChanged" },
		LoadSelections = function(self, list, pn)
			local val = PROFILEMAN:GetProfile(pn):GetGoalCalories()
			for i = 1,table.getn(self.Choices) do
				if val == IndexToCalories(i) then
					list[i] = true
					return
				end
			end
			list[13] = true	-- 150 cals
		end,
		SaveSelections = function(self, list, pn)
			for i = 1,table.getn(self.Choices) do
				if list[i] then
					PROFILEMAN:GetProfile(pn):SetGoalCalories( IndexToCalories(i) )
					return
				end
			end
		end,
	}	
	setmetatable( t, t )
	return t
end

function GetPlayersWithGoalType( gt )
	local t = { }
	for pn = PLAYER_1,NUM_PLAYERS-1 do 
		if GAMESTATE:IsHumanPlayer(pn) and WorkoutGetProfileGoalType(pn) == gt then 
			t[pn] = pn 
		end
	end
	return t
end

function OptionsRowGoalSeconds()
	local function IndexToSeconds(i)
		return i*60+4*60
	end
	
	local function AllChoices()
		local ret = { }
		for i = 1,56 do ret[i] = (IndexToSeconds(i)/60).." mins" end
		return ret
	end

	local t = 
	{
		-- Name is used to retrieve the header and explanation text.
		Name = "GoalTime",
		LayoutType = "ShowOneInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = false,
		ExportOnChange = true,
		Choices = AllChoices(),
		EnabledForPlayers = GetPlayersWithGoalType( GOAL_TIME ),
		ReloadRowMessages = { "GoalTypeChanged" },
		LoadSelections = function(self, list, pn)
			local val = PROFILEMAN:GetProfile(pn):GetGoalSeconds()
			for i = 1,table.getn(self.Choices) do
				if val == IndexToSeconds(i) then
					list[i] = true
					return
				end
			end
			list[6] = true	-- 10 mins
		end,
		SaveSelections = function(self, list, pn)
			for i = 1,table.getn(self.Choices) do
				if list[i] then
					PROFILEMAN:GetProfile(pn):SetGoalSeconds( IndexToSeconds(i) )
					return
				end
			end
		end,
	}	
	setmetatable( t, t )
	return t
end

function GetDefaultSort() return "EasyMeter" end

function OptionsRowSort()
	local Sorts = 
	{
		-- Disable "Group" for v1.
		-- "Group",
		"Title",
		"BPM",
		"Popularity",
		"TopGrade",
		"Artist",
		"EasyMeter",
		"MediumMeter",
		"HardMeter",
		"ChallengeMeter",
	}

	local t = 
	{
		Name = "DefaultSort",
		SaveTo = PROFILEMAN:GetMachineProfile():GetSaved(),

		LayoutType = "ShowOneInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = false,

		Default = GetDefaultSort(),
		Choices = TableMetricLookup( Sorts, "SortOrder" ), -- themed
		RawChoices = Sorts, -- not themed

		__index = OptionRowTable
	}

	setmetatable( t, t )
	return t
end

function GetDefaultDifficulty() return "Beginner" end

function OptionsRowDifficulty()
	local Difficulties = 
	{
		"Beginner",
		"Easy",
		"Medium",
		"Hard",
		"Challenge",
	}

	local t = 
	{
		Name = "DefaultDifficulty",
		SaveTo = PROFILEMAN:GetMachineProfile():GetSaved(),

		LayoutType = "ShowAllInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = false,

		Default = GetDefaultDifficulty(),
		Choices = TableMetricLookup( Difficulties, "Difficulty" ), -- themed
		RawChoices = Difficulties, -- not themed

		__index = OptionRowTable
	}

	setmetatable( t, t )
	return t
end

function OptionsRowJukeboxCourses()	
	local function AllChoices()
		local t = SONGMAN:GetAllCourses(false)
		local ret = { }
		for i = 1,table.getn(t) do 
			if t[i]:GetPlayMode() == PLAY_MODE_ONI then
				ret[i] = t[i]:GetDisplayFullTitle()
			end
		end
		return ret
	end

	local t = 
	{
		-- Name is used to retrieve the header and explanation text.
		Name = "OptionsRowJukeboxCourses",
		LayoutType = "ShowOneInRow",
		SelectType = "SelectOne",
		OneChoiceForAllPlayers = true,
		ExportOnChange = false,
		Choices = AllChoices(),
		LoadSelections = function(self, list, pn)
			local c = GAMESTATE:GetCurrentCourse()
			if c == nil then 
				list[1] = true
				return 
			end
			local title = c:GetDisplayFullTitle()
			local t = self.Choices
			for i = 1,table.getn(list) do
				if title == t[i] then
					list[i] = true
					return
				end
			end
		end,
		SaveSelections = function(self, list, pn)
			local t = self.Choices
			for i = 1,table.getn(list) do
				if list[i] then
					local c = SONGMAN:FindCourse( t[i] );
					GAMESTATE:SetCurrentCourse( c )
					return
				end
			end
		end,
	}	
	setmetatable( t, t )
	return t
end

function ApplyOptions()
	local sort = PROFILEMAN:GetMachineProfile():GetSaved()["DefaultSort"] or GetDefaultSort()
	GAMESTATE:ApplyGameCommand("sort," .. sort)
	Trace( "ApplyOptions: sort " .. sort )

	-- This is applied by GetInitialDifficulty().
--	local difficulty = PROFILEMAN:GetMachineProfile():GetSaved()["DefaultDifficulty"] or GetDefaultDifficulty()
--	GAMESTATE:ApplyGameCommand("difficulty," .. difficulty)
--	Trace( "ApplyOptions: difficulty " .. difficulty )
end

