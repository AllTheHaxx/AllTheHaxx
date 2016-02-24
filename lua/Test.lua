local tex = _graphics.LoadTexture("arrow.png", -1, -1, 1)

local function test()
	_graphics.BlendNormal()

	x = _graphics.GetScreenWidth()/2 - _graphics.GetScreenWidth()/16
	y = _graphics.GetScreenHeight()/2 - _graphics.GetScreenHeight()/16
	w = _graphics.GetScreenWidth()/4
	h = _graphics.GetScreenHeight()/4

	_ui.SetUiColor(0.2, 0.2, 0.7, 0.5)
	_ui.DrawUiRect(x, y, w, h, 0, 0)

	_graphics.SetColor(1, 0, 0, 1)
	_graphics.RenderTexture(tex, x, y, w, h, -_client.GetTick()/100)
end

RegisterEvent("OnRenderScoreboard", test) -- render level 20 is right before the scoreboard
