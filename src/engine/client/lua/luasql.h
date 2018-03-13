#ifndef ENGINE_CLIENT_LUA_LUASQL_H
#define ENGINE_CLIENT_LUA_LUASQL_H

#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/client/db_sqlite3.h>

using luabridge::LuaRef;

// passed to lua, handles the db (essentially a wrapper for CSql)
class CLuaSqlConn
{
	CSql m_Db;
	char m_aPath[512];
	class CLuaFile *m_pLuaFile;

public:
	CLuaSqlConn(const char *pFilename, CLuaFile *pLuaFile) : m_Db(pFilename, false), m_pLuaFile(pLuaFile) { str_copyb(m_aPath, pFilename); }
	CLuaSqlConn(const CLuaSqlConn& other) : m_Db(other.m_aPath, false) { str_copyb(m_aPath, other.m_aPath); dbg_assert_strict(false, "shouldn't be running this"); }
	~CLuaSqlConn();
	void Execute(const char *pStatement, LuaRef Callback, lua_State *L);
	unsigned int Work() { return m_Db.Work(); }
	void Flush() { m_Db.Flush(); }
	void Clear() { m_Db.Clear(); }

	const char *GetDatabasePath() const { return m_aPath; }
};

// invisible to lua
class CLuaSqlQuery : public CQuery
{
	luabridge::LuaRef m_CbFunc;

public:
	CLuaSqlQuery(char *pQueryBuf, const LuaRef& CbFunc) : CQuery(pQueryBuf), m_CbFunc(CbFunc){}

private:
	void OnData();
};


// global namespace

class CLuaSql
{
public:
	static CLuaSqlConn *Open(const char *pFilename, lua_State *L);
	static void Flush(CLuaSqlConn& Db) { Db.Flush(); }
	static void Clear(CLuaSqlConn& Db) { Db.Clear(); }
};

#endif
