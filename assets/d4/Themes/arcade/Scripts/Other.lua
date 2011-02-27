function Platform() return "arcade" end

function SelectButtonAvailable()
	if GetInputType() == "PIUIO" then return true end
	return false
end
