
#include <base/system++/threading.h>
#include "db_sqlite3.h"


CQuery::~CQuery()
{
	if(m_pQueryStr)
		sqlite3_free(m_pQueryStr);
}

bool CQuery::Next()
{
	int Ret = sqlite3_step(m_pStatement);
	return Ret == SQLITE_ROW;
}

void CQuery::OnData()
{
	Next();
}

int CQuery::GetID(const char *pName)
{
	for (int i = 0; i < GetColumnCount(); i++)
	{
		if (str_comp(GetName(i), pName) == 0)
			return i;
	}
	return -1;
}


CSql::CSql(const char *pFilename)
{
	char aFilePath[768], aFullPath[1024];
	fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
	str_format(aFullPath, sizeof(aFullPath), "%s/%s", aFilePath, pFilename);
	if(str_comp_nocase_num(aFullPath + str_length(aFullPath)-3, ".db", 3) != 0)
		str_append(aFullPath, ".db", sizeof(aFullPath));

	int rc = sqlite3_open(aFullPath, &m_pDB);
	if (rc)
	{
		dbg_msg("SQLite", "Can't open database");
		sqlite3_close(m_pDB);
	}

	m_Lock = lock_create();
	m_Running = true;
	m_pThread = thread_init_named(InitWorker, this, "sqlite");
}

CSql::~CSql()
{
	m_Running = false;
	if(m_pThread)
	{
		lock_wait(m_Lock);
		if(m_lpQueries.size())
			dbg_msg("sqlite", "[%s] waiting for worker thread to finish, %lu queries left", sqlite3_db_filename(m_pDB, "main"), (unsigned long)m_lpQueries.size());
		lock_unlock(m_Lock);
		thread_wait(m_pThread);
	}
	lock_destroy(m_Lock);
}

void CSql::InsertQuery(CQuery *pQuery)
{
	lock_wait(m_Lock);
	m_lpQueries.push(pQuery);
	lock_unlock(m_Lock);
}

void CSql::InitWorker(void *pUser)
{
	CSql *pSelf = (CSql *)pUser;
	pSelf->WorkerThread();
}

void CSql::WorkerThread()
{
	while(1)
	{
		if(m_Running);
			thread_sleep(500);

		LOCK_SECTION_DBG(m_Lock);
		if (m_lpQueries.size() > 0)
		{
			// do 250 queries per transaction - cache them so we can release the lock as early as possible
			enum { QUERIES_PER_TRANSACTION = 250 };
			CQuery *apQueries[QUERIES_PER_TRANSACTION];
			int NumQueries = 0;
			for(int i = 0; i < QUERIES_PER_TRANSACTION && m_lpQueries.size() > 0; i++)
			{
				apQueries[NumQueries++] = m_lpQueries.front();
				m_lpQueries.pop();
			}
			__SectionLock.Unlock();

			// begin transaction
			{
				char *pQueryStr = sqlite3_mprintf("BEGIN");
				CQuery Begin(pQueryStr);
				ExecuteQuery(&Begin);
			}

			// perform queries
			for(int i = 0; i < NumQueries; i++)
			{
				CQuery *pQuery = apQueries[i];
				ExecuteQuery(pQuery);
				delete pQuery;
			}

			// end transaction
			{
				char *pQueryStr = sqlite3_mprintf("END");
				CQuery End(pQueryStr);
				ExecuteQuery(&End);
			}
		}
		else if(!m_Running)
			return;
	}
}

void CSql::ExecuteQuery(CQuery *pQuery)
{
	int Ret = sqlite3_prepare_v2(m_pDB, pQuery->m_pQueryStr, -1, &pQuery->m_pStatement, 0);
	if (Ret == SQLITE_OK)
	{
		pQuery->OnData();

		sqlite3_finalize(pQuery->m_pStatement);
	}
	else
		dbg_msg("SQLite/error", "%s", sqlite3_errmsg(m_pDB));
}
