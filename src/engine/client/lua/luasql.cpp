#include <engine/client/luabinding.h>
#include "luasql.h"


CLuaSqlConn *CLuaSql::Open(const char *pFilename, lua_State *L)
{
	char aBuf[512];
	str_copyb(aBuf, pFilename);
	pFilename = CLuaBinding::SandboxPath(aBuf, sizeof(aBuf), L, false);

	dbg_msg("lua/sql/debug", "opening db '%s'", pFilename);

	CLuaFile *pLF = CLuaBinding::GetLuaFile(L);
	CLuaSqlConn *pConn = new CLuaSqlConn(pFilename, pLF);
	pLF->GetResMan()->RegisterLuaSqlConn(pConn);
	return pConn;
}

CLuaSqlConn::~CLuaSqlConn()
{
	Flush();
	m_pLuaFile->GetResMan()->DeregisterLuaSqlConn(this);
}

void CLuaSqlConn::Execute(const char *pStatement, LuaRef Callback, lua_State *L)
{
	if(Callback.isString())
	{
		// convert the function name (string) the the actual function
		LuaRef Func = luabridge::getGlobal(L, Callback.tostring().c_str());
		Callback = Func;
	}

	if(!Callback.isFunction() && !Callback.isNil())
		luaL_error(L, "SetCallback expects a string, a function or nil as first parameter (got %s)", lua_typename(L, Callback.type()));

	char *pQueryBuf = sqlite3_mprintf(pStatement);
	CLuaSqlQuery *pQuery = new CLuaSqlQuery(pQueryBuf, Callback);
	m_Db.InsertQuery(pQuery);
}

void CLuaSqlQuery::OnData()
{
	int i = 0;
	while(Next())
	{
		// call lua
		if(m_CbFunc.isFunction())
		{
			try
			{
				m_CbFunc((CQuery *)this, GetQueryString(), i); // need to downcast as lua only knows the base class
			} catch(luabridge::LuaException &e) {
				CLua::m_pClient->Lua()->HandleException(e, m_CbFunc.state());
			}
		}
		i++;
	};
}
