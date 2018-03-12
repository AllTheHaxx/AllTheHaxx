g_ScriptTitle = "Hud"
g_ScriptInfo = "Moba like Hud. Make with ♥ by YoungFlyme"
--[[#!
	#os
	#ffi
]]--#
-- Simple and efficient.

--[[
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	!			Currently no downloaded skins 				!
	!					are working 						!
	!	to make them work copy them into the skins folder 	!
	!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
]]
MaxHealth = 10
MaxArmor = 10
LastShot = {0,0,0,0,0}
LastTexture = Config.tex_game
WEP_COLORS = {
	[0] = vec4f(0.8, 0.8, 0.1,1),
	[1] = vec4f(0.8, 0.8, 0.8,1),
	[2] = vec4f(0.6, 0.5, 0.2,1),
	[3] = vec4f(0.8, 0.1, 0.0,1),
	[4] = vec4f(0.2, 0.1, 0.7,1)
}

Emote = {
	EMOTE_NORMAL = 1,
	EMOTE_PAIN = 2,
	EMOTE_HAPPY = 3,
	EMOTE_SURPRISE = 4,
	EMOTE_ANGRY = 5,
	EMOTE_BLINK = 6
}

NinjaTime = 0

FPS_Delay = Game.Client.LocalTime + 0.05
FPS_Num = Game.Client.FPS

ScoreFade = 0

TeeSkin = {}

TeeStats = {}

DBGTex = -1

 
function OnScriptInit()
	Config.cl_showhud = 0
	Tex = Engine.Graphics:LoadTexture("data/textures/game/".. Config.tex_game ..".png",-1,-1,1)
	if Import("ui") and Import("math") then
		return Import("sprites")
	else
		return false
	end
end
 
