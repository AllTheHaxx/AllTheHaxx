#include "lua.h"

CLua::CLua()
{
    //m_pProtocol = 0;

    m_pLua = luaL_newstate();
    lua_atpanic(m_pLua, this->Panic);
    lua_register(m_pLua, "errorfunc", this->ErrorFunc);

    //dont use openlibs. this include io and os functions
    //luaL_openlibs(m_pLua);
    luaopen_base(m_pLua);
    luaopen_math(m_pLua);
    luaopen_string(m_pLua);
    luaopen_table(m_pLua);
    //luaopen_io(m_pLua);
    //luaopen_os(m_pLua);
    //luaopen_package(m_pLua); //not sure whether we should load this
    luaopen_debug(m_pLua);
    luaopen_bit(m_pLua);
    luaopen_jit(m_pLua);
    luaopen_ffi(m_pLua); //dont know about this yet. could be a sand box leak.
    //ffi looks nice

    lua_pushnumber(m_pLua, 0);
    lua_setglobal(m_pLua, "Tick");

    lua_pushlightuserdata(m_pLua, this);
    lua_setglobal(m_pLua, "pLUA");

    //register custom modules
    //outsource this in a register function
    //client/setver -> base

    //register vec
    CLuaVec2::Init(m_pLua);
    CLuaVec3::Init(m_pLua);
    CLuaVec4::Init(m_pLua);

    //register protocol
    //m_pProtocol->Init(m_pLua);
    //we cant register the protocol here

    //entity controller
    m_EntityController.Init(m_pLua);

}

CLua::~CLua()
{
    //shutdown
    FunctionExec("atexit");



    lua_close(m_pLua);
}

bool CLua::LoadFile(const char *pFilename)
{
    str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
    int Status = luaL_loadfile(m_pLua, pFilename);
    if (Status)
    {
        //does this work?
        ErrorFunc(m_pLua);
        return false;
    }

    Status = lua_pcall(m_pLua, 0, LUA_MULTRET, 0);
    if (Status)
    {
        ErrorFunc(m_pLua);
        return false;
    }
    return true;
}


void CLua::PushString(const char *pString)
{
    if (m_pLua == 0)
        return;
    lua_pushstring(m_pLua, pString);
    m_FunctionVarNum++;
}
void CLua::PushData(const char *pData, int Size)
{
    if (m_pLua == 0)
        return;
    lua_pushlstring(m_pLua, pData, Size);
    m_FunctionVarNum++;
}
void CLua::PushInteger(int value)
{
    if (m_pLua == 0)
        return;
    lua_pushinteger(m_pLua, value);
    m_FunctionVarNum++;
}
void CLua::PushFloat(float value)
{
    if (m_pLua == 0)
        return;
    lua_pushnumber(m_pLua, value);
    m_FunctionVarNum++;
}
void CLua::PushBoolean(bool value)
{
    if (m_pLua == 0)
        return;
    lua_pushboolean(m_pLua, value);
    m_FunctionVarNum++;
}
bool CLua::FunctionExist(const char *pFunctionName)
{
    bool Ret = false;
    if (m_pLua == 0)
        return false;
    lua_getglobal(m_pLua, pFunctionName);
    Ret = lua_isfunction(m_pLua, -1);
    lua_pop(m_pLua, 1);
    return Ret;
}
void CLua::FunctionPrepare(const char *pFunctionName)
{
    if (m_pLua == 0)
        return;
    lua_getglobal(m_pLua, pFunctionName);
    m_FunctionVarNum = 0;
}
int CLua::FunctionExec(const char *pFunctionName)
{
    if (m_pLua == 0)
        return 0;
    if (pFunctionName)
    {
        if (FunctionExist(pFunctionName) == false)
            return 0;
        FunctionPrepare(pFunctionName);
    }
    int Ret = lua_pcall(m_pLua, m_FunctionVarNum, LUA_MULTRET, 0);
    if (Ret)
        ErrorFunc(m_pLua);
    m_FunctionVarNum = 0;
    return Ret;
}

//error handling

int CLua::Panic(lua_State *L)
{
    dbg_break();
    return 0;
}

void CLua::ErrorHandler(char *pError)
{
    //todo.
}

int CLua::ErrorFunc(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLua *pSelf = (CLua *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    int depth = 0;
    int frameskip = 1;
    lua_Debug frame;
    if (lua_tostring(L, -1) == 0)
        return 0;
    dbg_msg("Lua", pSelf->m_aFilename);
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

int CLua::Print(lua_State *L)
{
    lua_getglobal(L, "pLUA");
    CLua *pSelf = (CLua *)lua_touserdata(L, -1);
    if (lua_isstring(L, 1))
        dbg_msg("Lua", "%s", lua_tostring(L, 1));

    return 0;
}
