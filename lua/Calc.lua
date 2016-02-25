local function Calc(ClientID, Team, Message)
	if(string.sub(Message, 1, string.len(".calc")) == ".calc") then
		x = string.sub(Message, string.len(".calc"), string.len(Message))
		func = assert(load("return " .. x))
		y = func()
		_game.chat.Send(0, "Result: "..y)
		print("Result: "..y)
	end
end

RegisterEvent("OnChat", Calc)