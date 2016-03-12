g_ScriptTitle = "Walkbot"
g_ScriptInfo = "Walkwalkwalkwalk | by the AllTheHaxx-Team"

local LEFT = -1
local RIGHT = 1

NeedReset = false
WalkDir = 1
Jumped = 0

function Jump()
	Game.Input.Jump = 1
	NeedReset = true
end

function Reset()
	Game.Input.Jump = 0
	NeedReset = false
end

function IsSolid(x, y)
	c = Game.Collision:GetTile(x, y)
	return ( c == 1 or c == 2 or c == 3 )
end

function OnSnapInput()
	Game.Input.Direction = WalkDir
end

LastCall = 0
function OnTick()
	if(NeedReset == true) then
		Reset()
	end
	
	if(Game.Client.LocalTime < LastCall + 0.7) then
		return
	end
	LastCall = Game.Client.LocalTime
	
	
	x = Game.LocalTee.Pos.x
	y = Game.LocalTee.Pos.y

	
	Game.Input.Direction = WalkDir
	
	if(math.abs(Game.LocalTee.Vel.x) > 2) then
		return -- only need to do the following checks if we stand still
	end
	
	if (math.abs(Game.LocalTee.Vel.y) > 2) then -- we jump against a wall -> dj
		Jump()
		return
	end
	
	if (Jumped == 0) then 
		Jump()
		Jumped = 1
	else
		WalkDir = 0 - WalkDir
		Jumped = 0
	end

end

--RegisterEvent("OnSnapInput", "OnSnapInput")
RegisterEvent("OnTick", "OnTick")
