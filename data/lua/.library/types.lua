
function is_userdata(var)
	return type(var) == "userdata"
end

function is_vec2(var)
	if not is_userdata(var) then return false end
	if var.x == nil or var.y == nil then return false end
	return true
end

function is_vec3(var)
	if not is_userdata(var) then return false end
	if var.x == nil or var.y == nil or var.z == nil then return false end
	return true
end

function is_vec4(var)
	if not is_userdata(var) then return false end
	if var.r == nil or var.g == nil or var.b == nil or var.a == nil then return false end
	return true
end

function is_UIRect(var, strict)
	if strict == nil then 
		strict = true
	end
	if not is_userdata(var) then return false end
	if strict then
		if var.HSplitMid == nil or var.HSplitTop == nil or var.HSplitBottom == nil or 
				var.VSplitMid == nil or var.VSplitLeft == nil or var.VSplitRight == nil or 
				var.Margin == nil or var.VMargin == nil or var.HMargin == nil then
			return false
		end
	end
	if var.x == nil or var.y == nil or var.w == nil or var.h == nil then 
		return false 
	end
	return true
end
