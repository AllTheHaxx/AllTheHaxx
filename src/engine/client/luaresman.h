#ifndef ENGINE_CLIENT_LUARESMAN_H
#define ENGINE_CLIENT_LUARESMAN_H

#include <vector>
#include <base/system.h>

class CLuaSqlConn;


class CLuaRessourceMgr
{
	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) std::vector<TYPE> m_##VARNAME;
	#include "luaresmandef.h"
	#undef REGISTER_RESSOURCE

public:
	#define REGISTER_RESSOURCE(TYPE, VARNAME, DELETION) \
			void Register##VARNAME(TYPE VARNAME) \
			{ \
				for(std::vector<TYPE>::iterator it = m_##VARNAME.begin(); it != m_##VARNAME.end(); ++it) \
					dbg_assert_strict(*it != (VARNAME), "[LuaResMan] registered duplicate of '" #TYPE " " #VARNAME "'"); \
				m_##VARNAME.push_back(VARNAME); \
			} \
			\
			void Deregister##VARNAME(TYPE VARNAME) \
            { \
                for(std::vector<TYPE>::iterator it = m_##VARNAME.begin(); it != m_##VARNAME.end(); ++it) \
                { \
                    if(*it == (VARNAME)) \
                    { \
                        m_##VARNAME.erase(it); \
                        return; \
                    } \
                } \
				dbg_assert_strict(false, "[LuaResMan] tried deregistering a non-existing object of '" #TYPE " " #VARNAME "'"); \
            }

	#include "luaresmandef.h"
	#undef REGISTER_RESSOURCE

	/**
	 * Uses the defined deletion method to free all registered objects
	 */
	void FreeAll(class IKernel *pKernel);
};

#endif
