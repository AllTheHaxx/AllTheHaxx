#include <base/system.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include "debug.h"
#include "fetcher.h"

CFetchTask::CFetchTask(bool CanTimeout)
{
	m_pNext = NULL;
	m_CanTimeout = CanTimeout;
}

CFetcher::CFetcher()
{
	m_pStorage = NULL;
	m_pHandle = NULL;
	m_pFirst = NULL;
	m_pLast = NULL;
	m_pThread = NULL;
	m_Shutdown = false;
}

CFetcher::~CFetcher()
{
	LOCK_SECTION_MUTEX(m_Mutex);

	m_Shutdown = true;
	if(m_pThread)
	{
		if(m_pFirst)
			dbg_msg("fetcher", "waiting for thread %p to finish...", m_pThread);

		m_Mutex.unlock(); // unlock the mutex so that the thread can finish
		thread_wait(m_pThread);
		m_pThread = NULL;
		m_Mutex.lock(); // re-lock so we don't get undefined behavior when the section ends

		// clear the queue
		while(m_pFirst)
		{
			CFetchTask *pNext = m_pFirst->m_pNext;
			delete m_pFirst;
			m_pFirst = pNext;
		}
	}

	if(m_pHandle)
		curl_easy_cleanup(m_pHandle);
}

bool CFetcher::Init(IStorageTW *pStorage)
{
	CALLSTACK_ADD();

	if(pStorage)
		m_pStorage = pStorage;
	else
		m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	return (m_pHandle = curl_easy_init()) != NULL;
}

CFetchTask* CFetcher::QueueAdd(bool CanTimeout, const char *pUrl, const char *pDest, int StorageType, void *pUser, COMPFUNC pfnCompCb, PROGFUNC pfnProgCb)
{
	CALLSTACK_ADD();

	if(m_Shutdown)
	{
		dbg_msg("fetcher/warning", "rejecting task '%s' due to shutdown!", pUrl);
		return 0;
	}

	CFetchTask *pTask = new CFetchTask(CanTimeout);
	str_copy(pTask->m_aUrl, pUrl, sizeof(pTask->m_aUrl));
	str_copy(pTask->m_aDest, pDest, sizeof(pTask->m_aDest));
	pTask->m_StorageType = StorageType;
	pTask->m_pfnProgressCallback = pfnProgCb;
	pTask->m_pfnCompCallback = pfnCompCb;
	pTask->m_pUser = pUser;
	pTask->m_Size = pTask->m_Progress = 0;
	pTask->m_Abort = false;

	LOCK_SECTION_MUTEX(m_Mutex);
	if(!m_pThread)
		m_pThread = thread_init_named(&FetcherThread, this, "fetcher");

	if(!m_pFirst)
	{
		m_pFirst = pTask;
		m_pLast = m_pFirst;
	}
	else
	{
		m_pLast->m_pNext = pTask;
		m_pLast = pTask;
	}
	pTask->m_State = CFetchTask::STATE_QUEUED;

	return pTask;
}

void CFetcher::Escape(char *pBuf, size_t size, const char *pStr)
{
	CALLSTACK_ADD();

	char *pEsc = curl_easy_escape(0, pStr, 0);
	str_copy(pBuf, pEsc, size);
	curl_free(pEsc);
}

void CFetcher::FetcherThread(void *pUser)
{
	CALLSTACK_ADD();

	CFetcher *pFetcher = (CFetcher *)pUser;

	DEFER([&pFetcher](){
		LOCK_SECTION_MUTEX(pFetcher->m_Mutex)
		dbg_msg("fetcher", "thread %p stopped", pFetcher->m_pThread);
		//pFetcher->m_pThread = NULL;
	})


	{
		LOCK_SECTION_MUTEX(pFetcher->m_Mutex)
		dbg_msg("fetcher", "thread %p started...", pFetcher->m_pThread);
	}

	while(true)
	{
		CFetchTask *pTask;
		// take a task from the beginning of the queue
		{
			LOCK_SECTION_MUTEX(pFetcher->m_Mutex);
			pTask = pFetcher->m_pFirst;
			if(pTask)
				pFetcher->m_pFirst = pTask->m_pNext;
		}

		if(pTask)
		{
			if(!pFetcher->m_Shutdown)
			{
				dbg_msg("fetcher", "task got '%s' -> '%s'", pTask->m_aUrl, pTask->m_aDest);
				pFetcher->FetchFile(pTask);
				if(pTask->m_pfnCompCallback)
					pTask->m_pfnCompCallback(pTask, pTask->m_pUser);
				delete pTask;
			}
			else
				pTask->Abort(); // tasks will be deleted in the CFetcher dtor
		}
		else
		{
			if(pFetcher->m_Shutdown)
				break;
			thread_sleep(10);
		}
	}
}

