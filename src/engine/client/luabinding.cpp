#include <engine/client.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>
#include <game/client/components/console.h>
#include <game/client/components/menus.h>

#include "luabinding.h"


int CLuaBinding::LuaListdirCallback(const char *name, const char *full_path, int is_dir, int dir_type, void *user)
{
#if defined(FEATURE_LUA)
	lua_State *L = (lua_State*)user;
	lua_pushvalue(L, lua_gettop(L)); // duplicate the callback function which is on top of the stack
	lua_pushstring(L, name); // push arg 1 (element name)
	lua_pushstring(L, full_path); // push arg 2 (element path)
	lua_pushboolean(L, is_dir); // push arg 3 (bool indicating whether element is a folder)
	lua_pcall(L, 3, 1, 0);
	int ret = 0;
	if(lua_isnumber(L, -1))
		ret = round_to_int((float)lua_tonumber(L, -1));
	lua_pop(L, 1); // pop return
	return ret;
#else
	return 0;
#endif
}

CLuaFile* CLuaBinding::GetLuaFile(lua_State *L)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	if(L == pGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pLuaState)
	{
		static CLuaFile ConLuaFile(pGameClient->Client()->Lua(), "[console]", false);
		ConLuaFile.m_pLuaState = L;
		return &ConLuaFile;
	}

	for(int i = 0; i < pGameClient->Client()->Lua()->GetLuaFiles().size(); i++)
	{
		if(pGameClient->Client()->Lua()->GetLuaFiles()[i]->L() == L)
		{
			return pGameClient->Client()->Lua()->GetLuaFiles()[i];
		}
	}
	return 0;
}

// global namespace
int CLuaBinding::LuaImport(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	int n = lua_gettop(L);
	if(n != 1)
		return luaL_argerror(L, 1, "expected a string value, got nil");

	char aFilename[512];
	{
		const char *pFilename = luaL_checklstring(L, 1, 0);
		if(!pFilename)
			return false;

		// replace all backslashes with forward slashes
		char aTmp[512];
		{
			str_copy(aTmp, pFilename, sizeof(aTmp));
			for(char *p = aTmp; *p; p++)
				if(*p == '\\')
					*p = '/';
			pFilename = aTmp;
		}
		// sandbox it
		for(; *pFilename && ((*pFilename == '.' && pFilename[1] == '/') || *pFilename == '/'); pFilename++);
		str_copy(aFilename, pFilename, sizeof(aFilename));
		// auto-append the file ending if omitted
		if(str_comp_nocase(aFilename+str_length(aFilename)-4, ".lua") != 0 && str_comp_nocase(aFilename+str_length(aFilename)-4, ".clc") != 0)
			str_append(aFilename, ".lua", sizeof(aFilename)); // assume plain lua files as default
	}

	char aBuf[512];
	// try same folder as the script first as it's the most intuitive thing
	str_copy(aBuf, pLF->GetFilename(), sizeof(aBuf)); // get the whole path
	str_replace_char_rev_num(aBuf, 1, '/', '\0'); // cut off the file name
	str_append(aBuf, "/", sizeof(aBuf)); // re-append the slash
	str_append(aBuf, aFilename, sizeof(aBuf)); // append the wanted file's path
	bool ret = pLF->LoadFile(aBuf, true);

	// try libraries after that
	char aFullPath[512];
	if(!ret)
	{
		// search everywhere
		str_format(aBuf, sizeof(aBuf), "lua/.library/%s", aFilename);
		if(IOHANDLE tmp = CLua::m_pCGameClient->Storage()->OpenFile(aBuf, IOFLAG_READ, IStorageTW::TYPE_ALL, aFullPath, sizeof(aFullPath)))
			io_close(tmp);
		else
			str_copy(aFullPath, aBuf, sizeof(aFullPath)); // fall back to lua folder if it fails
		ret = pLF->LoadFile(aFullPath, true);

		if(!ret)
		{
			// try lua base folder if all fails
			if(str_comp_num(aFilename, "lua/", 4) != 0)
				str_format(aBuf, sizeof(aBuf), "lua/%s", aFilename);
			ret = pLF->LoadFile(aBuf, true);
		}
	}

	if(g_Config.m_Debug)
		dbg_msg("Lua/debug", "script '%s' %s '%s'", pLF->GetFilename()+4, ret ? "successfully Import()ed" : "failed to Import()", aFilename);

	// return some stuff to the script
	lua_pushboolean(L, (int)ret); // success?
	lua_pushstring(L, (const char *)aFullPath); // actual relative path of the loaded file
	return 2;
#else
	return 0;
#endif
}

