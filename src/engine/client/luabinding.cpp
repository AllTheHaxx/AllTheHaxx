#include <engine/client.h>

#include "luabinding.h"
#include "lua.h"

int CLuaBinding::LuaGetTick()
{
	return CLua::Client()->GameTick();
}
