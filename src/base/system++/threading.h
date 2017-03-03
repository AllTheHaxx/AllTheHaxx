#ifndef BASE_SYSTEMPP_THREADING_H
#define BASE_SYSTEMPP_THREADING_H

#include <base/system.h>

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

#define RETURN_ON_NOT_HANDLED(RET) if(!m_pThreadHandle) return RET;

	/**
	 * Start the thread
	 * @param pUser the userdata to pass to the thread function
	 * @return A bool indicating success
	 */
	bool Start(T *pUser)
	{
		if(IsRunning()) return false;
		m_pThreadHandle = thread_init((pFnThreadFunc)m_ThreadFunc, pUser);
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
	bool Join()
	{
		RETURN_ON_NOT_HANDLED(false)
		thread_wait(m_pThreadHandle);
		return true;
	}

	/**
	 * Detach (aka daemonize) the thread so that you don't need to Join it
	 * @return A bool indicating success
	 */
	bool Detach() const
	{
		RETURN_ON_NOT_HANDLED(false)
		return thread_detach(m_pThreadHandle) == 0;
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
};

#endif
