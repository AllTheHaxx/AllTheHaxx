-- Probably remove this before release, devs wouldn't approve
RapidFireActive = false

local function RapidFire()
	if(RapidFireActive == true) then
		_game.controls.SetInput("Fire", _game.controls.GetInput("Fire")+1)
	end
end

local function RapidFireOn(Key)
	if(Key == "mouse1") then
		RapidFireActive = true
	end
end

local function RapidFireOff(Key)
	if(Key == "mouse1") then
		RapidFireActive = false
	end
end

RegisterEvent("OnRenderLevel1", RapidFire)
RegisterEvent("OnKeyPress", RapidFireOn)
RegisterEvent("OnKeyRelease", RapidFireOff)