function DrawHud()
	if Game.Client.State ~= 3 then return end
	local DisplayID = GetDisplayID()

	CheckTexture()

	CheckMaxHealthArmor(DisplayID)

	-- Real Hud begins here	
	local Screen = Game.Ui:Screen()
	Engine.Graphics:MapScreen(0,0, Screen.w, Screen.h)

	-- Initialize all needed UIRect's
	local MainView = Screen:copy()
	local Stats = UIRect(0,0,0,0)
	local Time = UIRect(0,0,0,0)
	local Life = UIRect(0,0,0,0)
	local Armor = UIRect(0,0,0,0)
	local Ammo = UIRect(0,0,0,0)
	local Vote = UIRect(0,0,0,0)
	local FPS = UIRect(0,0,0,0)
	local Score = UIRect(0,0,0,0)

	MainView:HSplitTop(MainView.h/100, MainView, Stats)
	--MainView:VSplitLeft(MainView.w/3.75, Rect, MainView)

	--[[
		print(math.floor(Game.Client.PredGameTick/50%60))
		print(math.floor(Game.Client.PredGameTick/50/60%60))
		print(math.floor(Game.Client.PredGameTick/50/60/60%24))
		print(math.floor(Game.Client.PredGameTick/50/60/60%24),math.floor(Game.Client.PredGameTick/50/60%60),math.floor(Game.Client.PredGameTick/50%60))

	]]
	
	-- This gets drawed, no matter if we dont play or specate someone
	MainView:VSplitLeft(MainView.w/13,Time,MainView)
	Time:HSplitBottom(Time.h-55, Time, nil)
	Time:HSplitTop(Time.h/2, Time, Config.cl_showfps == 1 and FPS or Score)
	if Config.cl_showfps == 1 then
		FPS:HSplitTop(FPS.h/2,FPS,Score)
	else
		Score:HSplitTop(Score.h/2,Score,nil)
	end
	Game.RenderTools:DrawUIRect(Time, vec4f(0,0,0,0.3), (Config.cl_showfps == 1 or DisplayID ~= -1) and _CUI.CORNER_T or _CUI.CORNER_ALL, 10)
	Engine.TextRender:Text(nil,Time.x+Time.w/2-((Engine.TextRender:TextWidth(nil,10,("%02d:%02d:%02d"):format(os.date("*t").hour, os.date("*t").min, os.date("*t").sec),-1,-1))/2),Time.y+Time.h/2-10/1.5,10,("%02d:%02d:%02d"):format(os.date("*t").hour, os.date("*t").min, os.date("*t").sec),-1)
	Engine.TextRender:Text(nil,Screen.w/2+((Engine.TextRender:TextWidth(nil,20,("%02d:%02d:%02d"):format(math.floor(Game.Client.PredGameTick/50/60/60%24),math.floor(Game.Client.PredGameTick/50/60%60),math.floor(Game.Client.PredGameTick/50%60)),-1,-1))/2),Time.y+Time.h/2-10/1.5,20,("%02d:%02d:%02d"):format(math.floor(Game.Client.PredGameTick/50/60/60%24),math.floor(Game.Client.PredGameTick/50/60%60),math.floor(Game.Client.PredGameTick/50%60)),-1)
	if Config.cl_showfps == 1 then
		Game.RenderTools:DrawUIRect(FPS, vec4f(0,0,0,0.3), DisplayID ~= -1 and _CUI.CORNER_NONE or _CUI.CORNER_B, 10)
		if FPS_Delay <= Game.Client.LocalTime then
			FPS_Num = Game.Client.FPS
			FPS_Delay = Game.Client.LocalTime + 0.05
		end
		Engine.TextRender:TextColor(1,1,0.5,1)
		Engine.TextRender:Text(nil,FPS.x+FPS.w/2-((Engine.TextRender:TextWidth(nil,10,"FPS: 100",-1,-1))/2),FPS.y+FPS.h/2-10/1.5,10,"FPS: "..FPS_Num,-1)
		Engine.TextRender:TextColor(1,1,1,1)-- Reset Color here o.O
	end

	if DisplayID ~= -1 then
		-- Add score if we can!
		Score.h = Score.h + 7
		local ScorePoints = _client.GetPlayerScore(DisplayID)
		Engine.TextRender:TextColor(0.5,1,0.5,1)
		Game.RenderTools:DrawUIRect(Score, vec4f(0,0,0,0.3), _CUI.CORNER_B, 10)
		Engine.TextRender:Text(nil,Score.x+Score.w/2-((Engine.TextRender:TextWidth(nil,10,"Score: "..ScorePoints,-1,-1))/2),Score.y+Score.h/2-10/1.5,10,"Score: "..ScorePoints,-1)
		Engine.TextRender:TextColor(1,1,1,1)-- Reset Color here o.O

		local PanelColor = vec4f(0.1,0.1,0.1,1);

		-- Bottom panel
		Stats:HSplitBottom(15,Stats,nil)
		Stats:HSplitBottom(Screen.h/5,nil,Stats)
		local Chat = UIRect(0,0,0,0)
		Stats:VSplitLeft(Stats.w/2.30,Chat,Stats)

		-- Tee window
		if (not Scoreboard) then
			local TeeWindow = UIRect(0,0,0,0)
			Stats:VSplitRight(Stats.w/5,Stats,TeeWindow)
			Game.RenderTools:DrawUIRect(TeeWindow, PanelColor, _CUI.CORNER_ALL, 10)
			if (TeeStats[DisplayID] ~= nil) then
				Game.Ui:DoLabelScaled(TeeWindow,Game.Players(DisplayID).Name,15,0,TeeWindow.w,Game.Players(Game.LocalCID).Name)
				DrawTee(TeeWindow,DisplayID)
				TeeWindow:HSplitBottom(TeeWindow.h/5,nil,TeeWindow)
				Game.Ui:DoLabelScaled(TeeWindow,Game.Players(DisplayID).SkinName,15,0,TeeWindow.w,nil)
			end
			local HookedTee = UIRect(0,0,0,0)
			Stats:VSplitLeft(Stats.h+20,HookedTee,Stats)
			if Game.LocalTee.HookedPlayer ~= -1 then
				Game.RenderTools:DrawUIRect(HookedTee, PanelColor, _CUI.CORNER_ALL, 10)
				Game.Ui:DoLabelScaled(HookedTee,Game.Players(Game.LocalTee.HookedPlayer).Name,15,0,TeeWindow.w,Game.Players(Game.LocalCID).Name)
				DrawTee(HookedTee,Game.LocalTee.HookedPlayer)
				HookedTee:HSplitBottom(HookedTee.h/5,nil,HookedTee)
				Game.Ui:DoLabelScaled(HookedTee,Game.Players(DisplayID).SkinName,15,0,TeeWindow.w,nil)
			end
		else
			Stats:VSplitLeft(Stats.w/1.8,nil,Stats)
		end

		-- Stats Margin for little spacing
		Stats:VMargin(5,Stats)
		Game.RenderTools:DrawUIRect(Stats, PanelColor, _CUI.CORNER_ALL, 10)

		-- Annother Margin to not overdraw the corners
		Stats:Margin(5,Stats)
		-- Main box
		local LifeBar = UIRect(0,0,0,0)
		local ArmorBar = UIRect(0,0,0,0)
		local WeaponBoxes = UIRect(0,0,0,0)
		local IconSize = 28
		if Scoreboard then IconSize = IconSize/1.4 end
		Stats:HSplitTop(Stats.h/2.25,LifeBar,Stats)
		-- Life and Armor
		LifeBar:VSplitLeft(LifeBar.w/2,LifeBar,ArmorBar)
		if Game.CharSnap(DisplayID).Cur.Health/10 <= 1 then
			Engine.Graphics:TextureSet(Tex)
			Engine.Graphics:QuadsBegin()
				Game.RenderTools:SelectSprite(SPRITE_HEALTH_FULL,0,0,0)
				for i=1,Game.CharSnap(DisplayID).Cur.Health do
					Game.RenderTools:DrawSprite(LifeBar.x+(i-1)*IconSize/1.75+16,LifeBar.y+LifeBar.h/2,IconSize)
				end
			Engine.Graphics:QuadsEnd()
		else
			local Rendered = 0
			for l=1,2 do
				Engine.Graphics:TextureSet(Tex)
				Engine.Graphics:QuadsBegin()
					Game.RenderTools:SelectSprite(SPRITE_HEALTH_FULL,0,0,0)
					for i=1,(Game.CharSnap(DisplayID).Cur.Health > 20 and 10 or Game.CharSnap(DisplayID).Cur.Health-Rendered) do
						Rendered = Rendered+1
						Game.RenderTools:DrawSprite(LifeBar.x+(i-1)*IconSize/1.75+16,LifeBar.y+LifeBar.h/2+ (l==1 and -LifeBar.h/4 or LifeBar.h/4),IconSize)
						if i == 10 then break end
					end
				Engine.Graphics:QuadsEnd()
			end
		end
		if Game.CharSnap(DisplayID).Cur.Armor/10 <= 1 then
			Engine.Graphics:TextureSet(Tex)
			Engine.Graphics:QuadsBegin()
				Game.RenderTools:SelectSprite(SPRITE_ARMOR_FULL,0,0,0)
				for i=1,Game.CharSnap(DisplayID).Cur.Armor do
					Game.RenderTools:DrawSprite(ArmorBar.x+ArmorBar.w-((i-1)*IconSize/1.75+16),ArmorBar.y+ArmorBar.h/2,IconSize)
				end
			Engine.Graphics:QuadsEnd()
		else
			local Rendered = 0
			for l=1,2 do
				Engine.Graphics:TextureSet(Tex)
				Engine.Graphics:QuadsBegin()
					Game.RenderTools:SelectSprite(SPRITE_ARMOR_FULL,0,0,0)
					for i=1,(Game.CharSnap(DisplayID).Cur.Armor > 20 and 10 or Game.CharSnap(DisplayID).Cur.Armor-Rendered) do
						Rendered = Rendered+1
						Game.RenderTools:DrawSprite(ArmorBar.x+ArmorBar.w-((i-1)*IconSize/1.75+16),ArmorBar.y+ArmorBar.h/2+ (l==1 and -ArmorBar.h/4 or ArmorBar.h/4),IconSize)
						if i == 10 then break end
					end
				Engine.Graphics:QuadsEnd()
			end
		end
		--Game.RenderTools:DrawUIRect(Stats, vec4f(0,0,0,1), _CUI.CORNER_ALL, 10)
		for i=6,1,-1 do
			local Rect = UIRect(0,0,0,0)
			Stats:VSplitLeft(Stats.w/(i-1), Rect, Stats)
			if i == 2 then
				if Game.CharSnap(DisplayID).Cur.Weapon == 4 then
					Game.RenderTools:DrawUIRect(Rect, vec4f(1,1,1,0.5), _CUI.CORNER_R, 10.0)
					Engine.TextRender:Text(nil,Rect.x+Rect.w/3,Rect.y+Rect.h/1.45,10,Game.CharSnap(DisplayID).Cur.Ammo,-1)
					if Game.CharSnap(DisplayID).Cur.AttackTick >= Game.Client.Tick - (Game.Tuning().laser_fire_delay.Value/2200) then -- Why dafuq is the 2200?
						Engine.TextRender:Text(nil,Rect.x+Rect.w/2,Rect.y,10,round((Game.CharSnap(DisplayID).Cur.AttackTick-(Game.Client.Tick-(Game.Tuning().laser_fire_delay.Value/2200)))/50,2),-1)
					end
				else
					Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.5), _CUI.CORNER_R, 10.0)
				end
			elseif i == 3 then
				if Game.CharSnap(DisplayID).Cur.Weapon == 3 then
					Game.RenderTools:DrawUIRect(Rect, vec4f(1,1,1,0.5), _CUI.CORNER_NONE, 10.0)
					Engine.TextRender:Text(nil,Rect.x+Rect.w/3,Rect.y+Rect.h/1.45,10,Game.CharSnap(DisplayID).Cur.Ammo,-1)
					if Game.CharSnap(DisplayID).Cur.AttackTick >= Game.Client.Tick - (Game.Tuning().grenade_fire_delay.Value/2200) then -- Why dafuq is the 2200?
						Engine.TextRender:Text(nil,Rect.x+Rect.w/2,Rect.y,10,round((Game.CharSnap(DisplayID).Cur.AttackTick-(Game.Client.Tick-(Game.Tuning().grenade_fire_delay.Value/2200)))/50,2),-1)
					end
				else
					Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.5), _CUI.CORNER_NONE, 10.0)
				end
			elseif i == 4 then
				if Game.CharSnap(DisplayID).Cur.Weapon == 2 then
					Game.RenderTools:DrawUIRect(Rect, vec4f(1,1,1,0.5), _CUI.CORNER_NONE, 10.0)
					Engine.TextRender:Text(nil,Rect.x+Rect.w/3,Rect.y+Rect.h/1.45,10,Game.CharSnap(DisplayID).Cur.Ammo,-1)
					if Game.CharSnap(DisplayID).Cur.AttackTick >= Game.Client.Tick - (Game.Tuning().shotgun_fire_delay.Value/2200) then -- Why dafuq is the 2200?
						Engine.TextRender:Text(nil,Rect.x+Rect.w/2,Rect.y,10,round((Game.CharSnap(DisplayID).Cur.AttackTick-(Game.Client.Tick-(Game.Tuning().shotgun_fire_delay.Value/2200)))/50,2),-1)
					end
				else
					Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.5), _CUI.CORNER_NONE, 10.0)
				end
			elseif i == 5 then
				if Game.CharSnap(DisplayID).Cur.Weapon == 1 then
					Game.RenderTools:DrawUIRect(Rect, vec4f(1,1,1,0.5), _CUI.CORNER_NONE, 10.0)
					Engine.TextRender:Text(nil,Rect.x+Rect.w/3,Rect.y+Rect.h/1.45,10,Game.CharSnap(DisplayID).Cur.Ammo,-1)
					if Game.CharSnap(DisplayID).Cur.AttackTick >= Game.Client.Tick - (Game.Tuning().gun_fire_delay.Value/2200) then -- Why dafuq is the 2200?
						Engine.TextRender:Text(nil,Rect.x+Rect.w/2,Rect.y,10,round((Game.CharSnap(DisplayID).Cur.AttackTick-(Game.Client.Tick-(Game.Tuning().gun_fire_delay.Value/2200)))/50,2),-1)
					end
				else
					Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.5), _CUI.CORNER_NONE, 10.0)
				end
			elseif i == 6 then
				if Game.CharSnap(DisplayID).Cur.Weapon == 0 then
					Game.RenderTools:DrawUIRect(Rect, vec4f(1,1,1,0.5), _CUI.CORNER_L, 10.0)
					if Game.CharSnap(DisplayID).Cur.AttackTick >= Game.Client.Tick - (Game.Tuning().hammer_fire_delay.Value/2200) then -- Why dafuq is this 2200?
						Engine.TextRender:Text(nil,Rect.x+Rect.w/2,Rect.y,10,round((Game.CharSnap(DisplayID).Cur.AttackTick-(Game.Client.Tick-(Game.Tuning().hammer_fire_delay.Value/2200)))/50,2),-1)
					end
				else
					Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.5), _CUI.CORNER_L, 10.0)
				end
			elseif i == 1 then
				-- Currently no Support for Ninja I will add This soon :3
			end
			Engine.Graphics:TextureSet(Tex)
			Engine.Graphics:QuadsBegin()
				if i == 2 then
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_RIFLE_BODY,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/2,Rect.y+Rect.h/2,Rect.w/1.2)
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_RIFLE_PROJ,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/1.2,Rect.y+Rect.h/1.2,Rect.w/2)
				elseif i == 3 then
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_GRENADE_BODY,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/2,Rect.y+Rect.h/2,Rect.w/1.2)
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_GRENADE_PROJ,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/1.2,Rect.y+Rect.h/1.2,Rect.w/2)
				elseif i == 4 then
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_SHOTGUN_BODY,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/2,Rect.y+Rect.h/2,Rect.w/1.2)
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_SHOTGUN_PROJ,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/1.2,Rect.y+Rect.h/1.2,Rect.w/2)
				elseif i == 5 then
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_GUN_BODY,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/2,Rect.y+Rect.h/2,Rect.w/1.2)
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_GUN_PROJ,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/1.2,Rect.y+Rect.h/1.2,Rect.w/2)
				elseif i == 6 then
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_HAMMER_BODY,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/2,Rect.y+Rect.h/2,Rect.w/1.2)
					Game.RenderTools:SelectSprite(SPRITE_WEAPON_HAMMER_PROJ,0,0,0)
					Game.RenderTools:DrawSprite(Rect.x+Rect.w/1.2,Rect.y+Rect.h/1.2,Rect.w/2)
				end
			Engine.Graphics:QuadsEnd()
		end
	end
	if Game.Voting.IsVoting then -- Voting
		local Reason = Game.Voting.VoteReason
		local Description = Game.Voting.VoteDescription
		local Time = Game.Voting.SecondsLeft
		local Header = UIRect(0,0,0,0)
		local Fooder = UIRect(0,0,0,0)
		local Yes = UIRect(0,0,0,0)
		local No = UIRect(0,0,0,0)
		local CurrentVotes = Game.Voting.Total-Game.Voting.Pass
		Vote:HSplitBottom(Vote.h-Vote.h/6,Vote,nil)
		Vote:VSplitLeft(Vote.w-Vote.w/10,Vote,nil)
		Vote:HSplitTop(Vote.h/4.5,Header,Fooder)
		Fooder:HSplitTop(Header.h,nil,Fooder)
		Fooder:HSplitTop(Header.h,Fooder,nil)
		Fooder:VMargin(20,Fooder)
		Fooder:HMargin(0.25,Fooder)
		print(Game.Voting.Yes,Game.Voting.No,CurrentVotes)
		--if CurrentVotes > 0 then
			Fooder:VSplitLeft(Fooder.w/(100/((Game.Voting.Yes*100)/(Game.Voting.Total-Game.Voting.Pass))),Yes,No)
		--else
			--Fooder:VSplitLeft(Fooder.w/2,Yes,No)
		--end
		Game.RenderTools:DrawUIRect(Vote, vec4f(0,0,0,0.3), _CUI.CORNER_ALL, 7.0)
		Game.RenderTools:DrawUIRect(Header, vec4f(0,0,0,0.3), _CUI.CORNER_ALL, 7.0)
		if Yes.w > 0 then Game.RenderTools:DrawUIRect(Yes, vec4f(127/255,1,21/255,0.6),No.w == 0 and _CUI.CORNER_ALL or _CUI.CORNER_L, 7.0) end
		if No.w > 0 then Game.RenderTools:DrawUIRect(No, vec4f(1,51/255,51/255,0.6),Yes.w == 0 and _CUI.CORNER_ALL or _CUI.CORNER_R, 7.0) end
		Game.Ui:DoLabelScaled(UIRect(Header.x+5,Header.y,Header.w-10,Header.h),Description,Header.h-5,-1,Header.w,Game.Players(DisplayID).Name,nil)
		Game.Ui:DoLabelScaled(UIRect(Header.x+5,Header.y,Header.w-10,Header.h),Time,Header.h-5,1,Header.w,Game.Players(DisplayID).Name,nil)
		Game.Ui:DoLabelScaled(UIRect(Header.x+5,Header.y+Header.h,Header.w-10,Header.h),"Reason: "..Reason,Header.h-7,-1,Header.w,Game.Players(DisplayID).Name,nil)
		if Yes.w > 0 then Game.Ui:DoLabelScaled(UIRect(Yes.x+10,Yes.y,Yes.w-20,Yes.h),"Yes ".. ((Game.Voting.Yes*100)/(Game.Voting.Total-Game.Voting.Pass)) .."% - ".. Game.Voting.Yes,Yes.h-7,-1,Header.w,Game.Players(DisplayID).Name,nil) end
		if No.w > 0 then Game.Ui:DoLabelScaled(UIRect(No.x+10,No.y,No.w-20,No.h),"No ".. ((Game.Voting.No*100)/(Game.Voting.Total-Game.Voting.Pass)) .."% - ".. Game.Voting.No,No.h-7,1,Header.w,Game.Players(DisplayID).Name,nil) end
		Game.Ui:DoLabelScaled(UIRect(Header.x+5,Header.y+Header.h*3.5,Header.w-10,Header.h),Game.Voting.Total-Game.Voting.Pass .." from ".. Game.Voting.Total .." ("..(((Game.Voting.Total-Game.Voting.Pass)*100)/Game.Voting.Total) .."%) voted",Header.h-7,0,Header.w,Game.Players(DisplayID).Name,nil)
		if Game.Voting.TakenChoice == 1 then
			Engine.TextRender:TextColor(0,1,0,1)
		end
		Game.Ui:DoLabelScaled(UIRect(Header.x+5,Header.y+Header.h*3.5,Header.w-10,Header.h),"Yes",Header.h-7,-1,Header.w,Game.Players(DisplayID).Name,nil)
		Engine.TextRender:TextColor(1,1,1,1)
		if Game.Voting.TakenChoice == -1 then
			Engine.TextRender:TextColor(1,0,0,1)
		end
		Game.Ui:DoLabelScaled(UIRect(Header.x+5,Header.y+Header.h*3.5,Header.w-10,Header.h),"No",Header.h-7,1,Header.w,Game.Players(DisplayID).Name,nil)
	end
	if not Scoreboard then
		ScoreFade = 0
	end
	Scoreboard = false
