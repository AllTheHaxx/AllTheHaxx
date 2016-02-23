local function test(OldState, NewState)
	print("yo"..OldState..":"..NewState)
end

RegisterEvent("OnStateChange", test)