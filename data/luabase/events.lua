Events = {}

-- YOU SHOULD NOT TOUCH THIS FUNCTION IF YOU DON'T KNOW WHAT YOU ARE DOING!
function RegisterEvent(EventName, Func)
	if Events[EventName] == nil then  --create new table if it's empty
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
end

function OnConfigOpen(x, y, w, h)
	if Events["OnConfigOpen"] ~= nil then
		for script, event in pairs(Events["OnConfigOpen"]) do
			if event ~= nil then
				event(x, y, w, h)
			end
		end
	end
end

function OnConfigClose()
	if Events["OnConfigClose"] ~= nil then
		for script, event in pairs(Events["OnConfigClose"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnEnterGame()
	if Events["OnEnterGame"] ~= nil then
		for script, event in pairs(Events["OnEnterGame"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnKeyPress(Key)
	if Events["OnKeyPress"] ~= nil then
		for script, event in pairs(Events["OnKeyPress"]) do
			if event ~= nil then
				event(Key)
			end
		end
	end
end

function OnKill(Killer, Victim, Weapon)
	if Events["OnKill"] ~= nil then
		for script, event in pairs(Events["OnKill"]) do
			if event ~= nil then
				event(Killer, Victim, Weapon)
			end
		end
	end
end

function OnMessageIRC(From, User, Message)
	if Events["OnMessageIRC"] ~= nil then
		for script, event in pairs(Events["OnMessageIRC"]) do
			if event ~= nil then
				event(From, User, Message)
			end
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

function OnRenderLevel(level)
	if Events["OnRenderLevel" .. level] ~= nil then
		for script, event in pairs(Events["OnRenderLevel" .. level]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnRenderScoreboard()
	if Events["OnRenderScoreboard"] ~= nil then
		for script, event in pairs(Events["OnRenderScoreboard"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnStateChange(NewState, OldState)
	if Events["OnStateChange"] ~= nil then
		for script, event in pairs(Events["OnStateChange"]) do
			if event ~= nil then
				event(NewState, OldState)
			end
		end
	end
end
