_ButtonContainers = {}
_EditboxContainers = {}

local ActiveGroup = 1
local iCurrBC = {}
local iCurrEBC = {}

function ContainersStart(group)
    group = group or 1
    ActiveGroup = group
    iCurrBC[ActiveGroup] = 0
    iCurrEBC[ActiveGroup] = 0
    if _ButtonContainers[ActiveGroup] == nil then _ButtonContainers[ActiveGroup] = {} end
    if _EditboxContainers[ActiveGroup] == nil then _EditboxContainers[ActiveGroup] = {} end
end


function NextBC()
    if _ButtonContainers[ActiveGroup] == nil or iCurrBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use NextBC()", 2) end
    iCurrBC[ActiveGroup] = iCurrBC[ActiveGroup] + 1
    if _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]] == nil then
        _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]] = ButtonContainer()
    end
    return _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]]
end

function CurrBC()
    if _ButtonContainers[ActiveGroup] == nil or iCurrBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use CurrBC()", 2) end
    return _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]]
end

function SetBC(bc, i)
    if _ButtonContainers[ActiveGroup] == nil or iCurrBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use SetBC()", 2) end
    i = i or iCurrBC[ActiveGroup]
    _ButtonContainers[ActiveGroup][i] = bc
end

function GetBC(i)
    if _ButtonContainers[ActiveGroup] == nil or iCurrBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use GetBC()", 2) end
    if i == nil or i == 0 then i = iCurrBC[ActiveGroup] end
    if i < 0 then i = iCurrBC[ActiveGroup] + i end
    return _ButtonContainers[ActiveGroup][i]
end


function NextEBC()
    if _EditboxContainers[ActiveGroup] == nil or iCurrEBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use NextEBC()", 2) end
    iCurrEBC[ActiveGroup] = iCurrEBC[ActiveGroup] + 1
    if _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]] == nil then
        _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]] = EditboxContainer()
    end
    return _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]]
end

function CurrEBC()
    if _EditboxContainers[ActiveGroup] == nil or iCurrEBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use CurrEBC()", 2) end
    return _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]]
end

function SetEBC(ebc, i)
    if _EditboxContainers[ActiveGroup] == nil or iCurrEBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use SetEBC()", 2) end
    if i == nil then i = iCurrEBC[ActiveGroup] end
    _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]] = ebc
end

function GetEBC(i)
    if _EditboxContainers[ActiveGroup] == nil or iCurrEBC[ActiveGroup] == nil then error("You must use ContainersStart() before you can use GetEBC()", 2) end
    if i == nil or i == 0 then i = iCurrEBC[ActiveGroup] end
    if i < 0 then i = iCurrEBC[ActiveGroup] + i end
    return _EditboxContainers[ActiveGroup][i]
end
