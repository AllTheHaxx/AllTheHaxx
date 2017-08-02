-------------------- COMMON FUNCTIONS FOR TEEWORLDS --------------------

function IsVanilla()
	local gm = string.lower(Game.ServerInfo.GameMode)
	return gm == "dm" or gm == "tdm" or gm == "ctf"
end

-- returns the ID of the player closest to you, -1 on failure.
function GetClosestID()
	local ClosestID = -1
	local ClosestDist = 1337*1337
	for i = 0, 64 do
		if Game.Players(i).Active == true then
			if i ~= Game.LocalCID then
				local dist = Game.Collision:Distance(Game.LocalTee.Pos, Game.Players(i).Tee.Pos)
				if dist < 650 then
					if ClosestID == -1 or dist < ClosestDist then
						if Game.Collision:IntersectLine(Game.LocalTee.Pos, Game.Players(i).Tee.Pos, nil, nil, false) == 0 then
							ClosestID = i
							ClosestDist = dist
						end
					end
				end
			end
		end
	end
	return ClosestID, ClosestDist
end

-- checks whether you are on the ground.
function IsGrounded(Tee)
    local Tee = Tee or Game.LocalTee
	local c = Game.Collision:GetTile(Tee.Pos.x, Tee.Pos.y+16)
	if (c == 1 or c == 5) then
		return true
	end
	return false
end

-- finds the ID of the player with the given name
function GetPlayerID(Name)
	for i = 0, 64 do
		if Game.Players(i).Name == Name then
			return i
		end
	end
end


function GetDirection(angle)
    local a = angle/256.0
    return vec2f(math.cos(a), math.sin(a))
end

function GetDir(a)
    return vec2f(math.cos(a), math.sin(a))
end

function GetAngle(DirVec)
    if DirVec.x == 0 and DirVec.y == 0 then return 0.0 end
    local a = math.atan(DirVec.y/DirVec.x)
    if DirVec.x < 0 then a = a+math.pi end
    return a
end

function GetAngleDeg(DirVec)
	local Angle = math.deg(GetAngle(DirVec))
	if Angle < 0 then Angle = Angle + 360 end
	return Angle
end


function IsFreezeTile(x, y)
	local TILE_FREEZE = -1
	local gm = string.lower(Game.ServerInfo.GameMode)
	if(gm == "idd32+" or gm == "ddnet" or string.find(gm, "race") ~= nil) then
		TILE_FREEZE = 9
	elseif(gm == "if|city") then
		TILE_FREEZE = 191
	elseif(gm == "dm" or gm == "tdm" or gm == "ctf") then
		TILE_FREEZE = 2 -- kill
	end	
	return Game.Collision:GetTile(x, y) == TILE_FREEZE
end

function IsFreezed(Id)
	Id = Id or Game.LocalCID
	if Game.CharSnap(Id).Cur.Weapon == 5 and not IsVanilla() then
		if IsFreezeTile(Game.Players(Id).Tee.Pos.x, Game.Players(Id).Tee.Pos.y) then
			return 2 -- frozen and in freeze
		else
			return 1 -- frozen but not in freeze
		end
	else
		return 0
	end
end
