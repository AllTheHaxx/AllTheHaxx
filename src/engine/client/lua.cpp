#include <fstream>

#include <base/system.h>
#include <engine/storage.h>
#include <engine/client.h>
#include <engine/console.h>
#include <engine/shared/network.h>
#include <game/localization.h>
#include <game/client/gameclient.h>
#include <game/client/components/console.h>
#include <game/client/components/menus.h>

#include "lua.h"
#include "luabinding.h"
#include "db_sqlite3.h"


IClient * CLua::m_pClient = 0;
IGameClient * CLua::m_pGameClient = 0;
CGameClient * CLua::m_pCGameClient = 0;

using namespace luabridge;

CLua::CLua()
{
	m_pStorage = 0;
	m_pConsole = 0;
	m_pFullscreenedScript = 0;

	m_pDatabase = new CSql();

	char *pQueryBuf = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS lua_autoloads (path TEXT NOT NULL UNIQUE);");
	CQuery *pQuery = new CQuery(pQueryBuf);
	m_pDatabase->InsertQuery(pQuery);
}

CLua::~CLua()
{
	Shutdown();
}

void CLua::Init(IClient *pClient, IStorageTW *pStorage, IConsole *pConsole)
{
	CALLSTACK_ADD();

	m_pClient = pClient;
	m_pStorage = pStorage;
	m_pConsole = pConsole;
	m_aAutoloadFiles.clear();

	//LoadFolder(); // we can't do it that early
}

void CLua::Shutdown()
{
	CALLSTACK_ADD();

	m_pFullscreenedScript = 0;

	m_apLuaFiles.delete_all();
	m_apLuaFiles.clear();
}

void CLua::Reload()
{
	CALLSTACK_ADD();

	Shutdown();
	LoadFolder();
}

void CLua::SortLuaFiles()
{
	CALLSTACK_ADD();

	const int NUM = m_apLuaFiles.size();
	if(NUM < 2)
		return;

	for(int curr = 0; curr < NUM-1; curr++)
	{
		int minIndex = curr;
		for(int i = curr + 1; i < NUM; i++)
		{
			unsigned GodmodeI = m_apLuaFiles[i]->GetPermissionFlags()&CLuaFile::PERMISSION_GODMODE;
			unsigned GodmodeMin = m_apLuaFiles[minIndex]->GetPermissionFlags()&CLuaFile::PERMISSION_GODMODE;
			if(GodmodeI != GodmodeMin)
			{
				if(!GodmodeMin)
					minIndex = i;
			}
			else
			{
				int c = 0;
				for(; str_uppercase(m_apLuaFiles[i]->GetDisplayedFilename()[c]) == str_uppercase(m_apLuaFiles[minIndex]->GetDisplayedFilename()[c]); c++);
				if(str_uppercase(m_apLuaFiles[i]->GetDisplayedFilename()[c]) < str_uppercase(m_apLuaFiles[minIndex]->GetDisplayedFilename()[c]))
					minIndex = i;
			}
		}

		if(minIndex != curr)
		{
			CLuaFile* temp = m_apLuaFiles[curr];
			m_apLuaFiles[curr] = m_apLuaFiles[minIndex];
			m_apLuaFiles[minIndex] = temp;
		}
	}
}

