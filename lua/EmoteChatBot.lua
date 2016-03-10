--_lua.GetLuaFile():SetScriptTitle("AllThehaxx Awesome Lua API testing script")

local function test(ClientID, Team, Message)
	emote = math.floor(math.random()*100)%16
	Game.Emote:Send(emote)
	--_game.chat.Send(0 , "LUA-BOT got chat!! triggering random emote " .. emote)
	if ClientID >= 0 then
		Game.Chat:Send(0, Game.Players(ClientID).Name .. "(ID:" .. ClientID .. ") said '" .. Message .. "'")
	end
end

print("shit got loaded!! :D")

RegisterEvent("OnChat", "test")
