#include "updater.h"
#include <base/system.h>
#include <engine/fetcher.h>
#include <engine/storage.h>
#include <engine/client.h>
#include <engine/external/json-parser/json.h>
#include <game/version.h>

#include <stdlib.h> // system

using std::string;
using std::vector;

CUpdater::CUpdater()
{
	m_pClient = NULL;
	m_pStorage = NULL;
	m_pFetcher = NULL;
	m_State = CLEAN;
	m_Percent = 0;
	m_aVersion[0] = '0';
	m_aVersion[1] = '\0';
	m_IsWinXP = false;
	m_CheckOnly = false;
	Reset();
}

void CUpdater::Init()
{
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_pFetcher = Kernel()->RequestInterface<IFetcher>();
#if defined(CONF_FAMILY_WINDOWS)
	m_IsWinXP = os_compare_version(5, 1) <= 0;
#endif
}

void CUpdater::Reset()
{
	m_ServerUpdate = false;
	m_ClientUpdate = false;
	m_AddedFiles.clear();
	m_RemovedFiles.clear();
	m_ExternalFiles.clear();
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
	if(!str_comp(pTask->Dest(), "update.json"))
	{
		if(pTask->State() == CFetchTask::STATE_DONE)
			pUpdate->m_State = GOT_MANIFEST;
		else if(pTask->State() == CFetchTask::STATE_ERROR)
			pUpdate->m_State = FAIL;
	}
	else if(!str_comp(pTask->Dest(), pUpdate->m_aLastFile))
	{
		if(pTask->State() == CFetchTask::STATE_DONE)
		{
			if(pUpdate->m_ClientUpdate)
				pUpdate->ReplaceClient();
			if(pUpdate->m_ServerUpdate)
				pUpdate->ReplaceServer();
			if(pUpdate->m_pClient->State() == IClient::STATE_ONLINE || pUpdate->m_pClient->EditorHasUnsavedData())
				pUpdate->m_State = NEED_RESTART;
			else{
				if(!pUpdate->m_IsWinXP)
					pUpdate->m_pClient->Restart();
				else
					pUpdate->WinXpRestart();
			}
		}
		else if(pTask->State() == CFetchTask::STATE_ERROR)
			pUpdate->m_State = FAIL;
	}
	delete pTask;
}

void CUpdater::FetchFile(const char *pSource, const char *pFile, const char *pDestPath)
{
	char aBuf[256];
	if(pSource[0] == '~')
		str_format(aBuf, sizeof(aBuf), "https://raw.githubusercontent.com/AllTheHaxx/%s/%s", pSource+1, pFile);
	else
		str_format(aBuf, sizeof(aBuf), "https://github.com/AllTheHaxx/%s/%s", pSource, pFile);

	dbg_msg("updater", "fetching file from '%s'", aBuf);
	char aDestPath[512] = {0};
	if(!pDestPath)
		str_copy(aDestPath, pFile, sizeof(aDestPath));
	else
		str_copy(aDestPath, pDestPath, sizeof(aDestPath));
	if(aDestPath[str_length(aDestPath)-1] == '/' || aDestPath[str_length(aDestPath)-1] == '\\')
	{
		fs_makedir(aDestPath);
		str_append(aDestPath, pFile, sizeof(aDestPath));
	}

	CFetchTask *Task = new CFetchTask(false);
	m_pFetcher->QueueAdd(Task, aBuf, aDestPath, -2, this, &CUpdater::CompletionCallback, &CUpdater::ProgressCallback);
}

void CUpdater::Update()
{
	switch(m_State)
	{
		case GOT_MANIFEST:
			PerformUpdate();
		default:
			return;
	}
}

void CUpdater::AddNewFile(const char *pFile)
{
	// check if it's already on the download list
	for(vector<string>::iterator it = m_AddedFiles.begin(); it < m_AddedFiles.end(); ++it)
	{
		if(!str_comp(it->c_str(), pFile))
			return;
	}
	m_AddedFiles.push_back(string(pFile));
	////dbg_msg("DEBUG|updater", "Add: '%s'", m_AddedFiles.back().c_str());
}

void CUpdater::AddRemovedFile(const char *pFile)
{
	// remove the file from the download list
	for(vector<string>::iterator it = m_AddedFiles.begin(); it < m_AddedFiles.end(); ++it)
	{
		if(!str_comp(it->c_str(), pFile))
		{
			m_AddedFiles.erase(it);
			break;
		}
	}
	m_RemovedFiles.push_back(string(pFile));
	////dbg_msg("DEBUG|updater", "Remove: '%s'", m_RemovedFiles.back().c_str());
}

