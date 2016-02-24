-- TODO: UNFINISHED!


SetScriptTitle("Blood-Patch")
SetScriptInfo("(c) by MAP94 - AllTheHaxx edit by Henritees")
SetScriptHasSetting(1)

Config = table
Config.UseTeeColor = 1
Config.UseStdColor = 0
Config.StdColorR = 1
Config.StdColorG = 0
Config.StdColorB = 0

checkfile = io.open("lua/bloodpatch.config", "r")
if (checkfile == nil) then
    configout = io.open("lua/bloodpatch.config", "wb")
    configout:close()
else
    checkfile:close()
end
--Include ("lua/bloodpatch.config")
local Blood = graphics.LoadTexture("luares/bloodpatch/blood.png", -1, -1, 1)

local Ui = table
Ui.SettingsRect = nil
Ui.SettingsUseTeeColor = nil
Ui.SettingsUseStdColor = nil
Ui.SettingsStdColorR = nil
Ui.SettingsStdColorG = nil
Ui.SettingsStdColorB = nil
Ui.SettingsStdColorPreview = nil

BloodNum = 0
BloodSpots = {}

function RenderBlood()
    for k, v in ipairs(BloodSpots) do
        if (1 - (TimeGet() - v["s"]) / 20 > 0) then
        	--0, 0, 64, 64, v["r"], v["g"], v["b"], 1 - (TimeGet() - v["s"]) / 30
        	graphics.SetColor(v["r"], v["g"], v["b"], 1 - (TimeGet() - v["s"]) / 30)
            graphics.RenderTexture(Blood, v["x"] - 48, v["y"] - 48, 96, 96, v["rot"])
        end
    end
end

function Kill(Killer, Victim, Weapon)
    x, y = GetCharacterPos(Victim)
    r, g, b = GetPlayerColorBody(Victim)
    if (Config.UseStdColor == 1) then
        r = Config.StdColorR
        g = Config.StdColorG
        b = Config.StdColorB
    end
    for i = 0, 100 do
        a = math.random(0, math.pi * 2)
        x1 = math.cos(a) * math.random(0, 32)
        y1 = math.sin(a) * math.random(0, 32)
        CreateParticle(2, UiGetParticleTextureID(), x, y, x1 * 60, y1 * 60, 2, 0, 0, 25, 1, 0.8, x1 * 10, 2000 + y1 * 5, 0, r, g, b, 1, r, g, b, 0)
    end
    for i = 0, 49 do
        xm = math.random(-3, 3)
        ym = math.random(-3, 3)
        c = GetTile(x + xm * 32, y + ym * 32)
        if (c == 1 or c == 2 or c == 3) then
            BloodNum = BloodNum + 1
            if (BloodNum == 64) then
                BloodNum = 0
            end
            BloodSpots[BloodNum] = {}
            BloodSpots[BloodNum]["x"] = x + xm * 32
            BloodSpots[BloodNum]["y"] = y + ym * 32
            BloodSpots[BloodNum]["r"] = r
            BloodSpots[BloodNum]["g"] = g
            BloodSpots[BloodNum]["b"] = b
            BloodSpots[BloodNum]["rot"] = math.random(0, 6.283185308)
            BloodSpots[BloodNum]["s"] = TimeGet()
            break
        end
    end
end

function ButtonUseTeeColor(state)
    if (state == 1) then
        if (Config.UseTeeColor == 1) then
            Config.UseTeeColor = 0
            Config.UseStdColor = 1
        else
            Config.UseTeeColor = 1
            Config.UseStdColor = 0
        end
        if (Config.UseTeeColor == 1) then
            UiSetColor(Ui.SettingsUseTeeColor, 0, 1, 0, 0.5)
        else
            UiSetColor(Ui.SettingsUseTeeColor, 1, 1, 1, 0.5)
        end
        if (Config.UseStdColor == 1) then
            UiSetColor(Ui.SettingsUseStdColor, 0, 1, 0, 0.5)
        else
            UiSetColor(Ui.SettingsUseStdColor, 1, 1, 1, 0.5)
        end
    end
end

