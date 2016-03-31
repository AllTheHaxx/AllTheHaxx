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

	class UiContainer
	{
	public:
		vec4 Color;
	};
	static UiContainer *m_pUiContainer; // this will be moved to CLuaFile once it exists

	// global namespace
	static bool LuaImport(int UID, const char *pFilename);
	static bool LuaKillScript(int UID);

	// external info
	static int LuaGetPlayerScore(int ClientID);

	// ui namespace
	static void LuaSetUiColor(float r, float g, float b, float a);
	static int LuaDoButton_Menu(const char *pText, int Checked, float x, float y, float w, float h, const char *pTooltip, int Corners);

	// graphics namespace
	static void LuaDrawLine(float x0, float y0, float x1, float y1);
	static void LuaRenderTexture(int ID, float x, float y, float w, float h, float rot);
	static void LuaRenderQuadRaw(int x, int y, int w, int h);
};

struct CConfigProperties
{
	static CConfiguration * m_pConfig;


	static std::string GetConfigPlayerName() { return g_Config.m_PlayerName; }
	static void SetConfigPlayerName(std::string name) { str_copy(g_Config.m_PlayerName, name.c_str(), sizeof(g_Config.m_PlayerName)); }
	
	static std::string GetConfigPlayerClan() { return g_Config.m_PlayerClan; }
	static void SetConfigPlayerClan(std::string clan) { str_copy(g_Config.m_PlayerClan, clan.c_str(), sizeof(g_Config.m_PlayerClan)); }
	
	static int GetConfigPlayerCountry() { return g_Config.m_PlayerCountry; }
	static void SetConfigPlayerCountry(int c) { g_Config.m_PlayerCountry = c; }


	static std::string GetConfigPlayerSkin() { return g_Config.m_ClPlayerSkin; }
	static void SetConfigPlayerSkin(std::string skin) { str_copy(g_Config.m_ClPlayerSkin, skin.c_str(), sizeof(g_Config.m_ClPlayerSkin)); }
	
	static int GetConfigPlayerColorBody() { return g_Config.m_ClPlayerColorBody; }
	static void SetConfigPlayerColorBody(int c) { g_Config.m_ClPlayerColorBody = c; }

	static int GetConfigPlayerColorFeet() { return g_Config.m_ClPlayerColorFeet; }
	static void SetConfigPlayerColorFeet(int c) { g_Config.m_ClPlayerColorFeet = c; }

	static int GetConfigPlayerUseCustomColor() { return g_Config.m_ClPlayerUseCustomColor; }
	static void SetConfigPlayerUseCustomColor(int c) { g_Config.m_ClPlayerUseCustomColor = c; }
};

#endif
