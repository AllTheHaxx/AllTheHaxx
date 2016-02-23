local tex = graphics.LoadTexture("arrow.png", -1, -1, 1)

local function test()
	graphics.BlendNormal()
	
	x = graphics.GetScreenWidth()/2 - graphics.GetScreenWidth()/16
	y = graphics.GetScreenHeight()/2 - graphics.GetScreenHeight()/16
	w = graphics.GetScreenWidth()/4
	h = graphics.GetScreenHeight()/4
	graphics.RenderTexture(tex, x, y, w, h, client.GetTick()/100)
	
	graphics.SetColor(1, 0, 0, 0.3)
	x = graphics.GetScreenWidth()/2 + graphics.GetScreenWidth()/4
	y = graphics.GetScreenHeight()/2 + graphics.GetScreenHeight()/4
	w = graphics.GetScreenWidth()/4
	h = graphics.GetScreenHeight()/4
	graphics.RenderTexture(tex, x, y, w, h, -client.GetTick()/100)
	
end

RegisterEvent("OnRenderLevel20", test) -- render level 20 is right before the scoreboard
