#include <base/system.h>
#include <base/math.h>

#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>

#include "cskins.h"

int CcSkins::SkinScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CcSkins *pSelf = (CcSkins *)pUser;
	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".png") != 0)
		return 0;

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "cursor/%s", pName);
	CImageInfo Info;
	if(!pSelf->Graphics()->LoadPNG(&Info, aBuf, DirType))
	{
		str_format(aBuf, sizeof(aBuf), "failed to load cursor from %s", pName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
		return 0;
	}

	CcSkin Skin;
	Skin.m_Texture = pSelf->Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);

	// set skin data
	str_copy(Skin.m_aName, pName, min((int)sizeof(Skin.m_aName),l-3));
	if(g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "load cursor %s", Skin.m_aName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	}
	pSelf->m_aSkins.add(Skin);

	return 0;
}

void CcSkins::OnInit()
{
	// load skins
	m_aSkins.clear();
	
	// load Default 
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "gui_cursor.png");
	CImageInfo Info;
	if(!Graphics()->LoadPNG(&Info, aBuf,IStorageTW::TYPE_ALL))
	{
		str_format(aBuf, sizeof(aBuf), "failed to load default cursor");
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	}
	
	CcSkin DefaultSkin;
	DefaultSkin.m_Texture = Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	
	// set Default skin data
	str_format(DefaultSkin.m_aName, sizeof(DefaultSkin.m_aName), "!default");
	if(g_Config.m_Debug)
	{
		str_format(aBuf, sizeof(aBuf), "load default cursor", DefaultSkin.m_aName);
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "game", aBuf);
	}
	m_aSkins.add(DefaultSkin);
	
	Storage()->ListDirectory(IStorageTW::TYPE_ALL, "cursor", SkinScan, this);
		
	if(!m_aSkins.size())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "gameclient", "failed to load textures. folder='cursor/'");
		CcSkin DummySkin;
		DummySkin.m_Texture = -1;
		str_copy(DummySkin.m_aName, "dummy", sizeof(DummySkin.m_aName));
		m_aSkins.add(DummySkin);
	}
}

void CcSkins::OnReset()
{
	g_pData->m_aImages[IMAGE_CURSOR].m_Id = Get(Find(g_Config.m_GameCursor))->m_Texture;
}

int CcSkins::Num()
{
	return m_aSkins.size();
}

const CcSkins::CcSkin *CcSkins::Get(int Index)
{
	return &m_aSkins[max(0, Index%m_aSkins.size())];
}

int CcSkins::Find(const char *pName)
{
	for(int i = 0; i < m_aSkins.size(); i++)
	{
		if(str_comp(m_aSkins[i].m_aName, pName) == 0)
			return i;
	}
	return -1;
}

vec3 CcSkins::GetColorV3(int v)
{
	return HslToRgb(vec3(((v>>16)&0xff)/255.0f, ((v>>8)&0xff)/255.0f, 0.5f+(v&0xff)/255.0f*0.5f));
}

vec4 CcSkins::GetColorV4(int v)
{
	vec3 r = GetColorV3(v);
	return vec4(r.r, r.g, r.b, 1.0f);
}
