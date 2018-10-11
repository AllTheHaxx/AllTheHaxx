/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/system++/io.h>
#include <engine/storage.h>
#include <engine/client/luabinding.h>

#include "linereader.h"


class CStorage : public IStorageTW
{
public:
	enum
	{
		MAX_PATHS = 16,
		MAX_PATH_LENGTH = 512
	};

	char m_aaStoragePaths[MAX_PATHS][MAX_PATH_LENGTH];
	int m_NumPaths;
	char m_aDatadir[MAX_PATH_LENGTH];
	char m_aUserdir[MAX_PATH_LENGTH];
	char m_aCurrentdir[MAX_PATH_LENGTH];
	char m_aBinarydir[MAX_PATH_LENGTH];
	char m_aExecutableName[128];

	CStorage()
	{
		mem_zero(m_aaStoragePaths, sizeof(m_aaStoragePaths));
		m_NumPaths = 0;
		m_aDatadir[0] = 0;
		m_aUserdir[0] = 0;
	}
	virtual ~CStorage() {};

	int Init(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments)
	{
		// get executable name
		ExtractExecutableName(ppArguments[0]);

		// get userdir
		fs_storage_path(pApplicationName, m_aUserdir, sizeof(m_aUserdir));

		// get datadir
		FindDatadir(ppArguments[0]);

		// get currentdir
		if(!fs_getcwd(m_aCurrentdir, sizeof(m_aCurrentdir)))
			m_aCurrentdir[0] = 0;

		// load paths from storage.cfg
		LoadPaths(ppArguments[0]);

		if(!m_NumPaths)
		{
			dbg_msg("storage", "using standard paths");
			AddDefaultPaths();
		}

		// add save directories
		if(StorageType != STORAGETYPE_BASIC && m_NumPaths && (!m_aaStoragePaths[TYPE_SAVE][0] || !fs_makedir(m_aaStoragePaths[TYPE_SAVE])))
		{
			char aPath[MAX_PATH_LENGTH];
			if(StorageType == STORAGETYPE_CLIENT)
			{
				fs_makedir(GetPath(TYPE_SAVE, "screenshots", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "screenshots/auto", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "screenshots/auto/stats", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "maps", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "downloadedmaps", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "downloadedskins", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "identities", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "configs", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "presets", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "lua_sandbox", aPath, sizeof(aPath)));

				fs_makedir(GetPath(TYPE_SAVE, "logs", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "logs/ath_crashes", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "tmp", aPath, sizeof(aPath)));
				fs_makedir(GetPath(TYPE_SAVE, "tmp/cache", aPath, sizeof(aPath)));
			}
			fs_makedir(GetPath(TYPE_SAVE, "dumps", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "dumps/console_local", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "dumps/console_remote", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "dumps/memory", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "dumps/tilelayer", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "dumps/network", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "demos", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "demos/auto", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "editor", aPath, sizeof(aPath)));
			fs_makedir(GetPath(TYPE_SAVE, "ghosts", aPath, sizeof(aPath)));
		}

		return m_NumPaths ? 0 : 1;
	}

	void LoadPaths(const char *pArgv0)
	{
		// check current directory
		IOHANDLE File = io_open("storage.cfg", IOFLAG_READ);
		if(!File)
		{
			// check ATH root
			File = io_open(STORAGE_ATH_ROOT"/storage.cfg", IOFLAG_READ);
			if(!File)
			{
				// check usable path in argv[0]
				unsigned int Pos = ~0U;
				for(unsigned i = 0; pArgv0[i]; i++)
					if(pArgv0[i] == '/' || pArgv0[i] == '\\')
						Pos = i;
				if(Pos < MAX_PATH_LENGTH)
				{
					char aBuffer[MAX_PATH_LENGTH];
					str_copy(aBuffer, pArgv0, Pos+1);
					str_append(aBuffer, "/storage.cfg", sizeof(aBuffer));
					File = io_open(aBuffer, IOFLAG_READ);
				}

				if(Pos >= MAX_PATH_LENGTH || !File)
				{
					// check in edct dir
					File = io_open(STORAGE_EDTC_DIR"/storage.cfg", IOFLAG_READ);
					if(!File)
					{
						// couldn't find it anywhere, try to generate one
						dbg_msg("storage", "couldn't open storage.cfg, generating one");
						if(!GenerateStorageCfg())
						{
							dbg_msg("storage", "failed to generate storage.cfg");
							return;
						}
						File = io_open("storage.cfg", IOFLAG_READ);
						if(!File)
						{
							dbg_msg("storage", "couldn't open storage.cfg after generating it");
							return;
						}
					}
				}
			}
		}

		char *pLine;
		CLineReader LineReader;
		LineReader.Init(File);

		while((pLine = LineReader.Get()))
		{
			if(str_length(pLine) > 9 && !str_comp_num(pLine, "add_path ", 9))
				AddPath(pLine+9);
		}

		io_close(File);

		if(!m_NumPaths)
			dbg_msg("storage", "no paths found in storage.cfg");
	}

