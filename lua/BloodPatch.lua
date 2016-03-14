g_ScriptTitle = "Blood-Patch"
g_ScriptInfo = "Teeish Blood Effects 18+! | by the AllTheHaxx-Team"

Config = {}
Config.UseColor = 1 -- 1=tee color, 0=std color
--Config.UseStdColor = 0
Config.StdColorR = 1
Config.StdColorG = 0
Config.StdColorB = 0
Config.Lifetime = 45
Config.MaxBloodNum = 96

Blood = {}

function OnScriptInit()
	if(Import(g_ScriptUID, "bloodpatch.config") == true) then
		print("bloodpatch.lua: Config loaded!")
	else
		print("bloodpatch.lua: Failed to load config, creating one!")
		OnScriptSaveSettings()
		--return false -- this would force the script to enter error-state (thus unexecutable)
	end
	
	-- load the graphics
	Success = true
	for i = 1, 4 do
		Blood[i] = Graphics.Engine:LoadTexture("luares/bloodpatch/blood" .. i .. ".png", -1, -1, 1) -- luares/bloodpatch/blood.png
		if(Blood[i] <= 0) then
			Success = false
			break
		end
	end
	return Success
end

function OnScriptUnload()
-- unload all the textures to not leak memory!
	for k, v in next, Blood do
		if v > 0 then
			Graphics.Engine:UnloadTexture(v)
		end
	end
end


BloodNum = 0
BloodSpots = {}

function RenderBlood()
	for k, v in ipairs(BloodSpots) do
        if (Game.Client.LocalTime - v["s"] < Config.Lifetime) then
        	--print("Rendering quad id=" .. k .. " at x=" .. v["x"] .. "~" .. v["x"]/32 .. ", y=" .. v["y"] .. "~" .. v["y"]/32)
        	Graphics.Engine:TextureSet(Blood[v["tex"]])
        	Graphics.Engine:QuadsBegin()
        	if(Config.UseColor == 1) then
        		Graphics.Engine:SetColor(v.r/0xFF, v.g/0xFF, v.b/0xFF, 1)
        	else
        		Graphics.Engine:SetColor(Config.StdColorR, Config.StdColorG, Config.StdColorB, 1)
        	end
        	Graphics.Engine:SetRotation(v["rot"])
            _graphics.RenderQuad(v["x"] - 48, v["y"] - 48, 96, 96)
            Graphics.Engine:QuadsEnd()
        end
    end
end

function Kill(Killer, Victim, Weapon)

    x = Game.Players(Victim).Tee.Pos.x
    y = Game.Players(Victim).Tee.Pos.y
    if(math.abs(Game.LocalTee.Pos.x - Game.Players(Victim).Tee.Pos.x) > 850) then -- we don't get correct positions beyond this approx. limit
    	return
    end

    for n = 0, math.random(2, 4) do
		for i = 0, 100 do
		    a = math.random(0, math.pi * 2)
		    x1 = math.cos(a) * math.random(0, 32)
		    y1 = math.sin(a) * math.random(0, 32) -- TODO ↓
		    --CreateParticle(2, UiGetParticleTextureID(), x, y, x1 * 60, y1 * 60, 2, 0, 0, 25, 1, 0.8, x1 * 10, 2000 + y1 * 5, 0, r, g, b, 1, r, g, b, 0)
		end
		for i = 0, 49 do
		    xm = math.random(-3, 3)
		    ym = math.random(-3, 3)
		    c = Game.Collision:GetTile(x + xm * 32, y + ym * 32)
		    if (c == 1 or c == 2 or c == 3) then
		        BloodSpots[BloodNum] = {}
		        BloodSpots[BloodNum]["x"] = x + xm * 32
		        BloodSpots[BloodNum]["y"] = y + ym * 32
		        BloodSpots[BloodNum]["r"] = bit.rshift(bit.band(Game.Players(Victim).ColorBody, 0xFF0000), 16)--r
		        BloodSpots[BloodNum]["g"] = bit.rshift(bit.band(Game.Players(Victim).ColorBody, 0x00FF00), 8)--g
		        BloodSpots[BloodNum]["b"] = bit.rshift(bit.band(Game.Players(Victim).ColorBody, 0x0000FF), 0)--b
		        BloodSpots[BloodNum]["rot"] = math.random(0, 6.283185308)
		        BloodSpots[BloodNum]["tex"] = math.random(1, 4)
		        BloodSpots[BloodNum]["s"] = Game.Client.LocalTime
		        BloodNum = BloodNum + 1
		        if (BloodNum >= Config.MaxBloodNum) then
		            BloodNum = 0
		        end
		        break
		    end
		end
	end
end

function OnScriptRenderSettings(x, y, w, h)
    Spacing = Graphics.Engine.ScreenHeight * 0.01

	_ui.SetUiColor(0, 0, 0, 0.5)
    _ui.DrawUiRect(x, y, w, h, 15, 5.0)
    x = x + Spacing
    y = y + Spacing
    w = w - Spacing * 2
    
    rect = UIRect()
	rect.x = x
	rect.y = y
	rect.w = w
	rect.h = 20
   -- Game.Ui.DoLabel(rect, "Bloodpatch Config", 13.0, 0, -1, 0)
 	y = y + Spacing + 20
 
	_ui.SetUiColor(1, 1, 1, 0.5)
	label = ((Config.UseTeeColor == 1) and "Blood Color: Tee" or "Blood Color: Standard")
    if(_ui.DoButton_Menu(label, 0, x + Spacing, y + Spacing, 250, 20, "Click to toggle", 15) ~= 0) then
    	Config.UseTeeColor = ((Config.UseTeeColor == 1) and 0 or 1)
    end
    y = y + Spacing + 20
    
    _ui.SetUiColor(Config.StdColorR, Config.StdColorG, Config.StdColorB, 1)
 --[[   if (Config["UseTeeColor"] == 1) then
        _ui.SetUiColor(0, 1, 0, 0.5)
    else
        UiSetColor(Ui.SettingsUseTeeColor, 1, 1, 1, 0.5)
    end ]]
  --[[  if (Config.UseStdColor == 1) then
        UiSetColor(Ui.SettingsUseStdColor, 0, 1, 0, 0.5)
    else
        UiSetColor(Ui.SettingsUseStdColor, 1, 1, 1, 0.5)
    end ]]
    _ui.DrawUiRect(x + 250 + Spacing * 2, y + Spacing, Spacing * 3 + 60, Spacing * 2 + 45, 15, 5.0)
  
    --UiDoSlider(x + Spacing, y + Spacing, 250, 15, 0, Config.StdColorR, "SliderStdColorR")
    y = y + Spacing + 15
    --UiDoSlider(x + Spacing, y + Spacing, 250, 15, 0, Config.StdColorG, "SliderStdColorG")
    y = y + Spacing + 15
    --UiDoSlider(x + Spacing, y + Spacing, 250, 15, 0, Config.StdColorB, "SliderStdColorB")

end



function OnScriptSaveSettings()
	file = io.open("lua/bloodpatch.config", "w+")
	for k, v in next, Config do
		if(type(k) == "string") then
			file:write("Config[\"" .. k .. "\"] = " .. v .. "\n")
		else
			file:write("Config[" .. k .. "] = " .. v .. "\n")
		end
	end
	file:flush()
	file:close()
end

RegisterEvent("OnKill", "Kill")
RegisterEvent("OnRenderLevel8", "RenderBlood")

