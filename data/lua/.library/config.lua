--[[#!
	#io
]]--#

------------------- BASIC FUNCTION NEEDED FOR A SETTINGS PAGE --------------------

if Import("general") ~= true then error("The 'general' library is needed to use 'config'") end


g_ScriptConfig = {}
g_ScriptConfigFile = ScriptPath():sub(1, -5) .. ".conf.lua"

function _ConfigSet(varname, value)
	if varname == nil then return end
	g_ScriptConfig[varname] = value
end

function _ConfigGet(varname)
	if varname == nil then return nil end
	return g_ScriptConfig[varname]
end

function _ConfigLoad() -- call this in OnScriptInit()
	local ConfFile = g_ScriptConfigFile:sub(-g_ScriptConfigFile:reverse():find('/')+1, -1)
	if Config.debug == 1 then print("[config] Loading settings from '" .. ConfFile .. "'") end
	local ret, path = Import(ConfFile)
	if Config.debug == 1 then
		if ret == true then
			print("[config] Done! Loaded settings from '" .. path .. "'")
		else
			print("[config] Failed. No settings file found.")
		end
	end
	return ret, path
end

function _ConfigSave() -- call this in OnScriptUnload()
	local file = io.open(g_ScriptConfigFile, "w+")
	if file == nil then error("Failed to save config to " .. g_ScriptConfigFile) end

	for k, v in next, g_ScriptConfig do
		if(type(k) == "string") then k = '"' .. k .. '"' end
		if(type(v) == "string") then v = '"' .. v .. '"'
        elseif type(v) == "number" then v = string.gsub(tostring(v), ",", ".") end
		file:write("g_ScriptConfig[" .. k .. "] = " .. tostring(v) .. "\n")
	end
	file:flush()
	file:close()
end

function OnScriptRenderSettings(MainView) -- some default page to inform the user
	Game.Ui:DoLabelScaled(MainView, "To implement a custom settings page, put a function 'OnScriptRenderSettings(x, y, w, h)' into your script.", 13.0, 0, -1, "OnScriptRenderSettings(x, y, w, h)")
	MainView:HSplitTop(25.0, nil, MainView)
	Game.Ui:DoLabelScaled(MainView, "To remove this default page, write 'OnScriptRenderSettings=nil' after 'Import(\"config\")'", 12.0, 0, -1, "OnScriptRenderSettings=nil")

	MainView:HSplitTop(45.0, nil, MainView)
	Engine.TextRender:TextColor(0.8, 0.8, 0.8, 0.9)
	Game.Ui:DoLabelScaled(MainView, "Note from the developer: Maybe this page will even be auto-generated one day, who knows... ;)", 9.0, 0, -1, "")
	Engine.TextRender:TextColor(1,1,1,1)
end

function OnScriptSaveSettings() -- default save function
	_ConfigSave()
end

function OnScriptUnload()
    OnScriptSaveSettings()
    print("config saved on script unload!")
end
