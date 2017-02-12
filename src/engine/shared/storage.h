#ifndef ENGINE_SHARED_STORAGE_H
#define ENGINE_SHARED_STORAGE_H

#include <base/system.h>
#include <engine/storage.h>
#include "engine.h"

// compiled-in data-dir path
#define DATA_DIR "data"



class CStorage : public IStorageTW
{
public:
	char m_aApplicationSavePath[512];
	char m_aDatadir[512];

	CStorage()
	{
		m_aApplicationSavePath[0] = 0;
		m_aDatadir[0] = 0;
	}

	int Init(const char *pApplicationName, const char *pArgv0);

	void FindDatadir(const char *pArgv0);

	virtual void ListDirectory(int Types, const char *pPath, FS_LISTDIR_CALLBACK pfnCallback, void *pUser);
	virtual void ListDirectoryInfo(int Type, const char *pPath, FS_LISTDIR_INFO_CALLBACK pfnCallback, void *pUser) = 0;

	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, char *pBuffer = 0, int BufferSize = 0);

	static IStorageTW *Create(const char *pApplicationName, const char *pArgv0)
	{
		CStorage *p = new CStorage();
		if(p->Init(pApplicationName, pArgv0))
		{
			delete p;
			p = 0;
		}
		return p;
	}
};

IStorageTW *CreateStorage(const char *pApplicationName, const char *pArgv0);

#endif
