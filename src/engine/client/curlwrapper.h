#ifndef ENGINE_CLIENT_CURLWRAPPER_H
#define ENGINE_CLIENT_CURLWRAPPER_H

#define WIN32_LEAN_AND_MEAN
#include "curl/curl.h"
#include "curl/easy.h"
#include <engine/curlwrapper.h>

class CCurlWrapper : public ICurlWrapper
{
	struct CHTTPPOSTTask
	{
		char m_aUrl[256];
		char m_aFields[128];

		CHTTPPOSTTask(const char *pUrl, const char *pFields)
		{
			str_copy(m_aUrl, pUrl, sizeof(m_aUrl));
			str_copy(m_aFields, pFields, sizeof(m_aFields));
		}
	};


public:
	virtual void PerformSimplePOST(const char *pUrl, const char *pFields);


private:
	static void PerformPOST_ex(void *pUser);
};

#endif
