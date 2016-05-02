#include "updater.h"
#include <base/system.h>
#include <engine/fetcher.h>
#include <engine/storage.h>
#include <engine/client.h>
#include <engine/external/json-parser/json.h>
#include <game/version.h>

#include <stdlib.h> // system

using std::string;
using std::map;

CUpdater::CUpdater()
{
	m_pClient = NULL;
	m_pStorage = NULL;
	m_pFetcher = NULL;
	m_State = CLEAN;
	m_Percent = 0;
	m_CheckOnly = false;
	m_aVersion[0] = '0';
	m_aVersion[1] = '\0';
}

void CUpdater::Init()
{
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_pFetcher = Kernel()->RequestInterface<IFetcher>();
	#if defined(CONF_FAMILY_WINDOWS)
	m_IsWinXP = os_compare_version(5, 1) <= 0;
	#else
	m_IsWinXP = false;
	#endif
}

void CUpdater::ProgressCallback(CFetchTask *pTask, void *pUser)
{
	CUpdater *pUpdate = (CUpdater *)pUser;
	str_copy(pUpdate->m_Status, pTask->Dest(), sizeof(pUpdate->m_Status));
	pUpdate->m_Percent = pTask->Progress();
}

void CUpdater::CompletionCallback(CFetchTask *pTask, void *pUser)
{
	CUpdater *pUpdate = (CUpdater *)pUser;
	const char *b = 0;
	for(const char *a = pTask->Dest(); *a; a++)
		if(*a == '/')
			b = a + 1;
	b = b ? b : pTask->Dest();
	if(!str_comp(b, "update.json"))
	{
		if(pTask->State() == CFetchTask::STATE_DONE)
			pUpdate->m_State = GOT_MANIFEST;
		else if(pTask->State() == CFetchTask::STATE_ERROR)
			pUpdate->m_State = FAIL;
	}
	else if(!str_comp(b, pUpdate->m_aLastFile))
	{
		if(pTask->State() == CFetchTask::STATE_DONE)
			pUpdate->m_State = MOVE_FILES;
		else if(pTask->State() == CFetchTask::STATE_ERROR)
			pUpdate->m_State = FAIL;
	}
	delete pTask;
}

