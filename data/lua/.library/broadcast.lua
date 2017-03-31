-- provides broadcast-like messages for your script

local Broadcast = false
local BroadcastMessage = ""
local BroadcastDur = 0
local BroadcastTime = Game.Client.LocalTime
local BroadcastOffset = 50
local BroadcastTimer = 0
local BroadcastTimerWasSetted = false

function BroadcastStart(text, time, timer)
	if (Broadcast) then return end
	if timer == nil then	
		BroadcastTime = Game.Client.LocalTime
		Broadcast = true
		BroadcastMessage = text
		BroadcastDur = time
		BroadcastTimer = 0
		BroadcastTimerWasSetted = false
	else
		BroadcastTime = Game.Client.LocalTime
		Broadcast = true
		BroadcastMessage = text
		BroadcastDur = time
		BroadcastTimer = timer
		BroadcastTimerWasSetted = true
	end
end

function BroadcastReset()
	if (Broadcast) then	
		BroadcastTime = Game.Client.LocalTime-1
		Broadcast = false
		BroadcastMessage = ""
		BroadcastDur = 0
		BroadcastTimer = 0
	end
end

function BroadcastDraw()
	if (Broadcast) then
		if (BroadcastTimerWasSetted and round(BroadcastTime-Game.Client.LocalTime+BroadcastTimer,0) > 0 and BroadcastTime+BroadcastDur >= Game.Client.LocalTime) then
			Engine.Graphics:MapScreen(0,0,Engine.Graphics.ScreenWidth,Engine.Graphics.ScreenHeight) -- Simple broadcast screen.
			Engine.TextRender:Text(nil, Engine.Graphics.ScreenWidth/2-((Engine.TextRender:TextWidth(nil,BroadcastOffset,BroadcastMessage,-1,-1))/2), 50, 50.0, BroadcastMessage.." (".. round(BroadcastTime-Game.Client.LocalTime+BroadcastTimer,0) ..")", 0)
			return true -- Return true for working with this.
		elseif (BroadcastTimerWasSetted == false and BroadcastTime+BroadcastDur >= Game.Client.LocalTime) then
			Engine.Graphics:MapScreen(0,0,Engine.Graphics.ScreenWidth,Engine.Graphics.ScreenHeight) -- Simple broadcast screen.
			Engine.TextRender:Text(nil, Engine.Graphics.ScreenWidth/2-((Engine.TextRender:TextWidth(nil,BroadcastOffset,BroadcastMessage,-1,-1))/2), 50, 50.0, BroadcastMessage, 0)
			return true
		else
			Broadcast = false
			BroadcastReset()
			return false
		end
	end
end

RegisterEvent("OnRenderLevel15","BroadcastDraw")