int CLuaBinding::LuaExec(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	int n = lua_gettop(L);
	if(n != 1)
		return luaL_argerror(L, 1, "expected a string value, got nil");

	char aFilename[512];
	{
		const char *pFilename = luaL_checklstring(L, 1, 0);
		if(!pFilename)
			return false;

		// replace all backslashes with forward slashes
		char aTmp[512];
		{
			str_copy(aTmp, pFilename, sizeof(aTmp));
			for(char *p = aTmp; *p; p++)
				if(*p == '\\')
					*p = '/';
			pFilename = aTmp;
		}
		// sandbox it
		for(; *pFilename && ((*pFilename == '.' && pFilename[1] == '/') || *pFilename == '/'); pFilename++);
		str_copy(aFilename, pFilename, sizeof(aFilename));
		// auto-append the file ending if omitted
		if(str_comp_nocase(aFilename+str_length(aFilename)-4, ".lua") != 0 && str_comp_nocase(aFilename+str_length(aFilename)-4, ".clc") != 0)
			str_append(aFilename, ".lua", sizeof(aFilename)); // assume plain lua files as default
	}

	bool ret = pLF->LoadFile(aFilename, true);

	if(g_Config.m_Debug)
		dbg_msg("Lua/debug", "script '%s' %s '%s'", pLF->GetFilename()+4, ret ? "successfully exec()uted" : "failed to exec()ute", aFilename);

	// return some stuff to the script
	lua_pushboolean(L, (int)ret); // success?
	return 1;
#else
	return 0;
#endif
}

/* lua call: Listdir(<string> foldername, <string/function> callback) */
int CLuaBinding::LuaListdir(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	int nargs = lua_gettop(L);
	if(nargs != 2)
		return luaL_error(L, "Listdir expects 2 arguments");

	argcheck(lua_isstring(L, 1), 1, "string"); // path
	argcheck(lua_isstring(L, 2) || lua_isfunction(L, 2), 2, "string (function name) or function"); // callback function

	// convert the function name into the actual function
	if(lua_isstring(L, 2))
	{
		lua_getglobal(L, lua_tostring(L, 2)); // check if the given callback function actually exists / retrieve the function
		argcheck(lua_isfunction(L, -1), 2, "function name (does the given function exist?)");
	}

	const char *pDir = lua_tostring(L, 1); // arg1
	lua_Number ret = (lua_Number)fs_listdir_verbose(pDir, LuaListdirCallback, IStorageTW::TYPE_ALL, L);
	lua_pushnumber(L, ret);
	return 1;
#else
	return 0;
#endif
}

int CLuaBinding::LuaKillScript(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	pLF->Unload();
#endif
	return 0;
}

int CLuaBinding::LuaEnterFullscreen(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	CLua::m_pCGameClient->m_pMenus->LuaRequestFullscreen(pLF);
#endif
	return 0;
}

int CLuaBinding::LuaExitFullscreen(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	if(pLF != CLua::m_pClient->Lua()->GetFullscreenedScript())
		return luaL_error(L, "This script is not currently in fullscreen mode");

	CLua::m_pClient->Lua()->ExitFullscreen();
#endif
	return 0;
}

int CLuaBinding::LuaScriptPath(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	lua_pushstring(L, pLF->GetFilename());
	return 1;
#else
	return 0;
#endif
}

