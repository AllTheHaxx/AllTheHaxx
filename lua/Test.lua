local function test()
	_client.Connect("127.0.0.1:8303")
end

RegisterEvent("OnRenderScoreboard", test)
