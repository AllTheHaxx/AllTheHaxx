#ifndef ENGINE_UPDATER_H
#define ENGINE_UPDATER_H

#include "kernel.h"

class IUpdater : public IInterface
{
	MACRO_INTERFACE("updater", 0)
	int m_State;

protected:
	void SetState(int NewState) { m_State = NewState; }

public:
	enum
	{
		STATE_CLEAN = 0, // no update and currently idle
		STATE_SYNC_REFRESH, // waiting for version info from the GitHubAPI
		STATE_GETTING_MANIFEST, // updating: downloading the additional manifest
		STATE_SYNC_POSTGETTING,
		STATE_PARSING_UPDATE, // GitHubAPI has got all jobs and we have got the manifest -> start with the first step of the update
		STATE_DOWNLOADING, // downloading files
		STATE_MOVE_FILES, // installing update
		STATE_NEED_RESTART,
		STATE_FAIL,
	};

	virtual void Tick() = 0;
	virtual void CheckForUpdates(bool ForceRefresh = false) = 0;
	virtual void PerformUpdate() = 0;

	virtual const char *GetLatestVersion() const = 0;
	virtual const char *GetNews() const = 0;
	virtual const char *GetWhatFailed() const = 0;

	int State() const { return m_State; };
	virtual char *GetCurrentFile() = 0;
	virtual int GetCurrentPercent() const = 0;
	virtual int GetTotalNumJobs() const = 0;
	virtual int GetTotalProgress() const = 0;
};

#endif
