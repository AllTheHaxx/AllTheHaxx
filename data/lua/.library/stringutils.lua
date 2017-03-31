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

function _StringRTrim(str)
	local n = #str
	while n > 0 and str:find("^%s", n) do n = n - 1 end
	return str:sub(1, n)
end

_toString = {

	vec2 = function (v)
        if type(v) ~= "userdata" then error("_toString.vec2 expects a vec2f (got " .. type(v) .. ")", 2) end
		return "(" .. v.x .. "|" .. v.y .. ")"
	end,

	vec3 = function (v)
        if type(v) ~= "userdata" then error("_toString.vec3 expects a vec3 f (got " .. type(v) .. ")", 2) end
		return "(" .. v.x .. "|" .. v.y .. "|" .. v.z .. ")"
	end,

	vec4 = function (v)
        if type(v) ~= "userdata" then error("_toString.vec4 expects a vec4f (got " .. type(v) .. ")", 2) end
		return "(" .. v.r .. "|" .. v.g .. "|" .. v.b .."|" .. v.a .. ")"
	end,

}
