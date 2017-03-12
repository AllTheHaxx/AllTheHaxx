#ifndef ENGINE_CLIENT_DATA_UPDATER_H
#define ENGINE_CLIENT_DATA_UPDATER_H
#include <string>
#include <map>
#include <curl/curl.h>
#include <base/system++/threading.h>
#include <vector>

#define GITHUB_API_URL "https://api.github.com/repos/AllTheHaxx/AllTheHaxx"

class CGitHubAPI
{
public:
	enum
	{
		STATE_CLEAN = 0, // there is no update and we are currently idle
		STATE_REFRESHING, // is refreshing
		STATE_NEW_VERSION, // the refresh told us: there's a new version!
		STATE_COMPARING, // downloading and parsing compare
		STATE_DONE, // finished comparing, signalizes the updater that we're ready to apply changes
		STATE_ERROR, // uh-oh, no gud
	};


private:
	CURL *m_pHandle;

	int m_State;
	char m_aLatestVersion[10];
	char m_aLatestVersionTree[40+1];
	std::vector<std::string> m_DownloadJobs; // relative path of the file (e.g. "data/somedir/somefile.txt")
	std::vector<std::string> m_RemoveJobs;
	std::vector< std::pair<std::string, std::string> > m_RenameJobs; // old name - new name
	float m_Progress;


public:
	CGitHubAPI();
	~CGitHubAPI();

	int State() const { return m_State; }
	bool Done() const { return m_State == STATE_DONE; }
	float GetProgress() const { return m_Progress; }

	void CheckVersion();
	void DoUpdate();
	const char *GetLatestVersion() const { return m_aLatestVersion; }
	const char *GetLatestVersionTree() const { return m_aLatestVersionTree; }

	const std::vector<std::string>& GetDownloadJobs() const { return m_DownloadJobs; }
	const std::vector<std::string>& GetRemoveJobs() const { return m_RemoveJobs; }
	const std::vector< std::pair<std::string, std::string> >& GetRenameJobs() const { return m_RenameJobs; }


private:
	const std::string SimpleGET(const char *pURL);
//	static void CurlWriteFunction(char *pData, size_t size, size_t nmemb, void *userdata);

	static const std::string ParseReleases(const char *pJsonStr);
	bool ParseCompare(const char *pJsonStr);

	static void UpdateCheckerThread(CGitHubAPI *pSelf);
	static void CompareThread(CGitHubAPI *pSelf);

	static void GitHashStr(const char *pFile, char *pBuffer, unsigned BufferSize);
};


#endif
