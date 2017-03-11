#include <base/system.h>
#include <openssl/sha.h>
#include <engine/storage.h>
#include <base/system++/io.h>
#include <engine/shared/config.h>
#include <json-parser/json.hpp>
#include <game/version.h>
#include "data_updater.h"
#include "updater.h"
#include "curlwrapper.h"


using std::map;
using std::string;


GitHubAPI::GitHubAPI()
{
	if((m_pHandle = curl_easy_init()) == NULL)
		m_State = STATE_ERROR;
	else
		m_State = STATE_IDLE;

	mem_zerob(m_aLatestVersion);
	m_aLatestVersion[0] = '0';

	m_DownloadJobs.clear();
	m_RemoveJobs.clear();
	m_RenameJobs.clear();
}

GitHubAPI::~GitHubAPI()
{
	if(m_pHandle)
		curl_easy_cleanup(m_pHandle);
}

void GitHubAPI::CheckVersion()
{
	dbg_msg("github", "refreshing version info");
	m_State = STATE_REFRESHING;

	THREAD_SMART<GitHubAPI> Thread(GitHubAPI::UpdateCheckerThread);
	if(!Thread.StartDetached(this))
		m_State = STATE_ERROR;
}

void GitHubAPI::DoUpdate()
{
	dbg_msg("github", "performing data update");
	m_State = STATE_UPDATING;

	THREAD_SMART<GitHubAPI> Thread(GitHubAPI::CompareThread);
	if(!Thread.StartDetached(this))
		m_State = STATE_ERROR;
}

//---------------------------- STEP 1: UPDATE CHECKING ----------------------------//

void GitHubAPI::UpdateCheckerThread(GitHubAPI *pSelf)
{
	std::string Result;
	char aUrl[512];
	str_copyb(aUrl, GITHUB_API_URL "/releases?page=1&per_page=1");
	Result = pSelf->SimpleGET(aUrl);
	if(Result.empty() || Result.length() == 0)
	{
		dbg_msg("github/releases", "ERROR: result empty");
		pSelf->m_State = STATE_ERROR;
		return;
	}

	// check if we have the latest version
	{
		const char *pLatestVersion = ParseReleases(Result.c_str()).c_str();
		if(str_length(pLatestVersion) == 0)
		{
			pSelf->m_State = STATE_ERROR;
			return;
		}
		else
		{
			if(str_comp_nocase(pLatestVersion, GAME_ATH_VERSION) == 0)
			{
				dbg_msg("github", "AllTheHaxx is up to date.");
				pSelf->m_State = STATE_CLEAN;
				return;
			}
			else
			{
				str_copyb(pSelf->m_aLatestVersion, pLatestVersion);
				dbg_msg("github", " -- NEW VERSION: AllTheHaxx %s has been released! --", pLatestVersion);
				pSelf->m_State = STATE_NEWVERSION;
				return;
			}
		}
	}
}

const std::string GitHubAPI::ParseReleases(const char *pJsonStr)
{
	// gets json[0]["name"]

	json_value &jsonVersions = *json_parse(pJsonStr, (size_t)str_length(pJsonStr));
	const json_value &jsonName = jsonVersions[0]["name"];

	std::string Result((const char *)jsonName);

	json_value_free(&jsonVersions);

	return Result;
}

//---------------------------- STEP 2: CHANGELIST CREATION ----------------------------//

void GitHubAPI::CompareThread(GitHubAPI *pSelf)
{
	std::string Result;
	char aUrl[512];
	str_formatb(aUrl, GITHUB_API_URL "/compare/%s...%s", GAME_ATH_VERSION, pSelf->m_aLatestVersion);
	Result = pSelf->SimpleGET(aUrl);
	if(Result.empty() || Result.length() == 0)
	{
		dbg_msg("github/compare", "ERROR: result empty");
		pSelf->m_State = STATE_ERROR;
		return;
	}

	if(!pSelf->ParseCompare(Result.c_str()))
	{
		pSelf->m_State = STATE_ERROR;
		dbg_msg("github/compare", "ERROR: parsing failed");
	}
	else
	{
		pSelf->m_State = STATE_DONE;
		int NumTotal = 0;
		int NumDownload = (NumTotal += (int)pSelf->m_DownloadJobs.size());
		int NumRename = (NumTotal += (int)pSelf->m_RenameJobs.size());
		int NumRemove = (NumTotal += (int)pSelf->m_RemoveJobs.size());
		dbg_msg("github/compare", "got %i jobs; download=%i, rename=%i, delete=%i", NumTotal, NumDownload, NumRename, NumRemove);
	}
}



