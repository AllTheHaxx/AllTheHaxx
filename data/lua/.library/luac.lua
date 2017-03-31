--[[#!
	#io
]]--#


--[[
#	USAGE:
#		Import("luac") -- import this file
#		luac("out.clc", "a.lua", "b.lua") -- compiles a.lua and b.lua + links them into out.clc
#
]]

function luac(outfile, ...)
	local arg = {...}
	local chunk = {}
	for _, file in ipairs(arg) do
		chunk[#chunk + 1] = assert(loadfile(file))
		print(" -> " .. file)
	end
	if #chunk == 1 then
		chunk = chunk[1]
	else
		-- combine multiple input files into a single chunk
		for i, func in ipairs(chunk) do
			chunk[i] = ("loadstring%q(...);"):format(string.dump(func))
		end
		chunk = assert(loadstring(table.concat(chunk)))
	end

	f, err = io.open(outfile, "w+b")
	if f == nil then
		print(" !! ERROR: " .. err)
		return 1
	end
	print(" <- " .. outfile)
	f:write(string.dump(chunk))
	f:flush()
	f:close()
	
	--return #chunk
end

