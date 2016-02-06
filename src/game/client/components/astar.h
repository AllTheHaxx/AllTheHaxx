#ifndef GAME_CLIENT_ASTAR_H
#define GAME_CLIENT_ASTAR_H

#include <base/vmath.h>
#include <base/tl/array.h>

#include <game/client/component.h>


class CAStar : public CComponent
{
	bool m_MapReloaded;
protected:
	class CCollision *Collision() const { return m_pClient->Collision(); }
	class IClient *Client() { return m_pClient->Client(); }

	int GetStart();
	int GetFinish();
	void FillGrid(bool NoFreeze);
	void BuildPath();

	char *m_pField;
	array<vec2> m_Path;
	array<IGraphics::CLineItem> m_LineItems;
	bool m_PathFound;

	vec2 m_LastPos;

public:
	CAStar();
	~CAStar();

	virtual void OnReset();
	virtual void OnPlayerDeath();
	virtual void OnRender();
	virtual void OnMapLoad();
};

#endif