	void ExtractExecutableName(const char *pArgv0)
	{
		unsigned int Pos = ~0U;
		for(unsigned i = 0; pArgv0[i]; i++)
			if(pArgv0[i] == '/' || pArgv0[i] == '\\')
				Pos = i;
		if(Pos < MAX_PATH_LENGTH)
		{
			str_copy(m_aExecutableName, &pArgv0[Pos+1], sizeof(m_aExecutableName));
			dbg_msg("storage", "executable name is '%s'", m_aExecutableName);
		}
		else
		{
			dbg_msg("storage/error", "couldn't get executable name! pArgv0='%s' Pos=%u", pArgv0, Pos);
			dbg_msg("storage/error", "checking if argv[0] IS executable name...");
			if(str_comp_nocase_num(&pArgv0[str_length(pArgv0)-4], ".exe", 4) == 0)
			{
				dbg_msg("storage", "...yap, that worked!");
				str_copy(m_aExecutableName, pArgv0, sizeof(m_aExecutableName));
			}
			else
			{
				dbg_msg("storage", "...no, doesn't seem like. Defaulting to 'AllTheHaxx.exe'");
				str_copy(m_aExecutableName, "AllTheHaxx"
											#if defined(CONF_FAMILY_WINDOWS)
														".exe"
											#endif
											, sizeof(m_aExecutableName));
			}
		}
	}

	void AddDefaultPaths()
	{
		AddPath("$USERDIR");
		AddPath("$DATADIR");
		AddPath("$CURRENTDIR");
	}

	void AddPath(const char *pPath)
	{
		if(m_NumPaths >= MAX_PATHS || !pPath[0])
			return;

		if(!str_comp(pPath, "$USERDIR"))
		{
			if(m_aUserdir[0])
			{
				str_copy(m_aaStoragePaths[m_NumPaths++], m_aUserdir, MAX_PATH_LENGTH);
				dbg_msg("storage", "added path '$USERDIR' ('%s')", m_aUserdir);
			}
		}
		else if(!str_comp(pPath, "$DATADIR"))
		{
			if(m_aDatadir[0])
			{
				str_copy(m_aaStoragePaths[m_NumPaths++], m_aDatadir, MAX_PATH_LENGTH);
				dbg_msg("storage", "added path '$DATADIR' ('%s')", m_aDatadir);
			}
		}
		else if(!str_comp(pPath, "$CURRENTDIR"))
		{
			m_aaStoragePaths[m_NumPaths++][0] = 0;
			dbg_msg("storage", "added path '$CURRENTDIR' ('%s')", m_aCurrentdir);
		}
		else
		{
			if(fs_is_dir(pPath))
			{
				str_copy(m_aaStoragePaths[m_NumPaths++], pPath, MAX_PATH_LENGTH);
				dbg_msg("storage", "added path '%s'", pPath);
			}
		}
	}

