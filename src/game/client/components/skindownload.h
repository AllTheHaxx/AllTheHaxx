#ifndef GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H
#define GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H

#include <string>

#include <engine/fetcher.h>
#include <engine/storage.h>

#include <game/client/component.h>

class CSkinDownload : public CComponent
{
	class IFetcher *m_pFetcher;
	class IStorageTW *m_pStorage;
	LOCK m_Lock;

	enum { MAX_FETCHTASKS=3 };

	struct SkinDbUrl
	{
		int prior;
		std::string url;
		bool operator<(const SkinDbUrl& other) { return this->prior < other.prior; }
	};
	sorted_array<SkinDbUrl> m_SkinDbUrls;

	struct SkinFetchTask
	{
		CFetchTask* pCurlTask;
		std::string SkinName;
		int url;
		int Progress;
		int64 FinishTime;
		int State;
		int *pDestID;
	};

	array<SkinFetchTask> m_FetchTasks;
	array<std::string> m_FailedTasks;

	SkinFetchTask *FindTask(CFetchTask* pTask)
	{
		for(int i = 0; i < m_FetchTasks.size(); i++)
			if(m_FetchTasks[i].pCurlTask == pTask)
				return &m_FetchTasks[i];
		return 0;
	}

public:
	void OnConsoleInit();
	void OnInit();
	void OnRender();

	void RequestSkin(int *pDestID, const char *pName);
	void FetchSkin(const char *pName, int *pDestID = 0, int url = 0);

	void LoadUrls();

	static void ProgressCallback(CFetchTask *pTask, void *pUser);
	static void CompletionCallback(CFetchTask *pTask, void *pUser);

	static void ConFetchSkin(IConsole::IResult *pResult, void *pUserData);
};



#endif
