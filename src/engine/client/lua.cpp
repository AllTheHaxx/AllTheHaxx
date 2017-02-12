#include <fstream>

#include <openssl/sha.h>
#include <base/system.h>
#include <engine/storage.h>
#include <engine/client.h>
#include <engine/console.h>
#include <engine/shared/network.h>
#include <game/localization.h>
#include <game/client/gameclient.h>
#include <game/client/components/console.h>

#include "lua.h"
#include "luabinding.h"


IClient * CLua::m_pClient = 0;
CClient * CLua::m_pCClient = 0;
IGameClient * CLua::m_pGameClient = 0;
CGameClient * CLua::m_pCGameClient = 0;

#if defined(FEATURE_LUA)
using namespace luabridge;
#endif

CLua::CLua()
{
	m_pStorage = 0;
	m_pConsole = 0;
}

CLua::~CLua()
{
	Shutdown();
}

void CLua::Init(IClient *pClient, IStorageTW *pStorage, IConsole *pConsole)
{
	CALLSTACK_ADD();

	m_pClient = pClient;
	m_pCClient = (CClient*)pClient;
	m_pStorage = pStorage;
	m_pConsole = pConsole;
	m_aAutoloadFiles.clear();

	//LoadFolder(); // we can't do it that early
}

void CLua::Shutdown()
{
	CALLSTACK_ADD();

	SaveAutoloads();

	m_pLuaFiles.delete_all();
	m_pLuaFiles.clear();
}

void CLua::SaveAutoloads()
{
	CALLSTACK_ADD();
#if defined(FEATURE_LUA)
	char aFilePath[768];
	fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
	str_append(aFilePath, "/luafiles.cfg", sizeof(aFilePath));
	std::ofstream f(aFilePath, std::ios::out | std::ios::trunc);
	for(int i = 0; i < m_pLuaFiles.size(); i++)
		if(m_pLuaFiles[i]->GetScriptIsAutoload())
			f << m_pLuaFiles[i]->GetFilename() << std::endl;
	f.close();
#endif
}

void CLua::SortLuaFiles()
{
	CALLSTACK_ADD();

	const int NUM = m_pLuaFiles.size();
	if(NUM < 2)
		return;

	for(int curr = 0; curr < NUM-1; curr++)
	{
		int minIndex = curr;
		for(int i = curr + 1; i < NUM; i++)
		{
			int c = 4;
			for(; str_uppercase(m_pLuaFiles[i]->GetFilename()[c]) == str_uppercase(m_pLuaFiles[minIndex]->GetFilename()[c]); c++);
			if(str_uppercase(m_pLuaFiles[i]->GetFilename()[c]) < str_uppercase(m_pLuaFiles[minIndex]->GetFilename()[c]))
				minIndex = i;
		}

		if(minIndex != curr)
		{
			CLuaFile* temp = m_pLuaFiles[curr];
			m_pLuaFiles[curr] = m_pLuaFiles[minIndex];
			m_pLuaFiles[minIndex] = temp;
		}
	}
}

void CLua::SetGameClient(IGameClient *pGameClient)
{
	CALLSTACK_ADD();

	CLua::m_pGameClient = pGameClient;
	CLua::m_pCGameClient = (CGameClient*)pGameClient;
}

void CLua::AddUserscript(const char *pFilename)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
	if(!pFilename || pFilename[0] == '\0' || str_length(pFilename) <= 4 ||
			str_comp_nocase(&pFilename[str_length(pFilename)]-9, ".conf.lua") == 0 || // hide config files from the list
			str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua") != 0)
		return;

	// don't add duplicates
	for(int i = 0; i < m_pLuaFiles.size(); i++)
		if(str_comp(m_pLuaFiles[i]->GetFilename(), pFilename) == 0)
			return;

	std::string file = pFilename;

	// check for autoload
	bool Autoload = false;
	for(int i = 0; i < m_aAutoloadFiles.size(); i++)
		if(m_aAutoloadFiles[i] == file)
			Autoload = true;

	if(g_Config.m_Debug)
		dbg_msg("Lua", "adding script '%s' to the list", file.c_str());

	int index = m_pLuaFiles.add(new CLuaFile(this, file, Autoload));
	if(Autoload)
		m_pLuaFiles[index]->Init();
#endif
}

void CLua::LoadFolder()
{
	CALLSTACK_ADD();

	LoadFolder("lua");
}

