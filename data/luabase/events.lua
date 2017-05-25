local __CTRL = {}
__CTRL.Events = {}
__CTRL.Threads = {}	--3D Table which contains the 'threadobject' and its pause stuff

-- YOU SHOULD NOT TOUCH THIS FUNCTION IF YOU DON'T KNOW WHAT YOU ARE DOING!
function RegisterEvent(EventName, ...)
	local FuncNames = {...}
    if type(EventName) ~= "string" then
        error("RegisterEvent expects a string as first argument", 2)
        return
    end

	if #FuncNames < 1 then
		error("RegisterEvent(\"" .. EventName .. ") expects two or more arguments", 2)
		return
	end

	if __CTRL.Events[EventName] == nil then  --create new table if it's empty
		__CTRL.Events[EventName] = {}
	end

	for i, FuncName in next, FuncNames do
		local Func
		if type(FuncName) == "function" then
			Func = FuncName
		elseif type(FuncName) == "string" then
			Func = getfenv()[FuncName]
			FuncName = #__CTRL.Events[EventName] + 1
		else
			error("RegisterEvent expects only strings and/or functions as variadic arguments (got " .. type(FuncName) .. ")", 2)
		end

		if Func ~= nil then  -- Func can only be nil when given as a string
			__CTRL.Events[EventName][FuncName] = Func
		else
			error("Cannot register global '" .. FuncName .. "' (a nil value) for event " .. EventName, 2)
		end
	end
end

function RemoveEvent(EventName, ...)
	local FuncNames = {...}
	if #FuncNames < 1 or __CTRL.Events[EventName] == nil then
		return
	end

	for i, FuncName in next, FuncNames do
--[[		if type(FuncName) == "function" then
			for i,func in ipairs(__CTRL.Events[EventName]) do
				if func == FuncName then
					table.remove(__CTRL.Events[EventName], i)
					break
				end
			end
		else]]if type(FuncName) == "string" then
			__CTRL.Events[EventName][FuncName] = nil
		else
			error("RemoveEvent expects only strings as variadic arguments (got " .. type(FuncName) .. ")", 2)
		end
	end
end

function EventList(EventName)
	if type(EventName) ~= "string" then
		error("EventList expects a string as its argument (got " .. type(FuncName) .. ")", 2)
	end
	if __CTRL.Events[EventName] ~= nil then
		for name, func in pairs(__CTRL.Events[EventName]) do
			print(" -> " .. name)
		end
	else
		print("No Functions registered for event '" .. EventName .. "'")
	end
end

