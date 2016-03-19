g_ScriptTitle = "Guessing Game"
g_ScriptInfo = "Guess my numb0r, dude. | by the AllTheHaxx-Team"


MAX_RANGE = 999
DELAY = 2

function OnScriptInit()
	Reset()
	LoadHighscores()
	BroadcastGame()
	return true
end


function Reset() 
	math.randomseed( os.time() )
	MaxNum = nil
	League = nil
	MyNum = -1

	LastGuess = 0

	NumTries = 0
	
	TriedNums = nil
	TriedNums = {}
	
	Highscores = nil
	Highscores = {}
end

function SaveHighscores()
	print("Saving Highscores")
	file = io.open("lua/.GuessingGame_Highscores.config", "w+")
	for i, table in next, Highscores do
		for name, score in next, table do
			file:write("if Highscores[" .. i .. "] == nil then Highscores[" .. i .. "] = {} end\n")
			file:write("Highscores[" .. i .. "][\"" .. name .. "\"] = " .. score .. "\n")
		end
	end
	file:flush()
	file:close()
end

function LoadHighscores()
	if not Import(g_ScriptUID, ".GuessingGame_Highscores.config") then
		print("Could not load highscores.")
	else
		print("Highscores loaded.")
	end
end

function BroadcastGame()
	Game.Chat:Say(0, "→→ Guessing Game loaded! Use \"!start <num>\" or \"!rank <league>\"!")
	LastGuess = Game.Client.LocalTime
end

function Guesser(ID, Team, Msg)
    if ID == -1 then return end
    if Game.Client.LocalTime < LastGuess + DELAY then return end
    
    -- stats
    if Msg:find("!rank") then
   		msg = Msg:gsub("!rank ", "")
    	denum = tonumber(msg)
    	chat = "→→ Usage: !rank <league> (league is a number >= 100)"
    	if(denum ~= nil) then
    		if(denum >= 100) then
    			denum = math.floor(denum/100)
    			chat = "→→ " .. Game.Players(ID).Name .. " is not ranked in the " .. denum*100 .. "-league"
    			if(Highscores[denum] ~= nil and Highscores[denum][Game.Players(ID).Name] ~= nil) then
    				chat = "→→ Best round of " .. Game.Players(ID).Name .. " in the " .. denum*100 .. "-league was " .. Highscores[denum][Game.Players(ID).Name] .. " tries."
    			end
    			if(Team ~= 0 and string.lower(Game.ServerInfo.GameMode) == "if|city") then
    				chat = Game.Players(ID).Name .. ": " .. chat
    			end
    			Game.Chat:Say(Team, chat)
    			print(chat)
    			return
    		end
    	end
    	if(Team ~= 0 and string.lower(Game.ServerInfo.GameMode) == "if|city") then
			chat = Game.Players(ID).Name .. ": " .. chat
		end
    	Game.Chat:Say(Team, chat)
    	print(chat)
    	return
    end
    
    -- init the game
    if MaxNum == nil then
    	if Msg:find("!start") then
    		msg = Msg:gsub("!start ", "")
    		chat = "→→ Usage: !start <num> (num from 100 to " .. MAX_RANGE .. ")"
        	denum = tonumber(msg)
        	if(denum ~= nil) then
        		denum = math.floor(denum)
        		if(denum >= 100 and denum <= MAX_RANGE) then
		    		MaxNum = denum
		    		League = math.floor(MaxNum/100)
		    		MyNum = math.random(MaxNum)
		    		print(MyNum.."!!!!!!!!!!!!!!!!!!!!!!!")
		    		infostring = "→→ Guess my number! (1-"..MaxNum.."). Use: .guess <number>"
		    		if(string.lower(Game.ServerInfo.GameMode) == "if|city") then
		    			infostring = infostring .. "PLEASE USE TEAMCHAT"
		    		end
		    		Game.Chat:Say(0,  infostring)
		    		LastGuess = Game.Client.LocalTime
		    	end
        	end
        	if(Team ~= 0 and string.lower(Game.ServerInfo.GameMode) == "if|city") then
				chat = Game.Players(ID).Name .. ": " .. chat
			end
    		Game.Chat:Say(Team, chat)
    		print(chat)
        end
    	return
    end
    
	
    if Msg:find(".guess") then
    
    	-- initialize the table
    	if(Highscores[League] == nil) then
    		Highscores[League] = {}
    	end
		if(Highscores[League][Game.Players(ID).Name] == nil) then
			Highscores[League][Game.Players(ID).Name] = 0
		end
	
        msg = Msg:gsub(".guess ", "")
        num = tonumber(msg)
        
        if (num < 1 or num > MaxNum) then return end
        
        NumTries = NumTries + 1
       
        if MyNum == num then
	        winner = Game.Players(ID).Name
	        winniwie = "as best as his highscore"
	        if(Highscores[League][winner] > 0) then
		    	if(NumTries < Highscores[League][winner]) then
		    		winniwie = Highscores[League][winner]-NumTries .." better than his highscore"
		    	end
		    	if(NumTries > Highscores[League][winner]) then
		    		winniwie = NumTries-Highscores[League][winner] .. " worse than his highscore"
		    	end
		    else
		    	winniwie = "ranking with that"
		    end
        	winniwie = winniwie .. " on " .. League*100 .. "-league"
        	print(winner .. " won after " .. NumTries .. " tries, " .. winniwie .. "! The number was: " .. num)
            Game.Chat:Say(0, winner .. " won with " .. NumTries .. " tries, " .. winniwie .. "! The number was: " .. num)
            if(Highscores[League][winner] <= 0 or NumTries < Highscores[League][winner]) then
            	Highscores[League][winner] = NumTries
            end
            SaveHighscores()
            -- restart the game
            Reset()
            LoadHighscores()
        else
	        if(TriedNums[num] ~= nil and TriedNums[num] == true) then
	        	return
	        end
            if MyNum > num then
                string1 = "bigger"
            else
                string1 = "smaller"
            end
            TriedNums[num] = true
            Game.Chat:Say(0, Game.Players(ID).Name .. " guessed " .. num .. ". The searched number is " .. string1 .. ". Next guess in " .. DELAY+1 .. " seconds")
        end
        LastGuess = Game.Client.LocalTime
    end
end
 
RegisterEvent("OnChat", "Guesser")
