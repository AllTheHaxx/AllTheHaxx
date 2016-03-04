#ifndef ENGINE_CLIENT_LUA_H
#define ENGINE_CLIENT_LUA_H

#include <lua.hpp>
#include <base/tl/array.h>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>
#include "luafile.h"

#define LUA_FIRE_EVENT(EVENTNAME, ...) \
	for(int ijdfg = 0; ijdfg < Client()->Lua()->GetLuaFiles().size(); ijdfg++) \
	{ \
		if(Client()->Lua()->GetLuaFiles()[ijdfg]->State() != CLuaFile::LUAFILE_STATE_LOADED) \
			continue; \
		LuaRef lfunc = Client()->Lua()->GetLuaFiles()[ijdfg]->GetFunc(EVENTNAME); \
		if(lfunc) try { lfunc(__VA_ARGS__); } catch(std::exception &e) { printf("LUA EXCEPTION: %s\n", e.what()); } \
	} \
	LuaRef confunc = getGlobal(CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pLuaState, EVENTNAME); \
	if(confunc) try { confunc(__VA_ARGS__); } catch(std::exception &e) { printf("LUA EXCEPTION: %s\n", e.what()); } 

class IClient;
class CClient;
class IStorage;
class IGameClient;
class CGameClient;
class CLuaFile;

using namespace luabridge;

class CLua
{
	array<CLuaFile*> m_pLuaFiles;
	IStorage *m_pStorage;

public:
    CLua();
    ~CLua();
	
	void Init(IClient *pClient, IStorage *pStorage);
	void AddUserscript(const char *pFilename);
	void LoadFolder();

	static int ErrorFunc(lua_State *L);
    static int Panic(lua_State *L);

	static CClient * m_pCClient;
	static IClient *m_pClient;
	static IGameClient *m_pGameClient;
	static IClient *Client() { return m_pClient; }
	static IGameClient *GameClient() { return m_pGameClient; }
	static CGameClient * m_pCGameClient;
	
	void SetGameClient(IGameClient *pGameClient);
	array<CLuaFile*> &GetLuaFiles() { return m_pLuaFiles; }

	IStorage *Storage() const { return m_pStorage; }

	struct LuaLoadHelper
	{
		CLua * pLua;
		const char * pString;
	};

private:
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);

};

#endif
