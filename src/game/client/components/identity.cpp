#include <base/system.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "identity.h"

void CIdentity::OnInit()
{
	CALLSTACK_ADD();

	m_aIdentities.clear();
	Storage()->ListDirectory(IStorageTW::TYPE_SAVE, "identities", FindIDFiles, this);
	if(!m_aIdentities.size())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", "failed to load identities.");
	}
}

void CIdentity::SaveIdents()
{
	CALLSTACK_ADD();

	// delete all files, then rewrite them
	Storage()->ListDirectory(IStorageTW::TYPE_SAVE, "identities", CIdentity::UnlinkAllIdents, this);

	int Successful = 0;
	for(int i = 0; i < NumIdents(); i++)
	{
		CIdentEntry *pEntry = GetIdent(i);
		char aBuf[512];

		// cleanup identity files from old versions
		if(pEntry->m_aFilename[3] == '_')
		{
			char aPath[256];
			str_format(aBuf, sizeof(aBuf), "identities/%s", pEntry->m_aFilename);
			IOHANDLE tmp = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorageTW::TYPE_SAVE, aPath, sizeof(aPath));
			if(tmp)
			{
				io_close(tmp);
				int ret = fs_remove(aPath);
				dbg_msg("ident/cleanup", "removing '%s' %s", aPath, ret ? "failed" : "succeeded");
			}
			else
				dbg_msg("ident/cleanup", "unknown error occurred; identity '%s' may be duplicate now.", pEntry->m_aName);
		}

		str_format(aBuf, sizeof(aBuf), "identities/%03i.id", i);
		IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
		if(!File)
		{
			dbg_msg("ident/ERROR", "failed to open file '%s' for writing; cannot save identity '%s'", aBuf, pEntry->m_aName);
			continue;
		}

		if(g_Config.m_Debug)
			dbg_msg("ident", "saving %s:%s", aBuf, pEntry->m_aName);

		char aTeeEntry[NUM_ENTRIES][64];

		str_copyb(aTeeEntry[TITLE], pEntry->m_aTitle);
		str_copyb(aTeeEntry[NAME], pEntry->m_aName);
		str_copyb(aTeeEntry[CLAN], pEntry->m_aClan);

		str_formatb(aTeeEntry[COUNTRY], "%d", pEntry->m_Country);

		str_copyb(aTeeEntry[SKIN], pEntry->m_aSkin);
		str_formatb(aTeeEntry[USE_CUSTOM_COLOR], "%d", pEntry->m_UseCustomColor);
		str_formatb(aTeeEntry[COLOR_BODY], "%d", pEntry->m_ColorBody);
		str_formatb(aTeeEntry[COLOR_FEET], "%d", pEntry->m_ColorFeet);

		for(int j = 0; j < NUM_ENTRIES; j++)
		{
			io_write(File, aTeeEntry[j], (unsigned int)str_length(aTeeEntry[j]));
			io_write_newline(File);
		}
		io_close(File);
		Successful++;
	}
	dbg_msg("ident", "successfully saved %i/%i identities", Successful, NumIdents());
}

void CIdentity::OnShutdown()
{
	SaveIdents();
}

int CIdentity::UnlinkAllIdents(const char *pName, int IsDir, int DirType, void *pUser)
{
	CIdentity *pSelf = (CIdentity*)pUser;

	if(str_length(pName) < 4 || IsDir || str_comp(pName+str_length(pName)-3, ".id") != 0)
		return 0;

	char aFilePath[512];
	str_format(aFilePath, sizeof(aFilePath), "identities/%s", pName);
	bool Success = pSelf->Storage()->RemoveFile(aFilePath, IStorageTW::TYPE_SAVE);
	if(g_Config.m_Debug)
		dbg_msg("ident", "%s file '%s'", Success ? "removed" : "failed to remove", aFilePath);

	return 0;
}

int CIdentity::FindIDFiles(const char *pName, int IsDir, int DirType, void *pUser)
{
	CALLSTACK_ADD();

	CIdentity *pSelf = (CIdentity*)pUser;
	if(str_length(pName) < 4 || IsDir || str_comp(pName+str_length(pName)-3, ".id") != 0)
		return 0;

	char aBuf[256];
	char aFileName[64];
	char aFilePath[512];
	char aEntryItems[NUM_ENTRIES][64] = { { 0 } };
	str_format(aFilePath, sizeof(aFilePath), "identities/%s", pName);
	str_copyb(aFileName, pName);

	IOHANDLE File = pSelf->Storage()->OpenFile(aFilePath, IOFLAG_READ, IStorageTW::TYPE_SAVE);

	if(File)
	{
		char *pLine;
		CLineReader lr;

		lr.Init(File);

		for(int i = 0; i < NUM_ENTRIES; i++)
		{
			if(!(pLine = lr.Get()))
			{
				if(i == TITLE) // for backwards compatibility
				{
					str_copy(aEntryItems[i], "", sizeof(aEntryItems[i]));
					continue;
				}
				str_format(aBuf, sizeof(aBuf), "error while loading identity file");
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", aBuf);
				mem_zero(aEntryItems[i], sizeof(aEntryItems[i]));
				break;
			}
			str_copy(aEntryItems[i], pLine, sizeof(aEntryItems[i]));
		}

		io_close(File);
	}
	else
	{
		str_format(aBuf, sizeof(aBuf), "failed to open '%s'", aFileName);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", aBuf);
		return 1;
	}

	CIdentEntry Entry;
	str_copy(Entry.m_aFilename, aFileName, sizeof(Entry.m_aFilename));

	str_copy(Entry.m_aTitle, aEntryItems[TITLE], sizeof(Entry.m_aTitle));
	str_copy(Entry.m_aName, aEntryItems[NAME], sizeof(Entry.m_aName));
	str_copy(Entry.m_aClan, aEntryItems[CLAN], sizeof(Entry.m_aClan));

	Entry.m_Country = max(0, str_toint(aEntryItems[COUNTRY]));

	str_copyb(Entry.m_aSkin, aEntryItems[SKIN]);
	Entry.m_UseCustomColor = max(0, str_toint(aEntryItems[USE_CUSTOM_COLOR]));
	Entry.m_ColorBody = max(0, str_toint(aEntryItems[COLOR_BODY]));
	Entry.m_ColorFeet = max(0, str_toint(aEntryItems[COLOR_FEET]));

	// add the entry to our array
	pSelf->m_aIdentities.add(Entry);

	return 0;
}
