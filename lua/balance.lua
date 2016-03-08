g_ScriptTitle = "Balance"

function GetClosestID()
	ClosestID = -1
	ClosestDist = 100
	for i = 0, 15 do
		if Game.Players(i).Active == true then
			if i ~= Game.LocalCID then
				if Game.Collision:Distance(Game.LocalTee.Pos, Game.Players(i).Tee.Pos) < 650 then
					if ClosestID == -1 or Game.Collision:Distance(Game.LocalTee.Pos, Game.Players(i).Tee.Pos) < ClosestDist then
						if Game.Collision:IntersectLine(Game.LocalTee.Pos, Game.Players(i).Tee.Pos, nil, nil, false) == 0 then
							ClosestID = i
							ClosestDist = Game.Collision:Distance(Game.LocalTee.Pos, Game.Players(i).Tee.Pos);
						end
					end
				end
			end
		end
	end
	return ClosestID
end

Dir = 0
Jump = 0
DoIt = false
function OnSnapInput()
	if(DoIt == true) then
		Game.Input.Direction = Dir
		Game.Input.Jump = Jump
	end
end

function OnTick()
	c = GetClosestID()
	if(c == nil or c < 0 or Game.Collision:IntersectLine(Game.LocalTee.Pos, Game.Players(c).Tee.Pos, nil, nil, false) ~= 0) then
		DoIt = false
		return
	end
	DoIt = true
	if(Game.LocalTee.Vel.y == 0) then -- a little Jump.lua to get onto the tees
		Jump = 1
	else
		Jump = 0
	end
	
	if(Game.LocalTee.Pos.x > Game.Players(c).Tee.Pos.x) then
		Dir = -1
		return
	end
	if(Game.LocalTee.Pos.x < Game.Players(c).Tee.Pos.x) then
		Dir = 1
		return
	end
	Dir = 0
end
