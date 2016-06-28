#include <engine/client.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>

#include "luabinding.h"

CLuaFile* CLuaBinding::GetLuaFile(int UID)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	for(int i = 0; i < pGameClient->Client()->Lua()->GetLuaFiles().size(); i++)
	{
		if(pGameClient->Client()->Lua()->GetLuaFiles()[i]->GetUID() == UID)
		{
			return pGameClient->Client()->Lua()->GetLuaFiles()[i];
		}
	}
	return 0;
}

// global namespace
bool CLuaBinding::LuaImport(int UID, const char *pFilename)
{
	CLuaFile *L = GetLuaFile(UID);
	if(!L)
		return false;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "lua/%s", pFilename); // force path to prevent kids from importing events.lua
	return L->LoadFile(aBuf);
}

bool CLuaBinding::LuaKillScript(int UID)
{
	CLuaFile *L = GetLuaFile(UID);
	if(!L)
		return false;

	L->Unload();
	return true;
}

void CLuaFile::LuaPrintOverride(std::string str)
{
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "LUA|%s", m_Filename.c_str()+4);
	dbg_msg(aBuf, "%s", str.c_str());
}

// external info
int CLuaBinding::LuaGetPlayerScore(int ClientID)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();

	if(ClientID >= 0 && ClientID < MAX_CLIENTS)
	{
		const CNetObj_PlayerInfo *pInfo = pGameClient->m_Snap.m_paPlayerInfos[ClientID];
		if(pInfo)
		{
			return pInfo->m_Score;
		}
	}

	return -1;
}

void CLuaBinding::LuaRenderTexture(int ID, float x, float y, float w, float h, float rot) // depreciated
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = pGameClient->Kernel()->RequestInterface<IGraphics>();

	//pGraphics->MapScreen(0, 0, pGameClient->UI()->Screen()->w, pGameClient->UI()->Screen()->h);
	pGraphics->TextureSet(ID);
	pGraphics->QuadsBegin();
	IGraphics::CQuadItem Item;
	Item.m_X = x; Item.m_Y = y;
	Item.m_Width = w; Item.m_Height = h;
	pGraphics->QuadsSetRotation(rot);
	pGraphics->QuadsDraw(&Item, 1);
	pGraphics->QuadsEnd();

	//float Width = 400*3.0f*pGraphics->ScreenAspect();
	//float Height = 400*3.0f;
	//pGraphics->MapScreen(0, 0, Width, Height);
}

void CLuaBinding::LuaRenderQuadRaw(int x, int y, int w, int h)
{
	CGameClient *pGameClient = (CGameClient *)CLua::GameClient();
	IGraphics *pGraphics = pGameClient->Kernel()->RequestInterface<IGraphics>();

	IGraphics::CQuadItem Item;
	Item.m_X = x; Item.m_Y = y;
	Item.m_Width = w; Item.m_Height = h;
	pGraphics->QuadsDraw(&Item, 1);
}
