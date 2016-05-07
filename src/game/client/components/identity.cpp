#include <base/system.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "identity.h"

void CIdentity::OnInit()
{
	m_aIdentities.clear();
	Storage()->ListDirectory(IStorageTW::TYPE_SAVE, "identities", FindIDFiles, this);
	if(!m_aIdentities.size())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", "failed to load identities.");
	}
}

void CIdentity::SaveIdents()
{
	for(int i = 0; i < NumIdents(); i++)
	{
		CIdentEntry *pEntry = GetIdent(i);
		char aBuf[512];

		str_format(aBuf, sizeof(aBuf), "identities/%03i.id", i);
		IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
		if(!File)
		{
			dbg_msg("ident/ERROR", "failed to open file '%s' for writing; cannot save identity '%s'", aBuf, pEntry->m_aName);
			continue;
		}

		dbg_msg("ident", "saving %s:%s", aBuf, pEntry->m_aName);

		char aTeeEntry[NUM_ENTRIES][64];

		str_format(aTeeEntry[NAME], 64, pEntry->m_aName);
		str_format(aTeeEntry[CLAN], 64, pEntry->m_aClan);

		str_format(aTeeEntry[COUNTRY], 64, "%d", pEntry->m_Country);

		str_format(aTeeEntry[SKIN], 64, pEntry->m_aSkin);
		str_format(aTeeEntry[USE_CUSTOM_COLOR], 64, "%d", pEntry->m_UseCustomColor);
		str_format(aTeeEntry[COLOR_BODY], 64, "%d", pEntry->m_ColorBody);
		str_format(aTeeEntry[COLOR_FEET], 64, "%d", pEntry->m_ColorFeet);
		
		for(int j = 0; j < NUM_ENTRIES; j++)
		{
			io_write(File, aTeeEntry[j], str_length(aTeeEntry[j]));
			io_write_newline(File);
		}
		io_close(File);
	}	
}

int CIdentity::FindIDFiles(const char *pName, int IsDir, int DirType, void *pUser)
{
	CIdentity *pSelf = (CIdentity*)pUser;
	if(str_length(pName) < 4 || IsDir || str_comp(pName+str_length(pName)-3, ".id") != 0)
		return 0;

	char aBuf[256];
	char aFileName[64];
	char aFilePath[512];
	char aEntryItems[NUM_ENTRIES][64];
	str_format(aFilePath, sizeof(aFilePath), "identities/%s", pName);
	str_format(aFileName, sizeof(aFileName), pName);

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
				str_format(aBuf, sizeof(aBuf), "error while loading identity file");
				pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", aBuf);
				//io_close(File);
				mem_zero(aEntryItems[i], sizeof(aEntryItems[i]));
				break;
			}
			str_format(aEntryItems[i], sizeof(aEntryItems[i]), pLine);
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
	str_format(Entry.m_aFilename, sizeof(Entry.m_aFilename), aFileName);

	str_format(Entry.m_aName, sizeof(Entry.m_aName), aEntryItems[NAME]);
	str_format(Entry.m_aClan, sizeof(Entry.m_aClan), aEntryItems[CLAN]);

	Entry.m_Country = str_toint(aEntryItems[COUNTRY]);

	str_format(Entry.m_aSkin, sizeof(Entry.m_aSkin), aEntryItems[SKIN]);
	Entry.m_UseCustomColor = str_toint(aEntryItems[USE_CUSTOM_COLOR]);
	Entry.m_ColorBody = str_toint(aEntryItems[COLOR_BODY]);
	Entry.m_ColorFeet = str_toint(aEntryItems[COLOR_FEET]);

	// add the entry to our array
	pSelf->m_aIdentities.add(Entry);

	return 0;
}
