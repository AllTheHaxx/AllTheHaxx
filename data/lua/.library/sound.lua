-- Sound library made by YoungFlyme with ♥

local Sound = {}
local Channel = {}

function LoadSound(Path,Name) -- Dont need a Name but its recommended
	local NewSound = Game.Sound:LoadSoundOpus(Path)
	if NewSound == -1 then
		NewSound = Game.Sound:LoadSoundWave(Path)
		if NewSound == -1 then
			print("[Sound]: Got some errors by opening a sound file! (Incorrect path or file format?)")
			throw("[Sound]: Error while trying to load a sound file! (wrong format or path)")
			return false
		else
			table.insert(Sound,NewSound)
			if Name ~= nil then
				Sound[tostring(Name)] = Sound[#Sound] -- Make a table entry with a name (when given)
			end
			return true
		end
	else
		table.insert(Sound,NewSound)
		if Name ~= nil then
			Sound[tostring(Name)] = Sound[#Sound]
		end
		return true
	end
end

function PlaySound(ID,ChannelID,Flag)
	if type(ID) ~= "number" and type(ID) ~= "string" then
		print("[Sound]: Wrong ID format for playing a sound. Only Strings or numbers")
		throw("[Sound]: Error while trying to play a sound (wrong ID type)")
		return false
	else
		if Sound[ID] ~= nil and ChannelID ~= nil then
			Game.Sound:PlaySound(type(ChannelID) == "number" and ChannelID or GetChannel(ChannelID),Sound[ID],Flag or 0)
			return true
		else
			print("[Sound]: Cannot play a non loaded sound!")
			throw("[Sound]: Error while trying to play a sound (not initialized)")
			return false
		end
	end
end

function SetChannel(ID,Name,Volume,Panning)
	if ID ~= nil and Volume ~= nil and AutoPlay ~= nil then
		if type(ID) == "number" then
			Volume = tonumber(Volume)
			if type(Panning) == "number" then
				Panning = Panning
			elseif type(Panning) ~= "number" then
				if type(tonumber(Panning)) == "number" then
					Panning = tonumber(Panning) ~= 0
				else
					print("[Sound]: Channel Edit - Panning var need to be a number!")
					throw("[Sound]: Error while editing a channel (wrong Panning format)")
					return false
				end
			end
			if type(Volume) ~= "number" then
				print("[Sound]: Channel Edit - Volume var need to be a number!")
				throw("[Sound]: Error while editing a channel (wrong Volume format)")
				return false
			end
			Game.Sound:SetChannel(ID, 1, 0)
		elseif type(ID) == "string" then
			ID = GetChannel(ID)
			EditChannel(ID,Volume,Panning)
			return true
		end
	end
end

function GetChannel(Name)
	if type(Name) ~= "string" then
		print("[Sound]: Cannot get a channel by name without a vaild string!")
		throw("[Sound]: Error while getting a channel id (wrong Name format)")
	else
		return Channel[Name]
	end
end

function StopAllSounds()
	Game.Sound:StopAllSounds()
end

function StopSound(ID) -- ID can be Name OR ID
	if Sound[ID] ~= nil then
		Game.Sound:StopSound(Sound[ID])
		return true
	end
end

function OnScriptUnload()
	for i=1,#Sounds do
		Game.Sound:UnloadSound(Sounds[i])
	end
	print("all sounds a unloaded for not leaking memory")
end
