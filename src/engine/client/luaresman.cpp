#include <engine/graphics.h>
#include <engine/sound.h>
#include "lua/luasql.h"
#include "luaresman.h"

void CLuaRessourceMgr::FreeAll(IKernel *pKernel)
{
	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) \
	dbg_msg("DENNIS", "RESMAN START DELETE " #TYPE " " #VARNAME " %i", (int)m_##VARNAME.size()); \
			for(int i = 0; i < (int)m_##VARNAME.size(); i++)/*std::vector<TYPE>::iterator it = m_##VARNAME.begin(); it != m_##VARNAME.end(); ++it)*//*TYPE ELEM = m_##VARNAME.back(); !m_##VARNAME.empty(); m_##VARNAME.pop_back(), ELEM = m_##VARNAME.back())*/ \
				{ TYPE ELEM = m_##VARNAME.at(i); dbg_msg("DENNIS", "RESMAN DELETE " #TYPE " " #VARNAME " @ %i USING '" #DELETION "'", i); DELETION; } \
			m_##VARNAME.clear();
	#include "luaresmandef.h"
	#undef REGISTER_RESSOURCE
}

