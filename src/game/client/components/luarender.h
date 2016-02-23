#ifndef GAME_CLIENT_COMPONENTS_LUARENDER_H
#define GAME_CLIENT_COMPONENTS_LUARENDER_H
#include <game/client/component.h>

class CLuaRender : public CComponent
{
	int m_Level;

public:
	CLuaRender(int lv) { m_Level = lv; }
	virtual void OnRender();
};

#endif

