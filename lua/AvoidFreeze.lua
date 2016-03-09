g_ScriptTitle = "Avoid Freeze"
g_ScriptInfo = "Do not go into freeze on race maps"


TILE_FREEZE = -1

function Jump()
	Game.Input.Jump = 0
	Game.Input.Jump = 1
end

function GoRight()
	--Game.Input.Direction = 0
	Game.Input.Direction = 1
end

function GoLeft()
	--Game.Input.Direction = 0
	Game.Input.Direction = -1
end

function IsFreeze(x, y)
	return Game.Collision:GetTile(x, y) == TILE_FREEZE
end

NeedReset = false
function OnTick()
	gm = string.lower(Game.ServerInfo.GameMode)
	if(gm == "ddnet" or string.find(gm, "race") ~= nil) then
		TILE_FREEZE = 9
	end
	if(gm == "if|city") then
		TILE_FREEZE = 191
	end
	if(gm == "dm" or gm == "tdm" or gm == "ctf") then
		TILE_FREEZE = 2 -- kill
	end
	
	if(TILE_FREEZE < 0) then
		return
	end
	
	if(NeedReset == true) then
		TW.Game.Input.Jump = 0
		Game.Input.Direction = 0
		NeedReset = false
	end
	
	x = Game.LocalTee.Pos.x
	y = Game.LocalTee.Pos.y
	
	-- falling into it
	if(Game.LocalTee.Vel.y > 0 and IsFreeze(x, y+32*Game.LocalTee.Vel.y/15) and not IsFreeze(x, y)) then
		Jump()
		NeedReset = true
	end
	
	-- going to the right (doesn't work, dunno why)
	if(Game.LocalTee.Vel.x > 0 and IsFreeze(x+48, y)) then
		GoLeft()
		--NeedReset = true
	end
	
	-- going to the left (doesn't work, dunno why)
	if(Game.LocalTee.Vel.x < 0 and IsFreeze(x-48, y)) then -- going to the left
		GoRight()
		--NeedReset = true
	end
end

--[[function OnEnterGame()
	gm = string.lower(Game.ServerInfo.GameMode)
	if(gm ~= "ddnet" and string.find(gm, "race") == nil) then
		RemoveEvent("OnTick", "OnTick")
	else
		RegisterEvent("OnTick", "OnTick")
	end
end]]

RegisterEvent("OnTick", "OnTick")
