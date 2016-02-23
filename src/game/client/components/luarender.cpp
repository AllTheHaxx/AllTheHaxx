#include "luarender.h"

void CLuaRender::OnRender()
{
	// EVENT CALL
	LuaRef func = Client()->Lua()->GetFunc("OnRenderLevel");
	if(func)
		func(m_Level);
}
