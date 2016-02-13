#include <base/system.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "identity.h"

void CIdentity::OnInit()
{
	m_aIdentities.clear();
	Storage()->ListDirectory(IStorage::TYPE_SAVE, "identities", FindIDFiles, this);
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
		bool Rename = false;
		char aFilename[64];
		char aBuf[512];

		if(pEntry->m_aFilename[0] == '\0')
			str_format(aFilename, sizeof(aFilename), "%03d_%s.id", i, pEntry->m_aName);
		else
		{
			if(!(str_isdigit(pEntry->m_aFilename[0]) && str_isdigit(pEntry->m_aFilename[1]) && str_isdigit(pEntry->m_aFilename[2]) && pEntry->m_aFilename[3] == '_'))
			{
				str_format(aFilename, sizeof(aFilename), "%03d_%s.id", i, pEntry->m_aName);
				Rename = true;
			}
			else
			{ 
				char aNewIndex[4];
				char aOldIndex[4];
				str_format(aNewIndex, sizeof(aNewIndex), "%03d", i);
				str_format(aOldIndex, sizeof(aOldIndex), pEntry->m_aFilename);
				str_format(aFilename, sizeof(aFilename), pEntry->m_aFilename);
				if(str_toint(aNewIndex) != str_toint(aOldIndex))
				{
					for(int i = 0; i < 3; i++)
						aFilename[i] = aNewIndex[i];
					Rename = true;
				}
			}
		}

		if(Rename)
		{
			char OldName[512];
			char NewName[512];
			str_format(OldName, sizeof(OldName), "identities/%s", pEntry->m_aFilename);
			str_format(NewName, sizeof(NewName), "identities/%s", aFilename);
			Storage()->RenameFile(OldName, NewName, IStorage::TYPE_SAVE);
			Storage()->RemoveFile(OldName, IStorage::TYPE_SAVE);

			//str_format(aBuf, sizeof(aBuf), "renamed '%s' to %s", pEntry->m_aFilename, aFilename);
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", aBuf);
		}

		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", aBuf);
		str_format(aBuf, sizeof(aBuf), "identities/%s", aFilename);
		IOHANDLE File = Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorage::TYPE_SAVE);
		
		if(!File)
		{
			io_close(File);
			continue;
		}

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
			if(!File)
			{
				str_format(aBuf, sizeof(aBuf), "failed to save '%s'", aFilename);
				Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident", aBuf);
				io_close(File);
				mem_zero(aTeeEntry[j], sizeof(aTeeEntry[j]));
				break;
			}
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

	IOHANDLE File = pSelf->Storage()->OpenFile(aFilePath, IOFLAG_READ, IStorage::TYPE_SAVE);

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
				io_close(File);
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
