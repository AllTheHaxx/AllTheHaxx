/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/system++/threading.h>
#include "jobs.h"

CJobPool::CJobPool()
{
	m_pFirstJob = 0;
	m_pLastJob = 0;
	m_Running = true;
}

CJobPool::~CJobPool()
{
	m_Running = false;
	dbg_msg("jobs", "waiting for %i threads to finish...", (int)m_apThreads.size());
	for(void* &t : m_apThreads)
	{
		thread_wait(t);
	}
	m_apThreads.clear();
}

void CJobPool::WorkerThread(void *pUser)
{
	CJobPool *pPool = (CJobPool *)pUser;

	while(pPool->m_Running)
	{
		CJob *pJob = 0;

		// fetch job from queue
		{
			LOCK_SECTION_MUTEX(pPool->m_Lock);
			if(pPool->m_pFirstJob)
			{
				pJob = pPool->m_pFirstJob;
				pPool->m_pFirstJob = pPool->m_pFirstJob->m_pNext;
				if(pPool->m_pFirstJob)
					pPool->m_pFirstJob->m_pPrev = 0;
				else
					pPool->m_pLastJob = 0;
			}
		}

		// do the job if we have one
		if(pPool->m_Running)
		{
			if(pJob)
			{
				pJob->m_Status = CJob::STATE_RUNNING;
				pJob->m_Result = pJob->m_pfnFunc(pJob->m_pFuncData);
				pJob->m_Status = CJob::STATE_DONE;
			}
			else
				thread_sleep(10);
		}
	}

	dbg_msg("jobs", "worker thread %p exited cleanly", thread_get_current());

}

int CJobPool::Init(int NumThreads)
{
	// start threads
	m_Running = true;
	for(int i = 0; i < NumThreads; i++)
	{
		void *pThread = thread_init_named(WorkerThread, this, "jobs");
		dbg_msg("jobs", "started worker thread %p (%i/%i)", pThread, i+1, NumThreads);
		m_apThreads.push_back(pThread);
	}
	return 0;
}

int CJobPool::Add(CJob *pJob, JOBFUNC pfnFunc, void *pData)
{
	mem_zero(pJob, sizeof(CJob));
	pJob->m_pfnFunc = pfnFunc;
	pJob->m_pFuncData = pData;

	LOCK_SECTION_MUTEX(m_Lock);

	// add job to queue
	pJob->m_pPrev = m_pLastJob;
	if(m_pLastJob)
		m_pLastJob->m_pNext = pJob;
	m_pLastJob = pJob;
	if(!m_pFirstJob)
		m_pFirstJob = pJob;

	return 0;
}
