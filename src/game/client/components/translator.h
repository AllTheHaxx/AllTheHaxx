#ifndef GAME_CLIENT_COMPONENTS_TRANSLATOR_H
#define GAME_CLIENT_COMPONENTS_TRANSLATOR_H

#define WIN32_LEAN_AND_MEAN
#include <vector>
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
		char m_Text[1024];

		char m_SrcLang[16];
		char m_DstLang[16];

		bool m_In;
	};

	void RequestTranslation(const char *pSrcLang, const char *pDstLang, const char *pText, bool In);
	inline CTransEntry *GetTranslation()
	{
		if(!m_Results.size())
			return NULL;
		return &m_Results.front();
	}
	inline void RemoveTranslation()
	{
		if(!m_Results.size())
			return;
		m_Results.erase(m_Results.begin());
	}
private:
	void *m_pThread;
	CURL *m_pHandle;

	static void TranslationWorker(void *pUser);

	std::vector<CTransEntry> m_Queue;
	std::vector<CTransEntry> m_Results;
};

#endif
