function OnEnterGame()
	for script, event in pairs(Events["OnEnterGame"]) do
		if event ~= nil then
			event()
		end
	end
end

function OnChat(ID, Team, Message)
	for script, event in pairs(Events["OnChat"]) do
		if event ~= nil then
			event(ID, Team, Message)
		end
	end
end