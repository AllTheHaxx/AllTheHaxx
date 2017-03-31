

--USAGE: for k,v in spairs(TBL, function(t,a,b) return t[b] <- t[a] end) do ... end
function spairs(t, order)
    local keys = {}
    for k in pairs(t) do keys[#keys+1] = k end

    -- if order function given, sort by it by passing the table and keys a, b,
    -- otherwise just sort the keys
    if order then
        table.sort(keys, function(a,b) return order(t, a, b) end)
    else
        table.sort(keys)
    end

    -- return the iterator function
    local i = 0
    return function()
        i = i + 1
        if keys[i] then
            return keys[i], t[keys[i]]
        end
    end
end

function GetKey(tbl, value)
    for k,v in next, tbl do
        if v == value then
            return k
        end
    end
end
