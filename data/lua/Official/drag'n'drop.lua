g_ScriptTitle = "Drag em around!"
g_ScriptInfo = "(c) 2017 The AllTheHaxx Team"


Import("ui")

DEADZONE = 10

DRAG_INACTIVE=0
DRAG_START=1
DRAG_MOVING=2

Rects = {}
Dragging = DRAG_INACTIVE
DragOffset = vec2f(0,0)

function OnScriptInit()
	EnterFullscreen()
	return true
end

function OnEnterFullscreen()
	RegisterEvent("OnRenderLevel22", "Render")
	RegisterEvent("OnKeyPress", "KeyPress")
	RegisterEvent("OnKeyRelease", "KeyRelease")
end

function OnExitFullscreen()
	RemoveEvent("OnRenderLevel22", "Render")
	RemoveEvent("OnKeyPress", "KeyPress")
	RemoveEvent("OnKeyRelease", "KeyRelease")
end

function Render()
	local Screen = Game.Ui:Screen()
	Engine.Graphics:MapScreen(Screen.x, Screen.y, Screen.w, Screen.h)

	-- handle dragging (the currently dragged rect is always at the top)
	if Dragging == DRAG_START then
		-- consider the deadzone
		if Game.Collision:Distance(Game.Menus.MousePos, vec2f(Rects[#Rects].x, Rects[#Rects].y)+DragOffset) > DEADZONE then
			Dragging = DRAG_MOVING
			print("move", "left deadzone")
		end
	end
	if Dragging == DRAG_MOVING then
		Rects[#Rects].x = Game.Menus.MousePos.x/Game.Ui:Scale() - DragOffset.x
		Rects[#Rects].y = Game.Menus.MousePos.y/Game.Ui:Scale() - DragOffset.y
	end

	-- draw all
	for i in ipairs(Rects) do
		local Rect = Rects[i]
		local val = i / #Rects
		Game.RenderTools:DrawUIRect(Rect, vec4f(1-val,0,val,0.5), _CUI.CORNER_ALL, 5+5*math.sin(Game.Client.LocalTime * 2*math.pi * (Rect.x+Rect.y)/(Screen.w+Screen.h)))
	end

	-- rect at cursor for debugging mouse pos
	--local Rect = UIRect(Game.Menus.MousePos.x-24/2, Game.Menus.MousePos.y-24/2, 24, 24)
	--Game.RenderTools:DrawUIRect(Rect, vec4f(1,0,1,1), 0, 0)
end

function CreateRectAtMouse()
	local Dimensions = math.random(32, 64)
	local x = Game.Menus.MousePos.x/Game.Ui:Scale() - Dimensions/2
	local y = Game.Menus.MousePos.y/Game.Ui:Scale() - Dimensions/2

	table.insert(Rects, UIRect(x, y, Dimensions, Dimensions))

	print("create", "at " .. tostring(vec2f(x,y)) .. " #" .. #Rects)
end

function FindRectAtMouse()
	for i = #Rects, 1, -1 do -- find the top-most one
		if Game.Ui:MouseInside(Rects[i]) ~= 0 then
			return i
		end
	end
	return 0
end

function KeyPress(key)
	local Screen = Game.Ui:Screen()
	Engine.Graphics:MapScreen(0, 0, Screen.w, Screen.h)

	if key == "mouse1" then
		local Target = FindRectAtMouse()
		if Target > 0 then
			print("drag", "found rect " .. Target)
			-- bring it to the front
			table.insert(Rects, Rects[Target])
			table.remove(Rects, Target)
			-- enter dragging mode
			Dragging = DRAG_START
			DragOffset = Game.Menus.MousePos - vec2f(Rects[#Rects].x, Rects[#Rects].y)
		end
	elseif key == "mouse2" then
		if Dragging ~= DRAG_INACTIVE then
			table.remove(Rects, #Rects)
			Dragging = DRAG_INACTIVE
			print("remove", "dragged")
		else
			local ID = FindRectAtMouse()
			if ID > 0 then
				table.remove(Rects, ID)
				print("remove", ID)
			else
				CreateRectAtMouse()
			end
		end
	end
end

function KeyRelease(key)
	if key == "mouse1" and Dragging ~= DRAG_INACTIVE then
		Dragging = DRAG_INACTIVE
		print("move", "dragging end")
	end
end
