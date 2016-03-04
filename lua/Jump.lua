LastTick = 0
a = true

function Jump()
	
	if TW.Client.Tick < LastTick + 28 then
		return
	end
		
	if a == true then
		Game.Input.Jump = 1
		a = false
		LastTick = TW.Client.Tick
	else
		TW.Game.Input.Jump = 0
		a = true
		LastTick = TW.Client.Tick
	end
end

RegisterEvent("OnTick", "Jump")