void CUpdater::FetchFile(const char *pSource, const char *pFile, const char *pDestPath)
{
	char aBuf[256], aDestPath[512] = {0};
	if(pSource[0] == '~')
		str_format(aBuf, sizeof(aBuf), "https://raw.githubusercontent.com/AllTheHaxx/%s/%s", pSource+1, pFile);
	else
		str_format(aBuf, sizeof(aBuf), "https://github.com/AllTheHaxx/%s/%s", pSource, pFile);

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

void CUpdater::AddFileJob(const char *pFile, bool job)
{
	m_FileJobs[string(pFile)] = job;
}

void CUpdater::ReplaceClient()
{
	dbg_msg("updater", "Replacing " PLAT_CLIENT_EXEC);

	// replace running executable by renaming twice...
	if(!m_IsWinXP)
	{
		m_pStorage->RemoveBinaryFile(CLIENT_EXEC ".old");
		m_pStorage->RenameBinaryFile(PLAT_CLIENT_EXEC, CLIENT_EXEC ".old");
		m_pStorage->RenameBinaryFile("update/" CLIENT_EXEC ".tmp", PLAT_CLIENT_EXEC);
	}

#if !defined(CONF_FAMILY_WINDOWS)
	char aPath[512];
	m_pStorage->GetBinaryPath(PLAT_CLIENT_EXEC, aPath, sizeof aPath);
	char aBuf[512];
	str_format(aBuf, sizeof aBuf, "chmod +x %s", aPath);
	if(system(aBuf))
		dbg_msg("updater", "Error setting client executable bit");
#endif
}

void CUpdater::ReplaceServer()
{
	dbg_msg("updater", "Replacing " PLAT_SERVER_EXEC);

	// replace running executable by renaming twice...
	m_pStorage->RemoveBinaryFile(SERVER_EXEC ".old");
	m_pStorage->RenameBinaryFile(PLAT_SERVER_EXEC, SERVER_EXEC ".old");
	m_pStorage->RenameBinaryFile("update/" SERVER_EXEC ".tmp", PLAT_SERVER_EXEC);

#if !defined(CONF_FAMILY_WINDOWS)
	char aPath[512];
	m_pStorage->GetBinaryPath(PLAT_SERVER_EXEC, aPath, sizeof aPath);
	char aBuf[512];
	str_format(aBuf, sizeof aBuf, "chmod +x %s", aPath);
	if(system(aBuf))
		dbg_msg("updater", "Error setting server executable bit");
#endif
}

void CUpdater::ParseUpdate()
{
	char aPath[512];
	IOHANDLE File = m_pStorage->OpenFile(m_pStorage->GetBinaryPath("update/update.json", aPath, sizeof aPath), IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(File)
	{
		char aBuf[4096*4];
		mem_zero(aBuf, sizeof (aBuf));
		io_read(File, aBuf, sizeof(aBuf));
		io_close(File);

		json_value *pVersions = json_parse(aBuf);

		if(pVersions && pVersions->type == json_array)
		{
			for(int i = 0; i < json_array_length(pVersions); i++)
			{
				const json_value *pTemp;
				const json_value *pCurrent = json_array_get(pVersions, i);
				if(str_comp(json_string_get(json_object_get(pCurrent, "version")), GAME_ATH_VERSION))
				{
					// if we don't know the latest version yet, get it
					if(m_aVersion[0] == '0' && m_aVersion[1] == '\0')
						str_copy(m_aVersion, json_string_get(json_object_get(pCurrent, "version")), sizeof(m_aVersion));

					if(json_boolean_get(json_object_get(pCurrent, "client")))
						m_ClientUpdate = true;
					if(json_boolean_get(json_object_get(pCurrent, "server")))
						m_ServerUpdate = true;
					if((pTemp = json_object_get(pCurrent, "download"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
							AddFileJob(json_string_get(json_array_get(pTemp, j)), true);
					}
					if((pTemp = json_object_get(pCurrent, "remove"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
							AddFileJob(json_string_get(json_array_get(pTemp, j)), false);
					}
					if((pTemp = json_object_get(pCurrent, "external"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
						{
							const json_value *pFiles, *pDests, *pArr = json_array_get(pTemp, j);

							if((pFiles = json_object_get(pArr, "files"))->type == json_array
							&& (pDests = json_object_get(pArr, "dests"))->type == json_array)
							{
								// add the list of files to the entry
								std::map<string, string> e;
								std::string source(json_string_get(json_object_get(pArr, "source")));
								for(int k = 0; k < json_array_length(pFiles); k++)
								{
									std::string file(json_string_get(json_array_get(pFiles, k)));
									e[file] = string(json_string_get(json_array_get(pDests, k)));
									//dbg_msg("DEBUG|updater", "External (%i): src='%s', FILE='%s' TO='%s'", j, source.c_str(), file.c_str(), e.find(file)->second.c_str());
								}

								// store the entry
								m_ExternalFiles[source] = e;
							}
						}
					}
				}
				else
					break;
			}
		}
	}
}

void CUpdater::InitiateUpdate(bool CheckOnly, bool ForceRefresh)
{
	m_CheckOnly = CheckOnly;

	// get the version info if we don't have any yet
	if((m_aVersion[0] == '0' && m_aVersion[1] == '\0') || ForceRefresh)
	{
		m_State = GETTING_MANIFEST;
		dbg_msg("updater", "refreshing version info");
		FetchFile("~stuffility/master", "update.json");
	}
	else
		m_State = GOT_MANIFEST; // if we have the version, we can directly skip to this step
}

void CUpdater::PerformUpdate()
{
	m_State = PARSING_UPDATE;
	dbg_msg("updater", "Parsing update.json");
	ParseUpdate();
	if(m_CheckOnly)
	{
		m_CheckOnly = false;
		m_State = CLEAN;
		return;
	}

	dbg_msg("updater", "Starting download, got %i file jobs and %i external jobs", m_FileJobs.size(), m_ExternalFiles.size());
	m_State = DOWNLOADING;

	const char *aLastFile;
	aLastFile = "";
	for(map<string, bool>::reverse_iterator it = m_FileJobs.rbegin(); it != m_FileJobs.rend(); ++it)
	{
		if(it->second)
		{
			aLastFile = it->first.c_str();
			break;
		}
	}

	for(map<string, bool>::iterator it = m_FileJobs.begin(); it != m_FileJobs.end(); ++it)
	{
		if(it->second == JOB_ADD) // add/update file
		{
			const char *pFile = it->first.c_str();
			size_t len = str_length(pFile);
			if(str_comp_nocase(pFile + len - 4, ".dll") == 0)
			{
#if defined(CONF_FAMILY_WINDOWS)
				char aBuf[512];
				str_copy(aBuf, pFile, sizeof(aBuf)); // SDL
				str_copy(aBuf + len - 4, "-" PLAT_NAME, sizeof(aBuf) - len + 4); // -win32
				str_append(aBuf, pFile + len - 4, sizeof(aBuf)); // .dll
				FetchFile(aBuf, pFile);
#endif
				// Ignore DLL downloads on other platforms, on Linux we statically link anyway
			}
			else
			{
				FetchFile("~AllTheHaxx/master", pFile);
			}
			aLastFile = pFile;
		}
		else // remove file
			m_pStorage->RemoveBinaryFile(it->first.c_str());
	}

	// fetch all external files
	for(map<string, map<string, string> >::iterator it = m_ExternalFiles.begin(); it != m_ExternalFiles.end(); ++it)
		for(map<string, string>::iterator file = it->second.begin(); file != it->second.end(); ++file)
		{
			FetchFile(it->first.c_str(), file->first.c_str(), file->second.c_str());
			aLastFile = file->first.c_str();
		}

	/*if(m_ServerUpdate)
	{
		FetchFile(PLAT_SERVER_DOWN, SERVER_EXEC ".tmp");
		aLastFile = SERVER_EXEC ".tmp";
	}*/

	if(m_ClientUpdate)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "AllTheHaxx/releases/download/%s", m_aVersion);
		FetchFile(aBuf, PLAT_CLIENT_DOWN, "AllTheHaxx.tmp");
	}

	str_copy(m_aLastFile, aLastFile, sizeof(m_aLastFile));
}

void CUpdater::CommitUpdate()
{
	for(map<std::string, bool>::iterator it = m_FileJobs.begin(); it != m_FileJobs.end(); ++it)
		if(it->second == JOB_ADD)
			MoveFile(it->first.c_str());

	for(map<std::string, map<std::string, std::string> >::iterator it = m_ExternalFiles.begin(); it != m_ExternalFiles.end(); ++it)
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
	if(m_ServerUpdate)
		ReplaceServer();
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
