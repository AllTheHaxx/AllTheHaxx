--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test()
	_game.controls.LockInput()
	_game.controls.SetInput("TargetX", 200)
	_game.controls.SetInput("Fire", _game.controls.GetInput("Fire")+1)
end

RegisterEvent("OnRenderScoreboard", test)
