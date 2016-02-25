--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test()
	print("FPS: ".._client.GetFPS())
end

RegisterEvent("OnRenderScoreboard", test)
