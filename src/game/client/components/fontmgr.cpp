
#include <engine/console.h>

#include "fontmgr.h"

void CFontMgr::Init()
{
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_lFontFiles.clear();
	m_lFontFiles.hint_size(10);
	m_lMonoFontFiles.clear();
	m_lMonoFontFiles.hint_size(10);
	m_ActiveMonoFontIndex = -1;

	ReloadFontlist();

	if(g_Config.m_FtPreloadFonts)
		for(int i = 0; i < m_lFontFiles.size(); i++)
			InitFont(&m_lFontFiles[i]);

	// load default font
	char aFontFile[256];
	str_format(aFontFile, sizeof(aFontFile), "%s", g_Config.m_FtFont);
	if(str_comp(g_Config.m_FtFont, "DejaVuSansCJKName") == 0)
		if (str_find(g_Config.m_ClLanguagefile, "chinese") != NULL || str_find(g_Config.m_ClLanguagefile, "japanese") != NULL ||
			str_find(g_Config.m_ClLanguagefile, "korean") != NULL)
				str_format(aFontFile, sizeof(aFontFile), "DejavuWenQuanYiMicroHei");
	if(str_length(aFontFile) == 0)
		str_copy(aFontFile, "DejaVuSansCJKName", sizeof(aFontFile));

	for(int i = 0; i < m_lFontFiles.size(); i++)
	{
		dbg_msg("fonti", "'%s' '%s'", m_lFontFiles[i].m_Path.c_str(), aFontFile);
		if(str_comp(m_lFontFiles[i].m_Path.c_str(), aFontFile) == 0)
		{
			dbg_msg("fonti", "YEAH FOUND IT");
			ActivateFont(i);
			break;
		}
	}


	// load default mono font
	char aMonoFontFile[256];
	if(str_length(g_Config.m_FtMonoFont) > 0)
		str_format(aMonoFontFile, sizeof(aMonoFontFile), ".mono/%s", g_Config.m_FtMonoFont);
	else
		str_copy(aMonoFontFile, ".mono/UbuntuMono", sizeof(aFontFile));

	for(int i = 0; i < m_lMonoFontFiles.size(); i++)
	{
		// load default mono font
		if(str_comp(m_lMonoFontFiles[i].m_Path.c_str(), aMonoFontFile) == 0)
		{
			InitFont(&m_lMonoFontFiles[i]);
			m_ActiveMonoFontIndex = i;
			break;
		}
	}
}

void CFontMgr::ActivateFont(int ListIndex)
{
	if(ListIndex > m_lFontFiles.size())
	{
		dbg_msg("fontmgr/error", "tried to load font that doesn't exist (%i > %i)", ListIndex, m_lFontFiles.size());
		return;
	}

	CFontFile *f = &m_lFontFiles[ListIndex];
	if(!f->m_apFonts[CFontFile::REGULAR])
		InitFont(f);

	if(f->m_apFonts[CFontFile::REGULAR])
	{
		TextRender()->SetDefaultFont(f->m_apFonts[CFontFile::REGULAR]);
		m_ActiveFontIndex = ListIndex;
	}
}

void CFontMgr::ActivateMonoFont(int ListIndex)
{
	if(ListIndex > m_lMonoFontFiles.size())
	{
		dbg_msg("fontmgr/error", "tried to load font that doesn't exist (%i > %i)", ListIndex, m_lMonoFontFiles.size());
		return;
	}

	CFontFile *f = &m_lMonoFontFiles[ListIndex];
	if(!f->m_apFonts[CFontFile::REGULAR])
		InitFont(f);

	if(f->m_apFonts[CFontFile::REGULAR])
	{
		m_ActiveMonoFontIndex = ListIndex;
	}
}

void CFontMgr::InitFont(CFontFile *f)
{
	if(g_Config.m_Debug)
		dbg_msg("fontmgr/debug", "loading font %p '%s' into memory", f, f->m_Path.c_str());

	static const char s_apTypes[4][3] = { "r", "b", "i", "bi" };

	for(int i = 0; i < CFontFile::NUM_TYPES; i++)
	{
		char aPath[512];
		str_format(aPath, sizeof(aPath), "fonts/%s/%s.ttf", f->m_Path.c_str(), s_apTypes[i]);

		char aFilename[512];
		IOHANDLE File = Storage()->OpenFile(aPath, IOFLAG_READ, IStorageTW::TYPE_ALL, aFilename, sizeof(aFilename));
		if(File)
		{
			io_close(File);
			f->m_apFonts[i] = TextRender()->LoadFont(aFilename);
		}
		else if(i == 0)
		{
			Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "fontmgr/error", "failed to load font. file='%s'", f->m_Path.c_str());
		}
	}
}

void CFontMgr::LoadFolder(const char *pFolder, bool mono)
{
	dbg_msg("fontmgr", "scanning folder '%s' (mono=%x)", pFolder, mono);
	array<CFontFile> *pList = &m_lFontFiles;
	if(mono)
		pList = &m_lMonoFontFiles;

	m_pStorage->ListDirectory(IStorageTW::TYPE_ALL, pFolder, LoadFolderCallback, pList);

	SortList(mono);
}

int CFontMgr::LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser)
{
	if(pName[0] == '.')
		return 0;

	array<CFontFile> *pList = (array<CFontFile>*)pUser;

	char aFile[128];
	str_format(aFile, sizeof(aFile), "%s", pName);
	dbg_msg("fontloader", "%s", aFile);

	// make sure we don't add fonts multiple times
	for(int i = 0; i < pList->size(); i++)
		if((*pList)[i].m_Path == std::string(aFile))
			return 0;

	if(IsDir)
	{
		pList->add(CFontFile(aFile));
	}
	else
	{
		dbg_msg("fontmgr", "WARNING: found '%s' in the fonts root directory", aFile);
		dbg_msg("fontmgr", "WARNING: fonts must be in the subfolder <name>/r.ttf to be loaded!");
	}

	return 0;
}

void CFontMgr::SortList(bool mono)
{
	array<CFontFile> *pList = &m_lFontFiles;
	if(mono)
		pList = &m_lMonoFontFiles;
	array<CFontFile>& list = *pList;

	const int NUM = list.size();
	if(NUM < 2)
		return;

	for(int curr = 0; curr < NUM-1; curr++)
	{
		int minIndex = curr;
		for(int i = curr + 1; i < NUM; i++)
		{
			int c = 4;
			for(; str_uppercase(list[i].m_Path.c_str()[c]) == str_uppercase(list[minIndex].m_Path.c_str()[c]); c++);
			if(str_uppercase(list[i].m_Path.c_str()[c]) < str_uppercase(list[minIndex].m_Path.c_str()[c]))
				minIndex = i;
		}

		if(minIndex != curr)
		{
			CFontFile temp = list[curr];
			list[curr] = list[minIndex];
			list[minIndex] = temp;
		}
	}
}

void CFontMgr::ReloadFontlist()
{
	// delete the fonts from memory first and clear the list
/*	while(m_FontFiles.size() > 0)
	{
		if(m_FontFiles[0].m_pFont)
			TextRender()->DestroyFont(m_FontFiles[0].m_pFont);
		m_FontFiles.remove_index_fast(0);
	}*/

	LoadFolder("fonts", false);
	LoadFolder("fonts/.mono", true);
	for(int i = 0; i < m_lMonoFontFiles.size(); i++)
		m_lMonoFontFiles[i].m_Path = ".mono/" + m_lMonoFontFiles[i].m_Path;
}
