--[[#!
	#debug
]]--#

local function printkv(k, v)
    print(tostring(k),"=",tostring(v))
end


function locals()
    local variables = {}
    local idx = 1
    while true do
        local ln, lv = debug.getlocal(2, idx)
        if ln ~= nil then
            variables[ln] = lv
        else
            break
        end
        idx = 1 + idx
    end
    print("----- LOCALS -----")
    for k,v in next, variables do printkv(k,v) end
    print("--- END LOCALS ---")
    return variables
end

function globals()
    print("----- GLOBALS -----")
    for k,v in next, _G do printkv(k,v) end
    print("--- END GLOBALS ---")
    return _G
end

function upvalues()
    local variables = {}
    local idx = 1
    local func = debug.getinfo(2, "f").func
    while true do
        local ln, lv = debug.getupvalue(func, idx)
        if ln ~= nil then
            variables[ln] = lv
        else
            break
        end
        idx = 1 + idx
    end
    print("----- UPVALUES -----")
    for k,v in next, variables do printkv(k,v) end
    print("--- END UPVALUES ---")

    return variables
end
