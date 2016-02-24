#ifndef ENGINE_CLIENT_LUABINDING_H
#define ENGINE_CLIENT_LUABINDING_H
#include <base/vmath.h>

class CClient;

class CLuaBinding
{
public:
	class UiContainer
	{
	public:
		vec4 Color;
	};
	static UiContainer *m_pUiContainer; // this will be moved to CLuaFile once it exists

	// client namespace
	static void LuaConnect(const char *pAddr);
	static int LuaGetTick();

	// lua namespace
	static void LuaSetScriptTitle();
	static void LuaSetScriptInfo();
	static void LuaSetScriptHasSettings();

	// ui namespace
	static void LuaSetUiColor(float r, float g, float b, float a);
	static void LuaDrawUiRect(float x, float y, float w, float h, int corners, float rounding);


	// components namespace
	// --- chat
	static void LuaChatSend(int Team, const char *pMessage);
	static bool LuaChatActive();
	static bool LuaChatAllActive();
	static bool LuaChatTeamActive();
	// --- collision
	static int LuaColGetMapWidth();
	static int LuaColGetMapHeight();
	static int LuaColGetTile(int x, int y);
	// --- emote
	static void LuaEmoteSend(int Emote);


	// graphics namespace
	static int LuaGetScreenWidth();
	static int LuaGetScreenHeight();
	static void LuaBlendNone();
	static void LuaBlendNormal();
	static void LuaBlendAdditive();
	static void LuaSetColor(float r, float g, float b, float a);
	static int LuaLoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags); // e.g. CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_NORESAMPLE
	static void LuaRenderTexture(int ID, float x, float y, float w, float h, float rot);


};

#endif
