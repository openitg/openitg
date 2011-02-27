-- This is used both to fall back in EvaluationNextScreen(), and by ScreenAutoSaveAfterEvaluation
-- after autosaving.
-- EvaluationNextScreenNoAutosave = EvaluationNextScreen
function xxxEvaluationNextScreen()
	-- In event mode, autosave every three stages.
	local stage = GAMESTATE:StageIndex();
	Trace( "EvaluationNextScreen: stage " .. stage );
	if IsEventMode() and math.mod(stage,3) == 0 then
		Trace( "EvaluationNextScreen: Autosave" );
		return "ScreenPS2MemcardAutoSaveAfterEvaluation"
	else
		Trace( "EvaluationNextScreen: No autosave" );
		return EvaluationNextScreenNoAutosave
	end
end

