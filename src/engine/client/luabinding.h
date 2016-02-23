#ifndef ENGINE_CLIENT_LUABINDING_H
#define ENGINE_CLIENT_LUABINDING_H

class CClient;

class CLuaBinding
{
public:
	// client namespace
	static int LuaGetTick();

	// lua namespace
	static void LuaSetScriptTitle();
	static void LuaSetScriptInfo();
	static void LuaSetScriptHasSettings();

	// ui namespace


	// components namespace


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
