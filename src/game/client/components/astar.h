#ifndef GAME_CLIENT_COMPONENTS_ASTAR_H
#define GAME_CLIENT_COMPONENTS_ASTAR_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <game/client/component.h>


class CAStar : public CComponent
{
	enum
	{
		COST_AIR = 0,
		COST_NEAR_FREEZE = 1,
		COST_NEAR_DEATH = 2,
		COST_FREEZE = 5,
		COST_SOLID = 9
	};

	struct Node
	{
		int m_ID;
		vec2 m_Pos;

		Node() { }
		Node(int ID, vec2 Pos) : m_ID(ID), m_Pos(Pos) { }

		bool operator<(Node& other) { return m_ID < other.m_ID; }
	};

	struct CPathBuilderParams
	{
		CAStar *m_pSelf;
		vec2 m_From;
		vec2 m_To;

		CPathBuilderParams(CAStar *pSelf, const vec2& From, const vec2& To) : m_pSelf(pSelf), m_From(From), m_To(To) {}
	};

	bool m_MapReloaded;

	void *m_pBuilderThread;
	void *m_pScoreThread;
	bool m_ThreadsShouldExit;
	LOCK_SMART m_PathLock;

	char m_aCurrentMap[64];
	class AStarWorldMap *m_pCurrentMapGrid;
	int m_LastClosestNode; // death position

	bool GetTileAreaCenter(vec2 *pResult, int TileID, int x = 0, int y = 0, int w = -1, int h = -1);
	bool GetStart(vec2 *pStart) { return GetTileAreaCenter(pStart, TILE_BEGIN); }
	bool GetFinish(vec2 *pFinish) { return GetTileAreaCenter(pFinish, TILE_END); }

	void ScanMap();
	class AStarWorldMap* FillGrid(class AStarWorldMap *pMap);

	static void BuildPath(void *pParam);
	void InitPathBuilder(const vec2& From, const vec2& To);
	void BuildPathRace();
	bool PathFound() const { return !m_Path.empty(); }

	sorted_array<Node> m_Path;

	vec2 m_LastPos;

	void StopThreads();

	static void CalcScoreThreadProxy(void *pUser);
	void CalcScoreThread();

public:
	CAStar();

	virtual void OnConsoleInit();

	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMapLoad();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnShutdown();

	void OnPlayerDeath();

private:
	static void ConPathToMouse(IConsole::IResult *pResult, void *pUserData);
};

#endif
