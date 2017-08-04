#include <base/system.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <engine/external/json-builder/json-builder.h>
#include <base/system++/io.h>

#include "identity.h"


#define ALL_IDS_JSON_FILE "identities/all.json"


/* Identity JSON specification example:
[
  {
    "index": 0,                    // if the index key is non-existent when loading, the identity will simply be appended to the end
    "identity": {
      "title": "Main Ident",
      "name": "Me Pro!",
      "clan": "Cool Clan",
      "country": -1,
      "skin": "MyAwesomeSkin",
      "use_custom_color": false,
      "color_body": 0,
      "color_feet": 0
    }
  }
]
*/



void CIdentity::OnInit()
{
	LoadIdents();
}

void CIdentity::OnShutdown()
{
	SaveIdents();
}

void CIdentity::LoadIdents()
{
	m_aIdentities.clear();

	IOHANDLE_SMART File = Storage()->OpenFileSmart(ALL_IDS_JSON_FILE, IOFLAG_READ, IStorageTW::TYPE_SAVE);
	if(!File.IsOpen())
	{
		dbg_msg("ident", "failed to load identities from json file, trying legacy...");
		return LoadIdentsLegacy();
	}

	unsigned int Len;
	char *pText = File.ReadAllTextRaw(&Len);
	json_value *pJson = json_parse(pText, Len);
	mem_free(pText); pText = NULL; Len = 0; // no more text
	if(!pJson)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident/ERROR", "error while parsing identities json file!");
		return;
	}

	json_value& json = *pJson;
	if(json.type != json_array)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident/ERROR", "invalid identity json contents (not an array)");
		json_value_free(pJson);
		return;
	}

	for(unsigned int i = 0; i < json.u.array.length ; i++)
	{
		const json_value &jsonOuter = json[i];
		const json_value &jsonInner = jsonOuter["identity"];
		if(jsonInner.type != json_object)
		{
			Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "ident/WARM", "error while parsing identity #%i, skipping it", i);
			continue;
		}

		CIdentEntry Entry;
		Entry.m_StartingIndex = (int)(json_int_t)(jsonOuter["index"]);;
		str_copyb(Entry.m_aTitle, (const char *)jsonInner["title"]);
		str_copyb(Entry.m_aName, (const char *)jsonInner["name"]);
		str_copyb(Entry.m_aClan, (const char *)jsonInner["clan"]);
		Entry.m_Country = (int)(json_int_t)jsonInner["country"];
		str_copyb(Entry.m_aSkin, (const char *)jsonInner["skin"]);
		Entry.m_UseCustomColor = (bool)jsonInner["use_custom_color"];
		Entry.m_ColorBody = (int)(json_int_t)jsonInner["color_body"];
		Entry.m_ColorFeet = (int)(json_int_t)jsonInner["color_feet"];

		// add the entry to our array
		m_aIdentities.add(Entry);
	}

	m_aIdentities.sort_range();

	json_value_free(pJson);

}


void CIdentity::LoadIdentsLegacy()
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
	if(NumIdents() == 0)
		return;

	json_value *arr = json_array_new((unsigned)NumIdents());
	for(int i = 0; i < NumIdents(); i++)
	{
		const CIdentEntry *pIdent = GetIdent(i);
		json_value *outer_obj = json_object_new(2);
		json_value *inner_obj = json_object_new(8);

		json_object_push(inner_obj, "title", json_string_new(pIdent->m_aTitle));
		json_object_push(inner_obj, "name", json_string_new(pIdent->m_aName));
		json_object_push(inner_obj, "clan", json_string_new(pIdent->m_aClan));
		json_object_push(inner_obj, "country", json_integer_new(pIdent->m_Country));
		json_object_push(inner_obj, "skin", json_string_new(pIdent->m_aSkin));
		json_object_push(inner_obj, "use_custom_color", json_boolean_new(pIdent->m_UseCustomColor));
		json_object_push(inner_obj, "color_body", json_integer_new(pIdent->m_ColorBody));
		json_object_push(inner_obj, "color_feet", json_integer_new(pIdent->m_ColorFeet));

		json_object_push(outer_obj, "index", json_integer_new(i));
		json_object_push(outer_obj, "identity", inner_obj);

		json_array_push(arr, outer_obj);
	}

	json_serialize_opts opts;
	opts.mode = json_serialize_mode_multiline;
	opts.opts = 0;
	opts.indent_size = 2;
	char *pJsonBuf = mem_allocb(char, json_measure_ex(arr, opts));
	json_serialize_ex(pJsonBuf, arr, opts);
	json_value_free(arr);

	IOHANDLE_SMART File = Storage()->OpenFileSmart(ALL_IDS_JSON_FILE, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
	if(File.IsOpen())
	{
		if(g_Config.m_Debug)
			dbg_msg("ident", "saving %i identities to file '%s'", NumIdents(), File.GetPath());

		unsigned int Size = File.WriteLine(pJsonBuf);

		if(g_Config.m_Debug)
			dbg_msg("ident", "done, %u bytes written.", Size);
	}
	else
		dbg_msg("ident/ERROR", "failed to open file '%s' (%s) for writing; cannot save identities!", ALL_IDS_JSON_FILE, File.GetPath());

	mem_free(pJsonBuf);
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
	//str_copy(Entry.m_aFilename, aFileName, sizeof(Entry.m_aFilename));
	aFileName[3] = '\0';
	Entry.m_StartingIndex = str_toint(aFileName);

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
