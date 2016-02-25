--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test()
	print("ID 0 Name: ".._client.GetPlayerName(0))
end

RegisterEvent("OnRenderScoreboard", test)
