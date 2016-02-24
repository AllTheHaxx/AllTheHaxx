local function test()
	print("index: ".._game.collision.GetTile(0,0))
end

RegisterEvent("OnRenderScoreboard", test)
