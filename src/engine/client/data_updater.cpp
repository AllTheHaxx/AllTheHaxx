#include <base/system.h>
#include <openssl/sha.h>
#include <engine/storage.h>
#include <base/system++.h>
#include "data_updater.h"
#include "updater.h"


using std::map;
using std::string;

void CDataUpdater::Init(CUpdater *pUpdater, const char *pTreeStr)
{
	m_pUpdater = pUpdater;
	m_Tree = std::string(pTreeStr);

	m_LocalGlob.clear();
	m_RemoteGlob.clear();
	m_Progress = 0;

	thread_detach(thread_init(ListdirThread, this));
}

void CDataUpdater::ListdirThread(void *pUser)
{
	CDataUpdater *pSelf = (CDataUpdater *)pUser;

	dbg_msg("data-updater", "Data-Updater started globbing");
	int64 Start = time_get();
	fs_listdir_verbose("data", ListdirCallback, 0, pUser);
	dbg_msg("data-updater", "listdir thread finished after %i seconds. Hashing now...", (time_get()-Start)/time_freq());

	thread_detach(thread_init(HashingThread, pUser));
}


int CDataUpdater::ListdirCallback(const char *pName, const char *pFullPath, int is_dir, int dir_type, void *pUser)
{
	CDataUpdater *pSelf = (CDataUpdater *)pUser;

	if(pName[0] == '.')
		return 0;

	// only add it to our list, hashing will be done later
	pSelf->m_LocalGlob.insert(std::pair<string, string>(string(pFullPath), string("")));
}

void CDataUpdater::HashingThread(void *pUser)
{
	CDataUpdater *pSelf = (CDataUpdater *)pUser;

	int64 Start = time_get();

	for(map<string, string>::iterator it = pSelf->m_LocalGlob.begin(); it != pSelf->m_LocalGlob.end(); it++)
	{
		char aHexdigest[SHA_DIGEST_LENGTH * 2 + 1];
		const char *pPath = it->first.c_str();

		CDataUpdater::GitHashStr(pPath, aHexdigest, sizeof(aHexdigest));

		dbg_msg("data-updater", "%s for '%s'", aHexdigest, pPath);
		it->second = string(aHexdigest);

	}
		dbg_msg("data-updater", "Hashing finished after %i seconds", (time_get()-Start)/time_freq());

}

void CDataUpdater::GitHashStr(const char *pFile, char *pBuffer, unsigned BufferSize)
{
	unsigned char aHash[SHA_DIGEST_LENGTH];
	mem_zerob(aHash);

	SHA_CTX context;
	if(!SHA_Init(&context))
	{
		dbg_msg("GitHashStr", "SHA_Init failed for '%s'", pFile);
	}
	else
	{
		IOHANDLE_SMART File(pFile, IOFLAG_READ);

		// prepend what git does
		{
			char aBuffer[16 * 1024];
			str_formatb(aBuffer, "blob %d", File.Length());
			if(!SHA_Update(&context, aBuffer, (unsigned)str_length(aBuffer)+1)) // +1 because git wants the \0 to be hashed aswell!
			{
				dbg_msg("GitHashStr", "SHA_Update failed for the git identifier '%s'", aBuffer);
			}
			else
			{
				// read the data in portions of 16kb and hash it subsequently
				while(1)
				{
					mem_zerob(aBuffer);

					unsigned int BytesRead = File.Read(aBuffer, sizeof(aBuffer));
					if(BytesRead == 0)
						break;

					if(!SHA_Update(&context, aBuffer, BytesRead))
					{
						dbg_msg("GitHashStr", "SHA_Update failed for '%s' at 0x%x", pFile, File.Tell());
						continue;
					}
				}
			}
		}
	}

	if(!SHA_Final(aHash, &context))
	{
		dbg_msg("GitHashStr", "SHA_Final failed for '%s'", pFile);
		mem_zerob(aHash);
	}

	str_hex_simple(pBuffer, BufferSize, aHash, SHA_DIGEST_LENGTH);
}
