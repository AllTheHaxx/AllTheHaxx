LastTick = 0

a = true

function Jump()
	
	if _client.GetTick() < LastTick + 25 then
		return
	end
		
	if a == true then
		_game.controls.SetInput("Jump", 1)
		a = false
		LastTick = _client.GetTick()
	else
		_game.controls.SetInput("Jump", 0)
		a = true
		LastTick = _client.GetTick()
	end
end

RegisterEvent("OnTick", Jump)