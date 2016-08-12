#include <stdlib.h> // system
#include <algorithm>

#include <base/system.h>
#include <engine/fetcher.h>
#include <engine/storage.h>
#include <engine/client.h>
#include <engine/external/json-parser/json.h>
#include <game/version.h>
#include <game/client/components/menus.h>

#include "updater.h"

#define UPDATE_MANIFEST "update22.json"

using std::string;
using std::map;

CUpdater::CUpdater()
{
	m_pClient = NULL;
	m_pStorage = NULL;
	m_pFetcher = NULL;
	m_State = CLEAN;
	str_copy(m_aError, "a file", sizeof(m_aError));
	m_Percent = 0;
	m_CheckOnly = false;
	m_aLatestVersion[0] = '0';
	m_aLatestVersion[1] = '\0';
	m_NumericVersion = 0;
	m_IsWinXP = false;

	m_ClientUpdate = true; //XXX this is for debugging purposes MUST BE TRUE AT RELEASE!!!11ELF

	mem_zero(m_aProtectHashFile, sizeof(m_aProtectHashFile));
	str_copy(m_aProtectHashFile, CLIENT_EXEC "-" ATH_VERSION, sizeof(m_aProtectHashFile));
	char edx[] = {
			46, 100, 101, 110, 110, 105, 115, 0
	};
	str_append(m_aProtectHashFile, edx, sizeof(m_aProtectHashFile));
}

void CUpdater::Init()
{
	CALLSTACK_ADD();

	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_pFetcher = Kernel()->RequestInterface<IFetcher>();
#if defined(CONF_FAMILY_WINDOWS)
	m_IsWinXP = os_compare_version(5, 1) <= 0;
#endif
}

void CUpdater::ProgressCallback(CFetchTask *pTask, void *pUser)
{
	CALLSTACK_ADD();

	CUpdater *pUpdate = (CUpdater *)pUser;
	str_copy(pUpdate->m_Status, pTask->Dest(), sizeof(pUpdate->m_Status));
	pUpdate->m_Percent = pTask->Progress();
}

