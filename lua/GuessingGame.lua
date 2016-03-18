g_ScriptTitle = "Guessing Game"
g_ScriptInfo = "Guess my numb0r, dude. | by the AllTheHaxx-Team"

MAX_NUM = 500
math.randomseed( os.time() )
MyNum = math.random(MAX_NUM)
Game.Chat:Say(0, "Guess my number! (1-"..MAX_NUM.."). Use: .guess <number> PLEASE USE TEAMCHAT")

DELAY = 2
LastGuess = 0

NumTries = 0
Highscores = {}

function OnScriptInit()
	if not Import(g_ScriptUID, ".GuessingGame_Highscores_" .. MAX_NUM .. ".config") then
		print("Could not load highscores.")
	else
		print("Highscores loaded.")
	end
	return true
end

function SaveHighscores()
	print("SAVING HIGHSCORES")
	file = io.open("lua/.GuessingGame_Highscores_" .. MAX_NUM .. ".config", "w+")
	for k, v in next, Highscores do
		file:write("Highscores[\"" .. k .. "\"] = " .. v .. "\n")
	end
	file:flush()
	file:close()
	KillScript(g_ScriptUID)
end

function Guesser(ID, Team, Msg)
    if ID == -1 then return end
	
    if Game.Client.LocalTime < LastGuess + DELAY then return end

    if Msg:find(".guess") then
    
    	-- initialize the table
		if(Highscores[Game.Players(ID).Name] == nil) then
			Highscores[Game.Players(ID).Name] = 0
		end
	
        msg = Msg:gsub(".guess ", "")
        num = tonumber(msg)
        
        NumTries = NumTries + 1
       
        if MyNum == num then
	        winner = Game.Players(ID).Name
	        winniwie = "as best as his highscore"
	        if(Highscores[winner] > 0) then
		    	if(NumTries < Highscores[winner]) then
		    		winniwie = Highscores[winner]-NumTries .." better than his highscore"
		    	end
		    	if(NumTries > Highscores[winner]) then
		    		winniwie = NumTries-Highscores[winner] .. " worse than his highscore"
		    	end
		    else
		    	winniwie = " ranking with that"
		    end
        	winniwie = winniwie .. " (" .. MAX_NUM .. "-league)"
        	print(winner .. " won after " .. NumTries .. " tries, " .. winniwie .. "! The number was: " .. num)
            Game.Chat:Say(0, winner .. " won with " .. NumTries .. " tries, " .. winniwie .. "! The number was: " .. num)
            if(NumTries < Highscores[winner]) then
            	Highscores[winner] = NumTries
            end
            SaveHighscores()
            --KillScript(g_ScriptUID)
        else
            if MyNum > num then
                string1 = "bigger"
            else
                string1 = "smaller"
            end
            Game.Chat:Say(0, Game.Players(ID).Name .. " guessed " .. msg .. ". The searched number is " .. string1 .. ". Next guess in " .. DELAY+1 .. " seconds")
        end
        LastGuess = Game.Client.LocalTime
    end
end
 
RegisterEvent("OnChat", "Guesser")
