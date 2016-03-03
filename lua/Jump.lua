LastTick = 0

a = true

function Jump()
	
	if Game.Client.Tick < LastTick + 28 then
		return
	end
		
	if a == true then
		Game.Input.Jump = 1
		a = false
		LastTick = Game.Client.Tick
	else
		Game.Input.Jump = 0
		a = true
		LastTick = Game.Client.Tick
	end
end

RegisterEvent("OnTick", Jump)