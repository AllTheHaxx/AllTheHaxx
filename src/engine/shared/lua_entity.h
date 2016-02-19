/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* (c) MAP94 */
#ifndef ENGINE_SHARED_LUAENTITY_H
#define ENGINE_SHARED_LUAENTITY_H
#include <lua.hpp>

struct CLuaEntitiyPrototype
{
    char m_aName[256];
    unsigned long m_TypeID;
};

struct CLuaEntity
{
    CLuaEntitiyPrototype *m_pPrototype;
    unsigned long m_EntityID;
};

class CLuaEntityController
{
    lua_State *m_pLua;
public:
    void Init(lua_State *L);
    static int EntityRegister(lua_State *L);
    static int EntityCreate(lua_State *L);
};






#endif
