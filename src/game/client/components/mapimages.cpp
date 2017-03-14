/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <engine/serverbrowser.h>
#include <game/client/component.h>
#include <game/mapitems.h>
#include <game/generated/client_data.h>

#include "gametexture.h"
#include "mapimages.h"

CMapImages::CMapImages()
{
	m_Count = 0;
}

void CMapImages::OnMapLoad()
{
	IMap *pMap = Kernel()->RequestInterface<IMap>();

	// unload all textures
	for(int i = 0; i < m_Count; i++)
	{
		Graphics()->UnloadTexture(m_aTextures[i]);
		m_aTextures[i] = -1;
	}
	m_Count = 0;

	int Start;
	pMap->GetType(MAPITEMTYPE_IMAGE, &Start, &m_Count);

	// load new textures
	for(int i = 0; i < m_Count; i++)
	{
		m_aTextures[i] = 0;

		CMapItemImage *pImg = (CMapItemImage *)pMap->GetItem(Start+i, 0, 0);
		if(pImg->m_External)
		{
			char Buf[256];
			char *pName = (char *)pMap->GetData(pImg->m_ImageName);
			str_format(Buf, sizeof(Buf), "mapres/%s.png", pName);
			m_aTextures[i] = Graphics()->LoadTexture(Buf, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		}
		else
		{
			void *pData = pMap->GetData(pImg->m_ImageData);
			m_aTextures[i] = Graphics()->LoadTextureRaw(pImg->m_Width, pImg->m_Height, CImageInfo::FORMAT_RGBA, pData, CImageInfo::FORMAT_RGBA, 0);
			pMap->UnloadData(pImg->m_ImageData);
		}
	}
}

void CMapImages::LoadBackground(class IMap *pMap)
{
	// unload all textures
	for(int i = 0; i < m_Count; i++)
	{
		Graphics()->UnloadTexture(m_aTextures[i]);
		m_aTextures[i] = -1;
	}
	m_Count = 0;

	int Start;
	pMap->GetType(MAPITEMTYPE_IMAGE, &Start, &m_Count);

	// load new textures
	for(int i = 0; i < m_Count; i++)
	{
		m_aTextures[i] = 0;

		CMapItemImage *pImg = (CMapItemImage *)pMap->GetItem(Start+i, 0, 0);
		if(pImg->m_External)
		{
			char Buf[256];
			char *pName = (char *)pMap->GetData(pImg->m_ImageName);
			str_format(Buf, sizeof(Buf), "mapres/%s.png", pName);
			m_aTextures[i] = Graphics()->LoadTexture(Buf, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		}
		else
		{
			void *pData = pMap->GetData(pImg->m_ImageData);
			m_aTextures[i] = Graphics()->LoadTextureRaw(pImg->m_Width, pImg->m_Height, CImageInfo::FORMAT_RGBA, pData, CImageInfo::FORMAT_RGBA, 0);
			pMap->UnloadData(pImg->m_ImageData);
		}
	}
}

int CMapImages::GetEntities()
{
	static int s_EntitiesTextures = -1;
	static char s_aEntitiesGameType[32] = {0};

	CServerInfo Info;
	Client()->GetServerInfo(&Info);

	if(s_EntitiesTextures == -1 || str_comp(s_aEntitiesGameType, Info.m_aGameType))
	{
		// DDNet default to prevent delay in seeing entities
		const char *pFile = 0;
		if(IsDDNet(&Info))
			pFile = "ddnet";
		else if(IsDDRace(&Info))
			pFile = "ddrace";
		else if(IsRace(&Info))
			pFile = "race";
		else if(IsFNG(&Info))
			pFile = "fng";
		else if(IsVanilla(&Info))
			pFile = "vanilla";

		str_copy(s_aEntitiesGameType, Info.m_aGameType, sizeof(s_aEntitiesGameType));

		if(pFile)
		{
			char aPath[256];
			str_format(aPath, sizeof(aPath), "textures/entities/clear/%s.png", pFile);
			if(s_EntitiesTextures > 0)
				Graphics()->UnloadTexture(s_EntitiesTextures);
			s_EntitiesTextures = Graphics()->LoadTexture(aPath, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0); // don't worry about leaks here, this is being cached
		}
		else
		{
			s_EntitiesTextures = -1;
			return g_pData->m_aImages[IMAGE_ENTITIES].m_Id;
		}
	}
	return s_EntitiesTextures;
}
