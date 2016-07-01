#ifndef GAME_CLIENT_COMPONENTS_IDENTITY_H
#define GAME_CLIENT_COMPONENTS_IDENTITY_H

#include <algorithm>
#include <base/tl/sorted_array.h>
#include <engine/shared/linereader.h>
#include <game/client/component.h>

class CIdentity : public CComponent
{
public:
	void OnInit();
	void SaveIdents();

	struct CIdentEntry
	{
		char m_aFilename[64];
		char m_aTitle[16];

		char m_aName[16];
		char m_aClan[12];

		int m_Country;

		char m_aSkin[24];
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;

		// overloading n shit
		bool operator<(const CIdentEntry &Other) { return str_comp(m_aFilename, Other.m_aFilename) < 0; }
	};

	enum
	{
		NAME=0,
		CLAN,
		COUNTRY,
		SKIN,
		USE_CUSTOM_COLOR,
		COLOR_BODY,
		COLOR_FEET,
		TITLE,
		NUM_ENTRIES
	};

	// getter and setter functions
	inline int NumIdents() { return m_aIdentities.size(); }
	inline void AddIdent(CIdentEntry Entry) { m_aIdentities.add_unsorted(Entry); }
	inline void DeleteIdent(int Ident)
	{
		char aFile[64];
		str_format(aFile, sizeof(aFile), "identities/%s", m_aIdentities[Ident].m_aFilename);
		Storage()->RemoveFile(aFile, IStorageTW::TYPE_SAVE);
		m_aIdentities.remove_index(Ident);
	}
	inline int GetIdentID(const char *pName)
	{
		for(int i = 0; i < NumIdents(); i++)
		{
			if(str_comp(m_aIdentities[i].m_aFilename, pName) == 0)
				return i;
		}
		return -1;
	}
	CIdentEntry *GetIdent(int i)
	{
		if(NumIdents())
			return &m_aIdentities[max(0, i%NumIdents())];
		else
			return NULL;
	}
	inline void SwapIdent(int i, int Dir)
	{
		std::swap(m_aIdentities[i+Dir], m_aIdentities[i]);
	}

	bool UsingIdent(int i)
	{
		CIdentity::CIdentEntry *pIdent = GetIdent(i);
		if(pIdent &&
				str_comp(g_Config.m_PlayerName, pIdent->m_aName) == 0 &&
				str_comp(g_Config.m_PlayerClan, pIdent->m_aClan) == 0 &&
				str_comp(g_Config.m_ClPlayerSkin, pIdent->m_aSkin) == 0 &&
				g_Config.m_ClPlayerUseCustomColor == pIdent->m_UseCustomColor &&
				g_Config.m_ClPlayerColorBody == pIdent->m_ColorBody &&
				g_Config.m_ClPlayerColorFeet == pIdent->m_ColorFeet &&
				g_Config.m_PlayerCountry == pIdent->m_Country)
			return true;
		return false;
	};

	bool UsingIdentDummy(int i)
	{
		CIdentity::CIdentEntry *pIdent = GetIdent(i);
		if(pIdent &&
		   str_comp(g_Config.m_ClDummyName, pIdent->m_aName) == 0 &&
		   str_comp(g_Config.m_ClDummyClan, pIdent->m_aClan) == 0 &&
		   str_comp(g_Config.m_ClDummySkin, pIdent->m_aSkin) == 0 &&
		   g_Config.m_ClDummyUseCustomColor == pIdent->m_UseCustomColor &&
		   g_Config.m_ClDummyColorBody == pIdent->m_ColorBody &&
		   g_Config.m_ClDummyColorFeet == pIdent->m_ColorFeet &&
		   g_Config.m_ClDummyCountry == pIdent->m_Country)
			return true;
		return false;
	};

	void ApplyIdent(int i)
	{
		CIdentity::CIdentEntry *pIdent = m_pClient->m_pIdentity->GetIdent(i);
		str_format(g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), pIdent->m_aName);
		str_format(g_Config.m_PlayerClan, sizeof(g_Config.m_PlayerClan), pIdent->m_aClan);
		str_format(g_Config.m_ClPlayerSkin, sizeof(g_Config.m_ClPlayerSkin), pIdent->m_aSkin);
		g_Config.m_ClPlayerUseCustomColor = pIdent->m_UseCustomColor;
		g_Config.m_ClPlayerColorBody = pIdent->m_ColorBody;
		g_Config.m_ClPlayerColorFeet = pIdent->m_ColorFeet;
		m_pClient->SendInfo(false);
	};

	void ApplyIdentDummy(int i)
	{
		CIdentity::CIdentEntry *pIdent = m_pClient->m_pIdentity->GetIdent(i);
		str_format(g_Config.m_ClDummyName, sizeof(g_Config.m_PlayerName), pIdent->m_aName);
		str_format(g_Config.m_ClDummyClan, sizeof(g_Config.m_PlayerClan), pIdent->m_aClan);
		str_format(g_Config.m_ClDummySkin, sizeof(g_Config.m_ClPlayerSkin), pIdent->m_aSkin);
		g_Config.m_ClDummyUseCustomColor = pIdent->m_UseCustomColor;
		g_Config.m_ClDummyColorBody = pIdent->m_ColorBody;
		g_Config.m_ClDummyColorFeet = pIdent->m_ColorFeet;
		m_pClient->SendInfo(false);
	};

private:
	static int FindIDFiles(const char *pName, int IsDir, int DirType, void *pUser);

	sorted_array<CIdentEntry> m_aIdentities;
};

#endif
