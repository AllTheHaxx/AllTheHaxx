Events = {}

-- YOU SHOULD NOT TOUCH THIS FUNCTION IF YOU DON'T KNOW WHAT YOU ARE DOING!
function RegisterEvent(EventName, FuncName)
	if Events[EventName] == nil then  --create new table if it's empty
		Events[EventName] = {}
	end
	
	Func = getfenv()[FuncName]
	
	if Func ~= nil then
		Events[EventName][FuncName] = Func
	end
end

function RemoveEvent(EventName, FuncName)
	if Events[EventName] ~= nil then
		Events[EventName][FuncName] = nil
	end
end

function EventList(EventName)
	if Events[EventName] ~= nil then
		for name, func in pairs(Events[EventName]) do
			print(" -> " .. name)
		end
	else 
		print("No Functions found under this Eventname.")	
	end
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

function OnChatSend(Team, Msg)
	if Events["OnChatSend"] ~= nil then
		for script, event in pairs(Events["OnChatSend"]) do
			if event ~= nil then
				event(Team, Message)
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

function OnKeyRelease(Key)
	if Events["OnKeyRelease"] ~= nil then
		for script, event in pairs(Events["OnKeyRelease"]) do
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

function OnRenderScoreboard(FadeVal)
	if Events["OnRenderScoreboard"] ~= nil then
		for script, event in pairs(Events["OnRenderScoreboard"]) do
			if event ~= nil then
				event(FadeVal)
			end
		end
	end
end

function OnSnapInput()
	if Events["OnSnapInput"] ~= nil then
		for script, event in pairs(Events["OnSnapInput"]) do
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

function OnTick()
	if Events["OnTick"] ~= nil then
		for script, event in pairs(Events["OnTick"]) do
			if event ~= nil then
				event()
			end
		end
	end
end
