--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function Greet(ClientID, Team, Message)
	print(Game.Chat.Mode)
	if ClientID ~= -1 then  --only server messages
		return
	end
	
	if Message:find("entered and joined") and ClientID ~= _client.GetLocalCharacterID() then
	
		abc = string.match(Message, "'.-'")    -- returns everything between '' => name
		
		if math.random(1, 5) == 1 then
			Game.Chat:Say(0, abc:gsub("'", "") .. ": DENNIS!")
		else
			Game.Chat:Say(0, "Hello " .. abc:gsub("'", "") .. "! Nice to see you!")
		end
	end
	
	--Game.Chat:Say(0, "Hi")
	-- or Game.Client.Chat:Say(0, "Hi"), you can decide :D
end

RegisterEvent("OnChat", Greet)
--Game.Client.Chat:Say(0, "Script 'SuperMegaTestScript.lua' loaded")
