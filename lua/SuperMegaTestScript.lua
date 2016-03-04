_g_ScriptTitle = "Fettes Testskript ;)" -- this defines the script title, TODO: VAR NAME IS FORCED!
_g_ScriptInfo = "BOAH IS DAT GEIL!" -- this defines the subtext (info), TODO: VAR NAME IS FORCED!
config = {} -- this creates the table for storing config stuff into
config["TestBool"] = 0 -- build the config table
config["TestInt"] = 0

function OnScriptInit() -- XXX if there are script errors in this function, the luafile will enter error-state (nt usable!)
	if(_system.Import(_g_ScriptUID, "SuperMegaTestScript.config") and config ~= nil) then
		print("SuperMegaTestScript.lua: Config loaded!")
	else
		print("SuperMegaTestScript.lua: Failed to load config, creating one!")
		OnScriptSaveSettings()
		--return false -- this would force the script to enter error-state (thus unexecutable)
	end
	
	return true -- this is important to tell c++ that the script init was successful
	---------- XXX if OnScriptInit() doesn't return anything, the client WILL CRASH in any way!!
end

function OnScriptRenderSettings(x, y, w, h)
	-- just a red rect, much lame ._.
	_ui.SetUiColor(1,0,0,0.3)
	_ui.DrawUiRect(x, y, w, h, 15, 3.0)
	if(_ui.DoButton_Menu("Toggle TestBool", config["TestBool"], x+20, y+20, 170, 35, "mytooltip", 15) ~= 0) then
		config["TestBool"] = ((config["TestBool"] == 1) and 0 or 1)
	end
	
	if(_ui.DoButton_Menu("←", 0, x+60, y+60, 35, 35, "decrease", 15) ~= 0) then
		config["TestInt"] = config["TestInt"] - 1
	end
	
	_ui.DoButton_Menu(config["TestInt"], 0, x+60+35+2, y+60, 35, 35, "increase", 15)
	
	if(_ui.DoButton_Menu("→", 0, x+60+2*35+4, y+60, 35, 35, "increase", 15) ~= 0) then
		config["TestInt"] = config["TestInt"] + 1
	end
end

function OnScriptSaveSettings()
	file = io.open("lua/SuperMegaTestScript.config", "w+")
	for k, v in next, config do
		if(type(k) == "string") then
			file:write("config[\"" .. k .. "\"] = " .. v .. "\n")
		else
			file:write("config[" .. k .. "] = " .. v .. "\n")
		end
	end
	file:flush()
	file:close()
end


local function test()	
	print(Game.Collision:GetTile(Game.Local.Tee.PosX, Game.Local.Tee.PosY))
end

--[[
local function test()
	--print("ID 0 Name: ".._client.GetPlayerName(0))
	
	Game.Chat:Say(0, "Hi")
	-- or Game.Client.Chat:Say(0, "Hi"), you can decide :D
end ]]--

RegisterEvent("OnTick", test)
