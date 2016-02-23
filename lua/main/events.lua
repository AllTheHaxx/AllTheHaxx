function OnChat(ID, Team, Message)
	for script, event in pairs(Events["OnChat"]) do
		if event ~= nil then
			event(ID, Team, Message)
		end
	end
end

function OnEnterGame()
	for script, event in pairs(Events["OnEnterGame"]) do
		if event ~= nil then
			event()
		end
	end
end

function OnKill(Killer, Victim, Weapon)
	for script, event in pairs(Events["OnKill"]) do
		if event ~= nil then
			event(Killer, Victim, Weapon)
		end
	end
end

function OnRenderBackground()
	for script, event in pairs(Events["OnRenderBackground"]) do
		if event ~= nil then
			event()
		end
	end
end
