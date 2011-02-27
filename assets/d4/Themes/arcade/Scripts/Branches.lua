function GetArcadeStartScreen()
	-- If we havn't loaded the input driver, do that first; until this finishes, we have
	-- no input or lights.
	if GetInputType() == "" then return "ScreenArcadeStart" end

	if not PROFILEMAN:GetMachineProfile():GetSaved().TimeIsSet then
		return "ScreenSetTime"
	end

	return "ScreenCompany"
end

function GetDiagnosticsScreen()
	return "ScreenArcadeDiagnostics"
end

function GetUpdateScreen()
	return "ScreenArcadePatch"
end