void CLua::LoadFolder(const char *pFolder)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
	// get the files which should be auto-loaded from file
	{
		m_aAutoloadFiles.clear();
		char aFilePath[768];
		fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
		str_append(aFilePath, "/luafiles.cfg", sizeof(aFilePath));
		std::ifstream f(aFilePath);
		if(f.is_open())
		{
			std::string line;
			while(std::getline(f, line))
				m_aAutoloadFiles.add(line);
			f.close();
		}
	}

	//char FullDir[256];
	//str_format(FullDir, sizeof(FullDir), "lua");

	dbg_msg("Lua", "Loading Folder '%s'", pFolder);
	CLua::LuaLoadHelper * pParams = new CLua::LuaLoadHelper;
	pParams->pLua = this;
	pParams->pString = pFolder;

	m_pStorage->ListDirectory(IStorageTW::TYPE_ALL, pFolder, LoadFolderCallback, pParams);

	delete pParams;

	SortLuaFiles();
#endif
}

int CLua::LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser)
{
	CALLSTACK_ADD();

	if(pName[0] == '.')
		return 0;

	LuaLoadHelper *pParams = (LuaLoadHelper *)pUser;

	CLua *pSelf = pParams->pLua;
	const char *pFullDir = pParams->pString;

	char File[64];
	str_format(File, sizeof(File), "%s/%s", pFullDir, pName);
	//dbg_msg("Lua", "-> Found File %s", File);

	if(IsDir)
		pParams->pLua->LoadFolder(File);
	else
		pSelf->AddUserscript(File);
	return 0;
}

int CLua::HandleException(std::exception &e, lua_State *L)
{
	return HandleException(e.what(), L);
}

int CLua::HandleException(const char *pError, lua_State *L)
{
	return HandleException(pError, CLuaBinding::GetLuaFile(L));
}

int CLua::HandleException(std::exception &e, CLuaFile *pLF)
{
	return HandleException(e.what(), pLF);
}

int CLua::HandleException(const char *pError, CLuaFile *pLF)
{
	if(!pLF)
		return -1;
	bool Console = str_comp(pLF->GetFilename(), "[console]") == 0;

	if(str_comp_nocase(pError, "not enough memory") == 0)
	{
		if(!Console)
		{
			dbg_msg("lua/FATAL", "script %s hit OOM condition, killing it!", pLF->GetFilename());
			pLF->Unload(true);
		}
		else
		{
			lua_close(m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pLuaState);
			m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->InitLua();
			m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_FullLine = "";
			m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pDebugChild = 0;
			m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_ScopeCount = 0;
			m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_Inited = false;
		}
		return 0;
	}

	Client()->LuaCheckDrawingState(pLF->L(), "exception", true); // clean up the rendering pipeline if necessary

	// error messages come as "filename:line: message" - we don't need the 'filename' part if it's the current scriptfile's name
	{
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%s:", pLF->GetFilename());
		if(str_find_nocase(pError, aBuf)) // <- we need to truncate the filename in this case
		{
			const char *pFilenameEnding = str_find_nocase(pError, ".lua:");
			if(pFilenameEnding)
				pError = pFilenameEnding+4;
		}
	}

	pLF->m_Exceptions.add(std::string(pError));
	if(Console)
	{
		char aLine[512];
		str_format(aLine, sizeof(aLine), "EXCEPTION(%03i): %s", pLF->m_Exceptions.size(), pError);
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->PrintLine(aLine);
	}

	if(g_Config.m_Debug)
	{
		char aError[1024];
		str_format(aError, sizeof(aError), "{%i/100} [%s] %s", pLF->m_Exceptions.size(), pLF->GetFilename(), pError);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "lua|EXCEPTION", aError);
	}
	if(Console || pLF->m_Exceptions.size() < 100)
		return pLF->m_Exceptions.size();
	pLF->m_pErrorStr = Localize("Error count limit exceeded (too many exceptions thrown)");
	pLF->Unload(true);
	dbg_msg("lua|ERROR", "<<< unloaded script '%s' (error count exceeded limit)", pLF->GetFilename());

	return 0;
}


int CLua::Panic(lua_State *L)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
	if(g_Config.m_Debug)
		dbg_msg("LUA/FATAL", "[%s] error in unprotected call resulted in panic, throwing an exception:", CLuaBinding::GetLuaFile(L)->GetFilename());
	throw luabridge::LuaException(L, 0);
#else
	return 0;
#endif
}

int CLua::ErrorFunc(lua_State *L) // low level error handling (errors not thrown as an exception)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)

	if (!lua_isstring(L, -1))
		CLuaBinding::GetLuaFile(L)->Lua()->HandleException(": unknown error", L);
	else
		CLuaBinding::GetLuaFile(L)->Lua()->HandleException(lua_tostring(L, -1), L);

	lua_pop(L, 1); // remove error message
	lua_gc(L, LUA_GCCOLLECT, 0);

#endif
	return 0;
}
