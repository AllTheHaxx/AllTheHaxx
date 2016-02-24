local function test()
	print("width: ".._game.collision.GetMapWidth().." height: ".._game.collision.GetMapHeight())
end

RegisterEvent("OnRenderScoreboard", test)
