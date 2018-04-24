#ifndef GAME_CLIENT_COMPONENTS_TRANSLATOR_H
#define GAME_CLIENT_COMPONENTS_TRANSLATOR_H

#define WIN32_LEAN_AND_MEAN
#include <queue>
#include <atomic>
#include <mutex>
#include <game/client/component.h>
#include "curl/curl.h"
#include "curl/easy.h"

class CTranslator
{
	MACRO_ALLOC_HEAP()
public:
	CTranslator();
	~CTranslator();

	bool Init();
	//void Shutdown();

	struct CTransEntry
	{
		CTransEntry()
		{
			m_aText[0] = '\0';
			m_aSrcLang[0] = '\0';
			m_aDstLang[0] = '\0';
			m_In = true;
			m_aSaidBy[0] = '\0';
			m_aMentionedName[0] = '\0';
		}

		char m_aText[1024];

		char m_aSrcLang[16];
		char m_aDstLang[16];

		bool m_In;
		char m_aSaidBy[MAX_NAME_LENGTH];
		char m_aMentionedName[MAX_NAME_LENGTH];
	};

	void RequestTranslation(const char *pSrcLang, const char *pDstLang, const char *pText, bool In, const char *pMentionedName, const char *pSaidBy = "");
	inline bool HasTranslation()
	{
		LOCK_SECTION_MUTEX_OPT(m_ThreadMutex, return false)
		return !m_Results.empty();
	}
	inline bool HasQueue()
	{
		LOCK_SECTION_MUTEX_OPT(m_ThreadMutex, return false)
		return !m_Queue.empty();
	}

	CTransEntry NextTranslation()
	{
		LOCK_SECTION_MUTEX(m_ThreadMutex);
		CTransEntry Result;
		if(!dbg_assert_strict(!m_Results.empty(), "tried to get next translation but got no more results - USE 'HasTranslation()' !!!"))
		{
			Result = m_Results.front();
			m_Results.pop();
		}
		return Result;
	}

private:
	void * volatile m_pThread;
	CURL * volatile m_pHandle;

	std::atomic_bool m_ThreadRunning;
	std::mutex m_ThreadMutex;
	static void TranslationWorkerProxy(void *pUser);
	void TranslationWorker();

	std::queue<CTransEntry> m_Queue;
	std::queue<CTransEntry> m_Results;
};

#endif
