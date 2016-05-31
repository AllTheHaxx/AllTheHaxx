
#include <engine/console.h>

#include "fontmgr.h"

void CFontMgr::Init()
{
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_FontFiles.clear();
	m_FontFiles.hint_size(10);

	ReloadFontlist();

	if(g_Config.m_FtPreloadFonts)
		for(int i = 0; i < m_FontFiles.size(); i++)
			InitFont(&m_FontFiles[i]);

	// load default font
	const char *pFontFile = "fonts/DejaVuSansCJKName.ttf";
	if (str_find(g_Config.m_ClLanguagefile, "chinese") != NULL || str_find(g_Config.m_ClLanguagefile, "japanese") != NULL ||
		str_find(g_Config.m_ClLanguagefile, "korean") != NULL)
		pFontFile = "fonts/DejavuWenQuanYiMicroHei.ttf";

	for(int i = 0; i < m_FontFiles.size(); i++)
	{
		if(str_comp_nocase(m_FontFiles[i].m_Path.c_str(), pFontFile) == 0)
			ActivateFont(i);
	}
}

void CFontMgr::ActivateFont(int ListIndex)
{
	if(ListIndex > m_FontFiles.size())
	{
		dbg_msg("fontmgr/error", "tried to load font that doesn't exist (%i > %i)", ListIndex, m_FontFiles.size());
		return;
	}

	FontFile *f = &m_FontFiles[ListIndex];
	if(!f->m_pFont)
		InitFont(f);

	TextRender()->SetDefaultFont(f->m_pFont);
	m_ActiveFontIndex = ListIndex;
}

void CFontMgr::InitFont(FontFile *f)
{
	if(g_Config.m_Debug)
		dbg_msg("fontmgr/debug", "loading font %p '%s' into memory", f, f->m_Path.c_str());

	char aFilename[512];
	IOHANDLE File = Storage()->OpenFile(f->m_Path.c_str(), IOFLAG_READ, IStorageTW::TYPE_ALL, aFilename, sizeof(aFilename));
	if(File)
	{
		io_close(File);
		f->m_pFont = TextRender()->LoadFont(aFilename);
	}
	else
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "fontmgr/error", "failed to load font. file='%s'", f->m_Path.c_str());
}

void CFontMgr::LoadFolder(const char *pFolder)
{
	dbg_msg("fontmgr", "scanning folder '%s'", pFolder);
	IStorageTW::CLoadHelper<CFontMgr> *pParams = new IStorageTW::CLoadHelper<CFontMgr>;
	pParams->pSelf = this;
	pParams->pFullDir = pFolder;

	m_pStorage->ListDirectory(IStorageTW::TYPE_ALL, pFolder, LoadFolderCallback, pParams);

	delete pParams;

	SortList();
}

int CFontMgr::LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser)
{
	if(pName[0] == '.')
		return 0;

	IStorageTW::CLoadHelper<CFontMgr> *pParams = (IStorageTW::CLoadHelper<CFontMgr> *)pUser;

	CFontMgr *pSelf = pParams->pSelf;
	const char *pFullDir = pParams->pFullDir;

	char aFile[64];
	str_format(aFile, sizeof(aFile), "%s/%s", pFullDir, pName);
	//dbg_msg("fontloader", "%s", aFile);

	if(IsDir)
		pSelf->LoadFolder(aFile);
	else
		pSelf->m_FontFiles.add(FontFile(aFile));

	return 0;
}

void CFontMgr::SortList()
{
	const int NUM = m_FontFiles.size();
	if(NUM < 2)
		return;

	for(int curr = 0; curr < NUM-1; curr++)
	{
		int minIndex = curr;
		for(int i = curr + 1; i < NUM; i++)
		{
			int c = 4;
			for(; str_uppercase(m_FontFiles[i].m_Path.c_str()[c]) == str_uppercase(m_FontFiles[minIndex].m_Path.c_str()[c]); c++);
			if(str_uppercase(m_FontFiles[i].m_Path.c_str()[c]) < str_uppercase(m_FontFiles[minIndex].m_Path.c_str()[c]))
				minIndex = i;
		}

		if(minIndex != curr)
		{
			FontFile temp = m_FontFiles[curr];
			m_FontFiles[curr] = m_FontFiles[minIndex];
			m_FontFiles[minIndex] = temp;
		}
	}
}

void CFontMgr::ReloadFontlist()
{
	// delete the fonts from memory first and clear the list
	while(m_FontFiles.size() > 0)
	{
		if(m_FontFiles[0].m_pFont)
			TextRender()->DestroyFont(m_FontFiles[0].m_pFont);
		m_FontFiles.remove_index_fast(0);
	}

	LoadFolder("fonts");
}
