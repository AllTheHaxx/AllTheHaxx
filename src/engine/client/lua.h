#ifndef ENGINE_CLIENT_LUA_H
#define ENGINE_CLIENT_LUA_H

#include <string>
#include <lua.hpp>
#include <base/tl/array.h>
#include <engine/external/zlib/zlib.h>
#include "luafile.h"
#include "db_sqlite3.h"

#define LUA_FIRE_EVENT(EVENTNAME, ...) \
	if(!g_StealthMode && g_Config.m_ClLua) \
	{ \
		const int __NumLuaFiles = CLua::Client()->Lua()->GetActiveLuaFiles().size(); \
		for(int ijdfg = 0; ijdfg < __NumLuaFiles; ijdfg++) \
		{ \
			CLuaFile *pLF = CLua::Client()->Lua()->GetActiveLuaFiles()[ijdfg]; \
			if(pLF->State() != CLuaFile::STATE_LOADED) \
				continue; \
			LuaRef lfunc = pLF->GetFunc(EVENTNAME); \
			if(lfunc) try { lfunc(__VA_ARGS__); CLua::Client()->LuaCheckDrawingState(pLF->L(), EVENTNAME); } catch(std::exception &e) { CLua::Client()->Lua()->HandleException(e, pLF); } \
		} \
		if(CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pDebugChild == NULL) \
		{ \
			LuaRef confunc = getGlobal(CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pLuaState, EVENTNAME); \
			if(confunc) try { confunc(__VA_ARGS__); } catch(std::exception &e) { CLua::Client()->Lua()->HandleException(e, CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pLuaState); } \
		} \
	}


class IClient;
class CClient;
class IStorageTW;
class IGameClient;
class CGameClient;
class CLuaFile;

using namespace luabridge;

class CLua
{
	IStorageTW *m_pStorage;
	class IConsole *m_pConsole;
	class CSql *m_pDatabase;

	array<CLuaFile*> m_apLuaFiles;
	array<CLuaFile*> m_apActiveScripts;
	array<std::string> m_aAutoloadFiles;
	CLuaFile *m_pFullscreenedScript;

public:
	CLua();
	~CLua();

	void Init(IClient *pClient, IStorageTW *pStorage, IConsole *pConsole);
	bool Inited() const { return m_pClient != NULL && m_pStorage != NULL && m_pConsole != NULL; }
	void Shutdown();
	void Reload();
	void AddUserscript(const char *pFilename);
	void LoadFolder();
	void LoadFolder(const char *pFolder);
	void SortLuaFiles();
	int UnloadAll();

	void StartReceiveEvents(CLuaFile *pLF);
	void StopReceiveEvents(CLuaFile *pLF);

	static int ErrorFunc(lua_State *L);
	static int Panic(lua_State *L);
	int HandleException(std::exception &e, lua_State *L);
	int HandleException(std::exception &e, CLuaFile *pLF, bool CalledFromUnloadFromExceptionHandler = false);
	int HandleException(const char *pError, lua_State *L);
	int HandleException(const char *pError, CLuaFile *pLF, bool CalledFromUnloadFromExceptionHandler = false);

	static IClient *m_pClient;
	static IGameClient *m_pGameClient;
	static IClient *Client() { return m_pClient; }
	static IGameClient *GameClient() { return m_pGameClient; }
	static CGameClient * m_pCGameClient;

	void SetGameClient(IGameClient *pGameClient);
	array<CLuaFile*> &GetLuaFiles() { return m_apLuaFiles; }
	const array<CLuaFile*> &GetActiveLuaFiles() const { return m_apActiveScripts; }
	int NumActiveScripts() const
	{
		/*int num = 0;
		for(int i = 0; i < m_apLuaFiles.size(); i++)
			if(m_apLuaFiles[i]->State() == CLuaFile::STATE_LOADED)
				num++;
		return num;*/
		return m_apActiveScripts.size();
	}

	void AddAutoload(const CLuaFile *pLF);
	void RemoveAutoload(const CLuaFile *pLF);
	void ScriptEnterFullscreen(CLuaFile *pLF);
	void ExitFullscreen();
	CLuaFile *GetFullscreenedScript() const { return m_pFullscreenedScript; }

	IStorageTW *Storage() const { return m_pStorage; }

	static void DbgPrintLuaStack(lua_State *L, const char *pNote = 0);

private:
	static int LoadFolderCallback(const char *pName, const char *pFullPath, int IsDir, int DirType, void *pUser);

};


class CQueryAutoloads : public CQuery
{
	array<std::string> *m_paAutoloadFiles;

public:
	CQueryAutoloads(char *pQueryBuf, array<std::string> *paAutoloadFiles) : CQuery(pQueryBuf), m_paAutoloadFiles(paAutoloadFiles)
	{
	}

	virtual void OnData();
};


#endif
