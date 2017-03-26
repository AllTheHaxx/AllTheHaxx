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
	SetState(STATE_CLEAN);
	str_copy(m_aError, "something", sizeof(m_aError));
	m_Percent = 0;
	m_IsWinXP = false;

	m_ClientUpdate = true; //XXX this is for debugging purposes MUST BE TRUE AT RELEASE!!!11ELF
#if !defined(CONF_DEBUG)
	m_ClientUpdate = true; // just in case I forget it once again ._.
#endif

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

void CUpdater::Tick()
{
	CALLSTACK_ADD();


	// check for errors
	if(m_GitHubAPI.State() == CGitHubAPI::STATE_ERROR && State() != STATE_FAIL)
	{
		str_copyb(m_aError, m_GitHubAPI.GetWhatFailed());
		SetState(STATE_FAIL);
	}

	switch(State())
	{
		case STATE_SYNC_REFRESH:
			if(m_GitHubAPI.State() == CGitHubAPI::STATE_CLEAN || m_GitHubAPI.State() == CGitHubAPI::STATE_NEW_VERSION)
			{
				SetState(STATE_CLEAN);
			}
			break;

		case STATE_SYNC_POSTGETTING: // have downloaded the update manifest
			if(m_GitHubAPI.State() == CGitHubAPI::STATE_DONE)
			{
				ParseUpdate();
				DownloadUpdate();
			}
			break;

		default:
			return;
	}
}

void CUpdater::CheckForUpdates(bool ForceRefresh)
{
	// get the version info if we don't have any yet
	if((GetLatestVersion()[0] == '0' && GetLatestVersion()[1] == '\0') || ForceRefresh)
	{
		SetState(STATE_SYNC_REFRESH);
		dbg_msg("updater", "refreshing version info");
		FetchFile("stuffility/master", "ath-news.txt");
		m_GitHubAPI.CheckVersion();
	}
	else
		dbg_msg("updater", "skipping version check, already did it");
}

void CUpdater::PerformUpdate()
{
	CALLSTACK_ADD();

	if(GetLatestVersion()[1] != '\0')
	{
		dbg_msg("updater", "Starting update to version %s!", GetLatestVersion());
		SetState(STATE_GETTING_MANIFEST);
		FetchFile("stuffility/master", UPDATE_MANIFEST);
		m_GitHubAPI.DoUpdate();
	}
	else
		dbg_msg("updater", "can't initiate update before version check!");
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

	CUpdater *pSelf = (CUpdater *)pUser;
	const bool IS_ERROR = pTask->State() == (const int)CFetchTask::STATE_ERROR;

	const char *a = 0; // a is full path
	for(const char *c = pTask->Dest(); *c; c++)
		if(*c == '/')
			{ a = c + 1; break; }
	a = a ? a : pTask->Dest();

		const char *b = 0; // b is just the filename
	for(const char *c = pTask->Dest(); *c; c++)
		if(*c == '/')
			b = c + 1;
	b = b ? b : pTask->Dest();

	const char * const pFailedNewsMsg = Localize(
			"|<<< Failed to download news >>>|\n"
			"News will automatically be refreshed on next client start if available\n"
			"~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n\n");
	if(IS_ERROR)
	{
		if(str_comp(b, "ath-news.txt") == 0) // news are allowed to fail...
			str_copy(pSelf->m_aNews, pFailedNewsMsg, sizeof(pSelf->m_aNews));
		else
		{
			if(str_comp_nocase_num(a, "lua/", 4) != 0) // example scripts are allowed to fail, too
			{
				pSelf->SetState(STATE_FAIL);
				str_format(pSelf->m_aError, sizeof(pSelf->m_aError), "'%s'", a);
			}
			dbg_msg("update", "failed to download '%s'", a);
		}
		fs_remove(pTask->Dest()); // delete the empty file dummy
	}

	// handle the news
	if(str_comp(b, "ath-news.txt") == 0)
	{
		// dig out whether ATH news have been updated

		char aOldNews[NEWS_SIZE] = {0};
		char aNewsBackupPath[512];

		// read the old news
		IOHANDLE f = pSelf->m_pStorage->OpenFile("tmp/cache/ath-news.txt", IOFLAG_READ, IStorageTW::TYPE_SAVE, aNewsBackupPath, sizeof(aNewsBackupPath));
		if(f)
		{
			io_read(f, aOldNews, NEWS_SIZE);
			io_close(f);
		}

		// read the new news
		if(!IS_ERROR)
		{
			f = io_open("update/ath-news.txt", IOFLAG_READ);
			if(f)
			{
				io_read(f, pSelf->m_aNews, NEWS_SIZE);
				io_close(f);
			}
		}
		else
			str_append(pSelf->m_aNews, aOldNews, NEWS_SIZE);

		// dig out whether news have been updated
		if(str_comp(aOldNews, pSelf->m_aNews + (IS_ERROR ? str_length(pFailedNewsMsg) : 0)) != 0)
		{
			g_Config.m_UiPage = CMenus::PAGE_NEWS_ATH;

			// backup the new news file if we got one
			if(!IS_ERROR)
			{
				f = pSelf->m_pStorage->OpenFile("tmp/cache/ath-news.txt", IOFLAG_WRITE, IStorageTW::TYPE_SAVE, aNewsBackupPath, sizeof(aNewsBackupPath));
				if(f)
				{
					io_write(f, pSelf->m_aNews, (unsigned int)str_length(pSelf->m_aNews));
					io_flush(f);
					io_close(f);
				}
			}
		}
	}
	else if(pTask->State() == CFetchTask::STATE_DONE)
	{
		if(pSelf->State() == STATE_GETTING_MANIFEST && str_comp(b, UPDATE_MANIFEST) == 0)
		{
			pSelf->SetState(STATE_SYNC_POSTGETTING);
			dbg_msg("updater", "got manifest, waiting for github-compare to finish...");
		}
		else if(pSelf->State() == STATE_DOWNLOADING && str_comp(a, pSelf->m_aLastFile) == 0)
		{
			dbg_msg("updater", "finished downloading, installing update");
			pSelf->InstallUpdate(); // sets state
		}
	}
	delete pTask;
}