int CLuaBinding::LuaStrIsNetAddr(lua_State *L)
{
#if defined(FEATURE_LUA)
	int nargs = lua_gettop(L);
	if(nargs != 1)
		return luaL_error(L, "StrIsNetAddr expects 1 argument");

	argcheck(lua_isstring(L, 1), 1, "string");

	NETADDR temp;
	int ret = net_addr_from_str(&temp, lua_tostring(L, 1)); // arg1
	lua_pop(L, 1), // pop arg1

			lua_pushboolean(L, ret == 0);
	return 1;
#else
	return 0;
#endif
}


int CLuaBinding::LuaPrintOverride(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	int nargs = lua_gettop(L);
	if(nargs < 1)
		return luaL_error(L, "print expects 1 argument or more");

	char aSys[64];
	str_format(aSys, sizeof(aSys), "LUA|%s", pLF->GetFilename());

	// construct the message from all arguments
	char aMsg[512] = {0};
	for(int i = 1; i <= nargs; i++)
	{
		argcheck(lua_isstring(L, i) || lua_isnumber(L, i), i, "string or number");
		str_append(aMsg, lua_tostring(L, i), sizeof(aMsg));
		str_append(aMsg, "\t", sizeof(aMsg));
	}
	aMsg[str_length(aMsg)-1] = '\0'; // remove the last tab character

	// pop all to clean up the stack
	lua_pop(L, nargs);

	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	if(pGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pDebugChild == L)
		pGameClient->m_pGameConsole->m_pStatLuaConsole->PrintLine(aMsg);

	dbg_msg(aSys, "%s", aMsg);
#endif

	return 0;
}

int CLuaBinding::LuaThrow(lua_State *L)
{
#if defined(FEATURE_LUA)
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	int nargs = lua_gettop(L);
	if(nargs != 1)
		return luaL_error(L, "throw expects exactly 1 argument");

	argcheck(lua_isstring(L, nargs) || lua_isnumber(L, nargs), nargs, "string or number");

	// add the exception
	char aMsg[512];
	str_formatb(aMsg, "Custom error: %s", lua_tostring(L, nargs));
	int result = CLua::m_pClient->Lua()->HandleException(aMsg, pLF);

	// pop argument
	lua_pop(L, nargs);

	// push result
	lua_pushinteger(L, (lua_Integer)result);

	return 1;
#else
	return 0;
#endif
}

// external info
int CLuaBinding::LuaGetPlayerScore(int ClientID)
{
	dbg_assert(ClientID >= 0 && ClientID < MAX_CLIENTS, "invalid ClientID");
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();

	const CNetObj_PlayerInfo *pInfo = pGameClient->m_Snap.m_paPlayerInfos[ClientID];
	if(pInfo)
	{
		return pInfo->m_Score;
	}

	return -1;
}

void CLuaBinding::LuaRenderTexture(int ID, float x, float y, float w, float h, float rot) // depreciated
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = pGameClient->Kernel()->RequestInterface<IGraphics>();

	//pGraphics->MapScreen(0, 0, pGameClient->UI()->Screen()->w, pGameClient->UI()->Screen()->h);
	pGraphics->TextureSet(ID);
	pGraphics->QuadsBegin();
	IGraphics::CQuadItem Item;
	Item.m_X = x; Item.m_Y = y;
	Item.m_Width = w; Item.m_Height = h;
	pGraphics->QuadsSetRotation(rot);
	pGraphics->QuadsDraw(&Item, 1);
	pGraphics->QuadsEnd();

	//float Width = 400*3.0f*pGraphics->ScreenAspect();
	//float Height = 400*3.0f;
	//pGraphics->MapScreen(0, 0, Width, Height);
}

void CLuaBinding::LuaRenderQuadRaw(int x, int y, int w, int h)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = pGameClient->Kernel()->RequestInterface<IGraphics>();

	IGraphics::CQuadItem Item;
	Item.m_X = x; Item.m_Y = y;
	Item.m_Width = w; Item.m_Height = h;
	pGraphics->QuadsDraw(&Item, 1);
}
