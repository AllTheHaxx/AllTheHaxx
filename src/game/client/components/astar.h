#ifndef GAME_CLIENT_COMPONENTS_ASTAR_H
#define GAME_CLIENT_COMPONENTS_ASTAR_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <game/client/component.h>


class CAStar : public CComponent
{
	struct Node
	{
		int m_ID;
		vec2 m_Pos;

		Node() { }
		Node(int ID, vec2 Pos) : m_ID(ID), m_Pos(Pos) { }

		bool operator<(Node& other) { return m_ID < other.m_ID; }
	};

	bool m_MapReloaded;

	void *m_pBuilderThread;
	void *m_pScoreThread;
	bool m_ThreadsShouldExit;

	int m_LastClosestNode; // death position

	bool GetTileAreaCenter(vec2 *pResult, int TileID, int x = 0, int y = 0, int w = -1, int h = -1);
	bool GetStart(vec2 *pStart) { return GetTileAreaCenter(pStart, TILE_BEGIN); }
	bool GetFinish(vec2 *pFinish) { return GetTileAreaCenter(pFinish, TILE_END); }
	class AStarWorldMap* FillGrid(class AStarWorldMap *pMap);
	static void BuildPath(void *pUser);
	bool PathFound() const { return !m_Path.empty(); }

	sorted_array<Node> m_Path;

	vec2 m_LastPos;

	void StopThreads();

	static void StartCalcScoreThread(void *pUser);
	void CalcScoreThread();

public:
	CAStar();

	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMapLoad();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnShutdown();

	void OnPlayerDeath();
};

#endif
