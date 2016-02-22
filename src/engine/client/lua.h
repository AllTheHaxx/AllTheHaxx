#pragma once

#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>

class IClient;
class IStorage;

using namespace luabridge;

class CLua
{
public:
    CLua();
    ~CLua();
	
	void Init(IClient * pClient, IStorage * pStorage);
	void RegisterLuaCallbacks();
	LuaRef GetFunc(const char *pFuncName);
	void CallFunc(const char *pFuncName);
	bool LoadFile(const char *pFilename); 
	void LoadFolder(char *pFolder);
	
	static int ErrorFunc(lua_State *L);
    static int Panic(lua_State *L);
	
	
	static lua_State *m_pStaticLua;
	static IClient * Client()  { return m_pClient; }
	
	struct LuaLoadHelper
	{
		CLua * pLua;
		const char * pString;
	};
	
private:
	lua_State *m_pLuaState;
	
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);
	
	static IClient * m_pClient;
	IStorage *m_pStorage;
};
