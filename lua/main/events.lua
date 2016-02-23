Events = {}

-- YOU SHOULD NOT TOUCH THIS FUNCTION IF YOU DON'T KNOW WHAT YOU ARE DOING!
function RegisterEvent(EventName, Func)
	if Events[EventName] == nil then  --create new table if its empty
		Events[EventName] = {}
	end
	
	Events[EventName][Func] = Func
end

function OnChat(ID, Team, Message)
	if Events["OnChat"] ~= nil then
		for script, event in pairs(Events["OnChat"]) do
			if event ~= nil then
				event(ID, Team, Message)
			end
		end
	end
	
	return 2
end

function OnEnterGame()
	if Events["OnEnterGame"] ~= nil then
		for script, event in pairs(Events["OnEnterGame"]) do
				event()
		end
	end
end

function OnKill(Killer, Victim, Weapon)
	if Events["OnKill"] ~= nil then
		for script, event in pairs(Events["OnKill"]) do
				event(Killer, Victim, Weapon)
		end
	end
end

function OnMessageIRC(From, User, Message)
	if Events["OnMessageIRC"] ~= nil then
		for script, event in pairs(Events["OnMessageIRC"]) do
				event(From, User, Message)
		end
	end
end

function OnRenderBackground()
	if Events["OnRenderBackGround"] ~= nil then
		for script, event in pairs(Events["OnRenderBackground"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnRenderScoreboard()
	if Events["OnRenderScoreboard"] ~= nil then
		for script, event in pairs(Events["OnRenderScoreboard"]) do
				event()
		end
	end
end

function OnStateChange(NewState, OldState)
	if Events["OnStateChange"] ~= nil then
		for script, event in pairs(Events["OnStateChange"]) do
				event(NewState, OldState)
		end
	end
end
