#include <base/system.h>
#include <base/math.h>

#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>

#include "gametexture.h"

static const char *ms_pTextureDirs[] = {
		"game",
		"cursor",
		"emotes",
		"particles"
};


int CGameTextureManager::SkinScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	int l = str_length(pName);
	if(pName[0] == '!' || l < 4 || IsDir || str_comp(pName+l-4, ".png") != 0)
		return 0;

	CLoadHelper *pData = (CLoadHelper *)pUser;
	CGameTextureManager *pSelf = pData->m_pSelf;

	char aPath[512];
	str_format(aPath, sizeof(aPath), "data/textures/%s/%s", ms_pTextureDirs[pData->m_ScanType], pName);

	bool IsUsed = ((pData->m_ScanType == TEXTURE_GROUP_GAME && str_comp(pName, g_Config.m_TexGame) == 0)	||
	 			(pData->m_ScanType == TEXTURE_GROUP_CURSOR && str_comp(pName, g_Config.m_TexCursor) == 0)	||
				(pData->m_ScanType == TEXTURE_GROUP_EMOTE && str_comp(pName, g_Config.m_TexEmoticons) == 0)	||
				(pData->m_ScanType == TEXTURE_GROUP_PARTICLES && str_comp(pName, g_Config.m_TexParticles) == 0));


	// set skin data
	CGameSkin Skin;
	if(g_Config.m_TexLazyLoading && !IsUsed)
		Skin.m_Texture = 0;
	else
		Skin.m_Texture = pSelf->Graphics()->LoadTexture(aPath, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	str_copy(Skin.m_aName, pName, min((int)sizeof(Skin.m_aName),l-3));
	pSelf->m_aSkins[pData->m_ScanType].add(Skin);


	if(g_Config.m_Debug)
		pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "game", "loading %s-texture %s", ms_pTextureDirs[pData->m_ScanType], pName);

	return 0;
}

void CGameTextureManager::OnInit()
{
	// load skins
	for(int i = 0; i < NUM_TEXTURE_GROUPS; i++)
	{
		m_aSkins[i].clear();

		// load Default
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "textures/%s/!default.png", ms_pTextureDirs[i]);

		// set default skin data
		CGameSkin DefaultSkin;
		DefaultSkin.m_Texture = Graphics()->LoadTexture(aBuf, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		str_format(DefaultSkin.m_aName, sizeof(DefaultSkin.m_aName), "!default");
		m_aSkins[i].add(DefaultSkin);


		str_format(aBuf, sizeof(aBuf), "data/textures/%s", ms_pTextureDirs[i]);
		CLoadHelper LoadHelper(this, i);
		Storage()->ListDirectory(IStorageTW::TYPE_ALL, aBuf, SkinScan, &LoadHelper);
		if(!m_aSkins[i].size())
		{
			Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "gameclient", "failed to load textures. folder='textures/%s'", ms_pTextureDirs[i]);
			CGameSkin DummySkin;
			DummySkin.m_Texture = -1;
			str_copy(DummySkin.m_aName, "dummy", sizeof(DummySkin.m_aName));
			m_aSkins[i].add(DummySkin);
		}
	}
}

void CGameTextureManager::OnReset()
{
	SetTexture(IMAGE_GAME, g_Config.m_TexGame);
	SetTexture(IMAGE_HUDCURSOR, g_Config.m_TexCursor);
	SetTexture(IMAGE_EMOTICONS, g_Config.m_TexEmoticons);
	SetTexture(IMAGE_PARTICLES, g_Config.m_TexParticles);
}

int CGameTextureManager::SetTexture(int Image, const char *pName)
{
	int Group =	 Image == IMAGE_GAME  ? TEXTURE_GROUP_GAME :
				 Image == IMAGE_HUDCURSOR  ? TEXTURE_GROUP_CURSOR :
				 Image == IMAGE_EMOTICONS  ? TEXTURE_GROUP_EMOTE :
				 Image == IMAGE_PARTICLES  ? TEXTURE_GROUP_PARTICLES : -1;

	dbg_assert(Group >= 0 && Group < NUM_TEXTURE_GROUPS, "CGameTextureManager::SetTexture invalid group");

	return g_pData->m_aImages[Image].m_Id = FindTexture(Group, pName);
}

int CGameTextureManager::FindTexture(int Group, const char *pName)
{
	for(int i = 0; i < m_aSkins[Group].size(); i++)
	{
		if(str_comp(m_aSkins[Group][i].m_aName, pName) == 0)
		{
			if(m_aSkins[Group][i].m_Texture <= 0)
			{
				char aPath[512];
				str_format(aPath, sizeof(aPath), "textures/%s/%s.png", ms_pTextureDirs[Group], pName);
				m_aSkins[Group][i].m_Texture = Graphics()->LoadTexture(aPath, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
			}
			return m_aSkins[Group][i].m_Texture;
		}
	}
	return 0;
}
