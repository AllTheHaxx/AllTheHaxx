Events = {}

Events["OnEnterGame"] = {}
Events["OnChat"] = {}
Events["OnRenderBackground"] = {}

function RegisterEvent(EventName, Func)
	Events[EventName][Func] = Func
end