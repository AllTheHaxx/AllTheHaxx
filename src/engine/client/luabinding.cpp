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

	char aFilename[128];
	{
		const char *pFilename = luaL_checklstring(L, 1, 0);
		for(; *pFilename && (*pFilename == '.' || *pFilename == '/'); pFilename++); // sandbox it
		str_copy(aFilename, pFilename, sizeof(aFilename));
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
		dbg_msg("Lua/debug", "script '%s' %s '%s'", pLF->GetFilename()+4, ret ? "successfully Import()'ed" : "failed to Import()", aFilename);

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

#define argcheck(cond, narg, msg) if(!(cond)) { if(g_Config.m_Debug) dbg_msg("Lua/debug", "Listdir: narg=%i msg='%s'", narg, msg); return luaL_argerror(L, (narg), (msg)); }

	argcheck(lua_isstring(L, 1), 1, ""); // path
	argcheck(lua_isstring(L, 2), 2, "must be the name of the callback function (given as a string)"); // function callback

	lua_getglobal(L, lua_tostring(L, 2)); // check if the given callback function actually exists
	luaL_argcheck(L, lua_isfunction(L, -1), 2, "must be the name of the callback function (given as a string)");
	lua_pop(L, 1); // pop temporary lua function

	const char *pDir = luaL_optstring(L, 1, 0);
	LuaListdirCallbackParams params(L, lua_tostring(L, 2));
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

void CLuaFile::LuaPrintOverride(std::string str)
{
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "LUA|%s", m_Filename.c_str()+4);
	dbg_msg(aBuf, "%s", str.c_str());
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
