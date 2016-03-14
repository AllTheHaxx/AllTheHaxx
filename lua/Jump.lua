g_ScriptTitle = "Jumpbot"
g_ScriptInfo = "Hold down Space to perform a perfect double jump! | by the AllTheHaxx-Team"

function Jump()
	Game.Input.Jump = 1
	Game.Input.Jump = 0
end

function ResetJump()
	Game.Input.Jump = 0
end

function GetPlayerY()
	return Game.LocalTee.Pos.y
end

function GetPlayerX()
	return Game.LocalTee.Pos.x
end

function GetPlayerVelY()
	return Game.LocalTee.Vel.y
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

SpacePressed = false
function OnKeyPress(k)
	if k == "space" then SpacePressed = true end
end
function OnKeyRelease(k)
	if k == "space" then SpacePressed = false end
end

function OnTick()
	if(SpacePressed == true and GetPlayerVelY() > -800 and GetPlayerVelY() < 0) then -- perfectjump
		Jump()
		ResetJump()
	end
end

RegisterEvent("OnTick", "OnTick")
RegisterEvent("OnKeyPress", "OnKeyPress")
RegisterEvent("OnKeyRelease", "OnKeyRelease")
