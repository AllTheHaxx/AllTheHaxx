#include <engine/client.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

#include "luabinding.h"


int CLuaBinding::LuaListdirCallback(const char *name, int is_dir, int dir_type, void *user)
{
	LuaListdirCallbackParams *params = (LuaListdirCallbackParams*)user;
	lua_State *L = params->L;

	lua_getglobal(L, params->aCallbackFunc);
	lua_pushstring(L, name);
	lua_pushboolean(L, is_dir);
	lua_pcall(L, 2, 1, 0);
	int ret = 0;
	if(lua_isnumber(L, -1))
		ret = round_to_int((float)lua_tonumber(L, -1));
	lua_pop(L, 1); // pop return
	return ret;
}

CLuaFile* CLuaBinding::GetLuaFile(lua_State *l)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	for(int i = 0; i < pGameClient->Client()->Lua()->GetLuaFiles().size(); i++)
	{
		if(pGameClient->Client()->Lua()->GetLuaFiles()[i]->L() == l)
		{
			return pGameClient->Client()->Lua()->GetLuaFiles()[i];
		}
	}
	return 0;
}

// global namespace
int CLuaBinding::LuaImport(lua_State *L)
{
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

	if(!ret)
	{
		// try libraries after that
		str_format(aBuf, sizeof(aBuf), "lua/.library/%s", aFilename);
		ret = pLF->LoadFile(aBuf, true);

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
	lua_pushstring(L, (const char *)aBuf); // actual relative path of the loaded file
	return 2;
}

int CLuaBinding::LuaListdir(lua_State *L)
{
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	int nargs = lua_gettop(L);
	if(nargs != 2)
		return luaL_error(L, "Listdir expects 2 arguments");

	argcheck(lua_isstring(L, 1), 1, "string"); // path
	argcheck(lua_isstring(L, 2), 2, "function name (as a string)"); // function callback

	lua_getglobal(L, lua_tostring(L, 2)); // check if the given callback function actually exists
	argcheck(lua_isfunction(L, -1), 2, "function name (as a string)");
	lua_pop(L, 1); // pop temporary lua function

	const char *pDir = lua_tostring(L, 1); // arg1
	LuaListdirCallbackParams params(L, lua_tostring(L, 2)); // arg2
	lua_pop(L, 1); // pop arg2
	lua_Number ret = (lua_Number)fs_listdir(pDir, LuaListdirCallback, IStorageTW::TYPE_ALL, &params);
	lua_pop(L, 1); // pop arg1
	lua_pushnumber(L, ret);
	return 1;
}

int CLuaBinding::LuaKillScript(lua_State *L)
{
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	pLF->Unload();
	return 0;
}

int CLuaBinding::LuaScriptPath(lua_State *L)
{
	CLuaFile *pLF = GetLuaFile(L);
	if(!pLF)
		return luaL_error(L, "FATAL: got no lua file handler for this script?!");

	lua_pushstring(L, pLF->GetFilename());
	return 1;
}

int CLuaBinding::LuaStrIsNetAddr(lua_State *L)
{
	int nargs = lua_gettop(L);
	if(nargs != 1)
		return luaL_error(L, "StrIsNetAddr expects 1 argument");

	argcheck(lua_isstring(L, 1), 1, "string");

	NETADDR temp;
	int ret = net_addr_from_str(&temp, lua_tostring(L, 1)); // arg1
	lua_pop(L, 1), // pop arg1

	lua_pushboolean(L, ret == 0);
	return 1;
}


int CLuaBinding::LuaPrintOverride(lua_State *L)
{
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

	dbg_msg(aSys, "%s", aMsg);
	return 0;
}

// external info
int CLuaBinding::LuaGetPlayerScore(int ClientID)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();

	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CNetObj_PlayerInfo *pInfo = pGameClient->m_Snap.m_paPlayerInfos[ClientID];
		if(pInfo)
		{
			return pInfo->m_Score;
		}
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
