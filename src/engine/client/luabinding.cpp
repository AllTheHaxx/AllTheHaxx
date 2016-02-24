#include <engine/client.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

#include <game/client/components/chat.h>

#include "luabinding.h"
#include "lua.h"

CLuaBinding::UiContainer * CLuaBinding::m_pUiContainer = 0;

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
void CLuaBinding::LuaSetUiColor(float r, float g, float b, float a)
{
	if(r >= 0)
		CLuaBinding::m_pUiContainer->Color.r = r;
	if(g >= 0)
		CLuaBinding::m_pUiContainer->Color.g = g;
	if(b >= 0)
		CLuaBinding::m_pUiContainer->Color.b = b;
	if(a >= 0)
		CLuaBinding::m_pUiContainer->Color.a = a;
}

void CLuaBinding::LuaDrawUiRect(float x, float y, float w, float h, int corners, float rounding)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = (IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>();

	CUIRect rect;
	rect.x = x; rect.y = y;
	rect.w = w; rect.h = h;

	pGraphics->MapScreen(0, 0, pGameClient->UI()->Screen()->w, pGameClient->UI()->Screen()->h);
	pGameClient->RenderTools()->DrawUIRect(&rect, CLuaBinding::m_pUiContainer->Color, corners, rounding);

	float Width = 400*3.0f*pGraphics->ScreenAspect();
	float Height = 400*3.0f;
	pGraphics->MapScreen(0, 0, Width, Height);
}

// components namespace
// --- chat
void CLuaBinding::LuaChatSend(int Team, const char *pMessage)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	pGameClient->m_pChat->Say(Team, pMessage);
}

bool CLuaBinding::LuaChatActive()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return pGameClient->m_pChat->GetMode() != 0;
}

bool CLuaBinding::LuaChatAllActive()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return pGameClient->m_pChat->GetMode() == 1;
}

bool CLuaBinding::LuaChatTeamActive()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return pGameClient->m_pChat->GetMode() == 2;
}


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

	pGraphics->MapScreen(0, 0, pGameClient->UI()->Screen()->w, pGameClient->UI()->Screen()->h);
	pGraphics->TextureSet(ID);
	pGraphics->QuadsBegin();
	IGraphics::CQuadItem Item;
	Item.m_X = x; Item.m_Y = y;
	Item.m_Width = w; Item.m_Height = h;
	pGraphics->QuadsSetRotation(rot);
	pGraphics->QuadsDraw(&Item, 1);
	pGraphics->QuadsEnd();

	float Width = 400*3.0f*pGraphics->ScreenAspect();
	float Height = 400*3.0f;
	pGraphics->MapScreen(0, 0, Width, Height);
}
