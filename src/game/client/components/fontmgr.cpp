
#include <engine/console.h>
#include <engine/shared/config.h>

#include "fontmgr.h"

#define ConfigFtFont (m_Type == TYPE_BASIC ? g_Config.m_FtFont : g_Config.m_FtMonoFont)
static const char *s_apFontFolders[] = { "basic", "mono" };

void CFontMgr::OnShutdown()
{
	for(int i = 0; i < m_lFontFiles.size(); i++)
		UnloadFont(i);
	m_lFontFiles.clear();
}

void CFontMgr::Init()
{
	m_lFontFiles.clear();
	m_ActiveFontIndex = -1;

	ReloadFontlist();

	// load selected font
	if(str_length(ConfigFtFont) == 0)
		str_copy(ConfigFtFont, "DejaVuSansCJKName", sizeof(ConfigFtFont));

	char aFontFile[256] = {0};
	if(str_comp(ConfigFtFont, "DejaVuSansCJKName") == 0)
	{
		// for the combination of DejaVuSansCJKName and one of these languages, use a replacement font
		if(str_find(g_Config.m_ClLanguagefile, "chinese") != NULL || str_find(g_Config.m_ClLanguagefile, "japanese") != NULL || str_find(g_Config.m_ClLanguagefile, "korean") != NULL)
			str_copy(ConfigFtFont, "DejavuWenQuanYiMicroHei", sizeof(ConfigFtFont));
	}
	if(str_length(aFontFile) == 0)
		str_copy(aFontFile, ConfigFtFont, sizeof(ConfigFtFont));

	for(int i = 0; i < m_lFontFiles.size(); i++)
	{
		if(str_comp(m_lFontFiles[i].m_Name.c_str(), aFontFile) == 0)
		{
			ActivateFont(i);
			return;
		}
	}

	// if all fails, just pick the first in our list
	ActivateFont(0);
}

bool CFontMgr::ActivateFont(int ListIndex)
{
	if(ListIndex >= m_lFontFiles.size())
	{
		dbg_msg("fontmgr/error", "tried to load font that doesn't exist (%i >= %i)", ListIndex, m_lFontFiles.size());
		return false;
	}

	CFontFile *f = &m_lFontFiles[ListIndex];

	if(InitFont(f))
	{
		if(m_Type == TYPE_BASIC)
			TextRender()->SetDefaultFont(f->m_apFonts[FONT_REGULAR]);
		if(m_ActiveFontIndex >= 0)
			UnloadFont(m_ActiveFontIndex);
		m_ActiveFontIndex = ListIndex;
			str_copy(ConfigFtFont, m_lFontFiles[m_ActiveFontIndex].m_Name.c_str(), sizeof(ConfigFtFont));
		return true;
	}
	return false;
}

bool CFontMgr::InitFont(CFontFile *f)
{
	if(g_Config.m_Debug)
		dbg_msg("fontmgr/debug", "loading font %p '%s' into memory", f, f->m_Name.c_str());

	// load regular
	if(!InitFont_impl(f, FONT_REGULAR, "r"))
		if(!InitFont_impl(f, FONT_REGULAR, "R"))
			if(!InitFont_impl(f, FONT_REGULAR, "-R"))
				if(!InitFont_impl(f, FONT_REGULAR, "-Regular"))
					if(!InitFont_impl(f, FONT_REGULAR, "Regular"))
						if(!InitFont_impl(f, FONT_REGULAR, ""))
							Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "fontmgr/error", "failed to load REGULAR font type for font='%s'", f->m_Name.c_str());

	// load bold
	if(!InitFont_impl(f, FONT_BOLD, "b"))
		if(!InitFont_impl(f, FONT_BOLD, "B"))
			if(!InitFont_impl(f, FONT_BOLD, "-B"))
				if(!InitFont_impl(f, FONT_BOLD, "-Bold"))
					if(!InitFont_impl(f, FONT_BOLD, "Bold"))
						Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "fontmgr/error", "failed to load BOLD font type for font='%s'", f->m_Name.c_str());

	// load italic
	if(!InitFont_impl(f, FONT_ITALIC, "i"))
		if(!InitFont_impl(f, FONT_ITALIC, "I"))
			if(!InitFont_impl(f, FONT_ITALIC, "-I"))
				if(!InitFont_impl(f, FONT_ITALIC, "-Italic"))
					if(!InitFont_impl(f, FONT_ITALIC, "Italic"))
						if(!InitFont_impl(f, FONT_ITALIC, "-Oblique"))
							if(!InitFont_impl(f, FONT_ITALIC, "Oblique"))
								Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "fontmgr/error", "failed to load ITALIC font type for font='%s'", f->m_Name.c_str());

	// load bold italic
	if(!InitFont_impl(f, FONT_BOLD_ITALIC, "bi"))
		if(!InitFont_impl(f, FONT_BOLD_ITALIC, "BI"))
			if(!InitFont_impl(f, FONT_BOLD_ITALIC, "-BI"))
				if(!InitFont_impl(f, FONT_BOLD_ITALIC, "-BoldItalic"))
					if(!InitFont_impl(f, FONT_BOLD_ITALIC, "BoldItalic"))
						if(!InitFont_impl(f, FONT_BOLD_ITALIC, "-BoldOblique"))
							if(!InitFont_impl(f, FONT_BOLD_ITALIC, "BoldOblique"))
								Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "fontmgr/error", "failed to load BOLD ITALIC font type for font='%s'", f->m_Name.c_str());

	return f->m_apFonts[FONT_REGULAR] != NULL;
}