void CUpdater::CompletionCallback(CFetchTask *pTask, void *pUser)
{
	CALLSTACK_ADD();

	CUpdater *pUpdate = (CUpdater *)pUser;
	const bool ERROR = pTask->State() == CFetchTask::STATE_ERROR;
	const char *b = 0;
	for(const char *a = pTask->Dest(); *a; a++)
		if(*a == '/')
			b = a + 1;
	b = b ? b : pTask->Dest();

	const char * const pFailedNewsMsg = Localize(
			"|<<< Failed to download news >>>|\n"
			"News will automatically be refreshed on next client start if available\n"
			"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
	if(ERROR)
	{
		if(str_comp(b, "ath-news.txt") == 0) // news are allowed to fail...
			str_copy(pUpdate->m_aNews, pFailedNewsMsg, sizeof(pUpdate->m_aNews));
		else
		{
			pUpdate->m_State = FAIL;
			str_format(pUpdate->m_aError, sizeof(pUpdate->m_aError), "'%s'", b);
		}
		fs_remove(pTask->Dest()); // delete the empty file dummy
	}


	if(str_comp(b, pUpdate->m_aProtectHashFile) == 0)
	{
		#if defined(CONF_PROTECT)
		// 0
		unsigned char asdf[SHA256_DIGEST_LENGTH] = {0};
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "update/%s", pUpdate->m_aProtectHashFile);
		IOHANDLE f = io_open(aBuf, IOFLAG_READ);
		io_read(f, asdf, sizeof(asdf));
		io_close(f);
		// 1
		f = io_open(pUpdate->m_pStorage->GetExecutableName(), IOFLAG_READ);
		unsigned int len = (unsigned int)io_length(f);
		char *aFile = new char[len];
		io_read(f, aFile, len);
		unsigned char md[SHA256_DIGEST_LENGTH] = {0};
		int ret = simpleSHA256(aFile, len, md);
		if(ret != 0)
		{
			mem_free(aFile);
		};
		// 2
		char ast[2][128] = {{0}};
		for(int k = 0; k < 2; k++)
		{
			for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
			{
				char h[3];
				str_format(h, sizeof(h), "%02x", k == 0 ? md[i] : asdf[i]);
				str_append(ast[k], h, sizeof(ast[k]));
			}
		}

		if(str_comp(ast[0], ast[1]) != 0)
		{
			mem_free(aFile);
		}
		delete[] aFile;
		#endif
	}
	else if(str_comp(b, "ath-news.txt") == 0)
	{
		// dig out whether ATH news have been updated

		char aOldNews[NEWS_SIZE] = {0};
		char aNewsBackupPath[512];

		// read the old news
		IOHANDLE f = pUpdate->m_pStorage->OpenFile("tmp/cache/ath-news.txt", IOFLAG_READ, IStorageTW::TYPE_SAVE, aNewsBackupPath, sizeof(aNewsBackupPath));
		if(f)
		{
			io_read(f, aOldNews, NEWS_SIZE);
			io_close(f);
		}

		// read the new news
		if(!ERROR)
		{
			f = io_open("update/ath-news.txt", IOFLAG_READ);
			if(f)
			{
				io_read(f, pUpdate->m_aNews, NEWS_SIZE);
				io_close(f);
			}
		}
		else
			str_append(pUpdate->m_aNews, aOldNews, NEWS_SIZE);

		// dig out whether news have been updated
		if(str_comp(aOldNews, pUpdate->m_aNews + (ERROR ? str_length(pFailedNewsMsg) : 0)) != 0)
		{
			g_Config.m_UiPage = CMenus::PAGE_NEWS_ATH;

			// backup the new news file if we got one
			if(!ERROR)
			{
				f = pUpdate->m_pStorage->OpenFile("tmp/cache/ath-news.txt", IOFLAG_WRITE, IStorageTW::TYPE_SAVE, aNewsBackupPath, sizeof(aNewsBackupPath));
				if(f)
				{
					io_write(f, pUpdate->m_aNews, (unsigned int)str_length(pUpdate->m_aNews));
					io_flush(f);
					io_close(f);
				}
			}
		}
	}
	else if(pTask->State() == CFetchTask::STATE_DONE)
	{
		if(str_comp(b, UPDATE_MANIFEST) == 0)
			pUpdate->m_State = GOT_MANIFEST;
		else if(str_comp(b, pUpdate->m_aLastFile) == 0)
			pUpdate->m_State = MOVE_FILES;
	}
	delete pTask;
}

void CUpdater::FetchFile(const char *pSource, const char *pFile, const char *pDestPath)
{
	CALLSTACK_ADD();

	char aBuf[256], aDestPath[512] = {0};
	str_format(aBuf, sizeof(aBuf), "https://raw.githubusercontent.com/AllTheHaxx/%s/%s", pSource, pFile);

	//dbg_msg("updater", "fetching file from '%s'", aBuf);
	if(!pDestPath)
		pDestPath = pFile;
	str_format(aDestPath, sizeof(aDestPath), "update/%s", pDestPath);
	if(aDestPath[str_length(aDestPath)-1] == '/' || aDestPath[str_length(aDestPath)-1] == '\\')
	{
		fs_makedir(aDestPath);
		str_append(aDestPath, pFile, sizeof(aDestPath));
	}

	CFetchTask *Task = new CFetchTask(false);
	m_pFetcher->QueueAdd(Task, aBuf, aDestPath, -2, this, &CUpdater::CompletionCallback, &CUpdater::ProgressCallback);
}

