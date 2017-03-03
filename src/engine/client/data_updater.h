#ifndef ENGINE_CLIENT_DATA_UPDATER_H
#define ENGINE_CLIENT_DATA_UPDATER_H
#include <string>
#include <map>
#include <curl/curl.h>
#include <base/system++/threading.h>
#include <vector>

#define GITHUB_API_URL "https://api.github.com/repos/AllTheHaxx/AllTheHaxx"

class GitHubAPI
{
public:
	enum
	{
		STATE_IDLE,
		STATE_REFRESHING,
		STATE_NEWVERSION,
		STATE_CLEAN,
		STATE_ERROR,
		STATE_UPDATING,
		STATE_DONE,
	};


private:
	CURL *m_pHandle;

	int m_State;
	char m_aLatestVersion[10];
	std::vector<std::string> m_DownloadJobs; // relative path of the file (e.g. "data/somedir/somefile.txt")
	std::vector<std::string> m_RemoveJobs;
	std::vector< std::pair<std::string, std::string> > m_RenameJobs; // old name - new name


public:
	GitHubAPI();
	~GitHubAPI();

	void CheckVersion();
	void DoUpdate();
	const char *GetLatestVersion() const { return m_aLatestVersion; }

private:
	const std::string SimpleGET(const char *pURL);
//	static void CurlWriteFunction(char *pData, size_t size, size_t nmemb, void *userdata);

	static const std::string ParseReleases(const char *pJsonStr);
	bool ParseCompare(const char *pJsonStr);

	const std::vector<std::string>& GetDownloadJobs() const { return m_DownloadJobs; }
	const std::vector<std::string>& GetRemoveJobs() const { return m_RemoveJobs; }
	const std::vector< std::pair<std::string, std::string> >& GetRenameJobs() const { return m_RenameJobs; }

	static void UpdateCheckerThread(GitHubAPI *pSelf);
	static void CompareThread(GitHubAPI *pSelf);
	static void GitHashStr(const char *pFile, char *pBuffer, unsigned BufferSize);
};


#endif
