--[[#!
	$info This script demonstrates basic graphic rendering
]]--#

function Render()
	local ClipRect = UIRect(0,0,0,0)
	local DrawRect = UIRect(0,0,0,0)
	local MainView = Game.Ui:Screen()
	Engine.Graphics:MapScreen(0,0, MainView.w, MainView.h)

	--MainView:Margin(10, MainView)
	MainView:Margin(200, ClipRect)
	ClipRect:VMargin(70, ClipRect)

	ClipRect.x = ClipRect.x + math.sin(Game.Client.LocalTime) * 150 + math.tan(Game.Client.LocalTime) * 50
	ClipRect.y = ClipRect.y + math.cos(Game.Client.LocalTime) * 150

	Game.Ui:ClipEnable(ClipRect)
	-- horizontal
	for i = 7, 1, -1 do
		MainView:HSplitTop(MainView.h/i, DrawRect, MainView)
		Game.RenderTools:DrawUIRect(DrawRect, vec4f(i%2, (i+1)%2, 0.1, 0.7), 15, 7)
	end
	-- vertical
	MainView = Game.Ui:Screen()
	for i = 7, 1, -1 do
		MainView:VSplitLeft(MainView.w/i, DrawRect, MainView)
		Game.RenderTools:DrawUIRect(DrawRect, vec4f(i%2, (i+1)%2, 0.1, 0.7), 15, 7)
	end
	Game.Ui:ClipDisable()
end


RegisterEvent("OnRenderLevel14", "Render")
