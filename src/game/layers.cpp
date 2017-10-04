/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/serverbrowser.h>
#include <engine/client.h>
#include <base/vmath.h>
#include <base/math.h>
#include "layers.h"

int gs_ExtrasSizes[NUM_EXTRAS][EXTRATILE_DATA / 2] = { {},
		{},
		{},
		{4, 4, 5}, //EXTRAS_SPEEDUP
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
		{},
};


CLayers::CLayers()
{
	m_GroupsNum = 0;
	m_GroupsStart = 0;
	m_LayersNum = 0;
	m_LayersStart = 0;
	m_pGameGroup = 0;
	m_pGameLayer = 0;
	m_pMap = 0;

	// DDNet
	m_pTeleLayer = 0;
	m_pSpeedupLayer = 0;
	m_pFrontLayer = 0;
	m_pSwitchLayer = 0;
	m_pTuneLayer = 0;

	// BW
	m_NumExtrasLayer = 0;
	m_apExtrasData = 0x0;
	m_apExtrasTiles = 0x0;
	m_aExtrasWidth = 0x0;
	m_aExtrasHeight = 0x0;
}

CLayers::~CLayers()
{
	delete[] m_apExtrasData;
	delete[] m_apExtrasTiles;
	delete m_aExtrasWidth;
	delete m_aExtrasHeight;
}

void CLayers::Init(class IKernel *pKernel)
{
	m_pMap = pKernel->RequestInterface<IMap>();
	m_pMap->GetType(MAPITEMTYPE_GROUP, &m_GroupsStart, &m_GroupsNum);
	m_pMap->GetType(MAPITEMTYPE_LAYER, &m_LayersStart, &m_LayersNum);

	InitGameLayers();
	InitExtraLayers(); // BW
}

void CLayers::InitGameLayers()
{
	m_pTeleLayer = 0;
	m_pSpeedupLayer = 0;
	m_pFrontLayer = 0;
	m_pSwitchLayer = 0;
	m_pTuneLayer = 0;

	for(int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer+l);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);

				if(pTilemap->m_Flags&TILESLAYERFLAG_GAME)
				{
					m_pGameLayer = pTilemap;
					m_pGameGroup = pGroup;

					// make sure the game group has standard settings
					m_pGameGroup->m_OffsetX = 0;
					m_pGameGroup->m_OffsetY = 0;
					m_pGameGroup->m_ParallaxX = 100;
					m_pGameGroup->m_ParallaxY = 100;

					if(m_pGameGroup->m_Version >= 2)
					{
						m_pGameGroup->m_UseClipping = 0;
						m_pGameGroup->m_ClipX = 0;
						m_pGameGroup->m_ClipY = 0;
						m_pGameGroup->m_ClipW = 0;
						m_pGameGroup->m_ClipH = 0;
					}

					//break;
				}
				if(pTilemap->m_Flags&TILESLAYERFLAG_TELE)
				{
					if(pTilemap->m_Version <= 2)
					{
						pTilemap->m_Tele = *((int*)(pTilemap) + 15);
					}
					m_pTeleLayer = pTilemap;
				}
				if(pTilemap->m_Flags&TILESLAYERFLAG_SPEEDUP)
				{
					if(pTilemap->m_Version <= 2)
					{
						pTilemap->m_Speedup = *((int*)(pTilemap) + 16);
					}
					m_pSpeedupLayer = pTilemap;
				}
				if(pTilemap->m_Flags&TILESLAYERFLAG_FRONT)
				{
					if(pTilemap->m_Version <= 2)
					{
						pTilemap->m_Front = *((int*)(pTilemap) + 17);
					}
					m_pFrontLayer = pTilemap;
				}
				if(pTilemap->m_Flags&TILESLAYERFLAG_SWITCH)
				{
					if(pTilemap->m_Version <= 2)
					{
						pTilemap->m_Switch = *((int*)(pTilemap) + 18);
					}
					m_pSwitchLayer = pTilemap;
				}
				if(pTilemap->m_Flags&TILESLAYERFLAG_TUNE)
				{
					if(pTilemap->m_Version <= 2)
					{
						pTilemap->m_Tune = *((int*)(pTilemap) + 19);
					}
					m_pTuneLayer = pTilemap;
				}
			}
		}
	}
}

