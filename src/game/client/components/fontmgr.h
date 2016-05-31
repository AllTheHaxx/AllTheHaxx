
#include <string>

#include <base/tl/array.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/client/component.h>

class CFontMgr : public CComponent
{
	struct FontFile
	{
		FontFile() { m_Path = ""; m_pFont = 0; }
		FontFile(std::string Path) { m_Path = Path; m_pFont = 0; }
		std::string m_Path;
		CFont *m_pFont;
	};

	array<FontFile> m_FontFiles;
	int m_ActiveFontIndex;

	void SortList();
	void LoadFolder(const char *pFolder);
	void InitFont(FontFile *f);


public:
	void Init();
	void ReloadFontlist();
	void ActivateFont(int ListIndex);

	int GetNumFonts() const { return m_FontFiles.size(); }
	int GetNumLoadedFonts() const { int ret = 0; for(int i = 0; i < m_FontFiles.size(); i++) if(m_FontFiles[i].m_pFont) ret++; return ret; }
	int GetSelectedFontIndex() const { return m_ActiveFontIndex; }
	const char *GetFontPath(int i) const { if(i >= 0 && i < m_FontFiles.size()) return m_FontFiles[i].m_Path.c_str(); return ""; }
	CFont *GetFont(int i) const { if(i >= 0 && i < m_FontFiles.size()) return m_FontFiles[i].m_pFont; return 0; }

protected:
	IStorageTW *m_pStorage;

private:
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);

};
