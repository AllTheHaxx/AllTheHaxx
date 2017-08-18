#include <fstream>

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
#include "db_sqlite3.h"


IClient * CLua::m_pClient = 0;
IGameClient * CLua::m_pGameClient = 0;
CGameClient * CLua::m_pCGameClient = 0;

#if defined(FEATURE_LUA)
using namespace luabridge;
#endif

CLua::CLua()
{
	m_pStorage = 0;
	m_pConsole = 0;
	m_pFullscreenedScript = 0;

	m_pDatabase = new CSql();

	char *pQueryBuf = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS lua_autoloads (path TEXT NOT NULL UNIQUE);");
	CQuery *pQuery = new CQuery(pQueryBuf);
	m_pDatabase->InsertQuery(pQuery);
	char *pQueryBuf2 = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS lua_favorites (path TEXT NOT NULL UNIQUE);");
	CQuery *pQuery2 = new CQuery(pQueryBuf2);
	m_pDatabase->InsertQuery(pQuery2);
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
	m_aFavoriteFiles.clear();

	//LoadFolder(); // we can't do it that early
}

void CLua::Shutdown()
{
	CALLSTACK_ADD();

	m_pFullscreenedScript = 0;

	m_apLuaFiles.delete_all();
	m_apLuaFiles.clear();
}

