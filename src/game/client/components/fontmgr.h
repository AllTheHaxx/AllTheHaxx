#ifndef GAME_CLIENT_COMPONENTS_FONTMGR_H
#define GAME_CLIENT_COMPONENTS_FONTMGR_H

#include <string>

#include <base/tl/array.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/client/component.h>

struct CFontFile
{
	enum
	{
		REGULAR = 0,
		BOLD,
		ITALIC,
		BOLD_ITALIC,
		NUM_TYPES
	};

	std::string m_Path;
	CFont *m_apFonts[NUM_TYPES];

	CFontFile() { clear(); }
	CFontFile(const std::string& Path) { clear(); m_Path = Path; }

	void clear()
	{
		m_Path = "";
		mem_zero(m_apFonts, sizeof(m_apFonts));
	}
};

class CFontMgr : public CComponent
{
	array<CFontFile> m_lFontFiles;
	array<CFontFile> m_lMonoFontFiles;
	int m_ActiveFontIndex;
	int m_ActiveMonoFontIndex;

	void SortList(bool mono);
	void LoadFolder(const char *pFolder, bool mono);
	void InitFont(CFontFile *f);


public:
	void Init();
	void ReloadFontlist();
	void ActivateFont(int ListIndex);

	int GetNumFonts() const { return m_lFontFiles.size(); }
	int GetNumLoadedFonts() const { int ret = 0; for(int i = 0; i < m_lFontFiles.size(); i++) if(m_lFontFiles[i].m_apFonts[0]) ret++; return ret; }
	int GetSelectedFontIndex() const { return m_ActiveFontIndex; }
	int GetSelectedMonoFontIndex() const { return m_ActiveMonoFontIndex; }
	const char *GetFontPath(int i) const { if(i >= 0 && i < m_lFontFiles.size()) return m_lFontFiles[i].m_Path.c_str(); return ""; }
	CFontFile *GetFont(int i = -1)
	{
		if(i < 0) i = m_ActiveFontIndex;
		if(i >= 0 && i < m_lFontFiles.size())
			return &m_lFontFiles[i];
		return 0;
	}
	CFontFile *GetMonoFont(int i = -1)
	{
		if(i < 0) i = m_ActiveMonoFontIndex;
		if(i >= 0 && i < m_lMonoFontFiles.size())
			return &m_lMonoFontFiles[i];
		return 0;
	}

protected:
	IStorageTW *m_pStorage;

private:
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);

};

#endif

