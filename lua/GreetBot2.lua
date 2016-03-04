List = {}
Messages = 
{
	[1] = "/w @name@if @name: DENNIS",
	[2] = "/w @name@if @name: Hi, nice to see you @name! Have a great day :)",
	[3] = "/w @name@if @name: Come, enter, @you has warez, everything you need.",
	[4] = "/w @name@if @name: Hey psst, wanna buy some... magic?",
	[5] = "/w @name@if @name: Heyo, it's a beautiful day today, isn't it?"
}

-- @name = the targets name...
-- @if is for iF servers (they use teamchat -> name: msg)
-- @you = your name

local function GreetAdd(ClientID, Team, Message)
	if ClientID ~= -1 then  --only server messages
		return
	end

	if Message:find("entered") then
		abc = string.match(Message, "'.-'")    -- returns everything between '' => name
		abc = abc:gsub("'", "")                      -- string replace
				
		if abc ~= Config.PlayerName then       --dont greet ourselves!
			List[abc] = Game.Client.Tick + 30         --time stamp
		end
	end
end

local function Greet()
	for key,value in pairs(List) do     --key = name, value = timestamp
		if TW.Client.Tick >= value then
		
			rand = math.random(5)
					
			msg = Messages[rand]:gsub("@name", key)
			msg = msg:gsub("@you", Config.PlayerName)
			
			Mode = TW.Game.ServerInfo.GameMode
			
			print(Mode)

			if Mode == "iF|City" then
				msg = msg:gsub("@if", ":")
				msg = msg:gsub("/w", "")
			elseif Mode:find("DDr") or Mode:find("DDR") then
				msg = msg:gsub("@if", "")
			else     --all other gamemodes!
				msg = msg:gsub("/w", "")
				msg = msg:gsub("@if", ":")
			end
				
			if Mode == "iF|City" then
				TW.Game.Chat:Say(1, msg)
			else
				TW.Game.Chat:Say(0, msg)
			end
			
			List[key] = nil
		end
	end
end

local function SetSeed()
	math.randomseed (os.time ())
end

RegisterEvent("OnChat", "GreetAdd")
RegisterEvent("OnTick", "Greet")
RegisterEvent("OnEnterGame", "SetSeed")