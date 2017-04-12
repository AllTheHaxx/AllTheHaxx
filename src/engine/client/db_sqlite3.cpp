#include "db_sqlite3.h"

bool CQuery::Next()
{
	/*CALL_STACK_ADD();*/

	int Ret = sqlite3_step(m_pStatement);
	return Ret == SQLITE_ROW;
}
void CQuery::Query(CSql *pDatabase, char *pQuery)
{
	/*CALL_STACK_ADD();*/

	m_pDatabase = pDatabase;
	m_pDatabase->Query(this, pQuery);
}
void CQuery::OnData()
{
	/*CALL_STACK_ADD();*/

	Next();
}
int CQuery::GetID(const char *pName)
{
	/*CALL_STACK_ADD();*/

	for (int i = 0; i < GetColumnCount(); i++)
	{
		if (str_comp(GetName(i), pName) == 0)
			return i;
	}
	return -1;
}


CSql::CSql(const char *pFilename)
{
	/*CALL_STACK_ADD();*/

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
	/*CALL_STACK_ADD();*/

	m_Running = false;
	if(m_pThread)
	{
		dbg_msg("sqlite", "[%s] waiting for worker thread to finish...", sqlite3_db_filename(m_pDB, "main"));
		thread_wait(m_pThread);
	}
	lock_destroy(m_Lock);
}

CQuery *CSql::Query(CQuery *pQuery, std::string QueryString)
{
	/*CALL_STACK_ADD();*/

	pQuery->m_Query = QueryString;


	lock_wait(m_Lock);
	m_lpQueries.push(pQuery);
	lock_unlock(m_Lock);

	return pQuery;
}

void CSql::InitWorker(void *pUser)
{
	/*CALL_STACK_ADD();*/

	CSql *pSelf = (CSql *)pUser;
	pSelf->WorkerThread();
}

void CSql::WorkerThread()
{
	/*CALL_STACK_ADD();*/

	while(1)
	{
		lock_wait(m_Lock); // lock queue
		if (m_lpQueries.size() > 0)
		{
			CQuery *pQuery = m_lpQueries.front();
			m_lpQueries.pop();
			lock_unlock(m_Lock); // unlock queue

			int Ret;
			Ret = sqlite3_prepare_v2(m_pDB, pQuery->m_Query.c_str(), -1, &pQuery->m_pStatement, 0);
			if (Ret == SQLITE_OK)
			{
				pQuery->OnData();

				sqlite3_finalize(pQuery->m_pStatement);
			}
			else
				dbg_msg("SQLite", "%s", sqlite3_errmsg(m_pDB));

			delete pQuery;
		}
		else
		{
			lock_unlock(m_Lock); //unlock queue
			if(!m_Running)
				return;
			thread_sleep(100);
		}

		if(m_Running)
			thread_sleep(10);
	}
}
