_g_ScriptTitle = "Jumpbot"
_g_ScriptInfo = "Hold down Space to make a perfect double jump!"

function Jump()
	TW.Game.Input.Jump = 1
end

function ResetJump()
	TW.Game.Input.Jump = 0
end
--[[
function Fire()
	TW.Game.Input.Fire = 1
end

function ResetFire()
	TW.Game.Input.Fire = 0
end
]]
function GetPlayerY()
	return Game.Local.Tee.PosY
end

function GetPlayerX()
	return Game.Local.Tee.PosX
end
--[[
function GetPlayerVelX()
	return Game.Local.Tee.VelX
end
]]
function GetPlayerVelY()
	return Game.Local.Tee.VelY
end

function IsGrounded()
	for i = 48, 32, -16 do
		c = Game.Collision:GetTile(GetPlayerX(), GetPlayerY())
		if (c== 1 or c== 5) then
			return true
		end
		return false
	end
end

--y = -1
function OnTick()
	--if(IsGrounded() == true) then -- jump if we hit the ground
	--	y = GetPlayerY()
	--	Jump()
		--Fire()
	--else
		--ResetJump()
		--ResetFire()
		--print(math.abs(GetPlayerVelY()))
		--print(GetPlayerVelY()/32)
		if(Game.Local.Tee.Jumped == 1 and GetPlayerVelY() > -800 and GetPlayerVelY() < 0) then -- perfectjump
			Jump()
			ResetJump()
		end
	--	if(GetPlayerY() > y+12) then -- do doublejump if we would drop to death
	--		Jump()
	--	end
	--end
end

RegisterEvent("OnTick", "OnTick")
