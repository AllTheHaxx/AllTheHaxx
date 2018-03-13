#include <engine/graphics.h>
#include <engine/sound.h>
#include "lua/luasql.h"
#include "luaresman.h"

void CLuaRessourceMgr::FreeAll(IKernel *pKernel)
{
	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) \
			for(int i = 0; i < (int)m_##VARNAME.size(); i++) \
				{ TYPE ELEM = m_##VARNAME.at(i); DELETION; } \
			m_##VARNAME.clear();
	#include "luaresmandef.h"
	#undef REGISTER_RESSOURCE
}

