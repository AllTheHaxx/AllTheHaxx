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

	void* m_pThread;
	bool m_ThreadShouldExit;

	int m_LastClosestNode; // death position

protected:
	int GetTileAreaCenter(int TileID, int x = 0, int y = 0, int w = -1, int h = -1);
	int GetStart() { return GetTileAreaCenter(TILE_BEGIN); }
	int GetFinish() { return GetTileAreaCenter(TILE_END); }
	void FillGrid(bool NoFreeze);
	static void BuildPath(void *pUser);

	char *m_pField;
	sorted_array<Node> m_Path;
	bool m_PathFound;

	vec2 m_LastPos;

public:
	CAStar();
	~CAStar();

	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMapLoad();
	virtual void OnStateChange(int NewState, int OldState);

	void OnPlayerDeath();
};

#endif
