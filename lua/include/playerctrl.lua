function IsGrounded()
	for i = 48, 32, -16 do
		c = Game.Collision:GetTile(Game.Local.Tee.PosX, Game.Local.Tee.PosY+i)
		if (c== 1 or c== 5) then
			return true
		end
		return false
	end
end

function Jump()
	TW.Game.Input.Jump = 1
end

function ResetJump()
	TW.Game.Input.Jump = 0
end

function Fire()
	TW.Game.Input.Fire = 1
end

function ResetFire()
	TW.Game.Input.Fire = 0
end

function GetPlayerY()
	return Game.Local.Tee.PosY
end