	void FindDatadir(const char *pArgv0)
	{
		// 1) use data-dir in PWD if present
		if(fs_is_dir("data/mapres"))
		{
			str_copy(m_aDatadir, "data", sizeof(m_aDatadir));
			str_copy(m_aBinarydir, "", sizeof(m_aBinarydir));
			return;
		}

		// 2) use compiled-in data-dir if present
		if(fs_is_dir(STORAGE_DATA_DIR "/mapres"))
		{
			str_copy(m_aDatadir, STORAGE_DATA_DIR, sizeof(m_aDatadir));
			str_copy(m_aBinarydir, "", sizeof(m_aBinarydir));
			return;
		}

		// 3) check for usable path in argv[0]
		{
			unsigned int Pos = ~0U;
			for(unsigned i = 0; pArgv0[i]; i++)
				if(pArgv0[i] == '/' || pArgv0[i] == '\\')
					Pos = i;

			if(Pos < MAX_PATH_LENGTH)
			{
				char aBaseDir[MAX_PATH_LENGTH];
				str_copy(aBaseDir, pArgv0, Pos+1);
				str_copy(m_aBinarydir, aBaseDir, sizeof(m_aBinarydir));
				str_format(m_aDatadir, sizeof(m_aDatadir), "%s/data", aBaseDir);
				str_append(aBaseDir, "/data/mapres", sizeof(aBaseDir));

				if(fs_is_dir(aBaseDir))
					return;
				else
					m_aDatadir[0] = 0;
			}
		}

	#if defined(CONF_FAMILY_UNIX)
		// 4) check for all default locations
		{
			const char *aDirs[] = {
				"/usr/share/allthehaxx/data",
				"/usr/share/games/allthehaxx/data",
				"/usr/local/share/allthehaxx/data",
				"/usr/local/share/games/allthehaxx/data",
				"/usr/pkg/share/allthehaxx/data",
				"/usr/pkg/share/games/allthehaxx/data",
				"/opt/allthehaxx/data"
			};
			const int DirsCount = sizeof(aDirs) / sizeof(aDirs[0]);

			int i;
			for (i = 0; i < DirsCount; i++)
			{
				char aBuf[128];
				str_format(aBuf, sizeof(aBuf), "%s/mapres", aDirs[i]);
				if(fs_is_dir(aBuf))
				{
					str_copy(m_aBinarydir, aDirs[i], sizeof(aDirs[i])-5);
					str_copy(m_aDatadir, aDirs[i], sizeof(m_aDatadir));
					return;
				}
			}
		}
	#endif

		// no data-dir found
		dbg_msg("storage", "WARNING: no data directory found");
	}


