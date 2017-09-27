------------------- GENERALLY USEFUL STUFF --------------------

_CWD = ScriptPath():sub(1, -ScriptPath():reverse():find("/")-1) -- has no trailing backslash
_ScriptFileName = ScriptPath():sub(ScriptPath():reverse():find("/")+1, -1)

_copy = {
    UIRect = function(Rect) -- legacy alias
        return Rect:copy()
    end,

    vec2 = function(v)
        if v == nil then error("passed a nil value to _copy.vec2", 2) end
        local NewVec = vec2f(v.x, v.y)
        return NewVec
    end,

    vec3 = function(v)
        if v == nil then error("passed a nil value to _copy.vec3", 2) end
        local NewVec = vec3f(v.x, v.y, v.z)
        return NewVec
    end,

    vec4 = function(v)
        if v == nil then error("passed a nil value to _copy.vec4", 2) end
        local NewVec = vec4f(v.r, v.g, v.b, v.a)
        return NewVec
    end,
}

CopyUIRect = _copy.UIRect -- legacy alias, deprecated!


function int(var)
    local t = type(var)
    if t == "boolean" then return var and 1 or 0 end
    if t == "string" or t == "number" then return math.floor(tonumber(var)+0.5) end
    return nil
end

function GetKey(tbl, value)
    for k,v in next, tbl do
        if v == value then
            return k
        end
    end
end
