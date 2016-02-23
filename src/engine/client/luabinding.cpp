#include "luabinding.h"
#include "lua.h"
#include <engine/client.h>

int CLuaBinding::GetTickLua()
{
	return CLua::Client()->GameTick();
}