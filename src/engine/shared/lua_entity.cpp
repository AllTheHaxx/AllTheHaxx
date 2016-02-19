#include "lua_entity.h"

void CLuaEntityController::Init(lua_State *L)
{
    m_pLua = L;
    lua_register(m_pLua, "EntityRegister", this->EntityRegister);
    lua_register(m_pLua, "EntityCreate", this->EntityCreate);
}

int CLuaEntityController::EntityRegister(lua_State *L)
{
    return 0;
}

int CLuaEntityController::EntityCreate(lua_State *L)
{
    return 0;
}