bool CFontMgr::InitFont_impl(CFontFile *f, int Type, const char *pTypeStr)
{
	char aPath[512];
	mem_zerob(aPath);
	if(str_length(pTypeStr) > 2 || pTypeStr[0] == '-' || pTypeStr[0] == '\0')
		str_formatb(aPath, "fonts/%s/%s/%s%s.ttf", s_apFontFolders[m_Type], f->m_Name.c_str(), f->m_Name.c_str(), pTypeStr);
	else
		str_formatb(aPath, "fonts/%s/%s/%s.ttf", s_apFontFolders[m_Type], f->m_Name.c_str(), pTypeStr);

	char aFullPath[512];
	IOHANDLE File = Storage()->OpenFile(aPath, IOFLAG_READ, IStorageTW::TYPE_ALL, aFullPath, sizeof(aFullPath));
	if(File)
	{
		io_close(File);
		f->m_apFonts[Type] = TextRender()->LoadFont(aFullPath);
		return true;
	}
	else
	{
#if defined(CONF_FAMILY_UNIX)
		// search system fonts directories
		str_formatb(aFullPath, "/usr/local/share/%s", aPath);
		File = io_open(aFullPath, IOFLAG_READ);
		if(File)
		{
			io_close(File);
			f->m_apFonts[Type] = TextRender()->LoadFont(aFullPath);
			return true;
		}

		str_formatb(aFullPath, "/usr/share/%s", aPath);
		File = io_open(aFullPath, IOFLAG_READ);
		if(File)
		{
			io_close(File);
			f->m_apFonts[Type] = TextRender()->LoadFont(aFullPath);
			return true;
		}
#endif
		if(g_Config.m_Debug)
			dbg_msg("fontmgr/debug", "failed trying to load '%s'", aPath);
	}
	return false;
}

void CFontMgr::UnloadFont(int ListIndex)
{
	CFontFile *pFontFile = &m_lFontFiles[ListIndex];

	for(int i = 0; i < FONT_NUM_TYPES; i++)
	{
		CFont* pFont = pFontFile->m_apFonts[i];
		if(pFont)
		{
			if(g_Config.m_Debug)
				dbg_msg("fontmgr/debug", "destroying font data [%s:%i]:%p", pFontFile->m_Name.c_str(), i, pFont);
			TextRender()->DestroyFont(pFont);
		}
	}
	mem_zerob(pFontFile->m_apFonts);
	if(g_Config.m_Debug)
		dbg_msg("fontmgr", "unloaded font '%s'", pFontFile->m_Name.c_str());
}

void CFontMgr::LoadFolder()
{
	char aPath[64];
	str_formatb(aPath, "fonts/%s", s_apFontFolders[m_Type]);
	Storage()->ListDirectory(IStorageTW::TYPE_ALL, aPath, LoadFolderCallback, &m_lFontFiles);
	m_lFontFiles.sort_range();
}

int CFontMgr::LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser)
{
	if(pName[0] == '.')
		return 0;

	sorted_array<CFontFile> *pList = (sorted_array<CFontFile>*)pUser;

	char aFile[128];
	str_format(aFile, sizeof(aFile), "%s", pName);
	dbg_msg("fontloader", "%s", aFile);

	// make sure we don't add fonts multiple times
	for(int i = 0; i < pList->size(); i++)
		if((*pList)[i].m_Name == std::string(aFile))
			return 0;

	if(IsDir)
	{
		pList->add_unsorted(CFontFile(std::string(aFile)));
	}
	else
	{
		dbg_msg("fontmgr", "WARNING: found '%s' in the fonts root directory", aFile);
		dbg_msg("fontmgr", "WARNING: fonts must be in the subfolder <name>/r.ttf to be loaded!");
	}

	return 0;
}

void CFontMgr::ReloadFontlist()
{
	LoadFolder();
}
