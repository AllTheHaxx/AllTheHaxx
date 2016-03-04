_g_ScriptTitle = "Jumpbot"

function OnScriptInit()
	return _system.Import(_g_ScriptUID, "include/playerctrl.lua")
end

y = -1
function OnTick()
	if(IsGrounded() == true) then -- jump if we hit the ground
		y = GetPlayerY()
		Jump()
		--Fire()
	else
		ResetJump()
		--ResetFire()
		if(GetPlayerY() > y+12) then -- do doublejump if we dropped deeper than we jumped high
			Jump()
		end
	end
end

RegisterEvent("OnTick", "OnTick")
