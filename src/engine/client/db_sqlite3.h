/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef ENGINE_CLIENT_DB_SQLITE3_H
#define ENGINE_CLIENT_DB_SQLITE3_H

#include <base/system.h>
#include <engine/external/sqlite3/sqlite3.h>
#include <vector>
#include <queue>
#include <string>
#include <engine/server.h>

class CQuery
{
	MACRO_ALLOC_HEAP()
	friend class CSql;

private:
	std::string m_Query;
	sqlite3_stmt *m_pStatement;
	virtual void OnData();

public:
	virtual ~CQuery() {}

	bool Next();
	int GetColumnCount() { return sqlite3_column_count(m_pStatement); }
	const char *GetName(int i) { return sqlite3_column_name(m_pStatement, i); }
	int GetType(int i) { return sqlite3_column_type(m_pStatement, i); }

	int GetID(const char *pName);
	int GetInt(int i) { return sqlite3_column_int(m_pStatement, i); }
	float GetFloat(int i) { return (float)sqlite3_column_double(m_pStatement, i); }
	const char *GetText(int i) { return (const char *)sqlite3_column_text(m_pStatement, i); }
	const void *GetBlob(int i) { return sqlite3_column_blob(m_pStatement, i); }
	int GetSize(int i) { return sqlite3_column_bytes(m_pStatement, i); }

	class CSql *m_pDatabase;
	void Query(class CSql *pDatabase, char *pQuery);
};

class CSql
{
	MACRO_ALLOC_HEAP()
private:
	void WorkerThread();
	static void InitWorker(void *pSelf);
	LOCK m_Lock;

	std::queue<CQuery *>m_lpQueries;

	bool m_Running;

	sqlite3 *m_pDB;

public:
	CSql(const char *pFilename = "ath_data.db");
	~CSql();
	CQuery *Query(CQuery *pQuery, std::string QueryString);

};



#endif
