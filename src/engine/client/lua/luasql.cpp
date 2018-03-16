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
	delete m_pDb;
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

	bool CallbackIsFunc = Callback.isFunction();
	if(!CallbackIsFunc && !Callback.isNil())
		luaL_error(L, "Execute expects a string, a function or nil as second parameter (got %s)", lua_typename(L, Callback.type()));

	int len = str_length(pStatement);
	if(len == 0)
		luaL_error(L, "Empty statement");

	char *pQueryBuf = (char*)sqlite3_malloc(len+1);
	str_copy(pQueryBuf, pStatement, len+1);
	CLuaSqlQuery *pQuery = new CLuaSqlQuery(pQueryBuf, Callback);

	if(CallbackIsFunc)
		m_pDb->InsertQuerySync(pQuery); // when there's a callback we have to use the main thread else it would damage lua
	else
		m_pDb->InsertQuery(pQuery); // but when there's no callback we can happily send it off to the separate thread
}

void CLuaSqlQuery::OnData()
{
	int i = 0;
	while(Next())
	{
		if(!m_GotCb) // it's down in here because we have to call Next() at least once
			return;

		// call lua
		try
		{
			m_CbFunc((CQuery *)this, GetQueryString(), i); // need to downcast as lua only knows the base class
		} catch(luabridge::LuaException &e) {
			CLua::m_pClient->Lua()->HandleException(e, m_CbFunc.state());
		}
		i++;
	};
}
