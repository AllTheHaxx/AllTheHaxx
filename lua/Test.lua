local function test()
	_game.emote.Send(2)
end

RegisterEvent("OnChat", test)
