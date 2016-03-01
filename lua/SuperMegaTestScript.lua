_g_ScriptTitle = "Fettes Testskript ;)" -- this defines the script title, TODO: VAR NAME IS FORCED!
_g_ScriptInfo = "BOAH IS DAT GEIL!" -- this defines the subtext (info), TODO: VAR NAME IS FORCED!
config = {} -- this creates the table for storing config stuff into

function OnScriptInit() -- XXX if there are script errors in this function, the luafile will enter error-state (nt usable!)
	if(_system.Import(_g_ScriptUID, "SuperMegaTestScript.config") and config ~= nil) then
		print("Ya, config loaded successfully!")
		print("TestValue1: " .. config["TestValue1"])
		print("TestValue2: " .. config["TestValue2"])
	else
		return false -- this would force the script to enter error-state (thus cannot be used)
	end
	
	return true -- this is important to tell c++ that the script init was successful
	---------- XXX if OnScriptInit() doesn't return anything, the client WILL crash in any way!!
end

function OnScriptRenderSettings(x, y, w, h)
	-- just a red rect, much lame ._.
	_ui.SetUiColor(1,0,0,1)
	_ui.DrawUiRect(x, y, w, h, 15, 3.0)
end

function OnScriptSaveSettings()
	-- save the settings here, use io.open, io.write
	print("YAAA ME SAVED TEH ZETTINGZ!")
	print("MY SCRIPT UID IS: " .. _g_ScriptUID)
end

--[[
local function test()
	--print("ID 0 Name: ".._client.GetPlayerName(0))
	
	Game.Chat:Say(0, "Hi")
	-- or Game.Client.Chat:Say(0, "Hi"), you can decide :D
end

RegisterEvent("OnRenderScoreboard", test)
]] --
