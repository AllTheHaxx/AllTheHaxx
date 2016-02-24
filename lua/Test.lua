local function test()
	_components.chat.ChatSend(0, "DENNIS!")
end

RegisterEvent("OnChat", test)
