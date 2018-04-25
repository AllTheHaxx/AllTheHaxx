Import("stringutils")
Import("config")

SetScriptTitle("Tee Highlighter")
SetScriptInfo("Easily distinguish between friends and foes") -- $$ZZZ/ $$LLA![un]color <r> <g> <b> <name>

_ConfigSet("Names", {})
_ConfigLoad()


ColorList = {
	friendly = vec4(0, 1, 0, 0.75),
	enemy = vec4(1, 0, 0, 0.75),
	mark_green = vec4(0, 1, 0, 0.75),
	mark_red = vec4(1, 0, 0, 0.75),
	mark_blue = vec4(0, 0, 1, 0.75),
	mark_yellow = vec4(1, 1, 0, 0.75),
	mark_pink = vec4(1, 0, 1, 0.75)
}


function OnChatSend(Team, Message)
	if Message:sub(1,1) ~= "!" then return end

	-- check for meta command
	if Message:lower() == "!colorlist" then
		Game.Chat:AddLine(-2, 0, "--- List of player names to colorize tee's for ---")
		for name,command in next,_ConfigGet("Names") do
			Game.Chat:AddLine(-2, 0, " > '" .. name .. "' uses '" .. command .. "'")
		end
		return true
	end

	-- parse the command
	local IsUn = _StringStartsWith(Message:lower(), "!un") 
	if IsUn then
		Message = Message:sub(4, -1)
	else
		Message = Message:sub(2, -1)
	end

	-- figure out what we should do
	local LegalCommand = false
	for k,_ in next, ColorList do
		if _StringStartsWith(Message:lower(), k) then
			LegalCommand = true
			break
		end
	end

	if LegalCommand then
		local command = _StringUnpackArgs(Message)[1]:lower()
		local name = Message:sub(#command+1+1)
		if name:sub(-1,-1) == ' ' then name = name:sub(1, -2) end

		if IsUn then
			if _ConfigGet("Names")[name] == command then
				_ConfigGet("Names")[name] = nil
				Game.Chat:AddLine(-2, 0,"Not highlighting '" .. name .. "' as " .. command .. " anymore")
			else
				Game.Chat:AddLine(-2, 0,"'" .. name .. "' isn't currently highlighted as " .. command .. " (use !un" .. _ConfigGet("Names")[name] .. ")")
			end
		else
			_ConfigGet("Names")[name] = command
			Game.Chat:AddLine(-2, 0,"Highlighting '" .. name .. "' as " .. command)
		end

		return true
	end
end

function PostRenderPlayer(ID, PosX, PosY, DirX, DirY, OtherTeam)
	local name = Game.Players(ID).Name
	local command = _ConfigGet("Names")[name]
	if command ~= nil then
		if ColorList[command] == nil then
			throw("unknown command '" .. command .. "' for player '" .. name .. "', removing it")
			_ConfigGet("Names")[Game.Players(ID).Name] = nil
		end

		local color = ColorList[command]

		Engine.Graphics:TextureSet(-1)
		Engine.Graphics:QuadsBegin()
			Engine.Graphics:SetColor(color.r, color.g, color.b, color.a)
			Game.RenderTools:DrawCircle(PosX, PosY, 28, 32)
		Engine.Graphics:QuadsEnd()
	end
end


function OnScriptRenderInfo(View)
	Label = UIRect(0,0,0,0)

	View:HSplitTop(20, Label, View)
	Game.Ui:DoLabelScaled(Label, "Available Commands:", 15, -1, -1, nil)

	-- some margin at the left
	View:VSplitLeft(35, nil, View)
	
	-- display the meta commands
	View:HSplitTop(5, nil, View)
	View:HSplitTop(15, Label, View)
	Game.Ui:DoLabelScaled(Label, "$$ON5!colorlist $$ZZZ- See all players you have colorized", 12, -1, -1, nil)

	for cmd,_ in next, ColorList do
		View:HSplitTop(5, nil, View)
		View:HSplitTop(15, Label, View)
		Game.Ui:DoLabelScaled(Label, "$$AL5!" .. cmd .. " <player_name> $$ZZZ/ " .. "$$LA5!nu" .. cmd .. " <player_name>", 12, -1, -1, nil)
	end

end
