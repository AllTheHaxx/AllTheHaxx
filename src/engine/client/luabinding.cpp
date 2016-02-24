#include <engine/client.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

#include "luabinding.h"
#include "lua.h"


// client namespace
int CLuaBinding::LuaGetTick()
{
	return CLua::Client()->GameTick();
}

// lua namespace
void CLuaBinding::LuaSetScriptTitle()
{

}

void CLuaBinding::LuaSetScriptInfo()
{

}

void CLuaBinding::LuaSetScriptHasSettings()
{

}

// ui namespace


// components namespace


// graphics namespace
int CLuaBinding::LuaGetScreenWidth()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	//return ((IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>())->ScreenWidth();
	return pGameClient->UI()->Screen()->w; // which one is better...?
}

int CLuaBinding::LuaGetScreenHeight()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	//return ((IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>())->ScreenHeight();
	return pGameClient->UI()->Screen()->h; // which one is better...?
}

void CLuaBinding::LuaBlendNone()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	((IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>())->BlendNone();
}

void CLuaBinding::LuaBlendNormal()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	((IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>())->BlendNormal();
}

void CLuaBinding::LuaBlendAdditive()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	((IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>())->BlendAdditive();
}

void CLuaBinding::LuaSetColor(float r, float g, float b, float a)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = (IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>();
	pGraphics->QuadsBegin();
	pGraphics->SetColor(r, g, b, a);
	pGraphics->QuadsEnd();
}

int CLuaBinding::LuaLoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return ((IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>())->LoadTexture(pFilename, StorageType, StoreFormat, Flags);
}

void CLuaBinding::LuaRenderTexture(int ID, float x, float y, float w, float h, float rot)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = (IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>();
	float Width = 400*3.0f*pGraphics->ScreenAspect();
	float Height = 400*3.0f;
	pGraphics->MapScreen(0, 0, Width, Height);
	pGraphics->TextureSet(ID);
	pGraphics->QuadsBegin();
	IGraphics::CQuadItem Item;
	Item.m_X = x; Item.m_Y = y;
	Item.m_Width = w; Item.m_Height = h;
	pGraphics->QuadsSetRotation(rot);
	pGraphics->QuadsDraw(&Item, 1);
	pGraphics->QuadsEnd();
}
