--[[#!
	#io
]]--#

------------------- BASIC FUNCTION NEEDED FOR A SETTINGS PAGE --------------------

if Import("general") ~= true then error("The 'general' library is needed to use 'config'") end


g_ScriptConfig = {}
g_ScriptConfigFileOld = "autoconfig.json"
g_ScriptConfigFile = "autoconfig.json"

function _ConfigSet(varname, value)
	if varname == nil then return end
	g_ScriptConfig[varname] = value
end

function _ConfigGet(varname)
	if varname == nil then return nil end
	return g_ScriptConfig[varname]
end

function _ConfigLoad() -- call this in OnScriptInit()
	local ConfFile = g_ScriptConfigFile
	if Config.debug > 0 then print("[config] Loading settings from '" .. ConfFile .. "'") end

	local f,path = io.open(g_ScriptConfigFile)

	if f == nil then
		-- try alternative
		f,path = io.open(g_ScriptConfigFileOld)
	end

	if f == nil then
		if Config.debug > 0 then
			print("[config] Failed. No settings file found.")
		end
		return false, path
	end

	local json_string = f:read("*a")
	local success, result = pcall(json.Parse, json_string) -- lua style try-catch - result is either our json_value or an error message
	f:close()

	-- json parsing error
	if not success then
		throw("[config] Failed to load config! " .. result)
		return false, path
	end

	success, result = pcall(result.ToObject, result)
	if not success then
		throw("[config] Failed to load config! " .. result)
		return false, path
	end

	g_ScriptConfig = result

	if Config.debug > 0 then
		print("[config] Done! Loaded settings from '" .. path .. "'")
	end
	return true, path
end

function _ConfigSave() -- call this in OnScriptUnload()
	local file = io.open(g_ScriptConfigFile, "w+")
	if file == nil then error("Failed to save config to " .. g_ScriptConfigFile) end

	local json_value = json.Convert(g_ScriptConfig)
	local json_string = json.Serialize(json_value)
	json_value:Destroy()

	file:write(json_string)
	file:flush()
	file:close()
	if Config.debug > 0 then print("[config] Saved settings to '" .. g_ScriptConfigFile .. "'") end
end

function OnScriptRenderSettings(MainView) -- some default page to inform the user
	Game.Ui:DoLabelScaled(MainView, "To implement a custom settings page, put a function `OnScriptRenderSettings(<UIRect> MainView)` into your script.", 13.0, 0, -1, "OnScriptRenderSettings(<UIRect> MainView)", 0)
	MainView:HSplitTop(25.0, nil, MainView)
	Game.Ui:DoLabelScaled(MainView, "To remove this default page, write `OnScriptRenderSettings=nil` after `Import(\"config\")`", 12.0, 0, -1, "OnScriptRenderSettings=nil", 0)

	MainView:HSplitTop(45.0, nil, MainView)
	Engine.TextRender:TextColor(0.8, 0.8, 0.8, 0.9)
	Game.Ui:DoLabelScaled(MainView, "Note from the developer: Maybe this page will even be auto-generated one day, who knows... ;)", 9.0, 0, -1, "", 0)
	Engine.TextRender:TextColor(1,1,1,1)
end

function OnScriptSaveSettings() -- default save function
	_ConfigSave()
end

function OnScriptUnload()
    OnScriptSaveSettings()
    print("config saved on script unload!")
end
