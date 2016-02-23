Events = {}

Events["OnEnterGame"] = {}
Events["OnChat"] = {}

function RegisterEvent(EventName, Func)
	Events[EventName][Func] = Func
end