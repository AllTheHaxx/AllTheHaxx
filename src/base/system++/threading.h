#ifndef BASE_SYSTEMPP_THREADING_H
#define BASE_SYSTEMPP_THREADING_H

#include <base/system.h>
#include "system++.h"

template <class T = void>
class THREAD_SMART
{
	typedef void (*pFnTypedThreadFunc)(T *);
	typedef void (*pFnThreadFunc)(void *);

	const pFnTypedThreadFunc m_ThreadFunc;
	void *m_pThreadHandle;
	bool m_Detached;

public:
	THREAD_SMART(pFnTypedThreadFunc ThreadFunc) : m_ThreadFunc(ThreadFunc)
	{
		m_pThreadHandle = NULL;
		m_Detached = false;
	}

	~THREAD_SMART()
	{
		if(IsJoinable())
			Join();
	}

	/**
	 * Start the thread
	 * @param pUser the userdata to pass to the thread function
	 * @param pName (optional) name of the thread (for debugging)
	 * @return A bool indicating success
	 * @note IMPORTANT: When the THREAD_SMART object goes out of scope, a Joinable thread will be joined automatically!
	 */
	bool Start(T *pUser, const char *pName = 0)
	{
		if(IsRunning()) return false;
		m_pThreadHandle = thread_init_named((pFnThreadFunc)m_ThreadFunc, pUser, pName);
		return m_pThreadHandle != NULL;
	}

	/**
	 * Starts and automatically detaches the thread
	 * @param pUser userdata, see Start
	 * @return see Start && Detach
	 */
	bool StartDetached(T *pUser)
	{
		if(IsRunning()) return false;
		if(Start(pUser))
			return Detach();
		return false;
	}

	/**
	 * Makes the calling thread wait until this thread has finished
	 * @return false if the thread is not running or is detached, otherwise true
	 * @note This function does not return until the thread has finished!
	 */
	bool Join() const
	{
		if(!IsJoinable()) return false;
		thread_wait(m_pThreadHandle);
		return true;
	}

	/**
	 * Detach (aka daemonize) the thread so that you don't need to Join it
	 * @return A bool indicating success
	 */
	bool Detach()
	{
		if(!m_pThreadHandle) return false;
		int result = thread_detach(m_pThreadHandle);
		m_pThreadHandle = NULL; // invalidate the handle since further operations on it would cause undefined behavior
		m_Detached = true;
		if(result == 0)
		{
		}
/*#if defined(CONF_DEBUG)
		else
		{
			char aErr[1024];
			str_formatb(aErr, "failed to detach thread %p: error %i (%s)", m_pThreadHandle, result, strerror(result));
			dbg_assert(result == 0, aErr);
		}
#endif*/
		return m_Detached;
	}

	/**
	 * Check if the thread has been started using Start
	 * @return A bool indicating whether the thread is running
	 */
	bool IsRunning() const
	{
		return m_pThreadHandle != NULL || m_Detached;
	}

	/**
	 * Check if the thread has been detached using Detach
	 * @return A bool indicating whether the thread is detached
	 * @note Once detached, the thread can't be joined anymore!
	 */
	bool IsDetached() const
	{
		return m_Detached;
	}

	/**
	 * Check if the thread can be joined using the Join function
	 * @return IsRunning() && !IsDetached()
	 */
	bool IsJoinable() const
	{
		return IsRunning() && !IsDetached();
	}
};


//#define LOCK_SECTION_DBG(LOCKVAR) LOCK_SECTION_SMART __SectionLock(&(LOCKVAR), false, __FILE__, __LINE__); __SectionLock.WaitAndLock()
//#define UNLOCK_SECTION() __SectionLock.Unlock()
//#define LOCK_SECTION_LAZY_DBG(LOCKVAR) LOCK_SECTION_SMART __SectionLock(&(LOCKVAR), false, __FILE__, __LINE__)
#define LOCK_SECTION_MUTEX(LOCKVAR) std::lock_guard<std::mutex> __SectionMutex(LOCKVAR);
#define LOCK_SECTION_MUTEX_OPT(LOCKVAR, EXECUTE_ON_FAIL) std::unique_lock<std::mutex> __SectionMutex(LOCKVAR, std::defer_lock); if(!__SectionMutex.try_lock()) { EXECUTE_ON_FAIL; }
#define LOCK_SECTION_RECURSIVE_MUTEX(LOCKVAR) std::lock_guard<std::recursive_mutex> __SectionMutex(LOCKVAR);
#define LOCK_SECTION_RECURSIVE_MUTEX_OPT(LOCKVAR, EXECUTE_ON_FAIL) std::unique_lock<std::recursive_mutex> __SectionMutex(LOCKVAR, std::defer_lock); if(!__SectionMutex.try_lock()) { EXECUTE_ON_FAIL; }


/**
 * @deprecated use c++11 mutexes instead!
 */
class LOCK_SMART
{
	LOCK m_Lock;

	const char *m_pFile;
	int m_Line;

	const char *m_pFilePrev;
	int m_LinePrev;

public:
	LOCK_SMART()
	{
		mem_zero(this, sizeof(LOCK_SMART));

		m_Lock = lock_create();
	}

	~LOCK_SMART()
	{
		lock_wait(m_Lock);
		lock_unlock(m_Lock);
		lock_destroy(m_Lock);
	}

	LOCK Get(const char *pFile, int Line)
	{
		m_pFilePrev = m_pFile;
		m_LinePrev = m_Line;

		m_pFile = pFile;
		m_Line = Line;

		return m_Lock;
	}
};

class LOCK_SECTION_SMART
{
	LOCK_SMART *m_pLockWrapper;
	bool m_IsLocked;

	const char * const m_pFile;
	const int m_Line;

public:
	LOCK_SECTION_SMART(LOCK_SMART *pLockWrapper, bool IsLocked = false, const char *pFile = 0, int Line = -1)
			: m_pLockWrapper(pLockWrapper), m_IsLocked(IsLocked),
			  m_pFile(pFile), m_Line(Line)
	{
	}

	~LOCK_SECTION_SMART()
	{
		if(m_IsLocked)
			lock_unlock(m_pLockWrapper->Get(m_pFile, m_Line));
//		dbg_msg("lock/dbg", "%s:%i released lock %p", m_pFile, m_Line, m_Lock);
	}

	void WaitAndLock()
	{
#if defined(CONF_DEBUG)
		dbg_assert(!m_IsLocked, "Tried to lock the locked lock");
//		dbg_msg("lock/dbg", "%s:%i grabbed lock %p", m_pFile, m_Line, m_Lock);
#else
		if(m_IsLocked)
			return;
#endif
		lock_wait(m_pLockWrapper->Get(m_pFile, m_Line));
		m_IsLocked = true;
	}

	bool TryToLock()
	{
		if(m_IsLocked)
			return false;
//		dbg_msg("lock/dbg", "%s:%i grabbed lock %p", m_pFile, m_Line, m_Lock);
		m_IsLocked = lock_trylock(m_pLockWrapper->Get(m_pFile, m_Line)) == 0;
		return m_IsLocked;
	}

	void Unlock()
	{
#if defined(CONF_DEBUG)
		dbg_assert(m_IsLocked, "Tried to unlock the unlocked lock");
//		dbg_msg("lock/dbg", "%s:%i released lock %p", m_pFile, m_Line, m_Lock);
#else
		if(!m_IsLocked)
			return;
#endif
		m_IsLocked = false;
		lock_unlock(m_pLockWrapper->Get(m_pFile, m_Line));
	}
};

#endif
