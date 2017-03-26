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

	enum
	{
		ERROR_NONE = 0,
		ERROR_INIT,
		ERROR_CHECK,
		ERROR_UPDATE
	};


private:
	CURL *m_pHandle;

	int m_State;
	int m_Error;
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
	int GetErrorCode() const { return m_Error; }
	const char *GetWhatFailed() const
	{
		switch (m_Error)
		{
			case ERROR_NONE: return "Nothing";
			case ERROR_INIT: return "Init";
			case ERROR_CHECK: return "Update Check";
			case ERROR_UPDATE: return "Data Update";
			default: return "unknown GH-thing";
		}
	}
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
	void SetError(int Error)
	{
		m_State = STATE_ERROR;
		m_Error = Error;
	}

	const std::string SimpleGET(const char *pURL);

	static const std::string ParseReleases(const char *pJsonStr);
	bool ParseCompare(const char *pJsonStr);

	static void UpdateCheckerThread(CGitHubAPI *pSelf);
	static void CompareThread(CGitHubAPI *pSelf);

};


#endif
