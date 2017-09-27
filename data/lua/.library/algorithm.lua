Import("types")

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

function range(tbl, first, last)
    first = first or 1
    last = last or #tbl
    local new_tbl = {}
    for i = first, last do
        table.insert(new_tbl, tbl[i])
    end
    return new_tbl
end

function dump_custom_type(o)
    if is_UIRect(o) then
        return "UIRect(" .. o.x .. ", " .. o.y .. ", " .. o.w .. ", " .. o.h .. ")"
    elseif is_vec4(o) then
        return "vec4" .. string.gsub(tostring(o), "|", ", ")
    elseif is_vec3(o) then
        return "vec3" .. string.gsub(tostring(o), "|", ", ")
    elseif is_vec2(o) then
        return "vec2" .. string.gsub(tostring(o), "|", ", ")
    end
    return '"<' .. tostring(o) .. '>"'
end

function dump(o)
    if type(o) == 'table' then
        local s = '{ '
        for k,v in pairs(o) do
            if type(k) ~= 'number' then k = '"'..k..'"' end
            s = s .. '['..k..'] = ' .. dump(v) .. ', '
        end
        s = s:sub(1, -3) .. ' ' -- remove trailing comma
        return s .. '} '
    elseif type(o) == 'string' then
        return '"' .. o .. '"'
    elseif type(o) == 'number' then
        return tostring(o)
    else
        return dump_custom_type(o)
    end
end