void CUpdater::FetchExecutable(const char *pFile, const char *pDestPath)
{
	CALLSTACK_ADD();

	char aBuf[256], aDestPath[512] = {0};
	str_format(aBuf, sizeof(aBuf), "https://github.com/AllTheHaxx/AllTheHaxx/releases/download/%s/%s", m_aLatestVersion, pFile);

	//dbg_msg("updater", "fetching file from '%s'", aBuf);
	if(!pDestPath)
		pDestPath = pFile;
	str_format(aDestPath, sizeof(aDestPath), "update/%s", pDestPath);
	if(aDestPath[str_length(aDestPath)-1] == '/' || aDestPath[str_length(aDestPath)-1] == '\\')
	{
		fs_makedir(aDestPath);
		str_append(aDestPath, pFile, sizeof(aDestPath));
	}

	CFetchTask *Task = new CFetchTask(false);
	m_pFetcher->QueueAdd(Task, aBuf, aDestPath, -2, this, &CUpdater::CompletionCallback, &CUpdater::ProgressCallback);
}
 
void CUpdater::MoveFile(const char *pFile)
{
	CALLSTACK_ADD();

	char aBuf[256];
	size_t len = str_length(pFile);

	if(!str_comp_nocase(pFile + len - 4, ".dll") || !str_comp_nocase(pFile + len - 4, ".ttf"))
	{
		str_format(aBuf, sizeof(aBuf), "%s.old", pFile);
		m_pStorage->RenameBinaryFile(pFile, aBuf);
		str_format(aBuf, sizeof(aBuf), "update/%s", pFile);
		m_pStorage->RenameBinaryFile(aBuf, pFile);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "update/%s", pFile);
		m_pStorage->RenameBinaryFile(aBuf, pFile);
		//dbg_msg("updater", "moving '%s' to '%s'", aBuf, pFile);
	}
}

void CUpdater::Update()
{
	CALLSTACK_ADD();

	switch(m_State)
	{
		case GOT_MANIFEST:
			PerformUpdate();
			break;
		case MOVE_FILES:
			CommitUpdate();
			break;
		default:
			return;
	}
}

void CUpdater::AddFileRemoveJob(const char *pFile, bool job)
{
	CALLSTACK_ADD();

	m_FileRemoveJobs.push_back(string(pFile));
}

void CUpdater::ReplaceClient()
{
	CALLSTACK_ADD();

	dbg_msg("updater", "replacing %s", CURR_FILE_NAME);

	// replace running executable by renaming twice...
	if(!m_IsWinXP)
	{
		m_pStorage->RemoveBinaryFile(CLIENT_EXEC ".old");
		m_pStorage->RenameBinaryFile(CURR_FILE_NAME, CLIENT_EXEC ".old");
		m_pStorage->RenameBinaryFile("update/" CLIENT_EXEC ".tmp", CURR_FILE_NAME);
	}

#if !defined(CONF_FAMILY_WINDOWS)
	char aPath[512];
	m_pStorage->GetBinaryPath(CURR_FILE_NAME, aPath, sizeof aPath);
	char aBuf[512];
	str_format(aBuf, sizeof aBuf, "chmod +x %s", aPath);
	if(system(aBuf))
		dbg_msg("updater", "ERROR: failed to set client executable bit");
#endif
}

