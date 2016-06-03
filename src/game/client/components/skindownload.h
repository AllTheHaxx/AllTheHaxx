#ifndef GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H
#define GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H

#include <map>
#include <string>

#include <engine/fetcher.h>
#include <engine/storage.h>

#include <game/client/component.h>

class CSkinDownload : public CComponent
{
	class IFetcher *m_pFetcher;
	class IStorageTW *m_pStorage;

	struct SkinFetchTask
	{
		std::string SkinName;
		int Progress;
		int64 FinishTime;
		int State;
		int *pDestID;
	};
	std::map<CFetchTask*, SkinFetchTask> m_FetchTasks;
	array<std::string> m_FailedTasks;

public:
	void OnConsoleInit();
	void OnInit();
	void OnRender();

	void RequestSkin(int *pDestID, const char *pName);
	void FetchSkin(const char *pName, int *pDestID = 0);

	static void ProgressCallback(CFetchTask *pTask, void *pUser);
	static void CompletionCallback(CFetchTask *pTask, void *pUser);

	static void ConFetchSkin(IConsole::IResult *pResult, void *pUserData);
};



#endif
