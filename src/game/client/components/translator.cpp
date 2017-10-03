#include <string>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/external/json-parser/json.hpp>
#include <engine/client/curlwrapper.h>

#include "translator.h"


CTranslator::CTranslator()
{
	m_pHandle = NULL;
	m_pThread = NULL;
}

CTranslator::~CTranslator()
{
	// clean up
	if(m_pHandle)
		curl_easy_cleanup(m_pHandle);
	m_pHandle = NULL;

	thread_destroy(m_pThread);
}

bool CTranslator::Init()
{
	if((m_pHandle = curl_easy_init()))
	{
		m_pThread = thread_init_named(TranslationWorker, this, "transl. worker");
		return true;
	}
	return false;
}

void CTranslator::TranslationWorker(void *pUser)
{
	CTranslator *pTrans = (CTranslator *)pUser;

	while(pTrans->m_pHandle != NULL)
	{
		CALLSTACK_ADD();

		LOCK_SECTION_DBG(pTrans->m_Lock);
		if(!pTrans->m_Queue.empty())
		{
			CTransEntry Entry = pTrans->m_Queue.front();
			pTrans->m_Queue.pop();
			UNLOCK_SECTION();

			char aPost[16*1024];
			char aTranslated[8*1024];
			std::string Response;

			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_URL, "http://api.mymemory.translated.net/get");
			char *pEscapedTest = curl_easy_escape(pTrans->m_pHandle, Entry.m_aText, 0);
			str_format(aPost, sizeof(aPost), "q=%s&langpair=%s|%s&de=associatingblog@gmail.com", pEscapedTest, Entry.m_aSrcLang, Entry.m_aDstLang);
			curl_free(pEscapedTest);
			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_POSTFIELDS, aPost);
			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_WRITEFUNCTION, &CCurlWrapper::CurlCallback_WriteToStdString);
			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_WRITEDATA, &Response);
			curl_easy_perform(pTrans->m_pHandle);

			if(pTrans->m_pHandle == NULL)
				return;

			// parse response
			json_value &jsonValue = *json_parse(Response.c_str(), Response.length());
			const char *pResult = jsonValue["responseData"]["translatedText"];
			if(str_length(pResult) == 0)
			{
				dbg_msg("trans/warn", "failed to parse response\n%s", Response.c_str());
				continue;
			}
			str_copy(aTranslated, pResult, sizeof(aTranslated));
			if(str_comp_nocase(Entry.m_aText, aTranslated) != 0)
			{
				if(g_Config.m_Debug)
					dbg_msg("trans", "translated '%s' from '%s' to '%s', result: '%s'", Entry.m_aText, Entry.m_aSrcLang, Entry.m_aDstLang, aTranslated);

				// put the result to the queue
				str_copy(Entry.m_aText, aTranslated, sizeof(Entry.m_aText));
				LOCK_SECTION_DBG(pTrans->m_Lock);
				pTrans->m_Results.push(Entry);
				UNLOCK_SECTION();
			}
			else
				dbg_msg("trans/warn", "translating '%s' from '%s' to '%s' failed", Entry.m_aText, Entry.m_aSrcLang, Entry.m_aDstLang);
		}
		else
			UNLOCK_SECTION();

		thread_sleep(25);

	}
}

void CTranslator::RequestTranslation(const char *pSrcLang, const char *pDstLang, const char *pText, bool In, const char *pMentionedName, const char *pSaidBy)
{
	CALLSTACK_ADD();

	if(!str_utf8_check(pText))
	{
		dbg_msg("trans", "invalid UTF-8 string '%s'", pText);
		return;
	}

	// prepare the entry
	CTransEntry Entry;
	str_copyb(Entry.m_aText, pText);
	str_copyb(Entry.m_aSrcLang, pSrcLang);
	str_copyb(Entry.m_aDstLang, pDstLang);
	Entry.m_In = In;
	str_copyb(Entry.m_aSaidBy, pSaidBy);
	str_copyb(Entry.m_aMentionedName, pMentionedName);

	// insert the entry
	LOCK_SECTION_DBG(m_Lock);
	m_Queue.push(Entry);
}
