g_ScriptTitle = "Calculator"
g_ScriptInfo = "The classic calculator script! | by the AllTheHaxx-Team"

FORBIDDEN = {
	"game",
	"client",
	"os",
	"io",
	"shutdown",
	"open",
	"assert",
	"load",
	"print",
	"for",
	"while"
}

function Calc(ClientID, Team, Message)
	if(string.sub(Message, 1, string.len(".calc")) == ".calc") then
		x = string.sub(Message, string.len(".calc")+2, string.len(Message))
		
		illegal = false
		for k,v in next, FORBIDDEN do
			illegal = illegal or (x:lower():find(v) ~= nil)
		end
		
		if illegal then
			print("\tx = '" .. x .. "' is illegal")
			return
		end
		
		func = assert(load("return "..x))
		y = func()
		Game.Chat:Say(0, Game.Players(ClientID).Name .. ": " .. x.." = "..y)
		print("Result: "..x.." = "..y)
	end
end

RegisterEvent("OnChat", "Calc")
