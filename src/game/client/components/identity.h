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
		NUM_ENTRIES,
	};

	// getter and setter functions
	inline int NumIdents() { return m_aIdentities.size(); }
	inline void AddIdent(CIdentEntry Entry) { m_aIdentities.add(Entry); }
	inline void DeleteIdent(int Ident)
	{
		char aFile[64];
		str_format(aFile, sizeof(aFile), "identities/%s", m_aIdentities[Ident].m_aFilename);
		Storage()->RemoveFile(aFile, IStorage::TYPE_SAVE);
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

private:
	static int FindIDFiles(const char *pName, int IsDir, int DirType, void *pUser);

	sorted_array<CIdentEntry> m_aIdentities;
};

#endif
