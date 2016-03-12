g_ScriptTitle = "Guessing Game"
g_ScriptInfo = "Guess my numb0r, dude. | by the AllTheHaxx-Team"

MaxNum = 100
math.randomseed( os.time() )
MyNum = math.random(MaxNum)
Game.Chat:Say(0, "Guess my number! (1-"..MaxNum.."). Use: .guess <number> PLEASE USE TEAMCHAT")

DELAY = 3
LastGuess = 0

function Guesser(ID, Team, Msg)
    if ID == -1 then return end

    if Game.Client.LocalTime < LastGuess + DELAY then return end

    if Msg:find(".guess") then
        msg = Msg:gsub(".guess ", "")
        num = tonumber(msg)
       
        if MyNum == num then
            Game.Chat:Say(0, Game.Players(ID).Name .. " won! The number was: " .. num)
            KillScript(g_ScriptUID)
        else
            if MyNum > num then
                string1 = "bigger"
            else
                string1 = "smaller"
            end
            Game.Chat:Say(0, Game.Players(ID).Name .. " guessed " .. msg .. ". The searched number is " .. string1 .. ". Next guess in " .. DELAY .. " seconds")
        end
        LastGuess = Game.Client.LocalTime
    end
end
 
RegisterEvent("OnChat", "Guesser")
