#include <base/system.h>
#include <engine/engine.h>
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


static const json_serialize_opts json_opts_common = {
		json_serialize_mode_multiline,
		0,
		2
};
static const json_serialize_opts json_opts_packed = {
		json_serialize_mode_packed,
		json_serialize_opt_no_space_after_colon|json_serialize_opt_no_space_after_comma|json_serialize_opt_pack_brackets,
		0
};


void CIdentity::OnInit()
{
	if(!LoadIdents())
	{
		// back up the old file if there was any
		char aNewFilename[256];
		str_timestamp_format(aNewFilename, sizeof(aNewFilename), ALL_IDS_JSON_FILE ".FAILED_%Y%m%d-%H%M%S");
		Storage()->RenameFile(ALL_IDS_JSON_FILE, aNewFilename, IStorageTW::TYPE_SAVE);
	}
}

void CIdentity::OnShutdown()
{
	SaveIdents();
}

bool CIdentity::LoadIdents()
{
	m_aIdentities.clear();

	IOHANDLE_SMART File = Storage()->OpenFileSmart(ALL_IDS_JSON_FILE, IOFLAG_READ, IStorageTW::TYPE_SAVE);
	if(!File.IsOpen())
	{
		dbg_msg("ident", "failed to load identities from json file, trying legacy...");
		m_pClient->Engine()->WriteErrorLog("identities", "failed to load identities from json file (file not accessible)");
		return LoadIdentsLegacy();
	}

	unsigned int Len;
	char *pText = File.ReadAllTextRaw(&Len);
	json_value *pJson = json_parse(pText, Len);
	mem_free(pText); pText = NULL; Len = 0; // no more text
	if(!pJson)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident/ERROR", "failed to parse contents of json file!");
		m_pClient->Engine()->WriteErrorLog("identities", "failed to parse contents of json file");
		return false;
	}

	json_value& json = *pJson;
	if(json.type != json_array)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident/ERROR", "invalid identity json contents (not an array)");
		m_pClient->Engine()->WriteErrorLog("identities", "invalid identity json contents (not an array)");
		json_value_free(pJson);
		return false;
	}

	for(unsigned int i = 0; i < json.u.array.length ; i++)
	{
		const json_value &jsonOuter = json[i];
		const json_value &jsonInner = jsonOuter["identity"];
		if(jsonInner.type != json_object)
		{
			Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "ident/WARN", "error while parsing identity #%i, skipping it", i+1);
			m_pClient->Engine()->WriteErrorLog("identities", "error while parsing identity i=%i of %i total", i, json.u.array.length);
			continue;
		}

		CIdentEntry Entry = SingleIdentFromJson(jsonInner);
		Entry.m_StartingIndex = (int)(json_int_t)(jsonOuter["index"]);

		// add the entry to our array
		m_aIdentities.add(Entry);
	}

	m_aIdentities.sort_range();

	json_value_free(pJson);
	return true;
}

CIdentity::CIdentEntry CIdentity::SingleIdentFromJson(const json_value &jsonInner) const
{
	CIdentEntry Entry;

	str_copyb(Entry.m_aTitle, (const char *)jsonInner["title"]);
	str_copyb(Entry.m_aName, (const char *)jsonInner["name"]);
	str_copyb(Entry.m_aClan, (const char *)jsonInner["clan"]);
	Entry.m_Country = (int)(json_int_t)jsonInner["country"];
	str_copyb(Entry.m_aSkin, (const char *)jsonInner["skin"]);
	Entry.m_UseCustomColor = (bool)jsonInner["use_custom_color"];
	Entry.m_ColorBody = (int)(json_int_t)jsonInner["color_body"];
	Entry.m_ColorFeet = (int)(json_int_t)jsonInner["color_feet"];

	return Entry;
}

