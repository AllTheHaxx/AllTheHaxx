#include <string>
#include "curlwrapper.h"


void CCurlWrapper::PerformSimplePOST(const char *pUrl, const char *pFields)
{
	thread_detach(thread_init(CCurlWrapper::PerformPOST_ex, new CHTTPPOSTTask(pUrl, pFields)));
}

void CCurlWrapper::PerformPOST_ex(void *pUser)
{
	CHTTPPOSTTask *pTask = (CHTTPPOSTTask *)pUser;
	CURL *pHandle = curl_easy_init();

	if(!pHandle)
	{
		dbg_msg("curlwrapper", "failed to perform POST '%s' with fields '%s': invalid curl_easy handle", pTask->m_aUrl, pTask->m_aFields);
		delete pTask;
		return;
	}

	curl_easy_setopt(pHandle, CURLOPT_URL, pTask->m_aUrl);
	curl_easy_setopt(pHandle, CURLOPT_POSTFIELDS, pTask->m_aFields);
	dbg_msg("curlwrapper/POST", "performing '%s' with fields '%s'", pTask->m_aUrl, pTask->m_aFields);

	CURLcode res = curl_easy_perform(pHandle);
	if(res != CURLE_OK)
		dbg_msg("curlwrapper/POST", "'%s' with fields '%s' failed: %s", pTask->m_aUrl, pTask->m_aFields, curl_easy_strerror(res));

	curl_easy_cleanup(pHandle);
	delete pTask;
}

size_t CCurlWrapper::CurlCallback_WriteToStdString(void *ptr, size_t size, size_t count, void *stream)
{
	((std::string*)stream)->append((char*)ptr, 0, size*count);
	return size*count;
}

int CCurlWrapper::ProgressCallback(void *pUser, double DlTotal, double DlCurr, double UlTotal, double UlCurr)
{
	*((float*)pUser) = (float)((100.0 * DlCurr) / (DlTotal ? DlTotal : 1.0));
	return 0;
}
