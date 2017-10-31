/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_SKINS_H
#define GAME_CLIENT_COMPONENTS_SKINS_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

class CSkins : public CComponent
{
public:
	// do this better and nicer
	class CSkin
	{
		friend class CSkins;
	public:
		enum
		{
			SKIN_TEXTURE_NOT_LOADED = -3,
			SKIN_TEXTURE_LOADING = -2,
			SKIN_TEXTURE_NOT_FOUND = -1
		};

	private:
		CSkins *m_pSkins;

		volatile int m_OrgTexture;
		volatile int m_ColorTexture;
		char m_aName[64];
		vec3 m_BloodColor;
		bool m_IsVanilla;

		struct
		{
			char m_aFullPath[512];
			int m_DirType;
		} m_FileInfo;

	public:

		int GetColorTexture() const;
		int GetOrgTexture() const;
		const char *GetName() const { return m_aName; }
		const vec3& GetBloodColor() const { return m_BloodColor; }
		bool IsVanilla() const { return m_IsVanilla; }

		bool operator<(const CSkin &Other) { return str_comp(m_aName, Other.m_aName) < 0; }
	};

	void OnInit();
	void RefreshSkinList(bool clear = true);

	vec3 GetColorV3(int v);
	vec4 GetColorV4(int v);
	int Num();
	const CSkin *Get(int Index);
	int GetDefaultSkinColorTexture() const { return m_DefaultSkinColorTexture; }
	int GetDefaultSkinOrgTexture() const { return m_DefaultSkinOrgTexture; }
	int Find(const char *pName);
	void Clear();


private:
	int m_DefaultSkinColorTexture;
	int m_DefaultSkinOrgTexture;
	sorted_array<CSkin> m_aSkins;

	static int SkinScan(const char *pName, int IsDir, int DirType, void *pUser);

	void LoadTexturesImpl(CSkin *pSkin);
	void LoadTexturesThreaded(CSkin *pSkin);
	static void LoadTexturesThreadProxy(void *pUser);
};

#endif
