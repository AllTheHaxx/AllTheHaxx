Events = {}

Events["OnEnterGame"] = {}

function RegisterEvent(EventName, Func)
	Events[EventName][Func] = Func
end