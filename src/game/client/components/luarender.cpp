#include "luarender.h"
#include "console.h"

void CLuaRender::OnRender()
{
	// EVENT CALL
	LUA_FIRE_EVENT("OnRenderLevel", m_Level);
}
