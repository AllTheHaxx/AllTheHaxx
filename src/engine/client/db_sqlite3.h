/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#ifndef ENGINE_CLIENT_DB_SQLITE3_H
#define ENGINE_CLIENT_DB_SQLITE3_H

#include <vector>
#include <queue>
#include <string>
#include <base/system.h>
#include <base/system++/threading.h>
#include <engine/external/sqlite3/sqlite3.h>
#include <engine/server.h>
#include <mutex>
#include <atomic>

class CQuery
{
	MACRO_ALLOC_HEAP()
	friend class CSql;
private:
	char *m_pQueryStr;

	sqlite3_stmt *m_pStatement;

protected:
	virtual void OnData();
	bool Next();
	const char *GetQueryString() const { return (const char *)m_pQueryStr; }

public:
	CQuery(){};
	CQuery(char *pQueryBuf) : m_pQueryStr(pQueryBuf) {}
	virtual ~CQuery();

	int GetColumnCount() { return sqlite3_column_count(m_pStatement); }
	const char *GetName(int i) { return sqlite3_column_name(m_pStatement, i); }
	int GetType(int i) { return sqlite3_column_type(m_pStatement, i); }

	int GetID(const char *pName);
	int GetInt(int i) { return sqlite3_column_int(m_pStatement, i); }
	float GetFloat(int i) { return (float)sqlite3_column_double(m_pStatement, i); }
	const char *GetText(int i) { return (const char *)sqlite3_column_text(m_pStatement, i); }
	const void *GetBlob(int i) { return sqlite3_column_blob(m_pStatement, i); }
	int GetSize(int i) { return sqlite3_column_bytes(m_pStatement, i); }

	int GetIntN(const char *pName) { return GetInt(GetID(pName)); }
	float GetFloatN(const char *pName) { return GetFloat(GetID(pName)); }
	const char *GetTextN(const char *pName) { return GetText(GetID(pName)); }
	const void *GetBlobN(const char *pName) { return GetBlob(GetID(pName)); }
	int GetSizeN(const char *pName) { return GetSize(GetID(pName)); }
};

class CSql
{
	MACRO_ALLOC_HEAP()
private:
	sqlite3 *m_pDB;
	std::mutex m_Mutex;
	std::atomic_bool m_Running;
	void * volatile m_pThread;
	std::queue<CQuery *> m_lpQueries;

public:
	CSql(const char *pFilename = "ath_data.db");
	~CSql();

	/**
	 * Inserts a new query into the threaded execution queue
	 * @param pQuery pointer to the query object
	 */
	void InsertQuery(CQuery *pQuery);

	/**
	 * Synchronously flushes the query queue (i.e. circumvents the thread!)
	 * This forces immediate execution of all remaining queries and waits for their completion.
	 */
	void Flush();

	/**
	 * Discards all left queries
	 */
	void Clear();

	inline const char *GetDatabasePath() const;

private:
	void ExecuteQuery(CQuery *pQuery);
	void WorkerThread();
	unsigned int Work();
	static void InitWorker(void *pSelf);
};



#endif
