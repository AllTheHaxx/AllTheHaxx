--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test()
	emote = math.floor(math.random()*100)%16
	_game.emote.Send(emote)
	_game.chat.Send(0 , "LUA-BOT got chat!! triggering random emote " .. emote)
end

print("shit got loaded!! :D")

RegisterEvent("OnChat", test)
