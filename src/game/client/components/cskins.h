/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CSKINS_H
#define GAME_CLIENT_COMPONENTS_CSKINS_H
#include <base/vmath.h>
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

class CcSkins : public CComponent
{
public:
	// do this better and nicer
	struct CcSkin
	{
		int m_Texture;
		char m_aName[24];

		bool operator<(const CcSkin &Other) { return str_comp(m_aName, Other.m_aName) < 0; }
	};

	void OnInit();

	vec3 GetColorV3(int v);
	vec4 GetColorV4(int v);
	int Num();
	const CcSkin *Get(int Index);
	int Find(const char *pName);

private:
	sorted_array<CcSkin> m_aSkins;

	static int SkinScan(const char *pName, int IsDir, int DirType, void *pUser);
};
#endif
