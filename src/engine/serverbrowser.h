/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SERVERBROWSER_H
#define ENGINE_SERVERBROWSER_H

#include <engine/shared/protocol.h>

#include "kernel.h"
#include <string>

/*
	Structure: CServerInfo
*/
class CServerInfo
{
public:
	/*
		Structure: CInfoClient
	*/
	class CClient
	{
	public:
		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		int m_Score;
		bool m_Player;

		int m_FriendState;
	};

	int m_SortedIndex;
	int m_ServerIndex;

	int m_Type;
	uint64 m_ReceivedPackets;
	int m_NumReceivedClients;

	NETADDR m_NetAddr;

	int m_QuickSearchHit;
	int m_FriendState;

	int m_MaxClients;
	int m_NumClients;
	int m_MaxPlayers;
	int m_NumPlayers;
	int m_Flags;
	int m_Favorite;
	int m_Latency; // in ms
	char m_aGameType[16];
	char m_aName[64];
	char m_aMap[32];
	int m_MapCrc;
	int m_MapSize;
	char m_aVersion[32];
	char m_aAddress[NETADDR_MAXSTRSIZE];
	CClient m_aClients[MAX_CLIENTS];

	std::string LuaGetGameType() const { return std::string(m_aGameType); }
	std::string LuaGetName() const { return std::string(m_aName); }
	std::string LuaGetMap() const { return std::string(m_aMap); }
	std::string LuaGetVersion() const { return std::string(m_aVersion); }
	std::string LuaGetIP() const { return std::string(m_aAddress); }
};

bool IsVanilla(const CServerInfo *pInfo);
bool IsCatch(const CServerInfo *pInfo);
bool IsInsta(const CServerInfo *pInfo);
bool IsFNG(const CServerInfo *pInfo);
bool IsRace(const CServerInfo *pInfo);
bool IsDDRace(const char *pGameType);
bool IsDDRace(const CServerInfo *pInfo);
bool IsDDNet(const char *pGameType);
bool IsDDNet(const CServerInfo *pInfo);
bool IsBWMod(const char *pGameType);
bool IsBWMod(const CServerInfo *pInfo);

bool Is64Player(const CServerInfo *pInfo);
bool IsPlus(const CServerInfo *pInfo);

class IServerBrowser : public IInterface
{
	MACRO_INTERFACE("serverbrowser", 0)
public:

	/* Constants: Server Browser Sorting
		SORT_NAME - Sort by name.
		SORT_PING - Sort by ping.
		SORT_MAP - Sort by map
		SORT_GAMETYPE - Sort by game type. DM, TDM etc.
		SORT_NUMPLAYERS - Sort after how many players there are on the server.
	*/
	enum{
		CACHE_VERSION = 3,

		SORT_NAME = 0,
		SORT_PING,
		SORT_MAP,
		SORT_GAMETYPE,
		SORT_NUMPLAYERS,

		QUICK_SERVERNAME=1,
		QUICK_PLAYER=2,
		QUICK_MAPNAME=4,

		TYPE_NONE = 0,
		TYPE_INTERNET = 1,
		TYPE_LAN = 2,
		TYPE_FAVORITES = 3,
		TYPE_RECENT = 4,
		TYPE_DDNET = 5,
		TYPE_OUT_OF_BOUNDS,

		SET_MASTER_ADD=1,
		SET_FAV_ADD,
		SET_DDNET_ADD,
		SET_TOKEN,
		SET_RECENT,
	};

	virtual void Refresh(int Type) = 0;
	virtual void RefreshQuick() = 0;
	virtual void AbortRefresh() = 0;
	virtual void SaveCache() = 0;
	virtual void LoadCache() = 0;
	virtual void LoadCacheWait() = 0;
	virtual bool IsRefreshing() const = 0;
	virtual bool IsRefreshingMasters() const = 0;
	virtual int LoadingProgression() const = 0;
	virtual bool CacheExists() const = 0;

	virtual int NumServers() const = 0;
	virtual bool IsLocked() = 0;

	virtual int NumSortedServers() const = 0;
	virtual const CServerInfo *SortedGet(int Index) = 0;
	virtual const CServerInfo *Get(int Index) = 0;
	virtual int GetInfoAge(int Index) const = 0;
	virtual const char *GetDebugString(int Index) const = 0;

	virtual bool IsFavorite(const NETADDR &Addr) const = 0;
	virtual void AddFavorite(const NETADDR &Addr) = 0;
	virtual void RemoveFavorite(const NETADDR &Addr) = 0;

	virtual void AddRecent(const NETADDR& Addr) = 0;
	virtual void RemoveRecent(const NETADDR& Addr) = 0;
	virtual void ClearRecent() = 0;

	virtual int NumDDNetCountries() = 0;
	virtual int GetDDNetCountryFlag(int Index) = 0;
	virtual const char *GetDDNetCountryName(int Index) = 0;

	virtual int NumDDNetTypes() = 0;
	virtual const char *GetDDNetType(int Index) = 0;

	virtual void DDNetFilterAdd(char *pFilter, const char *pName) = 0;
	virtual void DDNetFilterRem(char *pFilter, const char *pName) = 0;
	virtual bool DDNetFiltered(char *pFilter, const char *pName) = 0;
	virtual int GetCurrentType() = 0;
};

#endif
