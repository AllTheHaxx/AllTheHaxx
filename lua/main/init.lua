Events = {}

Events["OnChat"] = {}
Events["OnEnterGame"] = {}
Events["OnKill"] = {}
Events["OnRenderBackground"] = {}

function RegisterEvent(EventName, Func)
	Events[EventName][Func] = Func
end