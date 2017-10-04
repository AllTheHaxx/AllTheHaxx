#include <engine/serverbrowser.h>
#include <base/system.h>

// gametypes

bool IsVanilla(const CServerInfo *pInfo)
{
	return !str_comp(pInfo->m_aGameType, "DM")
	    || !str_comp(pInfo->m_aGameType, "TDM")
	    || !str_comp(pInfo->m_aGameType, "CTF");
}

bool IsCatch(const CServerInfo *pInfo)
{
	return str_find_nocase(pInfo->m_aGameType, "catch") != 0;
}

bool IsInsta(const CServerInfo *pInfo)
{
	return str_find_nocase(pInfo->m_aGameType, "idm")
	    || str_find_nocase(pInfo->m_aGameType, "itdm")
	    || str_find_nocase(pInfo->m_aGameType, "ictf");
}

bool IsFNG(const CServerInfo *pInfo)
{
	return str_find_nocase(pInfo->m_aGameType, "fng") != 0;
}

bool IsRace(const CServerInfo *pInfo)
{
	return str_find_nocase(pInfo->m_aGameType, "race")
	    || str_find_nocase(pInfo->m_aGameType, "fastcap");
}

bool IsDDRace(const char *pGameType)
{
	return str_find_nocase(pGameType, "ddrace")
		   || str_find_nocase(pGameType, "mkrace");
}
bool IsDDRace(const CServerInfo *pInfo)
{
	return IsDDRace(pInfo->m_aGameType);
}
bool IsDDNet(const char *pGameType)
{
	return str_find_nocase(pGameType, "ddracenet")
	    || str_find_nocase(pGameType, "ddnet");

}
bool IsDDNet(const CServerInfo *pInfo)
{
	return IsDDNet(pInfo->m_aGameType);
}

// other
bool IsBWMod(const char *pGameType)
{
	return (str_comp_nocase_num(pGameType, "bw", 2) == 0 && str_length(pGameType) > 3 && pGameType[2] == ' ' && pGameType[3] == ' ')
		   || str_comp_nocase(pGameType, "bw") == 0;

}
bool IsBWMod(const CServerInfo *pInfo)
{
	return IsBWMod(pInfo->m_aGameType);
}

bool Is64Player(const CServerInfo *pInfo)
{
	return str_find(pInfo->m_aGameType, "64")
	    || str_find(pInfo->m_aName, "64")
		|| str_comp(pInfo->m_aGameType, "iF|City") == 0
	    || IsDDNet(pInfo) || IsBWMod(pInfo);
}

bool IsPlus(const CServerInfo *pInfo)
{
	return str_find(pInfo->m_aGameType, "+") != 0;
}
