#ifndef ENGINE_CLIENT_LUABINDING_H
#define ENGINE_CLIENT_LUABINDING_H
#include <base/vmath.h>
#include <string>
#include <engine/shared/config.h>
#include <engine/client.h>

#define argcheck(cond, narg, expected) \
		if(!(cond)) \
		{ \
			if(g_Config.m_Debug) \
				dbg_msg("Lua/debug", "%s: argcheck: narg=%i expected='%s'", __FUNCTION__, narg, expected); \
			char buf[64]; \
			str_format(buf, sizeof(buf), "expected a %s value, got %s", expected, lua_typename(L, lua_type(L, narg))); \
			return luaL_argerror(L, (narg), (buf)); \
		}


class CLuaBinding
{
public:
	static CLuaFile *GetLuaFile(lua_State *l);
	static int LuaListdirCallback(const char *name, const char *full_path, int is_dir, int dir_type, void *user);

	// global namespace
	static int LuaPrintOverride(lua_State *L);
	static int LuaThrow(lua_State *L);
	static int LuaStrIsNetAddr(lua_State *L);

	// low level lua callbacks
	/** @LuaFunc @code bool Import(const char *pFilename) */
	static int LuaImport(lua_State *L);
	/** @LuaFunc none */
	static int LuaKillScript(lua_State *L);
	/** @LuaFunc @code int Listdir(const char *pDir, LUA_FS_LISTDIR_CALLBACK *pFCallback)@endcode
	 * @typedef @code int (*LUA_FS_LISTDIR_CALLBACK)(const char *name, int is_dir);@endcode
	 */
	static int LuaListdir(lua_State *L);

	// some getters
	static int LuaScriptPath(lua_State *L);

	// external info
	static int LuaGetPlayerScore(int ClientID);

	// graphics namespace
	static void LuaRenderTexture(int ID, float x, float y, float w, float h, float rot);
	static void LuaRenderQuadRaw(int x, int y, int w, int h);
};

struct CConfigProperties
{
#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) \
		static std::string GetConfig_##Name() { if(!((Save)&CFGFLAG_CLIENT)) throw "invalid config type (this is not a client variable)"; return g_Config.m_##Name; } \
		static void SetConfig_##Name(std::string var) { if(!((Save)&CFGFLAG_CLIENT)) throw "invalid config type (this is not a client variable)"; str_copy(g_Config.m_##Name, var.c_str(), sizeof(g_Config.m_##Name)); }

#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) \
		static int GetConfig_##Name() { if(!((Save)&CFGFLAG_CLIENT)) throw "invalid config type (this is not a client variable)"; return g_Config.m_##Name; } \
		static void SetConfig_##Name(int var) { if(!((Save)&CFGFLAG_CLIENT)) throw "invalid config type (this is not a client variable)"; if (var < Min || var > Max) throw "config int override out of range"; g_Config.m_##Name = var; }

#include <engine/shared/config_variables.h>

#undef MACRO_CONFIG_STR
#undef MACRO_CONFIG_INT
};

#endif
