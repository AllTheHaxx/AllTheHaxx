/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* (c) MAP94 */
#ifndef ENGINE_SHARED_LUA_H
#define ENGINE_SHARED_LUA_H

#include <lua.hpp>
#include <base/system.h>
#include "lua_vec.h"
#include "lua_protocol.h"
#include "lua_entity.h"

class CLua
{
protected:
    lua_State *m_pLua;
    //CLuaProtocol *m_pProtocol; //interface for client/server
    CLuaEntityController m_EntityController;
    char m_aFilename[256];

public:
    CLua();
    virtual ~CLua();

    virtual bool LoadFile(const char *pFilename); //default loader

    virtual void ErrorHandler(char *pError); //error handler
    static int ErrorFunc(lua_State *L);
    static int Panic(lua_State *L);
    static int Print(lua_State *L);

    //helper
    void PushString(const char *pString);
    void PushData(const char *pData, int Size);
    void PushInteger(int value);
    void PushFloat(float value);
    void PushBoolean(bool value);

    bool FunctionExist(const char *pFunctionName);
    void FunctionPrepare(const char *pFunctionName);
    int FunctionExec(const char *pFunctionName);
    int m_FunctionVarNum;

};

#endif
