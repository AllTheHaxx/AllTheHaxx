local function test()
	print("\n" .. client.GetTick() .. "\n")
end

RegisterEvent("OnChat", test)