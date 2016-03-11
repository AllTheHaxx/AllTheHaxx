/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_GSKINS_H
#define GAME_CLIENT_COMPONENTS_GSKINS_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

class CgSkins : public CComponent
{
public:
	// do this better and nicer
	struct CgSkin
	{
		int m_Texture;
		char m_aName[24];

		bool operator<(const CgSkin &Other) { return str_comp(m_aName, Other.m_aName) < 0; }
	};

	void OnInit();

	vec3 GetColorV3(int v);
	vec4 GetColorV4(int v);
	int Num();
	const CgSkin *Get(int Index);
	int Find(const char *pName);

private:
	sorted_array<CgSkin> m_aSkins;

	static int SkinScan(const char *pName, int IsDir, int DirType, void *pUser);
};
#endif
