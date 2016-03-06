MaxNum = 100
math.randomseed( os.time() )
MyNum = math.random(MaxNum)
Game.Chat:Say(0, "Guess my number! (1-"..MaxNum.."). Use : .guess number. PLEASE USE TEAMCHAT")
 
LastGuess = 0
 
function Guesser(ID, Team, Msg)
    if ID == -1 then return end
   
    if Game.Client.Tick < LastGuess + 250 then return end
   
    if Msg:find(".guess") then
        msg = Msg:gsub(".guess ", "")
        num = tonumber(msg)
       
        if MyNum == num then
            Game.Chat:Say(0, Game.Players(ID).Name .. " won! The number was : " .. num)
            RemoveEvent("OnChat", "Guesser")
        else
            if MyNum > num then
                string1 = "bigger"
            else
                string1 = "smaller"
            end
            Game.Chat:Say(0, Game.Players(ID).Name .. " guessed " .. msg .. ". The searched number is " .. string1 .. ". Next guess in 5 seconds")
        end
        LastGuess = Game.Client.Tick
    end
 
end
 
RegisterEvent("OnChat", "Guesser")
