#ifndef ENGINE_CLIENT_LUA_H
#define ENGINE_CLIENT_LUA_H

#include <lua.hpp>
#include <base/tl/array.h>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>
//#include <game/client/gameclient.h>

class IClient;
class IStorage;
class IGameClient;

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
	void LoadFolder(const char *pFolder);
	
	static int ErrorFunc(lua_State *L);
    static int Panic(lua_State *L);


	static IGameClient *m_pGameClient;
	static lua_State *m_pStaticLua;
	static IClient * Client()  { return m_pClient; }
	static IGameClient *GameClient() { return m_pGameClient; }
	
	void SetGameClient(IGameClient *pGameClient) { CLua::m_pGameClient = pGameClient; }


	struct LuaLoadHelper
	{
		CLua * pLua;
		const char * pString;
	};
	
	struct LuaFile // for now it seems useless, but maybe we'll need this later
	{
		char aName[128];
		char aDir[512];
	};

    array<LuaFile> GetLuaFiles() { return m_LuaFiles; }


private:
	lua_State *m_pLuaState;
	array<LuaFile> m_LuaFiles;
	
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);
	
	static IClient * m_pClient;
	IStorage *m_pStorage;
};

#endif
