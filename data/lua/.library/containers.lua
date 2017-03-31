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
    iCurrBC[ActiveGroup] = iCurrBC[ActiveGroup] + 1
    if _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]] == nil then
        _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]] = ButtonContainer()
    end
    return _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]]
end

function CurrBC()
    return _ButtonContainers[ActiveGroup][iCurrBC[ActiveGroup]]
end

function SetBC(bc, i)
    i = i or iCurrBC[ActiveGroup]
    _ButtonContainers[ActiveGroup][i] = bc
end

function GetBC(i)
    if i == nil or i == 0 then i = iCurrBC[ActiveGroup] end
    if i < 0 then i = iCurrBC[ActiveGroup] + i end
    return _ButtonContainers[ActiveGroup][i]
end


function NextEBC()
    iCurrEBC[ActiveGroup] = iCurrEBC[ActiveGroup] + 1
    if _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]] == nil then
        _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]] = EditboxContainer()
    end
    return _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]]
end

function CurrEBC()
    return _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]]
end

function SetEBC(ebc, i)
    if i == nil then i = iCurrEBC[ActiveGroup] end
    _EditboxContainers[ActiveGroup][iCurrEBC[ActiveGroup]] = ebc
end

function GetEBC(i)
    if i == nil or i == 0 then i = iCurrEBC[ActiveGroup] end
    if i < 0 then i = iCurrEBC[ActiveGroup] + i end
    return _EditboxContainers[ActiveGroup][i]
end
