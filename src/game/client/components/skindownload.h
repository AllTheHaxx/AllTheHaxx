#ifndef GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H
#define GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H

#include <string>

#include <engine/fetcher.h>
#include <engine/storage.h>

#include <game/client/component.h>

class CSkinDownload : public CComponent
{
public:
	class CSkinFetchTask
	{
	MACRO_ALLOC_HEAP();

		std::string m_SkinName;
		int m_Url;
		int m_Progress;
		int m_State;
		int64 m_FinishTime;

	public:
		CSkinFetchTask(const char *pSkin) : m_SkinName(std::string(pSkin))
		{
			m_pCurlTask = 0;
			m_Url = 0;
			m_Progress = 0;
			m_FinishTime = -1;
		}

		void Next()
		{
			m_pCurlTask = 0;
			m_Url++;
			m_FinishTime = -1;
		}

		void Invalidate()
		{
			m_State = CFetchTask::STATE_ERROR;
			m_FinishTime = time_get();
			m_Progress = 100;
		}

		CFetchTask* m_pCurlTask;

		// getters
		CFetchTask *Task() const { return m_pCurlTask; }
		const char *SkinName() const { return m_SkinName.c_str(); }
		int Url() const { return m_Url; }
		int Progress() const { return m_Progress; }
		int State() const { return m_State; }
		int64 FinishTime() const { return m_FinishTime; }

		// setters
		void Finish() { m_FinishTime = time_get(); }

		// static shit
		static void ProgressCallback(CFetchTask *pTask, void *pUser);

	};

private:
	class IFetcher *m_pFetcher;
	class IStorageTW *m_pStorage;

	int m_DefaultSkin;

	enum
	{
		MAX_FETCHTASKS = 4,
	};

	static array<std::string> ms_aSkinDbUrls;
	static const char *GetURL(int i)
	{
		dbg_assert(i >= 0 && i < ms_aSkinDbUrls.size(), "GetURL called with index out of range");
		return ms_aSkinDbUrls[i].c_str();
	}
	static int NumURLs() { return ms_aSkinDbUrls.size(); }
	static CSkinFetchTask *ms_apFetchTasks[MAX_FETCHTASKS];
	static array<std::string> ms_FailedTasks;


	/**
	 * @threadsafety DOESN'T lock, but accesses the critical array
	 * @return The number of tasks
	 */
	static int NumTasks(bool ActiveOnly=false)
	{
		int ret = 0;
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(ms_apFetchTasks[i] != NULL)
				if(!ActiveOnly || ms_apFetchTasks[i]->FinishTime() < 0)
					ret++;
		return ret;
	}

	static CSkinFetchTask *FindTask(const CFetchTask* pTask)
	{
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(ms_apFetchTasks[i] != NULL)
				if(ms_apFetchTasks[i]->Task() == pTask)
					return ms_apFetchTasks[i];
		return NULL;
	}

	static CSkinFetchTask **FindFreeSlot()
	{
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(ms_apFetchTasks[i] == NULL)
				return &(ms_apFetchTasks[i]);
		return NULL;
	}

	void LoadUrls();

	void Fail(const char *pSkinName) { ms_FailedTasks.add(std::string(pSkinName)); }
	bool FetchNext(CSkinFetchTask *pTaskHandler);
	void FetchSkin(CSkinFetchTask *pTaskHandler);

public:
	~CSkinDownload()
	{
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(ms_apFetchTasks[i])
				delete ms_apFetchTasks[i];
	}

	void OnConsoleInit();
	void OnInit();
	void OnRender();

	/**
	 * Gets the ID of the requested skin.<br>
	 * If the skin doesn't exist, automatically try to download it.
	 * @param pDestID - Pointer to an integer to store the skin ID in
	 * @param pName - Name of the wanted skin
	 * @remark	Returns the ID of the default skin if the requested skin couldn't be
	 * 			downloaded or if it is currently being downloaded
	 */
	void RequestSkin(int *pDestID, const char *pName);

	//static void ProgressCallback(CFetchTask *pTask, void *pUser);
	static void CompletionCallback(CFetchTask *pTask, void *pUser);

	static void ConFetchSkin(IConsole::IResult *pResult, void *pUserData);
	static void ConDbgSpam(IConsole::IResult *pResult, void *pUserData);
};



#endif
