
#include <base/system++/threading.h>
#include <base/system.h>
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


CSql::CSql(const char *pFilename, bool Threaded)
{
	char aFilePath[768], aFullPath[1024];
	fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
	str_format(aFullPath, sizeof(aFullPath), "%s/%s", aFilePath, pFilename);
	if(str_comp_nocase_num(aFullPath + str_length(aFullPath)-3, ".db", 3) != 0)
		str_append(aFullPath, ".db", sizeof(aFullPath));

	fs_makedir_rec_for(aFullPath);

	int rc = sqlite3_open(aFullPath, &m_pDB);
	if (rc)
	{
		dbg_msg("SQLite", "Can't open database '%s' (%s)", pFilename, aFullPath);
		sqlite3_close(m_pDB);
	}

	m_Running = true;
	if(Threaded)
		m_pThread = thread_init_named(InitWorker, this, "sqlite");
	else
		m_pThread = NULL;
}

CSql::~CSql()
{
	m_Running = false;
	if(m_pThread)
	{
		{
			LOCK_SECTION_MUTEX(m_Mutex);
			if(!m_lpQueries.empty())
				dbg_msg("sqlite", "[%s] waiting for worker thread to finish, %lu queries left", GetDatabasePath(), (unsigned long)m_lpQueries.size());
		}
		thread_wait(m_pThread);
		m_pThread = NULL;
	}
	else
		Flush();
}

void CSql::InsertQuery(CQuery *pQuery)
{
	LOCK_SECTION_MUTEX(m_Mutex);
	m_lpQueries.push(pQuery);
}

void CSql::InsertQuerySync(CQuery *pQuery)
{
	Flush();
	LOCK_SECTION_MUTEX(m_Mutex);
	ExecuteQuery(pQuery);
	delete pQuery;
}

void CSql::InitWorker(void *pUser)
{
	CSql *pSelf = (CSql *)pUser;
	pSelf->WorkerThread();
}

void CSql::WorkerThread()
{
	while(true)
	{
		if(m_Running)
			thread_sleep(250);

		unsigned int QueriesLeft = Work();

		if(QueriesLeft == 0 && !m_Running)
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
	{
		dbg_msg("SQLite/error", "@@ %s", pQuery->m_pQueryStr);
		dbg_msg("SQLite/error", "%s", sqlite3_errmsg(m_pDB));
	}
}

unsigned int CSql::Work()
{
	m_Mutex.lock();
	if (!m_lpQueries.empty())
	{
		// do 250 queries per transaction - cache them so we can release the lock as early as possible
		enum { QUERIES_PER_TRANSACTION = 250 };
		CQuery *apQueries[QUERIES_PER_TRANSACTION];
		int NumQueries = 0;
		for(int i = 0; i < QUERIES_PER_TRANSACTION && !m_lpQueries.empty(); i++)
		{
			apQueries[NumQueries++] = m_lpQueries.front();
			m_lpQueries.pop();
		}
		m_Mutex.unlock();

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
	else
		m_Mutex.unlock();

	LOCK_SECTION_MUTEX(m_Mutex);
	unsigned int NewSize = (unsigned int)m_lpQueries.size();
	return NewSize;
}

void CSql::Flush()
{
	while(Work());;
}

void CSql::Clear()
{
	LOCK_SECTION_MUTEX(m_Mutex);
	while(!m_lpQueries.empty())
	{
		delete m_lpQueries.front();
		m_lpQueries.pop();
	}
}

const char *CSql::GetDatabasePath() const
{
	return sqlite3_db_filename(m_pDB, "main");
}
