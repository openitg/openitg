function SelectButtonAvailable()
	return true
end

-- Hide the timer if "MenuTimer" is disabled
function HideTimer()
	local enabled = PREFSMAN:GetPreference("MenuTimer")
	if enabled then return "0" else return "1" end
end
