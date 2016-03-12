g_ScriptTitle = "Chat Logger"
g_ScriptInfo = "Archive your chatlogs. | by the AllTheHaxx-Team"

File = io.open("chat.log", "a")
File:write("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n")
File:write(os.date("~~~~~ STARTED LOGGER AT %y-%m-%d %H:%M:%S ~~~~~\n"))
File:write("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n")
File:flush()

function Logger(ID, Team, Msg)
	local Date = os.date("[%y-%m-%d %H:%M:%S]")
	local Name = ""
	if ID ~= -1 then
		Name = Game.Players(ID).Name
	else
		Name = "***" -- server message
	end

	local Line = Date.." "..Name..": "..Msg.."\n"
	File:write(Line)
	File:flush()
end

RegisterEvent("OnChat", "Logger")
