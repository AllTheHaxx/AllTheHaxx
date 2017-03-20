#include <string>
#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/external/json-parser/json.h>

#include "translator.h"

size_t CTranslator::write_to_string(void *ptr, size_t size, size_t count, void *stream)
{
	((std::string*)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

CTranslator::CTranslator()
{
	m_pHandle = NULL;
}

CTranslator::~CTranslator()
{
	// clean up
	if(m_pHandle)
		curl_easy_cleanup(m_pHandle);
	m_pHandle = NULL;
	m_Queue.clear();

	thread_wait(m_pThread);
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

		if(pTrans->m_Queue.size())
		{
			CTransEntry Entry = pTrans->m_Queue.front();

			char aPost[2048*8];
			char aResponse[2048*8];
			char aTranslated[1024*8];
			std::string response;

			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_URL, "http://api.mymemory.translated.net/get");
			str_format(aPost, sizeof(aPost), "q=%s&langpair=%s|%s&de=associatingblog@gmail.com", Entry.m_Text, Entry.m_SrcLang, Entry.m_DstLang);
			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_POSTFIELDS, aPost);

			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_WRITEFUNCTION, &CTranslator::write_to_string);
			curl_easy_setopt(pTrans->m_pHandle, CURLOPT_WRITEDATA, &response);
			curl_easy_perform(pTrans->m_pHandle);

			str_copy(aResponse, response.c_str(), sizeof(aResponse));

			// parse response
			json_value *pValue = json_parse(aResponse);
			const char *pResult = json_string_get(json_object_get(json_object_get(pValue,"responseData"),"translatedText"));
			if(!pResult)
			{
				dbg_msg("trans", "got not text");
				return;
			}
			str_copy(aTranslated, pResult, sizeof(aTranslated));
			if(str_comp_nocase(Entry.m_Text, aTranslated) != 0)
			{
				dbg_msg("trans", "translated '%s' from '%s' to '%s', result: '%s'", Entry.m_Text, Entry.m_SrcLang, Entry.m_DstLang, aTranslated);

				// put the result to the queue
				str_copy(Entry.m_Text, aTranslated, sizeof(Entry.m_Text));
				pTrans->m_Results.push_back(Entry);
			}
			else
				dbg_msg("trans", "translating '%s' from '%s' to '%s' failed", Entry.m_Text, Entry.m_SrcLang, Entry.m_DstLang);

			// done, remove the element from our queue
			pTrans->m_Queue.erase(pTrans->m_Queue.begin());
		}

		thread_sleep(50);
	}
}

void CTranslator::RequestTranslation(const char *pSrcLang, const char *pDstLang, const char *pText, bool In)
{
	CALLSTACK_ADD();

	// prepare the entry
	CTransEntry Entry;
	str_copy(Entry.m_Text, pText, sizeof(Entry.m_Text));
	str_copy(Entry.m_SrcLang, pSrcLang, sizeof(Entry.m_SrcLang));
	str_copy(Entry.m_DstLang, pDstLang, sizeof(Entry.m_DstLang));
	Entry.m_In = In;

	// insert the entry
	m_Queue.push_back(Entry);
}
