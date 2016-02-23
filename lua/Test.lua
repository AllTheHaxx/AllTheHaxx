local function test(Killer, Victim, Weapon)
	print("yo"..Weapon)
end

RegisterEvent("OnKill", test)