-- Probably remove this before release, devs wouldn't approve

local function RapidFire(Key)
	print("1")
	if(Key == "mouse1") then
		print("2")
		_game.controls.SetInput("Fire", _game.controls.GetInput("Fire")+1)
	end
end

RegisterEvent("OnKeyPress", RapidFire)