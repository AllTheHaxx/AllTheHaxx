-------------------- COMMON FUNCTIONS FOR PROCESSING STRINGS --------------------


function _StringStartsWith(str, find)
	if str:sub(1, find:len()) == find then return true else return false end
end

function _StringSplit(str, split)
	if type(str) ~= "string" or type(split) ~= "string" then error("string expected, got " .. type(str)) end
	local a, b = string.find(str, split, 1, true)
	if (a == nil or b == nil) then return "","" end
	return string.sub(str, 0, a-1), string.sub(str, b+1, -1)
end

function _StringUnpackArgs(str, delim) -- splits a string by spaces into a table
	delim = delim or " "
	local tbl = {}
	str = _StringRTrim(str)
	index = str:find(delim)
	while true do
		if index == nil then
			table.insert(tbl, str)
			break
		end
		table.insert(tbl, str:sub(0, index-1))
		str = str:sub(index+1, -1)
		index = str:find(delim)
	end
	return tbl
end

function StringFromTable(tbl, delim)
	delim = delim or " "
	local str = ""
	for i,v in ipairs(tbl) do
		str = str .. tostring(v) .. delim
	end
	str = str:sub(1, -(1 + #delim))
	return str
end

function _StringRTrim(str)
	local n = #str
	while n > 0 and str:find("^%s", n) do n = n - 1 end
	return str:sub(1, n)
end

-- aliases for the function names
--# NOTE: The actual function names are deprecated and will be replaced by these aliases in a future release
StringStartsWith = _StringStartsWith
StringSplit = _StringSplit
StringUnpackArgs = _StringUnpackArgs
StringRTrim = _StringRTrim

--# DEPRECATED, use the tostring function on your vecs instead
_toString = {
	vec2 = function (v) return tostring(v) end,
	vec3 = function (v) return tostring(v) end,
	vec4 = function (v) return tostring(v) end
}
