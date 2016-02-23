function OnEnterGame()
	print("Calling OnEnterGames...")

	for script, event in pairs(Events["OnEnterGame"]) do
		if event ~= nil then
			event()
		end
	end
end