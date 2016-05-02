#ifndef ENGINE_CLIENT_UPDATER_H
#define ENGINE_CLIENT_UPDATER_H

#include <engine/updater.h>
#include <engine/fetcher.h>
#include <map>
#include <string>

#define CLIENT_EXEC "AllTheHaxx"
#define SERVER_EXEC "AllTheHaxx-Server"

#if defined(CONF_FAMILY_WINDOWS)
	#define PLAT_EXT ".exe"
	#define PLAT_NAME CONF_PLATFORM_STRING
#elif defined(CONF_FAMILY_UNIX)
	#define PLAT_EXT ""
	#if defined(CONF_ARCH_IA32)
		#define PLAT_NAME CONF_PLATFORM_STRING "-x86"
	#elif defined(CONF_ARCH_AMD64)
		#define PLAT_NAME CONF_PLATFORM_STRING "-x86_64"
	#else
		#define PLAT_NAME CONF_PLATFORM_STRING "-unsupported"
	#endif
#else
	#define PLAT_EXT ""
	#define PLAT_NAME "unsupported-unsupported"
#endif

#define PLAT_CLIENT_DOWN CLIENT_EXEC "-" PLAT_NAME PLAT_EXT
#define PLAT_SERVER_DOWN SERVER_EXEC "-" PLAT_NAME PLAT_EXT

#define PLAT_CLIENT_EXEC CLIENT_EXEC PLAT_EXT
#define PLAT_SERVER_EXEC SERVER_EXEC PLAT_EXT

#define JOB_ADD true
#define JOB_REMOVE false

class CUpdater : public IUpdater
{
	class IClient *m_pClient;
	class IStorageTW *m_pStorage;
	class IFetcher *m_pFetcher;

	bool m_IsWinXP;

	int m_State;
	char m_Status[256];
	int m_Percent;
	char m_aLastFile[256];

	bool m_ClientUpdate;
	bool m_ServerUpdate;

	bool m_CheckOnly;
	char m_aVersion[10];

	std::map<std::string, bool> m_FileJobs;
	std::map<std::string, std::map<std::string, std::string> > m_ExternalFiles; // source - dlpath, dest

	void AddFileJob(const char *pFile, bool job);
	void FetchFile(const char *pSource, const char *pFile, const char *pDestPath = 0);
	void MoveFile(const char *pFile);

	void ParseUpdate();
	void PerformUpdate();
	void CommitUpdate();

	void ReplaceClient();
	void ReplaceServer();

public:
	CUpdater();
	static void ProgressCallback(CFetchTask *pTask, void *pUser);
	static void CompletionCallback(CFetchTask *pTask, void *pUser);

	const char *GetLatestVersion() const { return m_aVersion; }

	int GetCurrentState() const { return m_State; };
	char *GetCurrentFile() { return m_Status; };
	int GetCurrentPercent() const { return m_Percent; };

	virtual void InitiateUpdate(bool CheckOnly = false, bool ForceRefresh = false);
	void Init();
	virtual void Update();
	void WinXpRestart();
};

#endif