void CLua::SortLuaFiles()
{
	CALLSTACK_ADD();

	const int NUM = m_apLuaFiles.size();
	if(NUM < 2)
		return;

	int FavoriteNum = 0;
	for(int f = 0; f < NUM-1; f++) // Favorite add
	{
		CLuaFile *L = Client()->Lua()->GetLuaFiles()[f];
		if (L->GetScriptIsFavorite()){
			FavoriteNum++;
		}
	}

	if(g_Config.m_Debug)
		dbg_msg("Lua", "there are currently %d Fav scripts", FavoriteNum);


	for(int curr = 0; curr < NUM-1; curr++) // Normal Sort.
	{
		int minIndex = curr;
		for(int i = curr + 1; i < NUM; i++)
		{
			int c = 4;
			for(; str_uppercase(m_apLuaFiles[i]->GetFilename()[c]) == str_uppercase(m_apLuaFiles[minIndex]->GetFilename()[c]); c++);
			if(str_uppercase(m_apLuaFiles[i]->GetFilename()[c]) < str_uppercase(m_apLuaFiles[minIndex]->GetFilename()[c]))
				minIndex = i; // Found file with same start but not same ending
		}

		if(minIndex != curr)
		{
			CLuaFile* temp = m_apLuaFiles[curr];
			m_apLuaFiles[curr] = m_apLuaFiles[minIndex];
			m_apLuaFiles[minIndex] = temp;
		}
	}

	int FavSort = 0;
	for(int curr = 0; curr < NUM-1; curr++) // Favorite sort
	{
		CLuaFile *L = Client()->Lua()->GetLuaFiles()[curr];
		if (L->GetScriptIsFavorite()){
			int minIndex = FavSort;

			CLuaFile* temp = m_apLuaFiles[curr]; // Currently Script position
			if(g_Config.m_Debug)
				dbg_msg("Lua", "Saved %s as temp file",m_apLuaFiles[curr]->GetFilename());
			//m_apLuaFiles[curr] = m_apLuaFiles[minIndex]; // File which we replace, better if we do it in a loop
			int CurFile = curr;
			
			for (int i = curr; i > FavSort;i--) // Loop all needed Scripts backwards to increase their pos by 1
			{
				if(g_Config.m_Debug)
					dbg_msg("Lua", "Setted %s",m_apLuaFiles[i]->GetFilename());
				//CLuaFile* temp = m_apLuaFiles[i+1];
				m_apLuaFiles[i] = m_apLuaFiles[i-1];
				//m_apLuaFiles[i] = temp;
				CurFile = i;
			}
			//m_apLuaFiles[curr] = m_apLuaFiles[curr-1];
			FavSort++;
			m_apLuaFiles[minIndex] = temp; // Set our file here!
			if(g_Config.m_Debug)
				dbg_msg("Lua", "Setted to place %d",minIndex);

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
	for(int i = 0; i < m_apLuaFiles.size(); i++)
		if(str_comp(m_apLuaFiles[i]->GetFilename(), pFilename) == 0)
			return;

	std::string file = pFilename;

	// check for autoload
	bool Autoload = false;
	for(int i = 0; i < m_aAutoloadFiles.size(); i++)
		if(m_aAutoloadFiles[i] == file)
			Autoload = true;

	bool Favorite = false;
	for (int i = 0; i < m_aFavoriteFiles.size(); i++)
		if(m_aFavoriteFiles[i] == file)
			Favorite = true;

	if(g_Config.m_Debug)
		dbg_msg("Lua", "adding script '%s' to the list", file.c_str());

	int index = m_apLuaFiles.add(new CLuaFile(this, file, Autoload, Favorite));
	if(Autoload)
		m_apLuaFiles[index]->Activate();
#endif
}

void CLua::LoadFolder()
{
	CALLSTACK_ADD();

	// get the files which should be auto-loaded
	{
		m_aAutoloadFiles.clear();
		m_pDatabase->InsertQuery(
				new CQueryAutoloads(
						sqlite3_mprintf("SELECT * FROM lua_autoloads;"),
						&m_aAutoloadFiles
				)
		);
		dbg_msg("Lua", "DENNIS A FILE AUOTO! %d",m_aFavoriteFiles.size());
	}
	{
		m_aFavoriteFiles.clear();
		m_pDatabase->InsertQuery(
				new CQueryAutoloads( /*Lol, just steal this :3*/
						sqlite3_mprintf("SELECT * FROM lua_favorites;"),
						&m_aFavoriteFiles
				)
		);
		dbg_msg("Lua", "DENNIS A FILE! %d",m_aFavoriteFiles.size());
	}
	m_pDatabase->Flush();
	LoadFolder("lua");
}

void CLua::LoadFolder(const char *pFolder)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
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

void CLua::StartReceiveEvents(CLuaFile *pLF)
{
	dbg_assert_strict(m_apActiveScripts.find(pLF, NULL) == NULL, "loaded a script twice!?");
	m_apActiveScripts.add(pLF);
}

void CLua::StopReceiveEvents(CLuaFile *pLF)
{
	bool Success = m_apActiveScripts.remove_fast(pLF);
	dbg_assert_strict(Success, "unloaded a script that wasn't even loaded!");
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
			#if defined(FEATURE_LUA)
			lua_close(m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pLuaState);
			#endif
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
		pLF->Unload(true);
		dbg_msg("lua|ERROR", "<<< unloaded script '%s' (error count exceeded limit)", pLF->GetFilename());
	}
	else
	{
		// reload the lua console
		//lua_close(m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pLuaState);
		//m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->InitLua();
		m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->LoadLuaFile("data/luabase/events.lua");
		#if defined(FEATURE_LUA)
		lua_gc(m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pLuaState, LUA_GCCOLLECT, 0),
		#endif
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
#if defined(FEATURE_LUA)
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
#endif
}


void CLua::ExitFullscreen()
{
#if defined(FEATURE_LUA)
	dbg_assert(m_pFullscreenedScript, "CLua::ExitFullscreen called with no fullscreened script");

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
#endif
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

void CLua::AddFavorite(const CLuaFile *pLF)
{
	m_aFavoriteFiles.add(std::string(pLF->GetFilename()));
	m_pDatabase->InsertQuery(
			new CQuery(
					sqlite3_mprintf("INSERT OR IGNORE INTO lua_favorites ('path') VALUES ('%q');", pLF->GetFilename())
			)
	);
}

void CLua::RemoveFavorite(const CLuaFile *pLF)
{
	m_aFavoriteFiles.remove_fast(std::string(pLF->GetFilename()));
	m_pDatabase->InsertQuery(
			new CQuery(
					sqlite3_mprintf("DELETE FROM lua_favorites WHERE path = '%q';", pLF->GetFilename())
			)
	);
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

void CLua::DbgPrintLuaStack(lua_State *L, const char *pNote)
{
#if defined(FEATURE_LUA)
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
#endif
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
