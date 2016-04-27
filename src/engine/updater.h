#ifndef ENGINE_UPDATER_H
#define ENGINE_UPDATER_H

#include "kernel.h"

class IUpdater : public IInterface
{
	MACRO_INTERFACE("updater", 0)
public:
	enum
	{
		CLEAN = 0,
		GETTING_MANIFEST,
		GOT_MANIFEST,
		PARSING_UPDATE,
		DOWNLOADING,
		NEED_RESTART,
		FAIL,
	};

	virtual void Update() = 0;
	virtual void InitiateUpdate(bool CheckOnly = false, bool ForceRefresh = false) = 0;

	virtual const char *GetLatestVersion() const = 0;
	virtual int GetCurrentState() const = 0;
	virtual const char *GetCurrentFile() const = 0;
	virtual int GetCurrentPercent() const = 0;
};

#endif
