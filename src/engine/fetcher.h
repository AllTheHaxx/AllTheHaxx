#ifndef ENGINE_FETCHER_H
#define ENGINE_FETCHER_H

#include <base/system++/system++.h>
#include "kernel.h"
#include "stddef.h"

class CFetchTask;

typedef void (*PROGFUNC)(CFetchTask *pTask, void *pUser);
typedef void (*COMPFUNC)(CFetchTask *pDest, void *pUser);

class CFetchTask
{
	MACRO_ALLOC_HEAP()
	friend class CFetcher;

	CFetchTask *m_pNext;

	char m_aUrl[256];
	char m_aDest[256];
	PROGFUNC m_pfnProgressCallback;
	COMPFUNC m_pfnCompCallback;
	void *m_pUser;

	double m_Current;
	double m_Size;
	int m_Progress;
	int m_State;
	volatile bool m_Abort;
	bool m_CanTimeout;
	int m_StorageType;

	CFetchTask(bool canTimeout);
public:
	~CFetchTask()
	{
		#if defined(CONF_DEBUG)
		dbg_assert_legacy(m_State != STATE_RUNNING && m_State != STATE_QUEUED, "deleted an active fetch task!");
		#endif
	}

	enum
	{
		STATE_ERROR = -1,
		STATE_QUEUED,
		STATE_RUNNING,
		STATE_DONE,
		STATE_ABORTED,
	};

	const double Current() const { return m_Current; }
	const double Size() const { return m_Size; }
	const int Progress() const { return m_Progress; }
	const int State() const { return m_State; }
	const char *Dest() const { return m_aDest; }

	void Abort() { m_Abort = true; };
};

class IFetcher : public IInterface
{
	MACRO_INTERFACE("fetcher", 0)
public:
	virtual bool Init(class IStorageTW *pStorage = 0, class IEngine *pEngine = 0) = 0;
	virtual CFetchTask* QueueAdd(bool CanTimeout, const char *pUrl, const char *pDest, int StorageType = -2, void *pUser = 0, COMPFUNC pfnCompCb = 0, PROGFUNC pfnProgCb = 0) = 0;
	virtual void Escape(char *pBud, size_t size, const char *pStr) = 0;
};

#endif
