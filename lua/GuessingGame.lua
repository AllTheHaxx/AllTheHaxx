g_ScriptTitle = "Guessing Game"
g_ScriptInfo = "Guess my numb0r, dude. | by the AllTheHaxx-Team"
math.randomseed( os.time() )

MAX_RANGE = 500
MaxNum = nil
MyNum = -1 --math.random(MaxNum)
--Game.Chat:Say(0, "Guess my number! (1-"..MaxNum.."). Use: .guess <number> PLEASE USE TEAMCHAT")

DELAY = 2
LastGuess = 0

NumTries = 0
TriedNums = {}
Highscores = {}

function OnScriptInit()
	LoadHighscores()
	BroadcastGame()
	return true
end


function SaveHighscores()
	print("SAVING HIGHSCORES")
	file = io.open("lua/.GuessingGame_Highscores.config", "w+")
	for i, table in next, Highscores do
		for name, score in next, table do
			file:write("if Highscores[" .. i .. "] == nil then Highscores[" .. i .. "] = {} end\n")
			file:write("Highscores[" .. i .. "][\"" .. name .. "\"] = " .. score .. "\n")
		end
	end
	file:flush()
	file:close()
	KillScript(g_ScriptUID)
end

function LoadHighscores()
	if not Import(g_ScriptUID, ".GuessingGame_Highscores.config") then
		print("Could not load highscores.")
	else
		print("Highscores loaded.")
	end
end

function BroadcastGame()
	Game.Chat:Say(0, "→→ Guessing Game loaded! Use \"!start <MaxNumber>\" (MaxNumber from 100 to " .. MAX_RANGE .. ") to start!")
	LastGuess = Game.Client.LocalTime+1
end

function Guesser(ID, Team, Msg)
    if ID == -1 then return end
    if Game.Client.LocalTime < LastGuess + DELAY then return end
    
    -- init the game
    if MaxNum == nil then
    	if Msg:find("!start") then
    		msg = Msg:gsub("!start ", "")
        	denum = math.floor(tonumber(msg))
        	if(denum ~= nil) then
        		if(denum >= 100 and denum <= MAX_RANGE) then
		    		MaxNum = denum
		    		MyNum = math.random(MaxNum)
		    		print(MyNum.."!!!!!!!!!!!!!!!!!!!!!!!")
		    		Game.Chat:Say(0, "→→ Guess my number! (1-"..MaxNum.."). Use: .guess <number> PLEASE USE TEAMCHAT")
		    	end
        	end
        end
        LastGuess = Game.Client.LocalTime
    	return
    end
    
	
    if Msg:find(".guess") then
    
    	-- initialize the table
    	if(Highscores[MaxNum] == nil) then
    		Highscores[MaxNum] = {}
    	end
		if(Highscores[MaxNum][Game.Players(ID).Name] == nil) then
			Highscores[MaxNum][Game.Players(ID).Name] = 0
		end
	
        msg = Msg:gsub(".guess ", "")
        num = tonumber(msg)
        
        if (num < 1 or num > MaxNum) then return end
        
        NumTries = NumTries + 1
       
        if MyNum == num then
	        winner = Game.Players(ID).Name
	        winniwie = "as best as his highscore"
	        if(Highscores[MaxNum][winner] > 0) then
		    	if(NumTries < Highscores[MaxNum][winner]) then
		    		winniwie = Highscores[MaxNum][winner]-NumTries .." better than his highscore"
		    	end
		    	if(NumTries > Highscores[MaxNum][winner]) then
		    		winniwie = NumTries-Highscores[MaxNum][winner] .. " worse than his highscore"
		    	end
		    else
		    	winniwie = "ranking with that"
		    end
        	winniwie = winniwie .. " on " .. MaxNum .. "-league"
        	print(winner .. " won after " .. NumTries .. " tries, " .. winniwie .. "! The number was: " .. num)
            Game.Chat:Say(0, winner .. " won with " .. NumTries .. " tries, " .. winniwie .. "! The number was: " .. num)
            if(Highscores[MaxNum][winner] <= 0 or NumTries < Highscores[MaxNum][winner]) then
            	Highscores[MaxNum][winner] = NumTries
            end
            SaveHighscores()
            --KillScript(g_ScriptUID)
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
