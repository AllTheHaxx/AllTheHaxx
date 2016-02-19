/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* (c) MAP94 */
#ifndef ENGINE_SHARED_LUAPROTOCOL_H
#define ENGINE_SHARED_LUAPROTOCOL_H
#include "engine/shared/compression.h"
#include "engine/shared/stream.h"
#include <lua.hpp>
/*

enum
{
    LUA_MSG_INVALID = 0,
    //dummy msg

    LUA_MSG_S2C_CHECK_VERSION,
    //static size msg. the struct should never change

    LUA_MSG_S2C_ENTITY_TYPE, //sends a new entity type
    //type_id name

    LUA_MSG_S2C_ENTITY_CREATE,
    //type_id entity_id

    LUA_MSG_S2C_ENTITY_DATA,
    //entity_id data

    LUA_MSG_CUSTOM_DATA,
    //custom_id data
};

struct CLuaPacket
{
    unsigned long long m_Type;
    unsigned long long m_Size;
    char *m_pData;
};


class CLuaProtocol
{
protected:
    lua_State *m_pLua;
    CStream m_RecvBuffer;
    CStream m_SendBuffer;
public:
    virtual void Init(lua_State *L);
    virtual void Update() = 0;
    bool SendRawPacket(CLuaPacket *pPacket);
    bool ProcessPacket(CLuaPacket *pPacket);
};
*/

#endif
