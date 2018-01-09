#ifndef GAME_CLIENT_COMPONENTS_FONTMGR_H
#define GAME_CLIENT_COMPONENTS_FONTMGR_H

#include <string>

#include <base/tl/sorted_array.h>
#include <game/client/component.h>
#include <engine/storage.h>
#include <engine/textrender.h>


struct CFontFile
{
	std::string m_Name;
	CFont *m_apFonts[FONT_NUM_TYPES];

	CFontFile() { clear(); }
	CFontFile(const std::string& Name) { clear(); m_Name = Name; }

	void clear()
	{
		m_Name = "";
		mem_zero(m_apFonts, sizeof(m_apFonts));
	}

	bool operator<(const CFontFile& other) { return str_comp(this->m_Name.c_str(), other.m_Name.c_str()) < 0; }
};

class IFontMgr : public CComponent
{
public:
	enum { TYPE_BASIC, TYPE_MONO };

	virtual void Init() = 0;
	virtual void ReloadFontlist() = 0;
	virtual bool ActivateFont(int ListIndex) = 0;

	virtual int GetNumFonts() const = 0;
	virtual int GetSelectedFontIndex() const = 0;
	virtual CFont *GetSelectedFont() = 0;
	virtual const char *GetFontPath(int ListIndex) const = 0;
	virtual CFont *GetFont(int FontFace) = 0;
};

class CFontMgr : public IFontMgr
{
	int m_ActiveFontIndex;
	sorted_array<CFontFile> m_lFontFiles;

	void LoadFolder();
	bool InitFont(CFontFile *f);
	bool InitFont_impl(CFontFile *f, int Type, const char *pTypeStr);
	void UnloadFont(int ListIndex);

public:
	const int m_Type;
	CFontMgr(int Type)
			: m_Type(Type)
	{
	}

	virtual void OnShutdown();

	void Init();
	void ReloadFontlist();
	bool ActivateFont(int ListIndex);

	inline int GetNumFonts() const { return m_lFontFiles.size(); }
	inline int GetSelectedFontIndex() const { return m_ActiveFontIndex; }
	CFont *GetSelectedFont() { return GetFont(m_ActiveFontIndex); }
	const char *GetFontPath(int i) const { if(i >= 0 && i < m_lFontFiles.size()) return m_lFontFiles[i].m_Name.c_str(); return ""; }
	CFont *GetFont(int FontFace)
	{
		if(FontFace < 0 || FontFace >= FONT_NUM_TYPES)
			FontFace = FONT_REGULAR;
		CFont *pFont = m_lFontFiles[m_ActiveFontIndex].m_apFonts[FontFace];
		if(!pFont)
			pFont = m_lFontFiles[m_ActiveFontIndex].m_apFonts[FONT_REGULAR]; // default to regular if wanted fontface not present
		return pFont;
	}

private:
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);

};

#endif

