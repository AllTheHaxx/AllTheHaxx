#ifndef ENGINE_CLIENT_DEBUG_H
#define ENGINE_CLIENT_DEBUG_H

#include <string>
#include <list>

#include <csignal>
#include <engine/storage.h>

extern void *main_thread_handle;
inline bool is_thread()
{
	return thread_get_current() != main_thread_handle;
}

class CCallstack
{
	friend class CDebugger;
	std::list <std::string> m_CallStack;

public:
	void CallstackAdd(const char *file, int line, const char *function);

};

void signalhandler(int sig);

//CCallstack gDebuggingInfo;
extern CCallstack gDebugInfo;

class CDebugger
{
	//void (*signal(int sig, void (*func)(int)))(int);
	//static void signalhandler(int sig);

public:
	CDebugger();
	static void signalhandler_ex(int sig);

	static IStorageTW *m_pStorage;

	static void SetStaticData(IStorageTW *pStorage)
	{
		m_pStorage = pStorage;
	}

public:

private:
	void RegisterSignals();

};

#define CALLSTACK_ADD() if(!is_thread()) gDebugInfo.CallstackAdd(__FILE__, __LINE__, __FUNCTION__)

#endif
