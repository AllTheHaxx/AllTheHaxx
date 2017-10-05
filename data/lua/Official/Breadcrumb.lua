g_ScriptTitle = "Breadcrumb"
g_ScriptInfo = "Draw a cool line behind your movement. | (c) YoungFlyme 2017"

-- Global Variables
MAX_LINES = 50 -- ADJUST: higher = longer lines, less performance; set to 100 for epic flow :D
Lines = {}
LastCoord = vec2f(Game.LocalTee.Pos.x,Game.LocalTee.Pos.y)
LinesM = {} -- 'M' stands for mouse
LastCoordM = vec2f(Game.Input.MouseX+Game.LocalTee.Pos.x,Game.Input.MouseY+Game.LocalTee.Pos.y)
LinesD = {}

function OnTick()
	-- only add lines if we are moving
	if (#Lines < MAX_LINES and LastCoord ~= Game.LocalTee.Pos) then
		table.insert(Lines, LineItem(LastCoord.x,LastCoord.y,Game.LocalTee.Pos.x,Game.LocalTee.Pos.y)) -- add a new line
		LastCoord = Game.LocalTee.Pos -- save the new line's endpoint as the starting point of the next line
	end
	-- remove old lines if we have got too many (emulates ringbuffer-behavior)
	if (#Lines >= MAX_LINES) then
		table.remove(Lines, 1)
	end
	
	-- the same as above, but for the mouse drawing
	if (#LinesM < MAX_LINES and LastCoordM ~= vec2f(Game.Input.MouseX,Game.Input.MouseY)+Game.LocalTee.Pos) then
		CurrX = Game.Input.MouseX+Game.LocalTee.Pos.x
		CurrY = Game.Input.MouseY+Game.LocalTee.Pos.y
		table.insert(LinesM, LineItem(LastCoordM.x, LastCoordM.y, CurrX, CurrY))
		LastCoordM = vec2f(CurrX, CurrY) -- save the new line's endpoint as the starting point of the next line
	end
	-- remove old lines if we have got too many (emulates ringbuffer-behavior)
	if (#LinesM >= MAX_LINES) then
		table.remove(LinesM, 1)
	end
end

function DrawIt()
	if (#Lines > 0) then
		Engine.Graphics:TextureSet(-1)
		Engine.Graphics:LinesBegin()
			Engine.Graphics:SetColor(0.5, 0.7, 0.3, 1)
			Engine.Graphics:LinesDraw(Lines)
		Engine.Graphics:LinesEnd()
	end
end

function DrawItM()
	if (#LinesM > 0) then
		Engine.Graphics:TextureSet(-1)
		Engine.Graphics:LinesBegin()
			Engine.Graphics:SetColor(0.2, 0.3, 0.7, 1)
			Engine.Graphics:LinesDraw(LinesM)
		Engine.Graphics:LinesEnd()
	end
end



RegisterEvent("OnTick", "OnTick")
RegisterEvent("PreRenderPlayer", "DrawIt") -- the tee's movement line will be drawn below the map and players
RegisterEvent("OnRenderLevel12", "DrawItM") -- the mouse line will be drawn onto the map