end

function DrawTee(TeeWindow,DisplayID,Size)
	-- Draw name at the Top

 	local MyTee = TeeRenderInfo() -- Ohne parameter???
	MyTee.Texture = Game.Players(DisplayID).RenderInfo.Texture
	MyTee.ColorBody = Game.Players(DisplayID).RenderInfo.ColorBody
	MyTee.ColorFeet = Game.Players(DisplayID).RenderInfo.ColorFeet
	MyTee.Size = Size or 80 --Game.Players(DisplayID).RenderInfo.Size
	MyTee.GotAirJump = Game.Players(DisplayID).RenderInfo.GotAirJump
	if TeeStats[DisplayID] ~= nil then
		Game.RenderTools:RenderTee( Game.CharSnap(DisplayID).Cur.Emote , MyTee , vec2f(TeeStats[DisplayID].DirX,TeeStats[DisplayID].DirY) , vec2f(TeeWindow.x+TeeWindow.w/2,TeeWindow.y+TeeWindow.h/2),false,0) -- (int Emote, CTeeRenderInfo *pInfo, const vec2& Dir, const vec2& Pos, bool UseTeeAlpha, float AlphaLimit)
	end
end

function GetTeeInfo(CID, PX, PY, DX, DY, T)
	if CID == Game.LoaclCID then
		TeeStats = {}
	end
	TeeStats[CID] = {
		PosX = PX,
		PosY = PY,
		DirX = DX,
		DirY = DY
	}
