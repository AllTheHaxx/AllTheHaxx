InfoBoxStart = {} --Game.Client.LocalTime
InfoBoxDuration = 3 -- x in seconds
InfoBoxHeader = {} -- displayed header
InfoBoxText = {} -- displayed Text in the box
InfoBoxMode = {} -- 1 = creating, 2 = drawing_normal, 3 = deleting
InfoBoxRectHeader = {}
InfoBoxRectText = {}
InfoBoxMaxWidth = 200
InfoBoxMaxHeight = 100
InfoBoxHeightOffset = 10
InfoBoxFadeunits = 1 -- +- 1 Pixels per draw!
InfoBoxMaxFPS = Game.Client.FPS

function NewInfoBox(Start,Header,Text)
	table.insert(InfoBoxStart,Start)
	table.insert(InfoBoxHeader,Header)
	table.insert(InfoBoxText,Text)
end
function InfoBox()
	if InfoBoxMaxFPS < InfoBoxMaxFPS then InfoBoxMaxFPS = Game.Client.FPS end
	local Screen = Game.Ui:Screen()
    Engine.Graphics:MapScreen(0,0, Screen.w, Screen.h)
	for i = 1, #InfoBoxStart do
		if InfoBoxMode[i] == nil then 
			-- local Rect = UIRect(Screen.w-InfoBoxMaxWidth,InfoBoxHeightOffset,InfoBoxMaxWidth,InfoBoxMaxHeight)
			local Rect = UIRect(Screen.w,InfoBoxHeightOffset+((i-1)*InfoBoxMaxHeight),InfoBoxMaxWidth,InfoBoxMaxHeight)
			local Text = UIRect(0,0,0,0)
			local Header = UIRect(0,0,0,0)
			Rect:HSplitTop(Rect.h/5,Header,Text)
			InfoBoxRectText[i] = Text
			InfoBoxRectHeader[i] = Header
			InfoBoxMode[i] = 1 
		end
		if true then -- if true just for syntax
			local Rect = UIRect(Screen.w,InfoBoxHeightOffset+((i-1)*InfoBoxMaxHeight),InfoBoxMaxWidth,InfoBoxMaxHeight)
			local Text = UIRect(0,0,0,0)
			local Header = UIRect(0,0,0,0)
			Rect:HSplitTop(Rect.h/5,Header,Text)
			InfoBoxRectHeader[i].y = Header.y
			InfoBoxRectText[i].y = Text.y
		end
		if InfoBoxMode[i] == 1 then
			InfoBoxRectHeader[i] = UIRect(InfoBoxRectHeader[i].x-(InfoBoxFadeunits*InfoBoxMaxFPS/Game.Client.FPS),InfoBoxRectHeader[i].y,InfoBoxRectHeader[i].w,InfoBoxRectHeader[i].h)
			InfoBoxRectText[i] = UIRect(InfoBoxRectText[i].x-(InfoBoxFadeunits*InfoBoxMaxFPS/Game.Client.FPS),InfoBoxRectText[i].y,InfoBoxRectText[i].w,InfoBoxRectText[i].h)
			if Screen.w-InfoBoxRectText[i].x >= InfoBoxMaxWidth and Screen.w-InfoBoxRectHeader[i].x >= InfoBoxMaxWidth then
				InfoBoxMode[i] = 2
				InfoBoxRectText[i].x = Screen.w-InfoBoxMaxWidth
				InfoBoxRectHeader[i].x = Screen.w-InfoBoxMaxWidth
				InfoBoxStart[i] = Game.Client.LocalTime
			end
		elseif InfoBoxMode[i] == 2 then
			Game.RenderTools:DrawUIRect(InfoBoxRectHeader[i],vec4f(118/255, 230/255, 74/255,0.9),_CUI.CORNER_TL,5)
			Game.RenderTools:DrawUIRect(InfoBoxRectText[i],vec4f(0,0,0,0.9),_CUI.CORNER_BL,5)
		elseif InfoBoxMode[i] == 3 then
			InfoBoxRectHeader[i] = UIRect(InfoBoxRectHeader[i].x+(InfoBoxFadeunits*InfoBoxMaxFPS/Game.Client.FPS),InfoBoxRectHeader[i].y,InfoBoxRectHeader[i].w,InfoBoxRectHeader[i].h)
			InfoBoxRectText[i] = UIRect(InfoBoxRectText[i].x+(InfoBoxFadeunits*InfoBoxMaxFPS/Game.Client.FPS),InfoBoxRectText[i].y,InfoBoxRectText[i].w,InfoBoxRectText[i].h)
		end
		if InfoBoxMode[i] ~= nil then
			Game.RenderTools:DrawUIRect(InfoBoxRectHeader[i],vec4f(118/255, 230/255, 74/255,0.9),_CUI.CORNER_TL,5)
			Game.RenderTools:DrawUIRect(InfoBoxRectText[i],vec4f(0,0,0,0.9),_CUI.CORNER_BL,5)
			Engine.TextRender:TextColor(118/255, 230/255, 74/255,0.9)
			Engine.TextRender:TextOutlineColor(118/255, 230/255, 74/255,0.4)
			Game.Ui:DoLabelScaled(UIRect(InfoBoxRectText[i].x+5,InfoBoxRectText[i].y,InfoBoxRectText[i].w,InfoBoxRectText[i].h),InfoBoxText[i],InfoBoxRectHeader[i].h-5,-1,InfoBoxMaxWidth-20,"",nil)
			Engine.TextRender:TextColor(0,0,0,1)
			Engine.TextRender:TextOutlineColor(0.4,0.4,0.4,0.4)
			Game.Ui:DoLabelScaled(InfoBoxRectHeader[i],InfoBoxHeader[i],InfoBoxRectHeader[i].h-5,0,InfoBoxMaxWidth-20,"",nil)
			if InfoBoxStart[i]+InfoBoxDuration <= Game.Client.LocalTime then
				InfoBoxMode[i] = 3
			end
		end
	end
	for i = 1, #InfoBoxStart do
		if InfoBoxMode[i] == 3 and InfoBoxRectText[i].x >= Screen.w and InfoBoxRectHeader[i].x >= Screen.w then
			table.remove(InfoBoxStart,i)
			table.remove(InfoBoxHeader,i)
			table.remove(InfoBoxText,i)
			table.remove(InfoBoxMode,i)
			table.remove(InfoBoxRectHeader,i)
			table.remove(InfoBoxRectText,i)
		end
	end
end
RegisterEvent("OnRenderLevel23","InfoBox")