#ifndef GAME_CLIENT_ASTAR_H
#define GAME_CLIENT_ASTAR_H

#include <base/vmath.h>
#include <base/tl/array.h>

#include <game/client/component.h>


class CAStar : public CComponent
{
	bool m_MapReloaded;

	void* m_pThread;
	bool m_ThreadShouldExit;

protected:
	class CCollision *Collision() const { return m_pClient->Collision(); }
	class IClient *Client() const { return m_pClient->Client(); }

	int GetTileAreaCenter(int TileID, int x = 0, int y = 0, int w = -1, int h = -1);
	int GetStart() {return GetTileAreaCenter(TILE_BEGIN); }
	int GetFinish() { return GetTileAreaCenter(TILE_END); }
	void FillGrid(bool NoFreeze);
	static void BuildPath(void *pUser);

	char *m_pField;
	array<vec2> m_Path;
	bool m_PathFound;

	vec2 m_LastPos;

public:
	CAStar();
	~CAStar();

	virtual void OnReset();
	virtual void OnPlayerDeath();
	virtual void OnRender();
	virtual void OnMapLoad();
	virtual void OnStateChange(int NewState, int OldState);
};

#endif
