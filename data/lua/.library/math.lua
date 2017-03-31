

function clamp(val, min, max)
    if val < min then return min elseif val > max then return max else return val end
end

function mix(a, b, amount)
    return a + (b-a)*amount
end

function round(val, decimals)
    decimals = decimals or 3
    return math.floor(val*(10^decimals))/(10^decimals)
end
