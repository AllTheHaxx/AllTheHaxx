g_ScriptTitle = "/disabledmg all - To be used on iF|City"
g_ScriptInfo = "Activate this and wait a minute to become the most peaceful tee on the server! =)"

time = Game.Client.LocalTime
i = 0
function OnTick()
	if(Game.Players(i).Name == "") then
		i = i + 1
	end
	if(Game.Client.LocalTime > time + 3) then
		print("Iteration " .. i)
		if(Game.Players(i).Name ~= "") then
		print("Disabling damage to " .. Game.Players(i).Name)
			Game.Chat:Say(0, "/disabledmg " .. Game.Players(i).Name)
			time = Game.Client.LocalTime
		end
		i = i + 1
	end
	if(i > 32) then
		KillScript(g_ScriptUID)
		print("DISABLE? DONE!")
	end
end

RegisterEvent("OnTick", "OnTick")
