--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script 2")

-- load much stuff here to show it's working and to see how long it takes
tex = {}

function OnScriptInit()
	tex[0] = Graphics.Engine:LoadTexture("arrow.png", -1, -1, 1)
	tex[1] = Graphics.Engine:LoadTexture("blob.png", -1, -1, 1)
	tex[2] = Graphics.Engine:LoadTexture("browse_icons.png", -1, -1, 1)
	tex[3] = Graphics.Engine:LoadTexture("console_bar.png", -1, -1, 1)
	tex[4] = Graphics.Engine:LoadTexture("debug_font.png", -1, -1, 1)
	tex[5] = Graphics.Engine:LoadTexture("demo_buttons.png", -1, -1, 1)
	tex[6] = Graphics.Engine:LoadTexture("demo_buttons2.png", -1, -1, 1)
	tex[7] = Graphics.Engine:LoadTexture("emoticons.png", -1, -1, 1)
	tex[8] = Graphics.Engine:LoadTexture("file_icons.png", -1, -1, 1)
	tex[9] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[10] = Graphics.Engine:LoadTexture("gui_buttons.png", -1, -1, 1)
	tex[11] = Graphics.Engine:LoadTexture("gui_cursor.png", -1, -1, 1)
	tex[12] = Graphics.Engine:LoadTexture("gui_icons.png", -1, -1, 1)
	tex[13] = Graphics.Engine:LoadTexture("gui_logo.png", -1, -1, 1)
	tex[14] = Graphics.Engine:LoadTexture("menu.png", -1, -1, 1)
	tex[15] = Graphics.Engine:LoadTexture("particles.png", -1, -1, 1)
	tex[16] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1) -- MUCH SPAM FOR MUCH TEST!
	tex[17] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[18] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[19] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[20] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[21] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[22] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[23] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[24] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[25] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[26] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[27] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[28] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[29] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[30] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[31] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[32] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[33] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[34] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[35] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[36] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[37] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[38] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[39] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[40] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[41] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[42] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[43] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[44] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[45] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[46] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[47] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[48] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[49] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[50] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[51] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[52] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[53] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[54] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[55] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[56] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[57] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[58] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[59] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[60] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[61] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	tex[62] = Graphics.Engine:LoadTexture("game.png", -1, -1, 1)
	--tex[63] = Graphics.Engine:LoadTexture("asdfj9dgMEISERROR.png", -1, -1, 1) -- TODO uncomment this to see that failing works
	
	-- check if all the textures got loaded successfully
	AllGood = true
	for k, v in next, tex do
		if v <= 0 then
			AllGood = false
			break
		end
	end
	return AllGood
end

function test(FadeVal)
	w = Graphics.Engine.GetScreenWidth()
	h = Graphics.Engine.GetScreenHeight()
	
	if FadeVal > 0.3 then FadeVal = 0.3 end
	_ui.SetUiColor(0,0,0,FadeVal)
	_ui.DrawUiRect(0, 0, w, h, 0, 0)
	
	-- draw some fancy things here, using the textures :D
	-- ...was to lazy for that right now.
end

function OnScriptUnload()
-- unload all the textures to not leak memory!
	for k, v in next, tex do
		if v > 0 then -- 0 is the "no texture"-texture, we shouldn't unload this!
			print(k .. "=" .. v)
			Graphics.Engine:UnloadTexture(v)
		end
	end
end

RegisterEvent("OnRenderScoreboard", "test")


