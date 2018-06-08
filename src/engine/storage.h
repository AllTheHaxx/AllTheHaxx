/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_STORAGE_H
#define ENGINE_STORAGE_H

#include "kernel.h"

struct lua_State;

// compiled in paths
#ifndef CONF_INSTALL_ROOT
	#define STORAGE_ATH_ROOT "."
	#define STORAGE_EDTC_DIR STORAGE_DATA_DIR "/edtc"
#else
	#define STORAGE_ATH_ROOT CONF_INSTALL_ROOT"/usr/share/allthehaxx"
	#define STORAGE_EDTC_DIR CONF_INSTALL_ROOT"/etc/allthehaxx"
#endif

#define STORAGE_DATA_DIR STORAGE_ATH_ROOT "/data"


class IStorageTW : public IInterface
{
	MACRO_INTERFACE("storage", 0)
public:
	enum
	{
		TYPE_SAVE = 0,
		TYPE_ALL = -1,
		TYPE_ABSOLUTE = -2,

		STORAGETYPE_BASIC = 0,
		STORAGETYPE_SERVER,
		STORAGETYPE_CLIENT,
	};

	template<class T> struct CLoadHelper // this is really useful to list dirs recursively
	{
		MACRO_ALLOC_HEAP()
	public:
		T *pSelf;
		const char *pFullDir;

	};

	virtual void ListDirectory(int Type, const char *pPath, FS_LISTDIR_CALLBACK pfnCallback, void *pUser) = 0;
	virtual int ListDirectoryVerbose(int Type, const char *pPath, FS_LISTDIR_CALLBACK_VERBOSE pfnCallback, void *pUser) = 0;
	virtual void ListDirectoryInfo(int Type, const char *pPath, FS_LISTDIR_INFO_CALLBACK pfnCallback, void *pUser) = 0;
	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, int Type, char *pBuffer = 0, int BufferSize = 0) = 0;
	virtual class IOHANDLE_SMART OpenFileSmart(const char *pFilename, int Flags, int Type) = 0;
	virtual bool FindFile(const char *pFilename, const char *pPath, int Type, char *pBuffer, int BufferSize) = 0;
	virtual bool RemoveFile(const char *pFilename, int Type) = 0;
	virtual bool RenameFile(const char* pOldFilename, const char* pNewFilename, int Type) = 0;
	virtual bool CreateFolder(const char *pFoldername, int Type) = 0;
	#if !defined(BUILD_TOOLS)
	virtual bool CreateFolderLua(const char *pFoldername, lua_State *L) = 0;
	#endif

	virtual bool RemoveBinaryFile(const char *pFilename) = 0;
	virtual bool RenameBinaryFile(const char* pOldFilename, const char* pNewFilename) = 0;
	virtual const char *GetBinaryPath(const char *pDir, char *pBuffer, unsigned BufferSize) = 0;

	virtual const char *GetExecutableName() const = 0;
	virtual const char *GetAppdataPath() = 0;

	virtual const char *SandboxPath(char *pBuffer, unsigned BufferSize, const char *pPrepend = 0, bool ForcePrepend = false) const = 0;
	virtual const char *GetFullPath(const char *pFilename, int StorageType, char *pBuffer, unsigned BufferSize) const = 0;
};

extern IStorageTW *CreateStorage(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments);
extern IStorageTW *CreateLocalStorage();


#endif
