/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* (c) MAP94 */
#ifndef GAME_CLIENT_LUA_H
#define GAME_CLIENT_LUA_H

#include <engine/shared/lua.h>

class CLuaClient : public CLua
{
public:
    CLuaClient();
    ~CLuaClient();
};

#endif
