--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test()
	_game.controls.LockInput();
end

RegisterEvent("OnRenderScoreboard", test)
