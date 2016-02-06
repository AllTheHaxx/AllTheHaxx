#ifndef GAME_CLIENT_ASTAR_H
#define GAME_CLIENT_ASTAR_H

#include <game/client/component.h>
#include <base/vmath.h>

#include <vector>

class CAStar : public CComponent
{
protected:
	class CCollision *Collision() const { return m_pClient->Collision(); }
	class IClient *Client() { return m_pClient->Client(); }

	int GetStart();
	int GetFinish();
	void FillGrid(bool NoFreeze);
	void BuildPath();

	char *m_pField;
	std::vector<vec2> m_Path;
	bool m_PathFound;

	vec2 m_LastPos;

public:
	CAStar();
	~CAStar();

	virtual void OnPlayerDeath();
	virtual void OnRender();
	virtual void OnMapLoad();
};

#endif