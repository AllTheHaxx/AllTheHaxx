-- Probably remove this before release, devs wouldn't approve
RapidFireActive = false

function RapidFire()
	if(RapidFireActive == true) then
		Game.Input.Fire = Game.Input.Fire + 1
	end
end

function RapidFireOn(Key)
	if(Key == "mouse1") then
		RapidFireActive = true
	end
end

function RapidFireOff(Key)
	if(Key == "mouse1") then
		RapidFireActive = false
	end
end

RegisterEvent("OnTick", "RapidFire")
RegisterEvent("OnKeyPress", "RapidFireOn")
RegisterEvent("OnKeyRelease", "RapidFireOff")