end

function OnScriptUnload()
	Engine.Graphics:UnloadTexture(Tex)
	Config.cl_showhud = 1
end

function GetDisplayID()
	if Game.CharSnap(Game.LocalCID).Active then
		return Game.LocalCID
	else
		return Game.SpecInfo.SpectatorID
	end
end

function CheckTexture()
	if Config.tex_game ~= LastTexture then
		Engine.Graphics:UnloadTexture(Tex)
		Tex = Engine.Graphics:LoadTexture("data/textures/game/".. Config.tex_game ..".png",-1,-1,1)
		LastTexture = Config.tex_game
	end
end

function CheckMaxHealthArmor(ID)
	if MaxHealth < Game.CharSnap(ID).Cur.Health then
		MaxHealth = Game.CharSnap(ID).Cur.Health
	end
	if MaxArmor < Game.CharSnap(ID).Cur.Armor then
		MaxArmor = Game.CharSnap(ID).Cur.Armor
	end
end

function OnRenderScoreboard(FadeVal)
	--print(tostring(Game.Menus.Active),tostring(Game.Menus.ActivePage))
	Scoreboard = true


	local Score = Game.Ui:Screen()

	if Scoreboard and Game.Menus.Active and Game.Menus.ActivePage == 3 then -- Some empty space, use this :3
		-- Super scoreboard

	elseif Scoreboard then
		Engine.Graphics:MapScreen(Score.x,Score.y,Score.w,Score.h)
		if ScoreFade ~= nil then
			-- Make Scoreboard with Clip (ScoreFade)
			if ScoreFade < Score.w then
				ScoreFade = ScoreFade +5000/Game.Client.FPS -- I like 5000 :3
			else
				ScoreFade = Score.w
			end
			Game.Ui:ClipEnable(UIRect(Score.x,Score.y,ScoreFade,Score.h))
				local Infos = UIRect(0,0,0,0)
				Score:VMargin(Score.w/5,Score)
				Score:HSplitTop(Score.h/10,Infos,Score)
				Score:HSplitBottom(Score.h/4,Score,nil)
				Infos:HSplitTop(Infos.h/1.75,nil,Infos)
				--Game.RenderTools:DrawUIRect(Infos, vec4f(0,0,0,0.3), _CUI.CORNER_T, 10)
				--RenderGradientRect(Score,vec4f(0,0,0,1),vec4f(0.75, 0,1,0.5),500,true)
				local ActivePlayers = GetCurPlayer()+1 < 16 and 16 or GetCurPlayer()
				local CurrRender = 0
				local TeeRect = UIRect(0,0,0,0)
				local SkinNameRect = UIRect(0,0,0,0)
				local ScoreRect = UIRect(0,0,0,0)
				local NameRect = UIRect(0,0,0,0)
				local ClanNameRect = UIRect(0,0,0,0)
				local PingRect = UIRect(0,0,0,0)
				local TopRect = UIRect(0,0,0,0)
				-- cl_show_ids_scoreboard
				local IDRect = UIRect(0,0,0,0)

				if Config.cl_show_ids_scoreboard == 1 then
					Score:VSplitLeft(20,IDRect,Score)
					Game.RenderTools:DrawUIRect(IDRect, vec4f(0,0,0,0.4), _CUI.CORNER_BL, 10)
					Infos:VSplitLeft(20,TopRect,Infos)
					Game.RenderTools:DrawUIRect(TopRect, vec4f(0,0,0,0.4), _CUI.CORNER_TL, 10)
					Game.Ui:DoLabelScaled(UIRect(TopRect.x,TopRect.y+13/2,TopRect.w,TopRect.h),"ID",13,0,TopRect.w,nil)
				end
				Score:VSplitLeft(Score.h/ActivePlayers,TeeRect,Score)
				Score:VSplitLeft(Score.w/5,SkinNameRect,Score)
				Score:VSplitLeft(Score.w/10,ScoreRect,Score)
				Score:VSplitLeft(Score.w/2.2,NameRect,Score)
				Score:VSplitLeft(NameRect.w,ClanNameRect,PingRect)
				Game.RenderTools:DrawUIRect(TeeRect, vec4f(0,0,0,0.3),Config.cl_show_ids_scoreboard == 1 and _CUI.CORNER_NONE or _CUI.CORNER_BL, 10)
				Game.RenderTools:DrawUIRect(SkinNameRect, vec4f(0,0,0,0.3),_CUI.CORNER_NONE, 10)
				Game.RenderTools:DrawUIRect(ScoreRect, vec4f(0,0,0,0.3),_CUI.CORNER_NONE, 10)
				Game.RenderTools:DrawUIRect(NameRect, vec4f(0,0,0,0.3),_CUI.CORNER_NONE, 10)
				Game.RenderTools:DrawUIRect(ClanNameRect, vec4f(0,0,0,0.3),_CUI.CORNER_NONE, 10)
				Game.RenderTools:DrawUIRect(PingRect, vec4f(0,0,0,0.3),_CUI.CORNER_BR, 10)
				for i = 0,64-1 do
					if Game.Players(i).Name ~= "" and Game.Snap:PlayerInfos(i) ~= nil then
						local Rect = UIRect(IDRect.x,IDRect.y+(IDRect.h/ActivePlayers)/4+(IDRect.h/ActivePlayers)*CurrRender,IDRect.w,IDRect.h/ActivePlayers)
						if Config.cl_show_ids_scoreboard == 1 then
							Game.Ui:DoLabelScaled(Rect,i,(IDRect.h/ActivePlayers)/2,0,Rect.w,nil)
						end
						Rect = UIRect(TeeRect.x,TeeRect.y+(TeeRect.h/ActivePlayers)*CurrRender,TeeRect.w,TeeRect.h/ActivePlayers)
						if CurrRender%2 == 0 then
							Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.1), _CUI.CORNER_NONE, 10)
						end
						DrawTee(Rect,i,TeeRect.h/ActivePlayers)

						Rect = UIRect(SkinNameRect.x,SkinNameRect.y+(SkinNameRect.h/ActivePlayers)*CurrRender,SkinNameRect.w,SkinNameRect.h/ActivePlayers)
						if CurrRender%2 == 0 then
							Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.1), _CUI.CORNER_NONE, 10)
						end
						Rect = UIRect(SkinNameRect.x,SkinNameRect.y+(SkinNameRect.h/ActivePlayers)/4+(SkinNameRect.h/ActivePlayers)*CurrRender,SkinNameRect.w,SkinNameRect.h/ActivePlayers)
						Game.Ui:DoLabelScaled(Rect,Game.Players(i).SkinName,(SkinNameRect.h/ActivePlayers)/2,0,SkinNameRect.w,nil)

						Rect = UIRect(ScoreRect.x,ScoreRect.y+(ScoreRect.h/ActivePlayers)*CurrRender,ScoreRect.w,ScoreRect.h/ActivePlayers)
						if CurrRender%2 == 0 then
							Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.1), _CUI.CORNER_NONE, 10)
						end
						Rect = UIRect(ScoreRect.x,ScoreRect.y+(ScoreRect.h/ActivePlayers)/4+(ScoreRect.h/ActivePlayers)*CurrRender,ScoreRect.w,ScoreRect.h/ActivePlayers)
						Game.Ui:DoLabelScaled(Rect,Game.Snap:PlayerInfos(i).Score,(ScoreRect.h/ActivePlayers)/2,0,ScoreRect.w,nil)

						Rect = UIRect(NameRect.x,NameRect.y+(NameRect.h/ActivePlayers)*CurrRender,NameRect.w,NameRect.h/ActivePlayers)
						if CurrRender%2 == 0 then
							Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.1), _CUI.CORNER_NONE, 10)
						end
						Rect = UIRect(NameRect.x,NameRect.y+(NameRect.h/ActivePlayers)/4+(NameRect.h/ActivePlayers)*CurrRender,NameRect.w,NameRect.h/ActivePlayers)
						Game.Ui:DoLabelScaled(Rect,Game.Players(i).Name,(NameRect.h/ActivePlayers)/2,0,NameRect.w,Game.Players(Game.LocalCID).Name)

						Rect = UIRect(ClanNameRect.x,ClanNameRect.y+(ClanNameRect.h/ActivePlayers)*CurrRender,ClanNameRect.w,ClanNameRect.h/ActivePlayers)
						if CurrRender%2 == 0 then
							Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.1), _CUI.CORNER_NONE, 10)
						end
						Rect = UIRect(ClanNameRect.x,ClanNameRect.y+(ClanNameRect.h/ActivePlayers)/4+(ClanNameRect.h/ActivePlayers)*CurrRender,ClanNameRect.w,ClanNameRect.h/ActivePlayers)
						Game.Ui:DoLabelScaled(Rect,Game.Players(i).Clan,(ClanNameRect.h/ActivePlayers)/2,0,ClanNameRect.w,Game.Players(Game.LocalCID).Name)

						Rect = UIRect(PingRect.x,PingRect.y+(PingRect.h/ActivePlayers)*CurrRender,PingRect.w,PingRect.h/ActivePlayers)
						if CurrRender%2 == 0 then
							Game.RenderTools:DrawUIRect(Rect, vec4f(0,0,0,0.1), _CUI.CORNER_NONE, 10)
						end
						Rect = UIRect(PingRect.x,PingRect.y+(PingRect.h/ActivePlayers)/4+(PingRect.h/ActivePlayers)*CurrRender,PingRect.w,PingRect.h/ActivePlayers)
						Game.Ui:DoLabelScaled(Rect,Game.Snap:PlayerInfos(i).Latency,(PingRect.h/ActivePlayers)/2,0,PingRect.w,Game.Players(Game.LocalCID).Name)
						CurrRender = CurrRender+1
					end
				end
				Infos:VSplitLeft(Score.h/ActivePlayers,TopRect,Infos)
				Game.RenderTools:DrawUIRect(TopRect, vec4f(0,0,0,0.4),Config.cl_show_ids_scoreboard == 1 and _CUI.CORNER_NONE or _CUI.CORNER_TL, 10)
				Infos:VSplitLeft(Infos.w/5,TopRect,Infos)
				Game.RenderTools:DrawUIRect(TopRect, vec4f(0,0,0,0.4), _CUI.CORNER_NONE, 10)
				Game.Ui:DoLabelScaled(UIRect(TopRect.x,TopRect.y+13/2,TopRect.w,TopRect.h),"Skinname",13,0,TopRect.w,nil)
				Infos:VSplitLeft(Infos.w/10,TopRect,Infos)
				Game.RenderTools:DrawUIRect(TopRect, vec4f(0,0,0,0.4), _CUI.CORNER_NONE, 10)
				Game.Ui:DoLabelScaled(UIRect(TopRect.x,TopRect.y+13/2,TopRect.w,TopRect.h),"Score",13,0,TopRect.w,nil)
				Infos:VSplitLeft(Infos.w/2.2,TopRect,Infos)
				Game.RenderTools:DrawUIRect(TopRect, vec4f(0,0,0,0.4), _CUI.CORNER_NONE, 10)
				Game.Ui:DoLabelScaled(UIRect(TopRect.x,TopRect.y+13/2,TopRect.w,TopRect.h),"Name",13,0,TopRect.w,nil)
				Infos:VSplitLeft(TopRect.w,TopRect,Infos)
				Game.RenderTools:DrawUIRect(TopRect, vec4f(0,0,0,0.4), _CUI.CORNER_NONE, 10)
				Game.Ui:DoLabelScaled(UIRect(TopRect.x,TopRect.y+13/2,TopRect.w,TopRect.h),"Clan-Name",13,0,TopRect.w,nil)
				TopRect = Infos
				Game.RenderTools:DrawUIRect(TopRect, vec4f(0,0,0,0.4), _CUI.CORNER_TR, 10)
				Game.Ui:DoLabelScaled(UIRect(TopRect.x,TopRect.y+13/2,TopRect.w,TopRect.h),"Ping",13,0,TopRect.w,nil)
			Game.Ui:ClipDisable()
		end
	end

	Engine.Graphics:MapScreen(0,0,0,0)