function ButtonUseStdColor(state)
    if (state == 1) then
        if (Config.UseStdColor == 1) then
            Config.UseStdColor = 0
            Config.UseTeeColor = 1
        else
            Config.UseStdColor = 1
            Config.UseTeeColor = 0
        end
        if (Config.UseTeeColor == 1) then
            UiSetColor(Ui.SettingsUseTeeColor, 0, 1, 0, 0.5)
        else
            UiSetColor(Ui.SettingsUseTeeColor, 1, 1, 1, 0.5)
        end
        if (Config.UseStdColor == 1) then
            UiSetColor(Ui.SettingsUseStdColor, 0, 1, 0, 0.5)
        else
            UiSetColor(Ui.SettingsUseStdColor, 1, 1, 1, 0.5)
        end
    end
end

function SliderStdColorR(val)
    Config.StdColorR = val
    UiSetColor(Ui.SettingsStdColorPreview, Config.StdColorR, Config.StdColorG, Config.StdColorB, 1)
end

function SliderStdColorG(val)
    Config.StdColorG = val
    UiSetColor(Ui.SettingsStdColorPreview, Config.StdColorR, Config.StdColorG, Config.StdColorB, 1)
end

function SliderStdColorB(val)
    Config.StdColorB = val
    UiSetColor(Ui.SettingsStdColorPreview, Config.StdColorR, Config.StdColorG, Config.StdColorB, 1)
end

function ConfigOpen(x, y, w, h)
    Spacing = UiGetScreenHeight() * 0.01

    Ui.SettingsRect = UiDoRect(x, y, w, h, 0, 15, 15, 0, 0, 0, 0.5)
    x = x + Spacing
    y = y + Spacing
    w = w - Spacing * 2


    Ui.SettingsUseTeeColor = UiDoButton(x + Spacing, y + Spacing, 250, 20, 0, "Use Tee color as blood color", "ButtonUseTeeColor")
    y = y + Spacing + 20
    Ui.SettingsUseStdColor = UiDoButton(x + Spacing, y + Spacing, 250, 20, 0, "Use standard color as blood color", "ButtonUseStdColor")
    y = y + Spacing + 20
    Ui.SettingsStdColorPreview = UiDoRect(x + 250 + Spacing * 2, y + Spacing, Spacing * 3 + 60, Spacing * 2 + 45, 0)
    UiSetColor(Ui.SettingsStdColorPreview, Config.StdColorR, Config.StdColorG, Config.StdColorB, 1)
    Ui.SettingsStdColorR = UiDoSlider (x + Spacing, y + Spacing, 250, 15, 0, Config.StdColorR, "SliderStdColorR")
    y = y + Spacing + 15
    Ui.SettingsStdColorG = UiDoSlider (x + Spacing, y + Spacing, 250, 15, 0, Config.StdColorG, "SliderStdColorG")
    y = y + Spacing + 15
    Ui.SettingsStdColorB = UiDoSlider (x + Spacing, y + Spacing, 250, 15, 0, Config.StdColorB, "SliderStdColorB")

    if (Config.UseTeeColor == 1) then
        UiSetColor(Ui.SettingsUseTeeColor, 0, 1, 0, 0.5)
    else
        UiSetColor(Ui.SettingsUseTeeColor, 1, 1, 1, 0.5)
    end
    if (Config.UseStdColor == 1) then
        UiSetColor(Ui.SettingsUseStdColor, 0, 1, 0, 0.5)
    else
        UiSetColor(Ui.SettingsUseStdColor, 1, 1, 1, 0.5)
    end
end

function ConfigClose()
    configout = io.open("lua/bloodpatch.config", "wb")
    configout:write("--Configfile for Bloodpatch\n")
    configout:write("Config = table\n")
    configout:write("Config.UseTeeColor = " .. Config.UseTeeColor .. "\n")
    configout:write("Config.UseStdColor = " .. Config.UseStdColor .. "\n")
    configout:write("Config.StdColorR = " .. Config.StdColorR .. "\n")
    configout:write("Config.StdColorG = " .. Config.StdColorG .. "\n")
    configout:write("Config.StdColorB = " .. Config.StdColorB .. "\n")
    configout:close()

    UiRemoveElement(Ui.SettingsRect)
    UiRemoveElement(Ui.SettingsUseTeeColor)
    UiRemoveElement(Ui.SettingsUseStdColor)
    UiRemoveElement(Ui.SettingsStdColorR)
    UiRemoveElement(Ui.SettingsStdColorG)
    UiRemoveElement(Ui.SettingsStdColorB)
    UiRemoveElement(Ui.SettingsStdColorPreview)

end

RegisterEvent("OnKill", Kill)
RegisterEvent("OnRenderLevel5", RenderBlood)