void CUpdater::ParseUpdate()
{
	CALLSTACK_ADD();

	// this function shall only be called when actually updating the client

	SetState(STATE_PARSING_UPDATE);
	dbg_msg("updater", "parsing " UPDATE_MANIFEST);

	char aPath[512];
	const char *pUpdateManifestPath = m_pStorage->GetBinaryPath("update/" UPDATE_MANIFEST, aPath, sizeof aPath);
	IOHANDLE File = m_pStorage->OpenFile(pUpdateManifestPath, IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(!File)
	{
		dbg_msg("updater", "failed to open '%s', skipping the manifest", pUpdateManifestPath);
		return;
	}

	char aBuf[4096*4];
	mem_zero(aBuf, sizeof (aBuf));
	io_read(File, aBuf, sizeof(aBuf));
	io_close(File);

	json_value *pJsonVersions = json_parse(aBuf, (size_t)str_length(aBuf));
	if(!pJsonVersions) // a check here because this part could actually go wrong
	{
		dbg_msg("updater/parse", "failed to parse the update manifest '%s' of length %i, skipping it", pUpdateManifestPath, str_length(aBuf));
		return;
	}

	json_value &jsonVersions = *pJsonVersions;
	if(jsonVersions.type != json_array)
	{
		dbg_msg("updater/parse", "invalid manifest contents (not an array)");
		json_value_free(pJsonVersions);
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

	json_value_free(pJsonVersions);

}

void CUpdater::DownloadUpdate()
{
	CALLSTACK_ADD();
/*
	// do cleanups - much hack.
#if defined(CONF_FAMILY_UNIX)
	system("rm -rf update");
#elif defined(CONF_FAMILY_WINDOWS)
	system("rd update /S /Q");
#endif
*/
	// merge our lists with the ones from github
	{
		// download jobs
		{
			std::map<string, string> e;
			std::string source(string("AllTheHaxx") + "/" + string(m_GitHubAPI.GetLatestVersionTree()));
			for(std::vector<std::string>::const_iterator it = m_GitHubAPI.GetDownloadJobs().begin(); it != m_GitHubAPI.GetDownloadJobs().end(); it++)
			{
				e[*it] = string(""); // leaving the destination folder empty will put everything in the right place
				dbg_msg("updater/DEBUG", "merging '%s' -> '%s'", source.c_str(), it->c_str());
			}
			// store the entries
			dbg_msg("updater/DEBUG", "adding map with %lu entries to the download jobs", (unsigned long)e.size());
			m_FileDownloadJobs[source] = e;
		}

		// remove jobs
		m_FileRemoveJobs.insert(m_FileRemoveJobs.end(), m_GitHubAPI.GetRemoveJobs().begin(), m_GitHubAPI.GetRemoveJobs().end());

		// (move jobs are done later when installing the update)
	}

	// start downloading
	dbg_msg("updater", "Starting download, got %lu file remove jobs and download jobs from %lu repos", (unsigned long)m_FileRemoveJobs.size(), (unsigned long)m_FileDownloadJobs.size());
	SetState(STATE_DOWNLOADING);

	const char *pLastFile;
	pLastFile = "";

	// remove files before downloading anything
	for(std::vector<string>::iterator it = m_FileRemoveJobs.begin(); it != m_FileRemoveJobs.end(); ++it)
	{
		m_pStorage->RemoveBinaryFile(it->c_str());
	}

	// fetch all download files
	for(map<string, map<string, string> >::iterator it = m_FileDownloadJobs.begin(); it != m_FileDownloadJobs.end(); ++it)
	{
		for(map<string, string>::iterator file = it->second.begin(); file != it->second.end(); ++file)
		{
			dbg_msg("updater/DEBUG", "fetching '%s' -> '%s' -> '%s'", it->first.c_str(), file->first.c_str(), file->second.c_str());
			FetchFile(it->first.c_str(), file->first.c_str(), file->second.c_str());
			pLastFile = file->first.c_str();
		}
	}

	if(m_ClientUpdate)
	{
		FetchExecutable(PLAT_CLIENT_DOWN, "AllTheHaxx.tmp");
		pLastFile = "AllTheHaxx.tmp";
	}

	str_copy(m_aLastFile, pLastFile, sizeof(m_aLastFile));

#if !defined(CONF_DEBUG)
	if(g_Config.m_Debug)
#endif
		dbg_msg("updater/debug", "last file is '%s'", m_aLastFile);
}

void CUpdater::InstallUpdate()
{
	CALLSTACK_ADD();

	SetState(STATE_MOVE_FILES);

	for(map<std::string, map<std::string, std::string> >::iterator it = m_FileDownloadJobs.begin(); it != m_FileDownloadJobs.end(); ++it)
		for(map<std::string, std::string>::iterator file = it->second.begin(); file != it->second.end(); ++file)
		{
			string destPath;
			if(str_length(file->second.c_str()) == 0 ||
					file->second.c_str()[str_length(file->second.c_str())-1] == '/' ||
					file->second.c_str()[str_length(file->second.c_str())-1] == '\\')
				destPath = string(file->second + file->first).c_str(); // append the filename to the dest folder path
			else
				destPath = file->second; // the full path is already given
			MoveFile(destPath.c_str());
			dbg_msg("updater", "installing '%s'", destPath.c_str());
		}

	// do the move jobs from github
	for(std::vector<std::pair<std::string, std::string> >::const_iterator it = m_GitHubAPI.GetRenameJobs().begin(); it != m_GitHubAPI.GetRenameJobs().end(); it++)
	{
		m_pStorage->RenameBinaryFile(it->first.c_str(), it->second.c_str());
		dbg_msg("data-update", "moving file '%s' -> '%s'", it->first.c_str(), it->second.c_str());
	}


	if(m_ClientUpdate)
		ReplaceClient();
	if(m_pClient->State() == IClient::STATE_ONLINE || m_pClient->EditorHasUnsavedData() || !m_ClientUpdate)
		SetState(STATE_NEED_RESTART);
	else
	{
		if(!m_IsWinXP)
			m_pClient->Restart();
		else
			WinXpRestart();
	}
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
