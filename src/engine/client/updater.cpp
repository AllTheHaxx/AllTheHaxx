#include <stdlib.h> // system
#include <algorithm>

#include <base/system.h>
#include <engine/fetcher.h>
#include <engine/storage.h>
#include <engine/client.h>
#include <engine/external/json-parser/json.hpp>
#include <game/version.h>
#include <game/client/components/menus.h>

#include "updater.h"

#define UPDATE_MANIFEST "update30.json"

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
	m_IsWinXP = false;

	m_ClientUpdate = true; //XXX this is for debugging purposes MUST BE TRUE AT RELEASE!!!11ELF
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

	const char *a = 0;
	for(const char *c = pTask->Dest(); *c; c++)
		if(*c == '/')
			{ a = c + 1; break; }
	a = a ? a : pTask->Dest();

		const char *b = 0;
	for(const char *c = pTask->Dest(); *c; c++)
		if(*c == '/')
			b = c + 1;
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
			if(str_comp_nocase_num(a, "lua/", 4) != 0)
			{
				pUpdate->m_State = FAIL;
				str_format(pUpdate->m_aError, sizeof(pUpdate->m_aError), "'%s'", a);
			}
			dbg_msg("update", "failed to download '%s'", a);
		}
		fs_remove(pTask->Dest()); // delete the empty file dummy
	}

	if(str_comp(b, "ath-news.txt") == 0)
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
	str_format(aBuf, sizeof(aBuf), "https://github.com/AllTheHaxx/AllTheHaxx/releases/download/%s/%s", GetLatestVersion(), pFile);

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
	int len = str_length(pFile);

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

void CUpdater::AddFileRemoveJob(const char *pFile)
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

	// this function shall only be called when actually updating the client

	char aPath[512];
	IOHANDLE File = m_pStorage->OpenFile(m_pStorage->GetBinaryPath("update/" UPDATE_MANIFEST, aPath, sizeof aPath), IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(File)
	{
		char aBuf[4096*4];
		mem_zero(aBuf, sizeof (aBuf));
		io_read(File, aBuf, sizeof(aBuf));
		io_close(File);

		json_value &jsonVersions = *json_parse(aBuf, (size_t)str_length(aBuf)); // TODO: free this!
		if(jsonVersions.type != json_array)
		{
			dbg_msg("updater/parse", "invalid manifest contents (not an array)");
			return;
		}

		for(unsigned int i = 0; i < jsonVersions.u.array.length ; i++)
		{
			const json_value &jsonCurrent = jsonVersions[i];

			// downgrades no gud
			if((json_int_t)(jsonCurrent["numeric"]) <= GAME_ATH_VERSION_NUMERIC)
				continue;

			if(jsonCurrent["remove"].type == json_array)
			{
				for(unsigned int j = 0; j < jsonCurrent["remove"].u.array.length; j++)
					AddFileRemoveJob(jsonCurrent["remove"][j]);
			}
			if(jsonCurrent["download"].type == json_array)
			{
				for(unsigned int j = 0; j < jsonCurrent["download"].u.array.length; j++)
				{
					const json_value &jsonRepoBatch = jsonCurrent["download"][j];
					const char  *pRepoStr = (const char *)jsonRepoBatch["repo"],
								*pTreeStr = (const char *)jsonRepoBatch["tree"],
								*pDestStr = (const char *)jsonRepoBatch["dest"];

					const json_value &jsonFilesArray = jsonRepoBatch["files"];
					if(jsonFilesArray.type == json_array)
					{
						// add the list of files to the entry
						std::map<string, string> e;
						std::string source(string(pRepoStr) + "/" + string(pTreeStr));
						for(unsigned int k = 0; k < jsonFilesArray.u.array.length; k++)
						{
							const char *pFileStr = jsonFilesArray[k];
							if(!pFileStr)
							{
								dbg_msg("updater/ERROR", "Failed to extract json data :");
								dbg_msg("updater/ERROR", "k=%i file='%s' @ %p", k, pFileStr, (void *)pFileStr);
								continue;
							}
							try {
								#if !defined(CONF_FAMILY_WINDOWS) // dll files only exist on windows
									if(str_comp_nocase(pFileStr + str_length(pFileStr) - 4, ".dll") == 0) // TODO: 64 bit support when time has come
										continue;
								#endif
								std::string FilePath(pFileStr);
								// only add the elements that are not on the remove list
								if(std::find(m_FileRemoveJobs.begin(), m_FileRemoveJobs.end(), string(pDestStr)+FilePath) == m_FileRemoveJobs.end())
									e[FilePath] = string(pDestStr);
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

void CUpdater::InitiateUpdate(bool CheckOnly, bool ForceRefresh)
{
	CALLSTACK_ADD();

	m_CheckOnly = CheckOnly;

	// get the version info if we don't have any yet
	if((GetLatestVersion()[0] == '0' && GetLatestVersion()[1] == '\0') || ForceRefresh)
	{
		m_State = GETTING_MANIFEST;
		dbg_msg("updater", "refreshing version info");
//		FetchFile("stuffility/master", UPDATE_MANIFEST);
		FetchFile("stuffility/master", "ath-news.txt");
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

	dbg_msg("updater", "Starting download, got %lu file remove jobs and download jobs from %lu repos", m_FileRemoveJobs.size(), m_FileDownloadJobs.size());
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
		io_write(bhFile, bBuf, (unsigned int)str_length(bBuf));
		io_close(bhFile);
		shell_execute(aBuf);
		m_pClient->Quit();
}
