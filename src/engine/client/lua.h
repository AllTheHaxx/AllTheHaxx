#ifndef ENGINE_CLIENT_LUA_H
#define ENGINE_CLIENT_LUA_H

#include <string>
#if defined(FEATURE_LUA)
#include <lua.hpp>
#endif
#include <base/tl/array.h>
#include <engine/external/openssl/sha.h>
#include <engine/external/zlib/zlib.h>
#include "luafile.h"

#if defined(FEATURE_LUA)
#define LUA_FIRE_EVENT(EVENTNAME, ...) \
	{ \
		if(g_Config.m_ClLua) \
			for(int ijdfg = 0; ijdfg < Client()->Lua()->GetLuaFiles().size(); ijdfg++) \
			{ \
				if(Client()->Lua()->GetLuaFiles()[ijdfg]->State() != CLuaFile::STATE_LOADED) \
					continue; \
				LuaRef lfunc = Client()->Lua()->GetLuaFiles()[ijdfg]->GetFunc(EVENTNAME); \
				if(lfunc) try { lfunc(__VA_ARGS__); } catch(std::exception &e) { Client()->Lua()->HandleException(e, Client()->Lua()->GetLuaFiles()[ijdfg]); } \
			} \
			LuaRef confunc = getGlobal(CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pLuaState, EVENTNAME); \
			if(confunc) try { confunc(__VA_ARGS__); } catch(std::exception &e) { printf("LUA EXCEPTION: console: %s\n", e.what()); } \
	}
#else
#define LUA_FIRE_EVENT(EVENTNAME, ...) ;
#endif

class IClient;
class CClient;
class IStorageTW;
class IGameClient;
class CGameClient;
class CLuaFile;

#if defined(FEATURE_LUA)
using namespace luabridge;
#endif

struct LuaBinaryCert
{
	char aIssuer[64];
	char aDate[64];
	unsigned char aHashMD[SHA256_DIGEST_LENGTH];
	int PermissionFlags;
};

struct LuaCertHeader
{
	enum { LUA_CERT_VERSION = 3 };
	short Version;
	bool FileBigEndian;
	int DataSize;
};

class CUnzip
{

};

class CLua
{
	array<CLuaFile*> m_pLuaFiles;
	array<std::string> m_aAutoloadFiles;
	IStorageTW *m_pStorage;
	class IConsole *m_pConsole;

public:
	CLua();
	~CLua();

	void Init(IClient *pClient, IStorageTW *pStorage, IConsole *pConsole);
	void Shutdown();
	void SaveAutoloads();
	void AddUserscript(const char *pFilename);
	void LoadFolder();
	void LoadFolder(const char *pFolder);
	void SortLuaFiles();


	static int ErrorFunc(lua_State *L);
	static int Panic(lua_State *L);
	int HandleException(std::exception &e, lua_State *L);
	int HandleException(std::exception &e, CLuaFile *pLF);
	int HandleException(const char *pError, lua_State *L);
	int HandleException(const char *pError, CLuaFile *pLF);

	static CClient * m_pCClient;
	static IClient *m_pClient;
	static IGameClient *m_pGameClient;
	static IClient *Client() { return m_pClient; }
	static IGameClient *GameClient() { return m_pGameClient; }
	static CGameClient * m_pCGameClient;

	void SetGameClient(IGameClient *pGameClient);
	array<CLuaFile*> &GetLuaFiles() { return m_pLuaFiles; }

	IStorageTW *Storage() const { return m_pStorage; }

	struct LuaLoadHelper
	{
		MACRO_ALLOC_HEAP()
	public:
		CLua * pLua;
		const char * pString;
	};

private:
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);

};

#endif