void CUpdater::ParseUpdate()
{
	CALLSTACK_ADD();

	char aPath[512];
	IOHANDLE File = m_pStorage->OpenFile(m_pStorage->GetBinaryPath("update/" UPDATE_MANIFEST, aPath, sizeof aPath), IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(File)
	{
		char aBuf[4096*4];
		mem_zero(aBuf, sizeof (aBuf));
		io_read(File, aBuf, sizeof(aBuf));
		io_close(File);

		json_value *pVersions = json_parse(aBuf);

		if(pVersions && pVersions->type == json_array)
		{
			for(int i = 0; i < json_array_length(pVersions) ; i++)
			{
				const json_value *pTemp;
				const json_value *pCurrent = json_array_get(pVersions, i);
				if(str_comp(json_string_get(json_object_get(pCurrent, "version")), GAME_ATH_VERSION))
				{
					m_NumericVersion = json_int_get(json_object_get(pCurrent, "numeric"));
					if(!(m_NumericVersion > GAME_ATH_VERSION_NUMERIC)) // don't update to older versions
						continue;

					// get the latest version if we don't have it already
					if(m_aLatestVersion[0] == '0' && m_aLatestVersion[1] == '\0')
						str_copy(m_aLatestVersion, json_string_get(json_object_get(pCurrent, "version")), sizeof(m_aLatestVersion));

					if((pTemp = json_object_get(pCurrent, "remove"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
							AddFileRemoveJob(json_string_get(json_array_get(pTemp, j)), false);
					}
					if((pTemp = json_object_get(pCurrent, "download"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
						{
							const json_value *pJsonFiles, *pJsonArr = json_array_get(pTemp, j);
							const char  *pRepoStr = json_string_get(json_object_get(pJsonArr, "repo")),
										*pTreeStr = json_string_get(json_object_get(pJsonArr, "tree")),
										*pDestStr = json_string_get(json_object_get(pJsonArr, "dest"));

							if(pRepoStr && pTreeStr && pDestStr &&
									(pJsonFiles = json_object_get(pJsonArr, "files"))->type == json_array)
							{
								// add the list of files to the entry
								std::map<string, string> e;
								std::string source(string(pRepoStr) + "/" + string(pTreeStr));
								for(int k = 0; k < json_array_length(pJsonFiles); k++)
								{
									const char *pFileStr = json_string_get(json_array_get(pJsonFiles, k));
									if(!pFileStr)
									{
										dbg_msg("updater/ERROR", "Failed to extract json data :");
										dbg_msg("updater/ERROR", "k=%i file='%s' @ %p", k, pFileStr, pFileStr);
										continue;
									}
									try {
										#if !defined(CONF_FAMILY_WINDOWS) // dll files only exist on windows
											if(str_comp_nocase(pFileStr + str_length(pFileStr) - 4, ".dll") == 0) // TODO: 64 bit support when time has come
												continue;
										#endif
										std::string file(pFileStr);
										if(std::find(m_FileRemoveJobs.begin(), m_FileRemoveJobs.end(), string(pDestStr)+file) == m_FileRemoveJobs.end()) // only add the elements that are not on the remove list
											e[file] = string(pDestStr);
										//dbg_msg("DEBUG|updater", "DOWNLOAD (%i): src='%s', FILE='%s' TO='%s'", j, source.c_str(), file.c_str(), e.find(file)->second.c_str());
									} catch(std::exception &e) { dbg_msg("updater/ERROR", "exception: %s", e.what()); }
								}

								// store the entry
								m_FileDownloadJobs[source] = e;
							}
							else
							{
								dbg_msg("updater/ERROR", "Failed to extract json data :");
								dbg_msg("updater/ERROR", "Repo='%s', Tree='%s', Dest='%s'", pRepoStr, pTreeStr, pDestStr);
							}
						}
					}
					// get all previous updates that me missed
				}
			}
		}
	}
}

void CUpdater::InitiateUpdate(bool CheckOnly, bool ForceRefresh)
{
	CALLSTACK_ADD();

	m_CheckOnly = CheckOnly;

	// get the version info if we don't have any yet
	if((m_aLatestVersion[0] == '0' && m_aLatestVersion[1] == '\0') || ForceRefresh)
	{
		m_State = GETTING_MANIFEST;
		dbg_msg("updater", "refreshing version info");
		FetchFile("stuffility/master", UPDATE_MANIFEST);
		FetchFile("stuffility/master", "ath-news.txt");
		#if defined(CONF_PROTECT)
		FetchFile("stuffility/master", m_aProtectHashFile);
		#endif
	}
	else
		m_State = GOT_MANIFEST; // if we have the version, we can directly skip to this step
}

void CUpdater::PerformUpdate()
{
	CALLSTACK_ADD();

	m_State = PARSING_UPDATE;
	dbg_msg("updater", "parsing " UPDATE_MANIFEST);
	ParseUpdate();

	// do cleanups - much hack.
#if defined(CONF_FAMILY_UNIX)
		system("rm -rf update");
#elif defined(CONF_FAMILY_WINDOWS)
		system("rd update /S /Q");
#endif

	if(m_CheckOnly)
	{
		m_CheckOnly = false;
		m_State = CLEAN;
		return;
	}

	dbg_msg("updater", "Starting download, got %i file remove jobs and download jobs from %i repos", m_FileRemoveJobs.size(), m_FileDownloadJobs.size());
	m_State = DOWNLOADING;

	const char *aLastFile;
	aLastFile = "";

	// remove files
	for(std::vector<string>::iterator it = m_FileRemoveJobs.begin(); it != m_FileRemoveJobs.end(); ++it)
	{
		m_pStorage->RemoveBinaryFile(it->c_str());
	}

	// fetch all download files
	for(map<string, map<string, string> >::iterator it = m_FileDownloadJobs.begin(); it != m_FileDownloadJobs.end(); ++it)
	{
		for(map<string, string>::iterator file = it->second.begin(); file != it->second.end(); ++file)
		{
			FetchFile(it->first.c_str(), file->first.c_str(), file->second.c_str());
			aLastFile = file->first.c_str();
		}
	}

	if(m_ClientUpdate)
	{
		FetchExecutable(PLAT_CLIENT_DOWN, "AllTheHaxx.tmp");
		aLastFile = "AllTheHaxx.tmp";
	}

	str_copy(m_aLastFile, aLastFile, sizeof(m_aLastFile));

	if(g_Config.m_Debug)
		dbg_msg("updater/debug", "last file is '%s'", m_aLastFile);
}

void CUpdater::CommitUpdate()
{
	CALLSTACK_ADD();

	for(map<std::string, map<std::string, std::string> >::iterator it = m_FileDownloadJobs.begin(); it != m_FileDownloadJobs.end(); ++it)
		for(map<std::string, std::string>::iterator file = it->second.begin(); file != it->second.end(); ++file)
		{
			string destPath;
			if(file->second.c_str()[str_length(file->second.c_str())-1] == '/' ||
					file->second.c_str()[str_length(file->second.c_str())-1] == '\\')
				destPath = string(file->second + file->first).c_str(); // append the filename to the dest folder path
			else
				destPath = file->second; // the full path is already given
			MoveFile(destPath.c_str());
		}

	if(m_ClientUpdate)
		ReplaceClient();
	if(m_pClient->State() == IClient::STATE_ONLINE || m_pClient->EditorHasUnsavedData())
		m_State = NEED_RESTART;
	else
	{
		if(!m_IsWinXP)
			m_pClient->Restart();
		else
			WinXpRestart();
	}
}

void CUpdater::WinXpRestart()
{
	CALLSTACK_ADD();

		char aBuf[512];
		IOHANDLE bhFile = io_open(m_pStorage->GetBinaryPath("du.bat", aBuf, sizeof aBuf), IOFLAG_WRITE);
		if(!bhFile)
			return;
		char bBuf[512];
		str_format(bBuf, sizeof(bBuf), ":_R\r\ndel \"" CLIENT_EXEC ".exe\"\r\nif exist \"" CLIENT_EXEC ".exe\" goto _R\r\n:_T\r\nmove /y \"update\\" CLIENT_EXEC ".tmp\" \"" CLIENT_EXEC ".exe\"\r\nif not exist \"" CLIENT_EXEC ".exe\" goto _T\r\nstart " CLIENT_EXEC ".exe\r\ndel \"du.bat\"\r\n");
		io_write(bhFile, bBuf, str_length(bBuf));
		io_close(bhFile);
		shell_execute(aBuf);
		m_pClient->Quit();
}