function OnChat(ID, Team, Message)
	local ret = false
	if __CTRL.Events["OnChat"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnChat"]) do
			if event ~= nil then
				ret = ret or event(ID, Team, Message)
			end
		end
	end
	return ret
end

function OnChatSend(Team, Msg)
	local ret = false
	if __CTRL.Events["OnChatSend"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnChatSend"]) do
			if event ~= nil then
				ret = ret or event(Team, Msg)
			end
		end
	end
	return ret
end

function OnConfigOpen(x, y, w, h)
	if __CTRL.Events["OnConfigOpen"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnConfigOpen"]) do
			if event ~= nil then
				event(x, y, w, h)
			end
		end
	end
end

function OnConfigClose()
	if __CTRL.Events["OnConfigClose"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnConfigClose"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnConsoleCommand(Command)
	if __CTRL.Events["OnConsoleCommand"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnConsoleCommand"]) do
			if event ~= nil then
				event(Command)
			end
		end
	end
end

function OnEnterGame()
	if __CTRL.Events["OnEnterGame"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnEnterGame"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnFlagGrab(PlayerID, TeamID)
	if __CTRL.Events["OnFlagGrab"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnFlagGrab"]) do
			if event ~= nil then
				event(PlayerID, TeamID)
			end
		end
	end
end

function OnGameStart()
	if __CTRL.Events["OnGameStart"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnGameStart"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnGameOver()
	if __CTRL.Events["OnGameOver"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnGameOver"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnKeyPress(Key)
	if __CTRL.Events["OnKeyPress"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnKeyPress"]) do
			if event ~= nil then
				event(Key)
			end
		end
	end
end

function OnKeyRelease(Key)
	if __CTRL.Events["OnKeyRelease"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnKeyRelease"]) do
			if event ~= nil then
				event(Key)
			end
		end
	end
end

function OnKill(Killer, Victim, Weapon)
	if __CTRL.Events["OnKill"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnKill"]) do
			if event ~= nil then
				event(Killer, Victim, Weapon)
			end
		end
	end
end

function OnMessageIRC(From, User, Message)
	if __CTRL.Events["OnMessageIRC"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnMessageIRC"]) do
			if event ~= nil then
				event(From, User, Message)
			end
		end
	end
end

function OnRenderBackground()
	if __CTRL.Events["OnRenderBackGround"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnRenderBackground"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnRenderLevel(level)
	if __CTRL.Events["OnRenderLevel" .. level] ~= nil then
		for script, event in pairs(__CTRL.Events["OnRenderLevel" .. level]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnInputLevel(level, KeyName, EventTable)
	if __CTRL.Events["OnInputLevel" .. level] ~= nil then
		for script, event in pairs(__CTRL.Events["OnInputLevel" .. level]) do
			if event ~= nil then
				return event(KeyName, EventTable)
			end
		end
	end
end

function OnRenderScoreboard(FadeVal)
	if __CTRL.Events["OnRenderScoreboard"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnRenderScoreboard"]) do
			if event ~= nil then
				event(FadeVal)
			end
		end
	end
end

function OnPredHammerHit(PlayerID)
	if __CTRL.Events["OnPredHammerHit"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnPredHammerHit"]) do
			if event ~= nil then
				event(PlayerID)
			end
		end
	end
end

function PreRenderPlayer(ID, PosX, PosY, DirX, DirY, OtherTeam)
	if __CTRL.Events["PreRenderPlayer"] ~= nil then
		for script, event in pairs(__CTRL.Events["PreRenderPlayer"]) do
			if event ~= nil then
				event(ID, PosX, PosY, DirX, DirY, OtherTeam)
			end
		end
	end
end

function PostRenderPlayer(ID, PosX, PosY, DirX, DirY, OtherTeam)
	if __CTRL.Events["PostRenderPlayer"] ~= nil then
		for script, event in pairs(__CTRL.Events["PostRenderPlayer"]) do
			if event ~= nil then
				event(ID, PosX, PosY, DirX, DirY, OtherTeam)
			end
		end
	end
end

function OnSnapInput()
	if __CTRL.Events["OnSnapInput"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnSnapInput"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function OnStateChange(NewState, OldState)
	if __CTRL.Events["OnStateChange"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnStateChange"]) do
			if event ~= nil then
				event(NewState, OldState)
			end
		end
	end
end

function OnTick()
	if __CTRL.Events["OnTick"] ~= nil then
		for script, event in pairs(__CTRL.Events["OnTick"]) do
			if event ~= nil then
				event()
			end
		end
	end
end

function RegisterThread(FuncName, argtable)
	local Func = getfenv()[FuncName]

	if Func ~= nil then
		if __CTRL.Threads[Func] == nil then  --create new table if it's empty
			__CTRL.Threads[Func] = {}
		end

		__CTRL.Threads[Func].Routine = coroutine.create(Func)
		__CTRL.Threads[Func].ResTick = 0
		__CTRL.Threads[Func].ResSec = 0

		--FOLLOWING CODE IS INFINITE LOOP PROTECTION!
		--This creates a hook which can register up to 10000 events; if that value is exceeded, the script is cancelled!
		--if there is some sort of sleeping function in that (so no infinite loop) then coroutine.resume will exit and the hook thus deleted!
		debug.sethook(function() print("Please, don't use infinite loops as they will crash your client!") error("Infinite Loop", 2) end, "", 10000)

		coroutine.resume(__CTRL.Threads[Func].Routine)

		debug.sethook()

		--END OF INFINITE LOOPS
		--this stuff is genius mate
	end
end

function ResumeThreads()
	for script, struct in pairs(__CTRL.Threads) do
		--ticksleep!
		if struct.ResTick > 0 and struct.ResTick <= Game.Client.Tick then
			struct.ResTick = 0
			coroutine.resume(__CTRL.Threads[script].Routine)
		elseif struct.ResSec > 0 and struct.ResSec <= os.clock() then
			struct.ResSec = 0
			coroutine.resume(__CTRL.Threads[script].Routine)
		end
	end
end

function RemoveThread(Name)
	local Func = getfenv()[Name]
	__CTRL.Threads[Func] = nil
end

function thread_sleep_ticks(num)
	__CTRL.Threads[debug.getinfo(2).func].ResTick = Game.Client.Tick + num
	coroutine.yield()
end

function thread_sleep_ms(num)
	__CTRL.Threads[debug.getinfo(2).func].ResSec = os.clock() + num/1000
	coroutine.yield()
end
