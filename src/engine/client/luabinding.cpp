#include <engine/client.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

#include <game/client/components/controls.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>

#include "luabinding.h"
#include "lua.h"

CLuaBinding::UiContainer * CLuaBinding::m_pUiContainer = 0;

// client namespace
void CLuaBinding::LuaConnect(const char *pAddr)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	str_copy(g_Config.m_UiServerAddress, pAddr, sizeof(g_Config.m_UiServerAddress));
	pGameClient->Client()->Connect(pAddr);
}

int CLuaBinding::LuaGetTick()
{
	return CLua::Client()->GameTick();
}

// local info
int CLuaBinding::LuaGetLocalCharacterID()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	if(pGameClient->m_Snap.m_pLocalCharacter)
		return pGameClient->m_Snap.m_LocalClientID;
	else
		return -1;
}

/*int CLuaBinding::LuaGetLocalCharacterPos()
{
	// not sure how to return a vector/two values in luabridge
}*/

int CLuaBinding::LuaGetLocalCharacterWeapon()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	if(pGameClient->m_Snap.m_pLocalCharacter)
		return pGameClient->m_Snap.m_pLocalCharacter->m_Weapon;
	else
		return -1;
}

int CLuaBinding::LuaGetLocalCharacterWeaponAmmo()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	if(pGameClient->m_Snap.m_pLocalCharacter)
		return pGameClient->m_Snap.m_pLocalCharacter->m_AmmoCount;
	else
		return -1;
}

int CLuaBinding::LuaGetLocalCharacterHealth()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	if(pGameClient->m_Snap.m_pLocalCharacter)
		return pGameClient->m_Snap.m_pLocalCharacter->m_Health;
	else
		return -1;
}

int CLuaBinding::LuaGetLocalCharacterArmor()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	if(pGameClient->m_Snap.m_pLocalCharacter)
		return pGameClient->m_Snap.m_pLocalCharacter->m_Armor;
	else
		return -1;
}

int CLuaBinding::LuaGetFPS()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return 1.0f/pGameClient->Client()->RenderFrameTime();
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

int CLuaBinding::LuaDoButton_Menu(const char *pText, int Checked, float x, float y, float w, float h, const char *pTooltip, int Corners)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	int ID = 0; // hm.
	CUIRect r;
	r.x = x; r.y = y;
	r.w = w; r.h = h;
	return pGameClient->m_pMenus->DoButton_Menu(&ID, pText ? pText : "", Checked, &r, pTooltip ? pTooltip : "", Corners, CLuaBinding::m_pUiContainer->Color);
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

// --- emote
void CLuaBinding::LuaEmoteSend(int Emote)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();

	CNetMsg_Cl_Emoticon Msg;
	Msg.m_Emoticon = Emote;
	pGameClient->Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

// --- controls
void CLuaBinding::LuaLockInput()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	pGameClient->m_pControls->m_LuaLockInput = true;
}

void CLuaBinding::LuaUnlockInput()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	pGameClient->m_pControls->m_LuaLockInput = false;
}

bool CLuaBinding::LuaInputLocked()
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	return pGameClient->m_pControls->m_LuaLockInput;
}

void CLuaBinding::LuaSetInput(const char *pInput, int Value)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();

	if(str_comp_nocase(pInput, "Direction") == 0)
		pGameClient->m_pControls->m_LuaInputData[g_Config.m_ClDummy].m_Direction = Value;
	else if(str_comp_nocase(pInput, "Fire") == 0)
		pGameClient->m_pControls->m_LuaInputData[g_Config.m_ClDummy].m_Fire = Value;
	else if(str_comp_nocase(pInput, "Hook") == 0)
		pGameClient->m_pControls->m_LuaInputData[g_Config.m_ClDummy].m_Hook = Value;
	else if(str_comp_nocase(pInput, "Jump") == 0)
		pGameClient->m_pControls->m_LuaInputData[g_Config.m_ClDummy].m_Jump = Value;
	else if(str_comp_nocase(pInput, "Weapon") == 0)
		pGameClient->m_pControls->m_LuaInputData[g_Config.m_ClDummy].m_WantedWeapon = Value;
	else if(str_comp_nocase(pInput, "TargetX") == 0)
		pGameClient->m_pControls->m_LuaInputData[g_Config.m_ClDummy].m_TargetX = Value;
	else if(str_comp_nocase(pInput, "TargetY") == 0)
		pGameClient->m_pControls->m_LuaInputData[g_Config.m_ClDummy].m_TargetY = Value;
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
