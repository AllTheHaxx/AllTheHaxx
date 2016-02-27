--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

List = {}
JoinTick = 0

local function GreetAdd(ClientID, Team, Message)
	if ClientID ~= -1 or _client.GetTick() < JoinTick+20 then  --only server messages
		return
	end

	if Message:find("entered and joined") then
		abc = string.match(Message, "'.-'")    -- returns everything between '' => name
		abc = abc:gsub("'", "")                      -- string replace
				
		if abc ~= _client.GetPlayerName(Client.Local.CID) then
			List[abc] = _client.GetTick() + 30        --time stamp
		end
	end
end

local function Greet()
	for key,value in pairs(List) do     --key = name, value = timestamp
		if _client.GetTick() >= value then
			if math.random(1, 5) == 1 then
				Game.Chat:Say(0, key .. ": DENNIS!")
			else
				Game.Chat:Say(0, "Hello " .. key .. "! Nice to see you!")
			end
			
			List[key] = nil
		end
	end
end

local function SetJoinTick()
	JoinTick = _client.GetTick()
end

RegisterEvent("OnChat", GreetAdd)
RegisterEvent("OnTick", Greet)
RegisterEvent("OnEnterGame", SetJoinTick)