#include <engine/client.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

#include <game/client/components/controls.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>

#include "luabinding.h"
#include "lua.h"

CLuaBinding::UiContainer * CLuaBinding::m_pUiContainer = 0;
CConfiguration * CConfigProperties::m_pConfig = 0;

// system namespace
bool CLuaBinding::LuaImport(int UID, const char *pFilename)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	for(int i = 0; i < pGameClient->Client()->Lua()->GetLuaFiles().size(); i++)
	{
		if(pGameClient->Client()->Lua()->GetLuaFiles()[i]->GetUID() == UID)
		{
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "lua/%s", pFilename); // force path to prevent kids from importing events.lua
			pGameClient->Client()->Lua()->GetLuaFiles()[i]->LoadFile(aBuf);
			return true;
		}
	}
	return false;
}

// local info
int CLuaBinding::LuaGetFPS()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return 1.0f/pGameClient->Client()->RenderFrameTime();
}

// external info
int CLuaBinding::LuaGetPlayerScore(int ClientID)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();

	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CNetObj_PlayerInfo *pInfo = pGameClient->m_Snap.m_paPlayerInfos[ClientID];
		if (pInfo)
		{
			return pInfo->m_Score;
		}
	}

	return -1;
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
	//IGraphics *pGraphics = (IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>();

	CUIRect rect;
	rect.x = x; rect.y = y;
	rect.w = w; rect.h = h;

	//pGraphics->MapScreen(0, 0, pGameClient->UI()->Screen()->w, pGameClient->UI()->Screen()->h); // remapping the screen breaks existing gui stuff :S
	pGameClient->RenderTools()->DrawUIRect(&rect, CLuaBinding::m_pUiContainer->Color, corners, rounding);

	//float Width = 400*3.0f*pGraphics->ScreenAspect();
	//float Height = 400*3.0f;
	//pGraphics->MapScreen(0, 0, Width, Height);
}

int CLuaBinding::LuaDoButton_Menu(const char *pText, int Checked, float x, float y, float w, float h, const char *pTooltip, int Corners)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	static int ID[1024] = {0}; // hm.
	int hash = round_to_int(x) << 12;
	hash |= round_to_int(y) << 8;
	hash |= round_to_int(w) << 4;
	hash |= round_to_int(h) << 0;
	CUIRect r;
	r.x = x; r.y = y;
	r.w = w; r.h = h;
	return pGameClient->m_pMenus->DoButton_Menu(&ID[hash/1024], pText ? pText : "", Checked, &r, pTooltip ? pTooltip : "", Corners, CLuaBinding::m_pUiContainer->Color);
}


// components namespace
// --- collision
int CLuaBinding::LuaColGetMapWidth()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return pGameClient->Collision()->GetWidth();
}

int CLuaBinding::LuaColGetMapHeight()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return pGameClient->Collision()->GetHeight();
}

int CLuaBinding::LuaColGetTile(int x, int y)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return pGameClient->Collision()->GetTileRaw(x, y);
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

void CLuaBinding::LuaDrawLine(float xFrom, float yFrom, float xTo, float yTo)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = (IGraphics *)pGameClient->Kernel()->RequestInterface<IGraphics>();

	IGraphics::CLineItem l;
	l.m_X0 = xFrom;
	l.m_X1 = xTo;
	l.m_Y0 = yFrom;
	l.m_Y1 = yTo;
	pGraphics->LinesBegin();
	pGraphics->LinesDraw(&l, 1);
	pGraphics->LinesEnd();
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
