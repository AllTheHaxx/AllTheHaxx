g_ScriptTitle = ""
g_ScriptInfo = ""

Import("stringutils")
Import("twutils")
Import("algorithm")

function OnChatSend(_, Message)
	-- syntaktic sugar
	if Message:lower():sub(1, 5) == "//kick" or Message:lower():sub(1, 5) == "//spec" then
		Message = "/vote " .. Message:sub(3, -1)
	end

	if Message:lower():sub(1, 5) ~= "/vote" then return end

	args = StringUnpackArgs(Message)
	if #args < 3 then
		Game.Chat:Print(-2, 0, "Syntax: /vote [kick|spec] [name]")
		return true
	end

	local action = args[2]:lower()
	local name = StringFromTable(range(args, 3))
	local victim = GetPlayerID(name)

	if victim == nil then
		Game.Chat:Print(-2, 0, "There was no player with the name '" .. name .. "' found!")
		return true
	end
	if action == "kick" then
		print("Calling vote to kick ID=" .. victim .. " '" .. name .. "'")
		Game.Voting:CallvoteKick(victim, "", false)
	elseif action:sub(1,4) == "spec" then
		print("Calling vote to spec ID=" .. victim .. " '" .. name .. "'")
		Game.Voting:CallvoteSpectate(victim, "", false)
	else
		Game.Chat:Print(-2, 0, "Syntax: /vote [kick|spec] [name]")
	end

	return true
end
