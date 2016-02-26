--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test()
	--print("ID 0 Name: ".._client.GetPlayerName(0))
	
	Game.Chat:Say(0, "Hi")
	-- or Game.Client.Chat:Say(0, "Hi"), you can decide :D
end

RegisterEvent("OnRenderScoreboard", test)
