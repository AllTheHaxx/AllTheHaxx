Events = {}

Events["OnChat"] = {}
Events["OnEnterGame"] = {}
Events["OnKill"] = {}
Events["OnRenderBackground"] = {}
Events["OnStateChange"] = {}
Events["OnScoreboardRender"] = {}

function RegisterEvent(EventName, Func)
	Events[EventName][Func] = Func
end