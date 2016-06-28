#ifndef ENGINE_CLIENT_LUABINDING_H
#define ENGINE_CLIENT_LUABINDING_H
#include <base/vmath.h>
#include <string>
#include <engine/shared/config.h>
#include <engine/client.h>

class CLuaBinding
{
public:
	static CLuaFile *GetLuaFile(int UID);

	// global namespace
	static bool LuaImport(int UID, const char *pFilename);
	static bool LuaKillScript(int UID);

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