bool GitHubAPI::ParseCompare(const char *pJsonStr)
{
	// gets json["files"][i]["filename"]

	json_value &jsonCompare = *json_parse(pJsonStr, (size_t)str_length(pJsonStr));

	const json_value &jsonFiles = jsonCompare["files"];
	if(jsonFiles.type != json_array)
		return false;

	// loop through the array of changed files
	for(unsigned int i = 0; i < jsonFiles.u.array.length; i++)
	{
		// we only want files in the data/ directory
		const char *pFilename = (const char *)(jsonFiles[i]["filename"]);
		if(str_comp_nocase_num(pFilename, "data/", str_length("data/")) != 0)
			continue;

		// find out what to do
		const char *pStatus = (const char *)(jsonFiles[i]["status"]);
		if(str_comp_nocase(pStatus, "modified") == 0 ||
		   str_comp_nocase(pStatus, "added") == 0)
		{
			// file has been added or modified since our version? -> queue for (re-)download
			m_DownloadJobs.push_back(std::string(pFilename));
		}
		else if(str_comp_nocase(pStatus, "renamed") == 0)
		{
			// if the file has only been renamed but not changed, we don't need to redownload it
			const char *pPreviousFilename = (const char *)(jsonFiles[i]["previous_filename"]);
			m_RenameJobs.push_back(std::pair<std::string, std::string>(std::string(pPreviousFilename), std::string()));
		}
		else if(str_comp_nocase(pStatus, "removed") == 0)
		{
			// files that have been removed from the repo can be deleted locally, too
			m_RemoveJobs.push_back(std::string(pFilename));
		}
	}

	json_value_free(&jsonCompare);

	return true;
}

//---------------------------- HELPER FUNCTIONS ----------------------------//

const std::string GitHubAPI::SimpleGET(const char *pUrl)
{
	std::string Result = "";

	char aErr[CURL_ERROR_SIZE];
	curl_easy_setopt(m_pHandle, CURLOPT_ERRORBUFFER, aErr);

	// create the headers
	curl_slist *list = NULL;
	list = curl_slist_append(list, "Content-Type: application/json");
	list = curl_slist_append(list, "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:51.0) Gecko/20100101 Firefox/51.0");
	list = curl_slist_append(list, "Accept: application/vnd.github.v3+json");
	curl_easy_setopt(m_pHandle, CURLOPT_HTTPHEADER, list);

	curl_easy_setopt(m_pHandle, CURLOPT_CONNECTTIMEOUT_MS, (long)g_Config.m_ClHTTPConnectTimeoutMs);
	curl_easy_setopt(m_pHandle, CURLOPT_LOW_SPEED_LIMIT, (long)g_Config.m_ClHTTPLowSpeedLimit);
	curl_easy_setopt(m_pHandle, CURLOPT_LOW_SPEED_TIME, (long)g_Config.m_ClHTTPLowSpeedTime);
	curl_easy_setopt(m_pHandle, CURLOPT_FOLLOWLOCATION, 1L);
	curl_easy_setopt(m_pHandle, CURLOPT_FAILONERROR, 1L);
	curl_easy_setopt(m_pHandle, CURLOPT_URL, pUrl);
	curl_easy_setopt(m_pHandle, CURLOPT_WRITEDATA, &Result);
	curl_easy_setopt(m_pHandle, CURLOPT_WRITEFUNCTION, &CCurlWrapper::CurlCallback_WriteToStdString);
	curl_easy_setopt(m_pHandle, CURLOPT_NOPROGRESS, 1L);
	curl_easy_setopt(m_pHandle, CURLOPT_NOSIGNAL, 1L);

	int ret = curl_easy_perform(m_pHandle);

	// clean up
	curl_slist_free_all(list);

	if(ret != CURLE_OK)
	{
		dbg_msg("github/error", "'%s' failed: %s", pUrl, aErr);
	}
	else
	{
		dbg_msg("github/debug", "'%s' -> %lu bytes", pUrl, Result.length());
	}
	return Result;
}

/* this one is overly complicated I think... let's not use it?
void GitHubAPI::CurlWriteFunction(char *pData, size_t size, size_t nmemb, void *userdata)
{
	std::string *pResult = (std::string *)userdata;

	unsigned int BufferSize = (unsigned int)(size*nmemb + 1);
	char *pBuf = mem_allocb(char, BufferSize);
	mem_zero(pBuf, BufferSize);
	mem_copy(pBuf, pData, (unsigned int)(size*nmemb));

	*pResult += std::string(pBuf);
	mem_free(pBuf);
}*/

void GitHubAPI::GitHashStr(const char *pFile, char *pBuffer, unsigned BufferSize)
{
	unsigned char aHash[SHA_DIGEST_LENGTH];
	mem_zerob(aHash);

	SHA_CTX context;
	if(!SHA_Init(&context))
	{
		dbg_msg("GitHashStr", "SHA_Init failed for '%s'", pFile);
	}
	else
	{
		IOHANDLE_SMART File(pFile, IOFLAG_READ);

		// prepend what git does
		{
			char aBuffer[16 * 1024];
			str_formatb(aBuffer, "blob %lu", File.Length());
			if(!SHA_Update(&context, aBuffer, (unsigned)str_length(aBuffer)+1)) // +1 because git wants the \0 to be hashed aswell!
			{
				dbg_msg("GitHashStr", "SHA_Update failed for the git identifier '%s'", aBuffer);
			}
			else
			{
				// read the data in portions of 16kb and hash it subsequently
				while(1)
				{
					mem_zerob(aBuffer);

					unsigned int BytesRead = File.Read(aBuffer, sizeof(aBuffer));
					if(BytesRead == 0)
						break;

					if(!SHA_Update(&context, aBuffer, BytesRead))
					{
						dbg_msg("GitHashStr", "SHA_Update failed for '%s' at 0x%x", pFile, (unsigned int)File.Tell());
						continue;
					}
				}
			}
		}
	}

	if(!SHA_Final(aHash, &context))
	{
		dbg_msg("GitHashStr", "SHA_Final failed for '%s'", pFile);
		mem_zerob(aHash);
	}

	str_hex_simple(pBuffer, BufferSize, aHash, SHA_DIGEST_LENGTH);
}
