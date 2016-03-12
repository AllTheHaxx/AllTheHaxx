g_ScriptTitle = "Fly"
g_ScriptInfo = "Life's a bitch until ya die, that's why we get high. | by the AllTheHaxx-Team"

function OnTick()
	Game.Input.Hook = ((Game.LocalTee.Vel.y > 0) and 1 or 0)
end

RegisterEvent("OnTick", "OnTick")
