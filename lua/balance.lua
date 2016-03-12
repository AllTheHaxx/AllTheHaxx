g_ScriptTitle = "Balance"
g_ScriptInfo = "Select the tee you want to balance on using your hook | by the AllTheHaxx-Team"

Dir = 0
DoIt = false
function OnSnapInput()
	if(DoIt == true) then
		Game.Input.Direction = Dir
	end
end

c = -1
function OnTick()
-- selection through hook
	if(Game.Input.Hook == 1) then
		c = Game.LocalTee.HookedPlayer
	end
	
-- is anybody selected? Can we get to him?
	if(c == nil or c < 0 or Game.Collision:IntersectLine(Game.LocalTee.Pos, Game.Players(c).Tee.Pos, nil, nil, false) ~= 0) then
		DoIt = false
		return
	end

	DoIt = true
	
-- balance
	if(Game.LocalTee.Pos.x - Game.Players(c).Tee.Pos.x > 5) then
		Dir = -1
		return
	end
	if(Game.Players(c).Tee.Pos.x - Game.LocalTee.Pos.x > 5) then
		Dir = 1
		return
	end
	Dir = 0
end