void CLayers::InitExtraLayers()
{
	m_NumExtrasLayer = 0;
	delete[] m_apExtrasData;
	delete[] m_apExtrasTiles;
	delete[] m_aExtrasWidth;
	delete[] m_aExtrasHeight;
	m_apExtrasData = NULL;
	m_apExtrasTiles = NULL;
	m_aExtrasWidth = NULL;
	m_aExtrasHeight = NULL;

	for (int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for (int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer + l);
			if (pLayer->m_Type == LAYERTYPE_EXTRAS)
			{
				CMapItemLayerTilemapBW *pTilemap = reinterpret_cast<CMapItemLayerTilemapBW *>(pLayer);
				if (pTilemap->m_ExtraVersion != EXTRA_VERSION)
				{
					dbg_msg("layers/warn", "Extras layer #%i could not be loaded due to the incorrect version (%i != %i)",
							m_NumExtrasLayer+1, pTilemap->m_ExtraVersion, EXTRA_VERSION);
					continue;
				}

				m_NumExtrasLayer++;
			}
		}
	}

	if(m_NumExtrasLayer <= 0)
	{
		if(g_Config.m_Debug)
			dbg_msg("layers/dbg", "map doesn't seem to have any ExtraLayers");
		return;
	}

	if(g_Config.m_Debug)
		dbg_msg("layers/dbg", "found %i ExtraLayer%s on the map", m_NumExtrasLayer, m_NumExtrasLayer == 1 ? "" : "s");

	m_apExtrasData = new CExtrasData*[m_NumExtrasLayer];
	m_apExtrasTiles = new CTile*[m_NumExtrasLayer];
	m_aExtrasWidth = new int[m_NumExtrasLayer];
	m_aExtrasHeight = new int[m_NumExtrasLayer];

	int Counter = 0;
	for (int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for (int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer + l);
			if (pLayer->m_Type == LAYERTYPE_EXTRAS)
			{
				CMapItemLayerTilemapBW *pTilemap = reinterpret_cast<CMapItemLayerTilemapBW *>(pLayer);
				if (pTilemap->m_ExtraVersion != EXTRA_VERSION)
					continue;

				m_apExtrasData[Counter] = static_cast<CExtrasData *>(m_pMap->GetData(pTilemap->m_ExtrasData));
				m_apExtrasTiles[Counter] = static_cast<CTile *>(m_pMap->GetData(pTilemap->m_Data));
				m_aExtrasWidth[Counter] = pTilemap->m_Width;
				m_aExtrasHeight[Counter] = pTilemap->m_Height;
				Counter++;
			}
		}
	}
}

void CLayers::InitBackground(class IMap *pMap)
{
	m_pMap = pMap;
	m_pMap->GetType(MAPITEMTYPE_GROUP, &m_GroupsStart, &m_GroupsNum);
	m_pMap->GetType(MAPITEMTYPE_LAYER, &m_LayersStart, &m_LayersNum);
	
	//following is here to prevent crash using standard map as background
	m_pTeleLayer = 0;
	m_pSpeedupLayer = 0;
	m_pFrontLayer = 0;
	m_pSwitchLayer = 0;
	m_pTuneLayer = 0;

	for(int g = 0; g < NumGroups(); g++)
	{
		CMapItemGroup *pGroup = GetGroup(g);
		for(int l = 0; l < pGroup->m_NumLayers; l++)
		{
			CMapItemLayer *pLayer = GetLayer(pGroup->m_StartLayer+l);

			if(pLayer->m_Type == LAYERTYPE_TILES)
			{
				CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);

				if(pTilemap->m_Flags&TILESLAYERFLAG_GAME)
				{
					m_pGameLayer = pTilemap;
					m_pGameGroup = pGroup;

					// make sure the game group has standard settings
					m_pGameGroup->m_OffsetX = 0;
					m_pGameGroup->m_OffsetY = 0;
					m_pGameGroup->m_ParallaxX = 100;
					m_pGameGroup->m_ParallaxY = 100;

					if(m_pGameGroup->m_Version >= 2)
					{
						m_pGameGroup->m_UseClipping = 0;
						m_pGameGroup->m_ClipX = 0;
						m_pGameGroup->m_ClipY = 0;
						m_pGameGroup->m_ClipW = 0;
						m_pGameGroup->m_ClipH = 0;
					}
					
					//We don't care about tile layers.	
				}
			}
		}
	}
}