bool CIdentity::LoadIdentsLegacy()
{
	m_aIdentities.clear();
	Storage()->ListDirectory(IStorageTW::TYPE_SAVE, "identities", FindIDFiles, this);
	if(!m_aIdentities.size())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "identities", "failed to load identities.");
		m_pClient->Engine()->WriteErrorLog("identities", "failed to load identities (legacy)");
		return false;
	}
	return true;
}

json_value *CIdentity::GetIdentAsJson(int i, bool Packed)
{
	const CIdentEntry *pIdent = GetIdent(i);
	json_value *inner_obj = json_object_new(8);

	#define add_str(NAME, STR) \
		if(!Packed || (STR)[0] != '\0') \
			json_object_push(inner_obj, NAME, json_string_new(STR));

	#define add_int(NAME, VAR) \
		if(!Packed || (VAR) != 0) \
			json_object_push(inner_obj, NAME, json_integer_new(VAR));

	#define add_bool(NAME, VAR) \
		if(!Packed || (VAR) != false) \
			json_object_push(inner_obj, NAME, json_boolean_new(VAR));

	add_str("title", pIdent->m_aTitle);
	add_str("name", pIdent->m_aName);
	add_str("clan", pIdent->m_aClan);
	add_int("country", pIdent->m_Country);
	add_str("skin", pIdent->m_aSkin);
	add_bool("use_custom_color", pIdent->m_UseCustomColor);
	add_int("color_body", pIdent->m_ColorBody);
	add_int("color_feet", pIdent->m_ColorFeet);

	#undef add_str
	#undef add_int
	#undef add_bool

	return inner_obj;
}

void CIdentity::SaveIdents()
{
	if(NumIdents() == 0)
		return;

	json_value *arr = json_array_new((unsigned)NumIdents());
	for(int i = 0; i < NumIdents(); i++)
	{
		json_value *outer_obj = json_object_new(2);
		json_value *inner_obj = GetIdentAsJson(i);

		json_object_push(outer_obj, "index", json_integer_new(i));
		json_object_push(outer_obj, "identity", inner_obj);

		json_array_push(arr, outer_obj);
	}

	char *pJsonBuf = mem_allocb(char, json_measure_ex(arr, json_opts_common));
	json_serialize_ex(pJsonBuf, arr, json_opts_common);
	json_value_free(arr);

	// use safe write; back up the old file before writing the new one
	Storage()->RenameFile(ALL_IDS_JSON_FILE, ALL_IDS_JSON_FILE ".bak", IStorageTW::TYPE_SAVE);
	IOHANDLE_SMART File = Storage()->OpenFileSmart(ALL_IDS_JSON_FILE, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
	if(File.IsOpen())
	{
		dbg_msg("ident", "saving %i identities to file '%s'", NumIdents(), File.GetPath());

		unsigned int Size = File.WriteString(pJsonBuf, false);
		unsigned int StrLen = (unsigned)str_length(pJsonBuf);
		if(Size != StrLen)
			m_pClient->Engine()->WriteErrorLog("identities", "Written size is not equal string length (written=%i, expected=%i)", Size, StrLen);

		if(g_Config.m_Debug)
			dbg_msg("ident", "done, %u bytes written.", Size);
	}
	else
		m_pClient->Engine()->WriteErrorLog("identities", "failed to open file '%s' (%s) for writing; cannot save identities!", ALL_IDS_JSON_FILE, File.GetPath());

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


char *CIdentity::GetIdentAsJsonStr(int i)
{
	json_value *pJson = GetIdentAsJson(i, true);

	char *pJsonBuf = mem_allocb(char, json_measure_ex(pJson, json_opts_packed));
	json_serialize_ex(pJsonBuf, pJson, json_opts_packed);

	json_value_free(pJson);

	return pJsonBuf;
}

void CIdentity::AddIdentFromJson(const char *pJsonString)
{
	json_value *pJson = json_parse(pJsonString, (unsigned)str_length(pJsonString));
	if(!pJson)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "ident/ERROR", "failed to json-parse string from clipboard", true);
		return;
	}

	AddIdent(SingleIdentFromJson(*pJson));

	json_value_free(pJson);
}