end

function GetCurPlayer()
	local CurrPlayer = 0
	for i = 1,64-1 do
		if Game.Players(i).Name ~= "" then
			CurrPlayer = CurrPlayer +1
		end
	end
	return CurrPlayer
end

function RenderGradientRect(Rect,ColorFrom,ColorTo,Steps,Alpha)
	local OldRed = ColorFrom.r
	local NewRed = ColorTo.r
	local RedStep = (NewRed - OldRed) / Steps
	local OldGreen = ColorFrom.g
	local NewGreen = ColorTo.g
	local GreenStep = (NewGreen - OldGreen) / Steps
	local OldBlue = ColorFrom.b
	local NewBlue = ColorTo.b
	local BlueStep = (NewBlue - OldBlue) / Steps
	local OldAlpha = ColorFrom.a
	local NewAlpha = ColorTo.a
	local AlphaStep = (NewAlpha - OldAlpha) / Steps
	for i=1, Steps do
		local Color = vec4f(ColorFrom.r+i*RedStep,ColorFrom.g+i*GreenStep,ColorFrom.b+i*BlueStep, Alpha and ColorFrom.a+i*AlphaStep or ColorFrom.a)
		local DrawRect = UIRect(Rect.x+(Rect.w/Steps)*i,Rect.y,Rect.w/Steps,Rect.h)
		Game.RenderTools:DrawUIRect(DrawRect, Color, _CUI.CORNER_NONE, 10)
	end
end

RegisterEvent("OnRenderLevel14","DrawHud")
RegisterEvent("PreRenderPlayer", "GetTeeInfo")
