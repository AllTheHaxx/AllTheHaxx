#ifndef GAME_CLIENT_COMPONENTS_ASTAR_H
#define GAME_CLIENT_COMPONENTS_ASTAR_H

#include <mutex>
#include <atomic>
#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <game/client/component.h>


class CAStar : public CComponent
{
	enum
	{
		COST_NULL = 0,
		COST_AIR = 1,
		COST_NEAR_FREEZE = 3,
		COST_NEAR_DEATH = 4,
		COST_FREEZE = (COST_NEAR_FREEZE*3 + 1) * 5 + COST_NEAR_FREEZE,
		COST_SOLID = 100
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

	std::atomic<void*> m_pBuilderThread;
	std::atomic<void*> m_pScoreThread;
	std::atomic_bool m_ThreadsShouldExit;
	std::mutex m_Mutex;

	char m_aCurrentMap[64];
	class CAStarWorldMap *m_pCurrentMapGrid;
	int m_LastClosestNode; // death position

	bool GetTileAreaCenter(vec2 *pResult, int TileID, int x = 0, int y = 0, int w = -1, int h = -1);

	void ScanMap();
	unsigned short CountTilesAround(int TileID, int x, int y);
	class CAStarWorldMap* FillGrid(class CAStarWorldMap *pMap);

	static void BuildPath(void *pParam);
	void BuildPathRace();
	bool PathFound()
	{
		LOCK_SECTION_MUTEX(m_Mutex)
		return !m_Path.empty();
	}

	sorted_array<Node> m_Path;

	vec2 m_LastPos;

	void StopThreads();

	static void CalcScoreThreadProxy(void *pUser);
	void CalcScoreThread();

public:
	CAStar();
	void InitPathBuilder(const vec2& From, const vec2& To);

	virtual void OnConsoleInit();

	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMapLoad();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnShutdown();

	void OnPlayerDeath();

	bool GetStart(vec2 *pStart) { return GetTileAreaCenter(pStart, TILE_BEGIN); }
	bool GetFinish(vec2 *pFinish) { return GetTileAreaCenter(pFinish, TILE_END); }

	int LuaGetNumNodes() const { return m_Path.size(); }
	vec2 LuaGetNode(int i, lua_State *L) const;
	bool SearchingPath() const { return m_pBuilderThread != NULL; }


private:
	static void ConPathToMouse(IConsole::IResult *pResult, void *pUserData);
};

#endif
