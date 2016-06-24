#ifndef GAME_CLIENT_COMPONENTS_TRANSLATOR_H
#define GAME_CLIENT_COMPONENTS_TRANSLATOR_H

#define WIN32_LEAN_AND_MEAN
#include <vector>
#include <game/client/component.h>
#include "curl/curl.h"
#include "curl/easy.h"

class CTranslator : public CComponent
{
public:
	CTranslator();
	virtual bool Init();
	~CTranslator();

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
		if(!Results.size())
			return NULL;
		return &Results.front();
	}
	inline void RemoveTranslation()
	{
		if(!Results.size())
			return;
		Results.erase(Results.begin());
	}
private:
	CURL *m_pHandle;
	static size_t write_to_string(void *ptr, size_t size, size_t count, void *stream);

	static void TranslationWorker(void *pUser);

	std::vector<CTransEntry> Queue;
	std::vector<CTransEntry> Results;
};

#endif