void CFetcher::FetchFile(CFetchTask *pTask)
{
	CALLSTACK_ADD();

	char aPath[512];
	if(pTask->m_StorageType == -2)
		m_pStorage->GetBinaryPath(pTask->m_aDest, aPath, sizeof(aPath));
	else
		m_pStorage->GetCompletePath(pTask->m_StorageType, pTask->m_aDest, aPath, sizeof(aPath));

	if(fs_makedir_rec_for(aPath) < 0)
		dbg_msg("fetcher", "i/o error, cannot create folder for: '%s'", aPath);

	IOHANDLE File = io_open(aPath, IOFLAG_WRITE);

	if(!File){
		dbg_msg("fetcher", "i/o error, cannot open file: '%s'", pTask->m_aDest);
		pTask->m_State = CFetchTask::STATE_ERROR;
		return;
	}

	char aCAFile[512];
	m_pStorage->GetBinaryPath("data/ca-ddnet.pem", aCAFile, sizeof aCAFile);

	char aErr[CURL_ERROR_SIZE];
	curl_easy_setopt(m_pHandle, CURLOPT_ERRORBUFFER, aErr);

	//curl_easy_setopt(m_pHandle, CURLOPT_VERBOSE, 1L);
	if(pTask->m_CanTimeout)
	{
		curl_easy_setopt(m_pHandle, CURLOPT_CONNECTTIMEOUT_MS, (long)g_Config.m_ClHTTPConnectTimeoutMs);
		curl_easy_setopt(m_pHandle, CURLOPT_LOW_SPEED_LIMIT, (long)g_Config.m_ClHTTPLowSpeedLimit);
		curl_easy_setopt(m_pHandle, CURLOPT_LOW_SPEED_TIME, (long)g_Config.m_ClHTTPLowSpeedTime);
	}
	else
	{
		curl_easy_setopt(m_pHandle, CURLOPT_CONNECTTIMEOUT_MS, 0);
		curl_easy_setopt(m_pHandle, CURLOPT_LOW_SPEED_LIMIT, 0);
		curl_easy_setopt(m_pHandle, CURLOPT_LOW_SPEED_TIME, 0);
	}
	curl_easy_setopt(m_pHandle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_pHandle, CURLOPT_MAXREDIRS, 4L);
	curl_easy_setopt(m_pHandle, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(m_pHandle, CURLOPT_SSL_VERIFYPEER, 0L); // yes i know... but stuff is weird and we are only downloading unimportant shit
	//curl_easy_setopt(m_pHandle, CURLOPT_CAINFO, aCAFile); // suppose this is only needed to access ddnet's server?
	curl_easy_setopt(m_pHandle, CURLOPT_URL, pTask->m_aUrl);
	curl_easy_setopt(m_pHandle, CURLOPT_WRITEDATA, File);
	curl_easy_setopt(m_pHandle, CURLOPT_WRITEFUNCTION, &CFetcher::WriteToFile);
	curl_easy_setopt(m_pHandle, CURLOPT_NOPROGRESS, 0);
	curl_easy_setopt(m_pHandle, CURLOPT_PROGRESSDATA, pTask);
	curl_easy_setopt(m_pHandle, CURLOPT_PROGRESSFUNCTION, &CFetcher::ProgressCallback);
	curl_easy_setopt(m_pHandle, CURLOPT_NOSIGNAL, 1L);

	if(g_Config.m_Debug)
		dbg_msg("fetcher", "downloading to '%s'", pTask->m_aDest);
	pTask->m_State = CFetchTask::STATE_RUNNING;
	int ret = curl_easy_perform(m_pHandle);
	io_close(File);
	if(ret != CURLE_OK)
	{
		dbg_msg("fetcher", "task failed. ('%s' -> '%s') libcurl error: %s", pTask->m_aUrl, pTask->m_aDest, aErr);
		pTask->m_State = (ret == CURLE_ABORTED_BY_CALLBACK) ? CFetchTask::STATE_ABORTED : CFetchTask::STATE_ERROR;
	}
	else
	{
		dbg_msg("fetcher", "task done %s", pTask->m_aDest);
		pTask->m_State = CFetchTask::STATE_DONE;
	}
}

void CFetcher::WriteToFile(char *pData, size_t size, size_t nmemb, void *pFile)
{
	CALLSTACK_ADD();

	io_write((IOHANDLE)pFile, pData, size*nmemb);
}

int CFetcher::ProgressCallback(void *pUser, double DlTotal, double DlCurr, double UlTotal, double UlCurr)
{
	CALLSTACK_ADD();

	CFetchTask *pTask = (CFetchTask *)pUser;
	pTask->m_Current = DlCurr;
	pTask->m_Size = DlTotal;
	pTask->m_Progress = (int)((100 * DlCurr) / (DlTotal ? DlTotal : 1));
	if(pTask->m_pfnProgressCallback)
		pTask->m_pfnProgressCallback(pTask, pTask->m_pUser);
	return pTask->m_Abort ? -1 : 0;
}