void CUpdater::ReplaceClient()
{
	dbg_msg("updater", "Replacing " PLAT_CLIENT_EXEC);

	// replace running executable by renaming twice...
	if(!m_IsWinXP)
	{
		m_pStorage->RemoveBinaryFile("AllTheHaxx.old");
		m_pStorage->RenameBinaryFile(PLAT_CLIENT_EXEC, "AllTheHaxx.old");
		m_pStorage->RenameBinaryFile("AllTheHaxx.tmp", PLAT_CLIENT_EXEC);
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
	m_pStorage->RemoveBinaryFile("AllTheHaxx-Server.old");
	m_pStorage->RenameBinaryFile(PLAT_SERVER_EXEC, "AllTheHaxx-Server.old");
	m_pStorage->RenameBinaryFile("AllTheHaxx-Server.tmp", PLAT_SERVER_EXEC);
	#if !defined(CONF_FAMILY_WINDOWS)
		char aPath[512];
		m_pStorage->GetBinaryPath(PLAT_SERVER_EXEC, aPath, sizeof aPath);
		char aBuf[512];
		str_format(aBuf, sizeof aBuf, "chmod +x %s", aPath);
		if (system(aBuf))
			dbg_msg("updater", "Error setting server executable bit");
	#endif
}

void CUpdater::ParseUpdate()
{
	Reset(); // clear out old values

	char aPath[512];
	IOHANDLE File = m_pStorage->OpenFile(m_pStorage->GetBinaryPath("update.json", aPath, sizeof aPath), IOFLAG_READ, IStorageTW::TYPE_ALL);
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

					// need to download the client binary? (well, that'd be good wouldn't it? o.O) TODO: useless.
					if(json_boolean_get(json_object_get(pCurrent, "client")))
						m_ClientUpdate = true;

					//if(json_boolean_get(json_object_get(pCurrent, "server")))
					//	m_ServerUpdate = true;

					// list of files to be downloaded
					if((pTemp = json_object_get(pCurrent, "download"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
							AddNewFile(json_string_get(json_array_get(pTemp, j)));
					}

					// list of files to be removed
					if((pTemp = json_object_get(pCurrent, "remove"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
							AddRemovedFile(json_string_get(json_array_get(pTemp, j)));
					}

					// list of external files from variable sources to be downloaded
					if((pTemp = json_object_get(pCurrent, "external"))->type == json_array)
					{
						for(int j = 0; j < json_array_length(pTemp); j++)
						{
							ExternalFile e;
							const json_value *pFiles, *pDests, *pArr = json_array_get(pTemp, j);

							e.source = string(json_string_get(json_object_get(pArr, "source")));

							if((pFiles = json_object_get(pArr, "files"))->type == json_array
							&& (pDests = json_object_get(pArr, "dests"))->type == json_array)
							{
								// add the list of files to the entry
								for(int k = 0; k < json_array_length(pFiles); k++)
								{
									std::pair<string, string> p( string(json_string_get(json_array_get(pFiles, k))),
											  	  	  	  	  	 string(json_string_get(json_array_get(pDests, k))) );
									e.files.push_back(p);
								}

								// store the entry
								m_ExternalFiles.push_back(e);
								////dbg_msg("DEBUG|updater", "External (%i): src='%s', FILE='%s' TO='%s'", j, e.source.c_str(), e.files.back().first.c_str(), e.files.back().second.c_str());
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

	if((m_aVersion[0] == '0' && m_aVersion[1] == '\0') || ForceRefresh)
	{
		m_State = GETTING_MANIFEST;
		dbg_msg("updater", "refreshing version info");
		FetchFile("~stuffility/master", "update.json");
	}
	else
		m_State = GOT_MANIFEST;
}

void CUpdater::PerformUpdate()
{
	m_State = PARSING_UPDATE;
	dbg_msg("updater", "Parsing update.json, %s", m_CheckOnly ? "checking for new version" : "updating the client!");
	ParseUpdate();
	if(m_CheckOnly)
	{
		m_CheckOnly = false;
		m_State = CLEAN;
		return;
	}

	m_State = DOWNLOADING;

	const char *aLastFile;
	if(m_ClientUpdate)
		aLastFile = "AllTheHaxx.tmp";
	else if(!m_AddedFiles.empty())
		aLastFile = m_AddedFiles.front().c_str();
	else
		aLastFile = "";

	str_copy(m_aLastFile, aLastFile, sizeof(m_aLastFile));
	while(!m_AddedFiles.empty())
	{
		FetchFile("~AllTheHaxx/master", m_AddedFiles.back().c_str());
		m_AddedFiles.pop_back();
	}
	while(!m_RemovedFiles.empty())
	{
		m_pStorage->RemoveBinaryFile(m_RemovedFiles.back().c_str());
		m_RemovedFiles.pop_back();
	}
	while(!m_ExternalFiles.empty())
	{
		while(!m_ExternalFiles.back().files.empty())
		{
			FetchFile(	m_ExternalFiles.back().source.c_str(), // repo
						m_ExternalFiles.back().files.back().first.c_str(), // source
						m_ExternalFiles.back().files.back().second == "" ? 0 : m_ExternalFiles.back().files.back().second.c_str() // destination
					  );
			m_ExternalFiles.back().files.pop_back();
		}
		m_ExternalFiles.pop_back();
	}

	if(m_ServerUpdate)
	{
		// there is no ath server
	}

	if(m_ClientUpdate)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "AllTheHaxx/releases/download/%s", m_aVersion);
		FetchFile(aBuf, PLAT_CLIENT_DOWN, "AllTheHaxx.tmp");
	}
}

void CUpdater::WinXpRestart()
{
		char aBuf[512];
		IOHANDLE bhFile = io_open(m_pStorage->GetBinaryPath("du.bat", aBuf, sizeof aBuf), IOFLAG_WRITE);
		if(!bhFile)
			return;
		char bBuf[512];
		str_format(bBuf, sizeof(bBuf), ":_R\r\ndel \"AllTheHaxx.exe\"\r\nif exist \"AllTheHaxx.exe\" goto _R\r\nrename \"AllTheHaxx.tmp\" \"AllTheHaxx.exe\"\r\n:_T\r\nif not exist \"AllTheHaxx.exe\" goto _T\r\nstart AllTheHaxx.exe\r\ndel \"du.bat\"\r\n");
		io_write(bhFile, bBuf, str_length(bBuf));
		io_close(bhFile);
		shell_execute(aBuf);
		m_pClient->Quit();
}