int CLua::UnloadAll()
{
	int NumLuaFiles = GetLuaFiles().size();
	int Counter = 0;
	for(int i = 0; i < NumLuaFiles; i++)
	{
		CLuaFile *pLF = GetLuaFiles()[i];
		if(pLF->State() == CLuaFile::STATE_LOADED)
		{
			pLF->Deactivate();
			Counter++;
		}
	}
	return Counter;
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

	if(g_StealthMode)
		return;

	if(!pFilename || pFilename[0] == '\0' || str_length(pFilename) <= 4 ||
			str_comp_nocase(&pFilename[str_length(pFilename)]-9, ".conf.lua") == 0 || // hide config files from the list
			str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua") != 0)
		return;

	// don't add duplicates
	for(int i = 0; i < m_apLuaFiles.size(); i++)
		if(str_comp(m_apLuaFiles[i]->GetFilename(), pFilename) == 0)
			return;

	std::string file = pFilename;

	// check for autoload
	bool Autoload = false;
	for(int i = 0; i < m_aAutoloadFiles.size(); i++)
		if(m_aAutoloadFiles[i] == file)
			Autoload = true;

	if(g_Config.m_Debug)
		dbg_msg("Lua", "adding script '%s' to the list", file.c_str());

	int index = m_apLuaFiles.add(new CLuaFile(this, file, Autoload));
	if(Autoload && !g_Config.m_Failsafe)
		m_apLuaFiles[index]->Activate();
}

void CLua::LoadFolder()
{
	CALLSTACK_ADD();
	if(g_StealthMode)
		return;

	// get the files which should be auto-loaded
	{
		m_aAutoloadFiles.clear();
		m_pDatabase->InsertQuery(
				new CQueryAutoloads(
						sqlite3_mprintf("SELECT * FROM lua_autoloads;"),
						&m_aAutoloadFiles
				)
		);
	}
	m_pDatabase->Flush();

	LoadFolder("lua");
}

void CLua::LoadFolder(const char *pFolder)
{
	CALLSTACK_ADD();
	if(g_StealthMode)
		return;

	dbg_msg("Lua", "Loading Folder '%s'", pFolder);
	m_pStorage->ListDirectoryVerbose(IStorageTW::TYPE_ALL, pFolder, LoadFolderCallback, this);
	SortLuaFiles();
}

int CLua::LoadFolderCallback(const char *pName, const char *pFullPath, int IsDir, int DirType, void *pUser)
{
	CALLSTACK_ADD();

	if(pName[0] == '.')
		return 0;

	CLua *pSelf = (CLua*)pUser;

	if(g_Config.m_Debug >= 2)
		dbg_msg("lua/debug", "found: '%s'", pFullPath);

	if(IsDir)
		pSelf->LoadFolder(pFullPath);
	else
		pSelf->AddUserscript(pFullPath);
	return 0;
}

void CLua::StartReceiveEvents(CLuaFile *pLF)
{
	dbg_assert_strict(m_apActiveScripts.find(pLF, NULL) == NULL, "loaded a script twice!?");
	m_apActiveScripts.add(pLF);
}

void CLua::StopReceiveEvents(CLuaFile *pLF)
{
	bool Success = m_apActiveScripts.remove_fast(pLF);
	if(!dbg_assert_strict(Success, "unloaded a script that wasn't even loaded!"))
		CLua::m_pCGameClient->m_pMenus->m_Nalf[pLF->GetPermissionFlags()&CLuaFile::PERMISSION_GODMODE?1:0]--;
}

int CLua::HandleException(std::exception &e, lua_State *L)
{
	return HandleException(e.what(), L);
}

int CLua::HandleException(const char *pError, lua_State *L)
{
	return HandleException(pError, CLuaBinding::GetLuaFile(L));
}

int CLua::HandleException(std::exception &e, CLuaFile *pLF, bool CalledFromUnloadFromExceptionHandler)
{
	return HandleException(e.what(), pLF, CalledFromUnloadFromExceptionHandler);
}

int CLua::HandleException(const char *pError, CLuaFile *pLF, bool CalledFromUnloadFromExceptionHandler)
{
	if(!pLF)
		return -1;

	bool Console = str_comp(pLF->GetFilename(), "[console]") == 0;

	if(!pError)
		pError = "no error message";

	if(str_comp_nocase(pError, "not enough memory") == 0)
	{
		if(!Console)
		{
			// unload the script
			dbg_msg("lua/FATAL", "script %s hit OOM condition, killing it!", pLF->GetFilename());
			pLF->Unload(true);
		}
		else
		{
			// restart the lua console
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

	// error messages come as "filename:line: message" - we don't need the 'filename' part if it's the current script's filename
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
	if(pLF->m_Exceptions.size() < 100)
		return pLF->m_Exceptions.size();
	if(!Console)
	{
		pLF->m_pErrorStr = Localize("Error count limit exceeded (too many exceptions thrown)");
		if(CalledFromUnloadFromExceptionHandler)
			dbg_msg("lua|ERROR", "exception while force-unloading script due to exception overflow");
		else
		{
			pLF->Unload(true, true);
			dbg_msg("lua|ERROR", "<<< unloaded script '%s' (error count exceeded limit)", pLF->GetFilename());
		}
	}
	else
	{
		// reload the lua console
		//lua_close(m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pLuaState);
		//m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->InitLua();
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->LoadLuaFile("data/luabase/events.lua");
		lua_gc(m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pLuaState, LUA_GCCOLLECT, 0),
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_FullLine = "";
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pDebugChild = 0;
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_ScopeCount = 0;
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_Inited = true; // this must be true if we don't recreate the whole lua state
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->PrintLine("--- LUA CONSOLE RELOADED ---");
		pLF->m_Exceptions.clear();
	}

	return 0;
}

void CLua::ScriptEnterFullscreen(CLuaFile *pLF)
{
	if(g_StealthMode)
		return;

	m_pFullscreenedScript = pLF;
	try
	{
		LuaRef func = pLF->GetFunc("OnEnterFullscreen");
		if(func.cast<bool>())
			func();
	}
	catch(std::exception &e)
	{
		HandleException(e, pLF);
	}
}


void CLua::ExitFullscreen()
{
	if(dbg_assert_strict(m_pFullscreenedScript != NULL, "CLua::ExitFullscreen called with no fullscreened script"))
		return;

	try
	{
		LuaRef func = m_pFullscreenedScript->GetFunc("OnExitFullscreen");
		if(func.cast<bool>())
			func();
	}
	catch(std::exception &e)
	{
		HandleException(e, m_pFullscreenedScript);
	}

	m_pFullscreenedScript = 0;
}

void CLua::AddAutoload(const CLuaFile *pLF)
{
	m_aAutoloadFiles.add(std::string(pLF->GetFilename()));
	m_pDatabase->InsertQuery(
			new CQuery(
					sqlite3_mprintf("INSERT OR IGNORE INTO lua_autoloads ('path') VALUES ('%q');", pLF->GetFilename())
			)
	);
}

void CLua::RemoveAutoload(const CLuaFile *pLF)
{
	m_aAutoloadFiles.remove_fast(std::string(pLF->GetFilename()));
	m_pDatabase->InsertQuery(
			new CQuery(
					sqlite3_mprintf("DELETE FROM lua_autoloads WHERE path = '%q';", pLF->GetFilename())
			)
	);
}

int CLua::Panic(lua_State *L)
{
	CALLSTACK_ADD();

	if(g_Config.m_Debug)
		dbg_msg("LUA/FATAL", "[%s] error in unprotected call resulted in panic, throwing an exception:", CLuaBinding::GetLuaFile(L)->GetFilename());
	throw luabridge::LuaException(L, 0);
}

int CLua::ErrorFunc(lua_State *L) // low level error handling (errors not thrown as an exception)
{
	CALLSTACK_ADD();

	if (!lua_isstring(L, -1))
		CLuaBinding::GetLuaFile(L)->Lua()->HandleException(": unknown error", L);
	else
	{
		CLuaBinding::GetLuaFile(L)->Lua()->HandleException(lua_tostring(L, -1), L);
		lua_pop(L, 1); // remove error message
	}

	return 0;
}

void CLua::DbgPrintLuaStack(lua_State *L, const char *pNote)
{
	dbg_msg("lua/debug", "--- BEGIN LUA STACK --- %s", pNote ? pNote : "");
	for(int i = 1; i <= lua_gettop(L); i++)
	{
		dbg_msg("lua/debug", "#%02i    %s    %s", i, luaL_typename(L, i),
				lua_isstring(L, i) || lua_isnumber(L, i) ? lua_tostring(L, i) :
				lua_isboolean(L, i) ? (lua_toboolean(L, i) ? "true" : "false") :
				""
		);
	}
	dbg_msg("lua/debug", "---- END LUA STACK ---- %s", pNote ? pNote : "");
}


void CQueryAutoloads::OnData()
{
	while(Next()) // process everything
	{
		const char *pPath = GetText(GetID("path"));

#if defined(CONF_DEBUG)
		dbg_assert(pPath != NULL, "query returned invalid string");
#else
		if(pPath)
#endif
		if(m_paAutoloadFiles->find(std::string(pPath)) == NULL)
			m_paAutoloadFiles->add(std::string(pPath));
	}
}
