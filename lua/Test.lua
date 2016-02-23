local function xyz()
print("xyz\n")
end

local function abc()
	print("abc\n")
end

RegisterEvent("OnEnterGame", xyz)
RegisterEvent("OnEnterGame", abc)