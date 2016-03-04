#ifndef ENGINE_CLIENT_LUABINDING_H
#define ENGINE_CLIENT_LUABINDING_H
#include <base/vmath.h>
#include <string>
#include <engine/shared/config.h>
#include <engine/client.h>

class CLuaBinding
{
public:
	class UiContainer
	{
	public:
		vec4 Color;
	};
	static UiContainer *m_pUiContainer; // this will be moved to CLuaFile once it exists

	// system namespace
	static bool LuaImport(int UID, const char *pFilename);

	// external info
	static int LuaGetPlayerScore(int ClientID);

	// ui namespace
	static void LuaSetUiColor(float r, float g, float b, float a);
	static void LuaDrawUiRect(float x, float y, float w, float h, int corners, float rounding);
	static int LuaDoButton_Menu(const char *pText, int Checked, float x, float y, float w, float h, const char *pTooltip, int Corners);

	// graphics namespace
	static int LuaGetScreenWidth();
	static int LuaGetScreenHeight();
	static void LuaSetColor(float r, float g, float b, float a);
	static void LuaDrawLine(float x0, float y0, float x1, float y1);
	static int LuaLoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags); // e.g. CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_NORESAMPLE
	static void LuaRenderTexture(int ID, float x, float y, float w, float h, float rot);
};

struct CConfigProperties
{
	static CConfiguration * m_pConfig;
	static std::string GetConfigPlayerName() { return g_Config.m_PlayerName; }
	static void SetConfigPlayerName(std::string name) { str_copy(g_Config.m_PlayerName, name.c_str(), sizeof(g_Config.m_PlayerName)); }
	
	static std::string GetConfigPlayerClan() { return g_Config.m_PlayerClan; }
	static void SetConfigPlayerClan(std::string clan) { str_copy(g_Config.m_PlayerClan, clan.c_str(), sizeof(g_Config.m_PlayerClan)); }
	
	static std::string GetConfigPlayerSkin() { return g_Config.m_ClPlayerSkin; }
	static void SetConfigPlayerSkin(std::string skin) { str_copy(g_Config.m_ClPlayerSkin, skin.c_str(), sizeof(g_Config.m_ClPlayerSkin)); }
};

#endif
