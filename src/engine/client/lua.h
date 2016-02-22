#pragma once

#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>

class IClient;

using namespace luabridge;

class CLua
{
public:
    CLua();
    ~CLua();
	
	void Init(IClient * pClient);
	
	static int ErrorFunc(lua_State *L);
    static int Panic(lua_State *L);
	
	
	
	
	static IClient * Client()  { return m_pClient; }
	
private:
	lua_State *m_pLuaState;
	
	static IClient * m_pClient;
};
