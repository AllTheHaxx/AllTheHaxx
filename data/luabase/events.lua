Events = {}
Threads = {}    --3D Table which contains the 'threadobject' and its pause stuff

-- YOU SHOULD NOT TOUCH THIS FUNCTION IF YOU DON'T KNOW WHAT YOU ARE DOING!
function RegisterEvent(EventName, ...)
    FuncNames = {...}
    if #FuncNames < 1 then
        return
    end
    
    if Events[EventName] == nil then  --create new table if it's empty
        Events[EventName] = {}
    end
    
    for i, FuncName in next, FuncNames do
        Func = getfenv()[FuncName]
        
        if Func ~= nil then
            Events[EventName][FuncName] = Func
        end
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

function RegisterThread(FuncName)
    Func = getfenv()[FuncName]
    
    if Func ~= nil then
        if Threads[Func] == nil then  --create new table if it's empty
            Threads[Func] = {}
        end
        
        Threads[Func].Routine = coroutine.create(Func)
        Threads[Func].ResTick = 0
        Threads[Func].ResSec = 0
        
        --FOLLOWING CODE IS INFINITE LOOP PROTECTION!
        --This creates a hook which can register up to 10000 events; if that value is exceeded, the script is cancelled!
        --if there is some sort of sleeping function in that (so no infinite loop) then coroutine.resume will exit and the hook thus deleted!
        debug.sethook(function() print("Please, don't use infinite loops as they will crash your client!") error("Infinite Loop") end, "", 20000)
        
        coroutine.resume(Threads[Func].Routine)
        
        debug.sethook()
        
        --END OF INFINITE LOOPS
        --this stuff is genius mate
    end
end

function ResumeThreads()
    for script, struct in pairs(Threads) do
        --ticksleep!
        if struct.ResTick > 0 and struct.ResTick <= Game.Client.Tick then
            struct.ResTick = 0
            coroutine.resume(Threads[script].Routine)
        elseif struct.ResSec > 0 and struct.ResSec <= os.clock() then
            struct.ResSec = 0
            coroutine.resume(Threads[script].Routine)
        end
    end
end

function thread_sleep_ticks(num)
    Threads[debug.getinfo(2).func].ResTick = Game.Client.Tick + num    
    coroutine.yield()
end

function thread_sleep_ms(num)
    Threads[debug.getinfo(2).func].ResSec = os.clock() + num/1000
    coroutine.yield()
end
