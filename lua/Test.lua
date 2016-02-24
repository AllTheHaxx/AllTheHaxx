local function test()
	if(_components.chat.Active()) then
		print("yo")
	end
end

RegisterEvent("OnRenderLevel1", test)
