--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script 2")

local function test()
	w = _graphics.GetScreenWidth()
	h = _graphics.GetScreenHeight()
	
	_ui.SetUiColor(0,0,0,0.3)
	_ui.DrawUiRect(0, 0, w, h, 0, 0)
end

RegisterEvent("OnRenderScoreboard", test)
