#ifndef GAME_CLIENT_COMPONENTS_LUARENDER_H
#define GAME_CLIENT_COMPONENTS_LUARENDER_H
#include <game/client/component.h>
#include <engine/input.h>

class CLuaComponent : public CComponent
{
	const int m_Level;

public:
	CLuaComponent(int lv) : m_Level(lv) { }
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent e);
};

#endif

