--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test()
	_game.controls.LockInput()
	_game.controls.SetInput("Direction", 1)
end

RegisterEvent("OnRenderScoreboard", test)
