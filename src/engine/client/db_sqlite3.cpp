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

void CSql::WorkerThread()
{
	/*CALL_STACK_ADD();*/

    while(m_Running)
    {
        lock_wait(m_Lock); //lock queue
        if (m_lpQueries.size() > 0)
        {
            CQuery *pQuery = m_lpQueries.front();
            m_lpQueries.pop();
            lock_unlock(m_Lock); //unlock queue

            int Ret;
            Ret = sqlite3_prepare_v2(m_pDB, pQuery->m_Query.c_str(), -1, &pQuery->m_pStatement, 0);
            if (Ret == SQLITE_OK)
            {
                if (!m_Running) //last check
                    break;
                pQuery->OnData();

                sqlite3_finalize(pQuery->m_pStatement);
            }
            else
                dbg_msg("SQLite", "%s", sqlite3_errmsg(m_pDB));

            delete pQuery;
        }
        else
        {
            thread_sleep(100);
            lock_unlock(m_Lock); //unlock queue
        }

        thread_sleep(10);
    }
}

void CSql::InitWorker(void *pUser)
{
	/*CALL_STACK_ADD();*/

    CSql *pSelf = (CSql *)pUser;
    pSelf->WorkerThread();
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

CSql::CSql()
{
	/*CALL_STACK_ADD();*/

    sqlite3 *test;
    int rc = sqlite3_open("data.db", &m_pDB);
    if (rc)
    {
        dbg_msg("SQLite", "Can't open database");
        sqlite3_close(m_pDB);
    }

    m_Lock = lock_create();
    m_Running = true;
    thread_init(InitWorker, this);
}

CSql::~CSql()
{
	/*CALL_STACK_ADD();*/

    m_Running = false;
    lock_wait(m_Lock);
    while (m_lpQueries.size())
    {
        CQuery *pQuery = m_lpQueries.front();
        m_lpQueries.pop();
        delete pQuery;
    }
    lock_unlock(m_Lock);
    lock_destroy(m_Lock);
}
