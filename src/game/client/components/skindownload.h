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

		CFetchTask* m_pCurlTask;
		std::string m_SkinName;
		//int *m_pDestID;
		int m_Url;
		int m_Progress;
		int m_State;
		int64 m_FinishTime;

	public:
		/*CSkinFetchTask()
		{
			m_pCurlTask = 0;
			m_SkinName = "";
			m_Url = 0;
			m_FinishTime = -1;
		}*/

		CSkinFetchTask(const char *pSkin) : m_SkinName(std::string(pSkin))
		{
			m_pCurlTask = new CFetchTask(false);
			m_Url = 0;
			m_Progress = 0;
			m_FinishTime = -1;
		}

		~CSkinFetchTask()
		{
			//dbg_assert(m_pCurlTask != NULL, "SkinFetchTask::pCurlTask has been modified externally");
			if(m_pCurlTask)
				delete m_pCurlTask;
			else
				dbg_msg("skinfetcher/debug", "!! WARNING !! > Taskhandler for '%s' had no more curltask at destruction!", m_SkinName.c_str());
		}

		void Next()
		{
			dbg_assert(m_pCurlTask != NULL, "SkinFetchTask::pCurlTask == NULL");
			delete m_pCurlTask;
			m_pCurlTask = new CFetchTask(false);
			m_Url++;
			m_FinishTime = -1;
		}

		void Invalidate()
		{
			m_State = CFetchTask::STATE_ERROR;
			m_FinishTime = time_get();
			m_Progress = 100;
		}

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

		MAX_URLS = 64,
		MAX_URL_LEN = 512,
	};

	std::string m_aSkinDbUrls[MAX_URLS];
	const char *GetURL(int i) const
	{
		dbg_assert(i >= 0 && i < m_NumUrls, "GetURL called with index out of range");
		return m_aSkinDbUrls[i].c_str();
	}
	int m_NumUrls;
	int NumURLs() const { return m_NumUrls; }
	CSkinFetchTask *m_apFetchTasks[MAX_FETCHTASKS];
	array<std::string> m_FailedTasks;


	/**
	 * @Warning DON'T use this to iterate over the array! ALWAYS use <code>MAX_FETCHTASKS</code>!
	 * @threadsafety DOESN'T lock, but accesses the critical array
	 * @return The number of tasks
	 */
	int NumTasks(bool ActiveOnly=false)
	{
		int ret = 0;
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(m_apFetchTasks[i] != NULL)
				if(!ActiveOnly || m_apFetchTasks[i]->FinishTime() < 0)
					ret++;
		return ret;
	}

	CSkinFetchTask *FindTask(CFetchTask* pTask)
	{
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(m_apFetchTasks[i] != NULL)
				if(m_apFetchTasks[i]->Task() == pTask)
					return m_apFetchTasks[i];
		return NULL;
	}

	CSkinFetchTask **FindFreeSlot()
	{
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(m_apFetchTasks[i] == NULL)
				return &(m_apFetchTasks[i]);
		return NULL;
	}

	void LoadUrls();

	void Fail(const char *pSkinName) { m_FailedTasks.add(std::string(pSkinName)); }
	bool FetchNext(CSkinFetchTask *pTaskHandler);
	void FetchSkin(CSkinFetchTask *pTaskHandler);

public:
	~CSkinDownload()
	{
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(m_apFetchTasks[i])
				delete m_apFetchTasks[i];
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
