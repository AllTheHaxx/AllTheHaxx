/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_LAYERS_H
#define GAME_LAYERS_H

#include <base/vmath.h>
#include <engine/map.h>
#include <game/mapitems.h>

class CLayers
{
	int m_GroupsNum;
	int m_GroupsStart;
	int m_LayersNum;
	int m_LayersStart;
	CMapItemGroup *m_pGameGroup;
	CMapItemLayerTilemap *m_pGameLayer;
	class IMap *m_pMap;

	// BW mod
	int m_NumExtrasLayer;
	CExtrasData **m_apExtrasData;
	CTile **m_apExtrasTiles;
	int *m_aExtrasWidth;
	int *m_aExtrasHeight;

	void InitGameLayers();
	void InitExtraLayers();

public:
	CLayers();
	~CLayers();
	void Init(class IKernel *pKernel);
	void InitBackground(class IMap *pMap);
	int NumGroups() const { return m_GroupsNum; };
	class IMap *Map() const { return m_pMap; };
	CMapItemGroup *GameGroup() const { return m_pGameGroup; };
	CMapItemLayerTilemap *GameLayer() const { return m_pGameLayer; };
	CMapItemGroup *GetGroup(int Index) const;
	CMapItemLayer *GetLayer(int Index) const;

	// DDRace

	void Dest();
	CMapItemLayerTilemap *TeleLayer() const { return m_pTeleLayer; };
	CMapItemLayerTilemap *SpeedupLayer() const { return m_pSpeedupLayer; };
	CMapItemLayerTilemap *FrontLayer() const { return m_pFrontLayer; };
	CMapItemLayerTilemap *SwitchLayer() const { return m_pSwitchLayer; };
	CMapItemLayerTilemap *TuneLayer() const { return m_pTuneLayer; };

	// BW
	int ExtrasIndex(int Index, float x, float y);
	int GetNumExtrasLayer() const { return m_NumExtrasLayer; };
	CTile *GetExtrasTile(int Index) const { return m_apExtrasTiles[Index]; };
	CExtrasData *GetExtrasData(int Index) const { return m_apExtrasData[Index]; };
	int GetExtrasWidth(int Index) const { return m_aExtrasWidth[Index]; };
	int GetExtrasHeight(int Index) const { return m_aExtrasHeight[Index]; };

	bool IsHookThrough(const vec2& Last, const vec2& Pos);
	bool IsExtrasSpeedup(const vec2& Pos);
	void GetExtrasSpeedup(const vec2& Pos, int *pOutForce, int *pOutMaxSpeed, vec2 *pOutDirection);

private:

	CMapItemLayerTilemap *m_pTeleLayer;
	CMapItemLayerTilemap *m_pSpeedupLayer;
	CMapItemLayerTilemap *m_pFrontLayer;
	CMapItemLayerTilemap *m_pSwitchLayer;
	CMapItemLayerTilemap *m_pTuneLayer;
};

#endif
