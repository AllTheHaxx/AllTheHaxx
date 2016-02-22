#include "lua.h"

#include <engine/storage.h>
#include <engine/client.h>

IClient * CLua::m_pClient = 0; 

using namespace luabridge;

CLua::CLua()
{
}

CLua::~CLua()
{
}

void CLua::Init(IClient * pClient)
{
	m_pClient = pClient;
	
    m_pLuaState = luaL_newstate();
	//m_pStatLua = m_pLua;
	
    lua_atpanic(m_pLuaState, this->Panic);
    lua_register(m_pLuaState, "errorfunc", this->ErrorFunc);

    //dont use openlibs. this include io and os functions
    luaL_openlibs(m_pLuaState);
    luaopen_base(m_pLuaState);
    luaopen_math(m_pLuaState);
    luaopen_string(m_pLuaState);
    luaopen_table(m_pLuaState);
    //luaopen_io(m_pLua);
    luaopen_os(m_pLuaState);
    //luaopen_package(m_pLua); //not sure whether we should load this
    luaopen_debug(m_pLuaState);
    luaopen_bit(m_pLuaState);
    luaopen_jit(m_pLuaState);
    luaopen_ffi(m_pLuaState); //dont know about this yet. could be a sand box leak.
}

int CLua::Panic(lua_State *L)
{
    dbg_break();
    return 0;
}

int CLua::ErrorFunc(lua_State *L)
{
	dbg_msg("Lua", "Lua Script Error! :");
    lua_getglobal(L, "pLUA");
    CLua *pSelf = (CLua *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    int depth = 0;
    int frameskip = 1;
    lua_Debug frame;

    if (lua_tostring(L, -1) == 0)
	{
		//dbg_msg("Lua", "PANOS");
        return 0;
	}
	
    //dbg_msg("Lua", pSelf->m_aFilename);
    dbg_msg("Lua", lua_tostring(L, -1));
    dbg_msg("Lua", "Backtrace:");

    while(lua_getstack(L, depth, &frame) == 1)
    {
        depth++;
        lua_getinfo(L, "nlSf", &frame);
        /* check for functions that just report errors. these frames just confuses more then they help */
        if(frameskip && str_comp(frame.short_src, "[C]") == 0 && frame.currentline == -1)
            continue;
        frameskip = 0;
        /* print stack frame */
        dbg_msg("Lua", "%s(%d): %s %s", frame.short_src, frame.currentline, frame.name, frame.namewhat);
    }
    lua_pop(L, 1); // remove error message
    lua_gc(L, LUA_GCCOLLECT, 0);
    return 0;
}