CMapItemGroup *CLayers::GetGroup(int Index) const
{
	return static_cast<CMapItemGroup *>(m_pMap->GetItem(m_GroupsStart+Index, 0, 0));
}

CMapItemLayer *CLayers::GetLayer(int Index) const
{
	return static_cast<CMapItemLayer *>(m_pMap->GetItem(m_LayersStart+Index, 0, 0));
}

// DDRace

void CLayers::Dest()
{
	/*m_pTeleLayer = 0;
	m_pSpeedupLayer = 0;
	m_pFrontLayer = 0;
	m_pSwitchLayer = 0;
	m_pTuneLayer = 0;*/
}

// BW
int CLayers::ExtrasIndex(int Index, float x, float y)
{
	int Nx = clamp(round_to_int(x) / 32, 0, m_aExtrasWidth[Index] - 1),
			Ny = clamp(round_to_int(y) / 32, 0, m_aExtrasHeight[Index] - 1);
	return Ny * m_aExtrasWidth[Index] + Nx;
}

bool CLayers::IsHookThrough(const vec2& Last, const vec2& Pos)
{
	for (int i = 0; i < m_NumExtrasLayer; i++)
	{
		int Tile = GetExtrasTile(i)[ExtrasIndex(i, round_to_int(Pos.x), round_to_int(Pos.y))].m_Index;
		if (Tile == EXTRAS_HOOKTHROUGH)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_TOP && Last.y <= Pos.y)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_BOTTOM && Last.y >= Pos.y)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_LEFT && Last.x >= Pos.x)
			return true;
		if (Tile == EXTRAS_HOOKTHROUGH_RIGHT && Last.x <= Pos.x)
			return true;
	}
	return false;
}

bool CLayers::IsExtrasSpeedup(const vec2& Pos)
{
	for (int i = 0; i < m_NumExtrasLayer; i++)
	{
		int Tile = GetExtrasTile(i)[ExtrasIndex(i, round_to_int(Pos.x), round_to_int(Pos.y))].m_Index;
		if (Tile == EXTRAS_SPEEDUP)
			return true;
	}
	return false;
}

void CLayers::GetExtrasSpeedup(const vec2& Pos, int *pOutForce, int *pOutMaxSpeed, vec2 *pOutDirection)
{
	for (int i = 0; i < m_NumExtrasLayer; i++)
	{
		int Tile = GetExtrasTile(i)[ExtrasIndex(i, round_to_int(Pos.x), round_to_int(Pos.y))].m_Index;
		if(Tile != EXTRAS_SPEEDUP)
			continue; // that'd be wrong right?

		int Index = ExtrasIndex(i, Pos.x, Pos.y);
		CExtrasData ExtrasData = GetExtrasData(i)[Index];
		const char *pData = ExtrasData.m_aData;
		int Force = str_toint(pData);
		pData += +gs_ExtrasSizes[Tile][0];
		int MaxSpeed = str_toint(pData);
		pData += +gs_ExtrasSizes[Tile][1];
		int Angle = str_toint(pData);
		pData += +gs_ExtrasSizes[Tile][2];
		float AngleRad = Angle * (3.1415f/180.0f);
		*pOutDirection = vec2(cosf(AngleRad), sinf(AngleRad));
		*pOutForce = Force;
		*pOutMaxSpeed = MaxSpeed;
	}

}
