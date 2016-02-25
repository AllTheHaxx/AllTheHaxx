local function GreetBot(ClientID, Team, Message)
	if(string.find(Message, _client.GetPlayerName(_client.GetLocalCharacterID()))) then
		if(string.find(string.lower(Message), "hi") or
		string.find(string.lower(Message), "hello") or
		string.find(string.lower(Message), "hey") or
		string.find(string.lower(Message), "yo") or
		string.find(string.lower(Message), "s up") or -- to detect whats up, what's up
		string.find(string.lower(Message), "howdie") or
		string.find(string.lower(Message), "sup")) then
			_game.chat.Send(0, "Hey ".._client.GetPlayerName(ClientID))
		end
	end
end

RegisterEvent("OnChat", GreetBot)