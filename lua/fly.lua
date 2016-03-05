g_ScriptTitle = "Fly"
g_ScriptInfo = "Life's a bitch until ya die, that's why we get high."

function OnTick()
	if(Game.LocalTee.Vel.y > 0) then
		Game.Input.Hook = 1
	else
		Game.Input.Hook = 0
	end
end

RegisterEvent("OnTick", "OnTick")