	virtual void ListDirectoryInfo(int Type, const char *pPath, FS_LISTDIR_INFO_CALLBACK pfnCallback, void *pUser)
	{
		char aBuffer[MAX_PATH_LENGTH];
		if(Type == TYPE_ALL)
		{
			// list all available directories
			for(int i = 0; i < m_NumPaths; ++i)
				fs_listdir_info(GetPath(i, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, i, pUser);
		}
		else if(Type >= 0 && Type < m_NumPaths)
		{
			// list wanted directory
			fs_listdir_info(GetPath(Type, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, Type, pUser);
		}
	}

	virtual void ListDirectory(int Type, const char *pPath, FS_LISTDIR_CALLBACK pfnCallback, void *pUser)
	{
		char aBuffer[MAX_PATH_LENGTH];
		if(Type == TYPE_ALL)
		{
			// list all available directories
			for(int i = 0; i < m_NumPaths; ++i)
				fs_listdir(GetPath(i, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, i, pUser);
		}
		else if(Type >= 0 && Type < m_NumPaths)
		{
			// list wanted directory
			fs_listdir(GetPath(Type, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, Type, pUser);
		}
	}

	virtual int ListDirectoryVerbose(int Type, const char *pPath, FS_LISTDIR_CALLBACK_VERBOSE pfnCallback, void *pUser)
	{
		char aBuffer[MAX_PATH_LENGTH];
		int result = 0;
		if(Type == TYPE_ALL)
		{
			// list all available directories
			for(int i = 0; i < m_NumPaths; ++i)
				result = fs_listdir_verbose(GetPath(i, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, i, pUser);
		}
		else if(Type >= 0 && Type < m_NumPaths)
		{
			// list wanted directory
			result = fs_listdir_verbose(GetPath(Type, pPath, aBuffer, sizeof(aBuffer)), pfnCallback, Type, pUser);
		}
		else
			dbg_msg("storage", "tried to list directory '%s' with invalid type %i", pPath, Type);

		return result;
	}

	virtual const char *GetPath(int Type, const char *pDir, char *pBuffer, unsigned BufferSize) const
	{
		str_format(pBuffer, BufferSize, "%s%s%s", m_aaStoragePaths[Type], !m_aaStoragePaths[Type][0] ? "" : "/", pDir);
		return pBuffer;
	}

	virtual IOHANDLE_SMART OpenFileSmart(const char *pFilename, int Flags, int Type)
	{
		char aBuffer[MAX_PATH_LENGTH];
		IOHANDLE f = OpenFile(pFilename, Flags, Type, aBuffer, sizeof(aBuffer));
		IOHANDLE_SMART File(aBuffer, f);
		return File;
	}

	virtual IOHANDLE OpenFile(const char *pFilename, int Flags, int Type, char *pBuffer = 0, int BufferSize = 0)
	{
		char aBuffer[MAX_PATH_LENGTH];
		if(!pBuffer)
		{
			pBuffer = aBuffer;
			BufferSize = sizeof(aBuffer);
		}

		if(Type == TYPE_ABSOLUTE)
		{
			return io_open(pFilename, Flags);
		}
		if(str_comp_num(pFilename, "mapres/../skins/", 16) == 0) {
			pFilename = pFilename + 10; // just start from skins/
		}
		if(pFilename[0] == '/' || pFilename[0] == '\\' || str_find(pFilename, "../") != NULL || str_find(pFilename, "..\\") != NULL
		#ifdef CONF_FAMILY_WINDOWS
			|| (pFilename[0] && pFilename[1] == ':')
		#endif
		)
		{
			// don't escape base directory
		}
		else if(Flags&IOFLAG_WRITE)
		{
			return io_open(GetPath(TYPE_SAVE, pFilename, pBuffer, BufferSize), Flags);
		}
		else
		{
			IOHANDLE Handle = 0;

			if(Type <= TYPE_ALL)
			{
				// check all available directories
				for(int i = 0; i < m_NumPaths; ++i)
				{
					Handle = io_open(GetPath(i, pFilename, pBuffer, BufferSize), Flags);
					if(Handle)
						return Handle;
				}
			}
			else if(Type >= 0 && Type < m_NumPaths)
			{
				// check wanted directory
				Handle = io_open(GetPath(Type, pFilename, pBuffer, BufferSize), Flags);
				if(Handle)
					return Handle;
			}
		}

		pBuffer[0] = 0;
		return 0;
	}

	struct CFindCBData
	{
		CStorage *pStorage;
		const char *pFilename;
		const char *pPath;
		char *pBuffer;
		int BufferSize;
	};

	static int FindFileCallback(const char *pName, int IsDir, int Type, void *pUser)
	{
		CFindCBData Data = *static_cast<CFindCBData *>(pUser);
		if(IsDir)
		{
			if(pName[0] == '.')
				return 0;

			// search within the folder
			char aBuf[MAX_PATH_LENGTH];
			char aPath[MAX_PATH_LENGTH];
			str_format(aPath, sizeof(aPath), "%s/%s", Data.pPath, pName);
			Data.pPath = aPath;
			fs_listdir(Data.pStorage->GetPath(Type, aPath, aBuf, sizeof(aBuf)), FindFileCallback, Type, &Data);
			if(Data.pBuffer[0])
				return 1;
		}
		else if(!str_comp(pName, Data.pFilename))
		{
			// found the file = end
			str_format(Data.pBuffer, Data.BufferSize, "%s/%s", Data.pPath, Data.pFilename);
			return 1;
		}

		return 0;
	}

	virtual bool FindFile(const char *pFilename, const char *pPath, int Type, char *pBuffer, int BufferSize)
	{
		if(BufferSize < 1)
			return false;

		pBuffer[0] = 0;
		char aBuf[MAX_PATH_LENGTH];
		CFindCBData Data;
		Data.pStorage = this;
		Data.pFilename = pFilename;
		Data.pPath = pPath;
		Data.pBuffer = pBuffer;
		Data.BufferSize = BufferSize;

		if(Type == TYPE_ALL)
		{
			// search within all available directories
			for(int i = 0; i < m_NumPaths; ++i)
			{
				fs_listdir(GetPath(i, pPath, aBuf, sizeof(aBuf)), FindFileCallback, i, &Data);
				if(pBuffer[0])
					return true;
			}
		}
		else if(Type >= 0 && Type < m_NumPaths)
		{
			// search within wanted directory
			fs_listdir(GetPath(Type, pPath, aBuf, sizeof(aBuf)), FindFileCallback, Type, &Data);
		}

		return pBuffer[0] != 0;
	}

	virtual bool RemoveFile(const char *pFilename, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;

		char aBuffer[MAX_PATH_LENGTH];
		return !fs_remove(GetPath(Type, pFilename, aBuffer, sizeof(aBuffer)));
	}

	virtual bool RemoveBinaryFile(const char *pFilename)
	{
		char aBuffer[MAX_PATH_LENGTH];
		return !fs_remove(GetBinaryPath(pFilename, aBuffer, sizeof(aBuffer)));
	}

	virtual bool RenameFile(const char* pOldFilename, const char* pNewFilename, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;
		char aOldBuffer[MAX_PATH_LENGTH];
		char aNewBuffer[MAX_PATH_LENGTH];
		return !fs_rename(GetPath(Type, pOldFilename, aOldBuffer, sizeof(aOldBuffer)), GetPath(Type, pNewFilename, aNewBuffer, sizeof (aNewBuffer)));
	}

	virtual bool RenameBinaryFile(const char* pOldFilename, const char* pNewFilename)
	{
		char aOldBuffer[MAX_PATH_LENGTH];
		char aNewBuffer[MAX_PATH_LENGTH];

		GetBinaryPath(pOldFilename, aOldBuffer, sizeof(aOldBuffer));
		GetBinaryPath(pNewFilename, aNewBuffer, sizeof(aNewBuffer));

		if(fs_makedir_rec_for(aNewBuffer) < 0)
			dbg_msg("storage", "cannot create folder for: %s", aNewBuffer);

		return !fs_rename(aOldBuffer, aNewBuffer);
	}

	virtual bool CreateFolder(const char *pFoldername, int Type)
	{
		if(Type < 0 || Type >= m_NumPaths)
			return false;

		char aBuffer[MAX_PATH_LENGTH];
		return !fs_makedir(GetPath(Type, pFoldername, aBuffer, sizeof(aBuffer)));
	}

#if !defined(BUILD_TOOLS)
	virtual bool CreateFolderLua(const char *pFoldername, lua_State *L)
	{
		if(g_StealthMode)
			return false;

		char aBuf[MAX_PATH_LENGTH];
		str_copyb(aBuf, pFoldername);
		CLuaBinding::SandboxPath(aBuf, sizeof(aBuf), L);
		char aFullPath[MAX_PATH_LENGTH];
		GetCompletePath(TYPE_SAVE, aBuf, aFullPath, sizeof(aFullPath));

		str_appendb(aFullPath, "/file"); // dummy file to satisfy fs_makedir_rec_for
		return fs_makedir_rec_for(aFullPath) == 0;
	}
#endif

	virtual const char *GetCompletePath(int Type, const char *pDir, char *pBuffer, unsigned BufferSize)
	{
		if(Type < 0 || Type >= m_NumPaths)
		{
			if(BufferSize > 0)
				pBuffer[0] = 0;
			return pBuffer;
		}

		return GetPath(Type, pDir, pBuffer, BufferSize);
	}

	virtual const char* GetBinaryPath(const char *pDir, char *pBuffer, unsigned BufferSize)
	{
		str_format(pBuffer, BufferSize, "%s%s%s", m_aBinarydir, !m_aBinarydir[0] ? "" : "/", pDir);
		return pBuffer;
	}

	const char* GetExecutableName() const
	{
		return m_aExecutableName;
	}

	virtual const char *GetAppdataPath()
	{
		static char aAppdataPath[512] = {0};
		if(aAppdataPath[0] == '\0')
			GetCompletePath(IStorageTW::TYPE_SAVE, "", aAppdataPath, sizeof(aAppdataPath));
		return aAppdataPath;
	}

	bool GenerateStorageCfg() const
	{
		IOHANDLE file = io_open("storage.cfg", IOFLAG_WRITE);
		if(!file)
			return false;

			#define write_line(line) io_write(file, line, sizeof(line)-1); io_write_newline(file)

		write_line("####");
		write_line("# This specifies where and in which order Teeworlds looks");
		write_line("# for its data (sounds, skins, ...). The search goes top");
		write_line("# down which means the first path has the highest priority.");
		write_line("# Furthermore the top entry also defines the save path where");
		write_line("# all data (settings.cfg, screenshots, ...) are stored.");
		write_line("# There are 3 special paths available:");
		write_line("#    $USERDIR");
		write_line("#    - ~/.appname on UNIX based systems");
		write_line("#    - ~/Library/Applications Support/appname on Mac OS X");
		write_line("#    - %APPDATA%/Appname on Windows based systems");
		write_line("#    $DATADIR");
		write_line("#    - the 'data' directory which is part of an official");
		write_line("#    release");
		write_line("#    $CURRENTDIR");
		write_line("#    - current working directory");
		write_line("#");
		write_line("#");
		write_line("# The default file has the following entries:");
		write_line("#    add_path $USERDIR");
		write_line("#    add_path $DATADIR");
		write_line("#    add_path $CURRENTDIR");
		write_line("#");
		write_line("# A customised one could look like this:");
		write_line("#    add_path user");
		write_line("#    add_path mods/mymod");
		write_line("####");
		write_line("");
		write_line("add_path $USERDIR");
		write_line("add_path $DATADIR");
		write_line("add_path $CURRENTDIR");

		#undef write_line

		io_close(file);

		return true;
	}

	const char *SandboxPath(char *pInOutBuffer, unsigned BufferSize, const char *pPrepend, bool ForcePrepend) const
	{
		if(dbg_assert_strict(BufferSize > 0, "SandboxPath: zero-size buffer?!"))
			return NULL;

		// replace all backslashes with forward slashes
		for(char *p = pInOutBuffer; p < pInOutBuffer+BufferSize && *p; p++)
			if(*p == '\\')
				*p = '/';

		// don't allow entering the root directory / another partition
		{
			#if defined(CONF_FAMILY_UNIX)
			char *p = pInOutBuffer;
			while(p[0] == '/') p++;
			if(p != pInOutBuffer)
			{
				char aTmp[512];
				str_copyb(aTmp, p);
				str_copy(pInOutBuffer, aTmp, (int)BufferSize);
			}
			#elif defined(CONF_FAMILY_WINDOWS)
			const char *p = str_find_rev(pInOutBuffer, ":");
		if(p)
		{
			char aTmp[512];
			str_copyb(aTmp, p+1);
			str_copy(pInOutBuffer, aTmp, (int)BufferSize);
		}
			#endif
		}

		// split it into pieces
		std::vector<std::string> PathStack;
		StringSplit(pInOutBuffer, "/", &PathStack);

		// reassemble and prettify it
		std::vector<std::string> FinalResult;
		for(std::vector<std::string>::iterator it = PathStack.begin(); it != PathStack.end(); it++)
		{
			if(*it == "..")
			{
				if(!FinalResult.empty())
					FinalResult.pop_back();
			}
			else if(it->length() > 0 && *it != ".")
				FinalResult.push_back(*it);
		}

		pInOutBuffer[0] = '\0';
		if(pPrepend)
		{
			if(ForcePrepend || fs_compare(FinalResult[0].c_str(), pPrepend) != 0)
			{
				str_copy(pInOutBuffer, pPrepend, BufferSize);
				if(pPrepend[str_length(pPrepend)-1] != '/')
					str_append(pInOutBuffer, "/", BufferSize);
			}
		}

		for(std::vector<std::string>::iterator it = FinalResult.begin(); it != FinalResult.end(); it++)
			str_append(pInOutBuffer, (*it + std::string("/")).c_str(), BufferSize);
		pInOutBuffer[str_length(pInOutBuffer)-1] = '\0'; // remove the trailing slash

		return pInOutBuffer;
	}

	const char *GetFullPath(const char *pFilename, int Type, char *pBuffer, unsigned BufferSize) const
	{
		if(Type <= TYPE_ALL)
		{
			// check all available directories
			for(int i = 0; i < m_NumPaths; ++i)
			{
				GetPath(i, pFilename, pBuffer, BufferSize);
				if(fs_exists(pBuffer))
					return pBuffer;
			}
		}
		else if(Type >= 0 && Type < m_NumPaths)
		{
			// check wanted directory
			GetPath(Type, pFilename, pBuffer, BufferSize);
			return pBuffer;
		}

		pBuffer[0] = '\0';
		return pBuffer;
	}

	static IStorageTW *Create(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments)
	{
		CStorage *p = new CStorage();
		if(p && p->Init(pApplicationName, StorageType, NumArgs, ppArguments))
		{
			dbg_msg("storage", "initialisation failed");
			delete p;
			p = 0;
		}
		return p;
	}
};

IStorageTW *CreateStorage(const char *pApplicationName, int StorageType, int NumArgs, const char **ppArguments) { return CStorage::Create(pApplicationName, StorageType, NumArgs, ppArguments); }

IStorageTW *CreateLocalStorage()
{
	CStorage *pStorage = new CStorage();
	if(pStorage)
	{
		if(!fs_getcwd(pStorage->m_aCurrentdir, sizeof(pStorage->m_aCurrentdir)))
		{
			delete pStorage;
			return NULL;
		}
		pStorage->AddPath("$CURRENTDIR");
	}
	return pStorage;
}
