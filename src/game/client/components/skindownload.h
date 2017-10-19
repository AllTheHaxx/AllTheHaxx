#ifndef GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H
#define GAME_CLIENT_COMPONENTS_SKINDOWNLOAD_H

#include <string>
#include <map>

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
			m_State = -1;
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
			m_State = IFetchTask::STATE_ERROR;
			m_FinishTime = time_get();
			m_Progress = 100;
		}

		IFetchTask* m_pCurlTask;

		// getters
		IFetchTask *Task() const { return m_pCurlTask; }
		const char *SkinName() const { return m_SkinName.c_str(); }
		int Url() const { return m_Url; }
		int Progress() const { return m_Progress; }
		int State() const { return m_State; }
		int64 FinishTime() const { return m_FinishTime; }

		// setters
		void Finish() { m_FinishTime = time_get(); }

		// static shit
		static void ProgressCallback(IFetchTask *pTask, void *pUser);

	};

private:
	class IFetcher *m_pFetcher;
	class IStorageTW *m_pStorage;

	int m_DefaultSkin;

	enum
	{
		MAX_FETCHTASKS = 4,
	};

	array<std::string> m_aSkinDbUrls;
	const char *GetURL(int i)
	{
		if(dbg_assert_strict(i >= 0 && i < m_aSkinDbUrls.size(), "GetURL called with index out of range"))
			return "";
		return m_aSkinDbUrls[i].c_str();
	}
	int NumURLs() const { return m_aSkinDbUrls.size(); }
	std::map<const IFetchTask*, CSkinFetchTask*> m_lpFetchTasks;
	array<std::string> m_FailedTasks;


	/**
	 * @threadsafety DOESN'T lock, but accesses the critical array
	 * @return The number of tasks
	 */
	int NumTasks(bool ActiveOnly=false)
	{
		int ret = 0;
		for(auto &it : m_lpFetchTasks)
		{
			if(!ActiveOnly || it.second->FinishTime() < 0)
				ret++;
		}
		return ret;
	}

	CSkinFetchTask *FindTask(const IFetchTask* pTask)
	{
		try
		{
			CSkinFetchTask *pTaskHandler = m_lpFetchTasks.at(pTask);
			return pTaskHandler;
		} catch(std::out_of_range&) {
			return NULL;
		}
	}

	void LoadUrls();

	void Fail(const char *pSkinName) { m_FailedTasks.add(std::string(pSkinName)); }
	bool FetchNext(CSkinFetchTask *pTaskHandler);
	void FetchSkin(CSkinFetchTask *pTaskHandler);

public:
	CSkinDownload() : m_pFetcher(0), m_pStorage(0), m_DefaultSkin(-1)
	{
	}

	~CSkinDownload()
	{
		for(auto &it : m_lpFetchTasks)
			delete it.second;
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
	static void CompletionCallback(IFetchTask *pTask, void *pUser);

	static void ConFetchSkin(IConsole::IResult *pResult, void *pUserData);
	static void ConDbgSpam(IConsole::IResult *pResult, void *pUserData);
};



#endif
