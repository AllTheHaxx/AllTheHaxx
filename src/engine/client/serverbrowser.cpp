/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <cstdio> // file io  TODO: remove this, use tw's own storage system instead
#include <algorithm> // sort  TODO: remove this

#include <base/math.h>
#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/shared/memheap.h>
#include <engine/shared/network.h>
#include <engine/shared/protocol.h>

#include <engine/config.h>
#include <engine/console.h>
#include <engine/friends.h>
#include <engine/masterserver.h>
#include <engine/storage.h>

#include <mastersrv/mastersrv.h>

#include <engine/external/json-parser/json.hpp>

#include <game/client/components/menus.h>
#include <json-parser/json.hpp>
#include <base/system++/threading.h>

#include "serverbrowser.h"
class SortWrap
{
	typedef bool (CServerBrowser::*SortFunc)(int, int) const;
	SortFunc m_pfnSort;
	CServerBrowser *m_pThis;
public:
	SortWrap(CServerBrowser *t, SortFunc f) : m_pfnSort(f), m_pThis(t) {}
	bool operator()(int a, int b) { return (g_Config.m_BrSortOrder ? (m_pThis->*m_pfnSort)(b, a) : (m_pThis->*m_pfnSort)(a, b)); }
};

void CQueryRecent::OnData()
{
	while(Next()) // process everything
	{
		if(m_paRecentList) // we only have this when writing to the db
		{
			CServerBrowser::RecentServer e;
			mem_zero(&e, sizeof(CServerBrowser::RecentServer));

			e.m_ID = GetInt(GetID("id"));

			const char *pAddrStr = GetText(GetID("addr"));
			if(pAddrStr)
				net_addr_from_str(&e.m_Addr, pAddrStr);

			const char *pLastJoined = GetText(GetID("last_joined"));
			if(pLastJoined)
				str_copy(e.m_LastJoined, pLastJoined, sizeof(e.m_LastJoined));

			m_paRecentList->add(e);
		}
	}
}

CServerBrowser::CServerBrowser()
{
	m_pMasterServer = 0;
	m_ppServerlist = 0;
	m_pSortedServerlist = 0;
	m_pConsole = 0;
	m_pFriends = 0;
	m_pNetClient = 0;

	m_NumFavoriteServers = 0;

	mem_zero(m_aServerlistIp, sizeof(m_aServerlistIp));

	m_pFirstReqServer = 0; // request list
	m_pLastReqServer = 0;
	m_NumRequests = 0;
	m_MasterServerCount = 0;
	m_CurrentMaxRequests = g_Config.m_BrMaxRequests;

	m_NeedRefresh = 0;
	m_aRecentServers.clear();
	m_aRecentServers.hint_size(50);

	m_NumSortedServers = 0;
	m_NumSortedServersCapacity = 0;
	m_NumServers = 0;
	m_NumServerCapacity = 0;

	m_NumDDNetTypes = 0;
	m_NumDDNetCountries = 0;

	m_Sorthash = 0;
	m_aFilterString[0] = 0;
	m_aFilterGametypeString[0] = 0;

	// the token is to keep server refresh separated from each other
	m_CurrentToken = 1;
	m_LastPacketTick = 0;

	m_ServerlistType = 0;
	m_BroadcastTime = 0;
	m_BroadcastExtraToken = -1;

	m_pRecentDB = new CSql("ath_recent.db");

	// make sure the "recent"-table exists
	{
		char *pQueryBuf = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS recent (" \
			"id INTEGER PRIMARY KEY, " \
			"addr TEXT NOT NULL UNIQUE, " \
			"last_joined TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
		CQueryRecent *pQuery = new CQueryRecent(pQueryBuf);
		m_pRecentDB->InsertQuery(pQuery);
	}

	// read the entries from it
	{
		char *pQueryBuf = sqlite3_mprintf("SELECT * FROM 'recent' ORDER BY 'last_joined' DESC;");
		CQueryRecent *pQuery = new CQueryRecent(pQueryBuf, &m_aRecentServers);
		m_pRecentDB->InsertQuery(pQuery);
	}
}

CServerBrowser::~CServerBrowser()
{
	if(m_ppServerlist)
		mem_free(m_ppServerlist);
	if(m_pSortedServerlist)
		mem_free(m_pSortedServerlist);
	if(m_pRecentDB)
		delete m_pRecentDB;
}

void CServerBrowser::SetBaseInfo(class CNetClient *pClient, const char *pNetVersion)
{
	m_pNetClient = pClient;
	str_copy(m_aNetVersion, pNetVersion, sizeof(m_aNetVersion));
	m_pMasterServer = Kernel()->RequestInterface<IMasterServer>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pFriends = Kernel()->RequestInterface<IFriends>();
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	if(pConfig)
		pConfig->RegisterCallback(ConfigSaveCallback, this);

	IStorageTW *pStorage = Kernel()->RequestInterface<IStorageTW>();
	IOHANDLE File = pStorage->OpenFile("tmp/cache/serverlist", IOFLAG_READ, IStorageTW::TYPE_SAVE);
	m_CacheExists = true;
	if(!File)
	{
		if(g_Config.m_Debug)
			dbg_msg("browser", "no serverlist cache detected");
		m_CacheExists = false;
	}
}

const CServerInfo *CServerBrowser::SortedGet(int Index)// const
{
	LOCK_SECTION_LAZY_DBG(m_Lock);
	if(!__SectionLock.TryToLock())
		return 0;

	if(Index < 0 || Index >= m_NumSortedServers)
		return 0;
	return &m_ppServerlist[m_pSortedServerlist[Index]]->m_Info;
}

const CServerInfo *CServerBrowser::Get(int Index)// const
{
	LOCK_SECTION_LAZY_DBG(m_Lock);
	if(!__SectionLock.TryToLock())
		return 0;

	if(Index < 0 || Index >= m_NumSortedServers)
		return 0;
	return &m_ppServerlist[Index]->m_Info;
}

int CServerBrowser::GetInfoAge(int Index) const
{
	if(Index < 0 || Index >= m_NumSortedServers)
		return 0;

	if(m_ppServerlist[Index]->m_RequestTime < 0)
		return (int)m_ppServerlist[Index]->m_RequestTime;

	return (int)((time_get() - m_ppServerlist[Index]->m_RequestTime) / time_freq());
}

const char *CServerBrowser::GetDebugString(int Index) const
{
	static char s_aBuffer[512] = {0};
	if(Index < 0 || Index >= m_NumSortedServers)
	{
		str_formatb(s_aBuffer, "<invalid index: %i>", Index);
		return s_aBuffer;
	}

	const CServerEntry *pInfo = m_ppServerlist[Index];

	char aAddr[NETADDR_MAXSTRSIZE];
	net_addr_str(&(m_ppServerlist[Index]->m_Addr), aAddr, sizeof(aAddr), true);

	char aInfoAge[64];
	str_clock_secb(aInfoAge, GetInfoAge(Index));

	str_format(s_aBuffer, sizeof s_aBuffer,
			   "%i/%i/%p\n"
			   "Addr: %s\n"
			   "Waiting for Info: %s\n"
			   "GotInfo: %s\n"
			   "64-legacy: %s\n"
			   "\n"
			   "NextIp = %p\n"
			   "PrevReq = %p\n"
			   "NextReq = %p\n"
			   "\n"
			   "CurrentMaxRequests = %i"
			,
			   Index, m_NumSortedServers, pInfo,
			   aAddr,
			   aInfoAge,
			   pInfo->m_GotInfo ? "true" : "false",
			   pInfo->m_Request64Legacy ? "true" : "false",
			   pInfo->m_pNextIp,
			   pInfo->m_pPrevReq,
			   pInfo->m_pNextReq,
			   m_CurrentMaxRequests
	);

	return s_aBuffer;
}

bool CServerBrowser::SortCompareName(int Index1, int Index2) const
{
	CServerEntry *a = m_ppServerlist[Index1];
	CServerEntry *b = m_ppServerlist[Index2];
	//	make sure empty entries are listed last
	return (a->m_GotInfo && b->m_GotInfo) || (!a->m_GotInfo && !b->m_GotInfo) ? str_comp(a->m_Info.m_aName, b->m_Info.m_aName) < 0 : a->m_GotInfo != 0;
}

bool CServerBrowser::SortCompareMap(int Index1, int Index2) const
{
	CServerEntry *a = m_ppServerlist[Index1];
	CServerEntry *b = m_ppServerlist[Index2];
	return str_comp(a->m_Info.m_aMap, b->m_Info.m_aMap) < 0;
}

bool CServerBrowser::SortComparePing(int Index1, int Index2) const
{
	CServerEntry *a = m_ppServerlist[Index1];
	CServerEntry *b = m_ppServerlist[Index2];
	return a->m_Info.m_Latency < b->m_Info.m_Latency;
}

bool CServerBrowser::SortCompareGametype(int Index1, int Index2) const
{
	CServerEntry *a = m_ppServerlist[Index1];
	CServerEntry *b = m_ppServerlist[Index2];
	return str_comp(a->m_Info.m_aGameType, b->m_Info.m_aGameType) < 0;
}

bool CServerBrowser::SortCompareNumPlayers(int Index1, int Index2) const
{
	CServerEntry *a = m_ppServerlist[Index1];
	CServerEntry *b = m_ppServerlist[Index2];
	return a->m_Info.m_NumPlayers < b->m_Info.m_NumPlayers;
}

bool CServerBrowser::SortCompareNumClients(int Index1, int Index2) const
{
	CServerEntry *a = m_ppServerlist[Index1];
	CServerEntry *b = m_ppServerlist[Index2];
	return a->m_Info.m_NumClients < b->m_Info.m_NumClients;
}

void CServerBrowser::Filter()
{
	LOCK_SECTION_LAZY_DBG(m_Lock);
	if(!__SectionLock.TryToLock())
		return;

	int i = 0, p = 0;
	m_NumSortedServers = 0;

	// allocate the sorted list
	if(m_NumSortedServersCapacity < m_NumServers)
	{
		if(m_pSortedServerlist)
			mem_free(m_pSortedServerlist);
		m_NumSortedServersCapacity = m_NumServers;
		m_pSortedServerlist = (int *)mem_alloc(m_NumSortedServersCapacity*sizeof(int), 1);
	}

	// filter the servers
	for(i = 0; i < m_NumServers; i++)
	{
		int Filtered = 0;

		if(g_Config.m_BrFilterEmpty && ((g_Config.m_BrFilterSpectators && m_ppServerlist[i]->m_Info.m_NumPlayers == 0) || m_ppServerlist[i]->m_Info.m_NumClients == 0))
			Filtered = 1;
		else if(g_Config.m_BrFilterNonEmpty && ((g_Config.m_BrFilterSpectators && m_ppServerlist[i]->m_Info.m_NumPlayers != 0) || m_ppServerlist[i]->m_Info.m_NumClients != 0))
			Filtered = 1;
		else if(g_Config.m_BrFilterFull && ((g_Config.m_BrFilterSpectators && m_ppServerlist[i]->m_Info.m_NumPlayers == m_ppServerlist[i]->m_Info.m_MaxPlayers) ||
				m_ppServerlist[i]->m_Info.m_NumClients == m_ppServerlist[i]->m_Info.m_MaxClients))
			Filtered = 1;
		else if(g_Config.m_BrFilterPw && (m_ppServerlist[i]->m_Info.m_Flags&SERVER_FLAG_PASSWORD))
			Filtered = 1;
		else if(g_Config.m_BrFilterPure &&
			(str_comp(m_ppServerlist[i]->m_Info.m_aGameType, "DM") != 0 &&
			str_comp(m_ppServerlist[i]->m_Info.m_aGameType, "TDM") != 0 &&
			str_comp(m_ppServerlist[i]->m_Info.m_aGameType, "CTF") != 0))
		{
			Filtered = 1;
		}
		else if(g_Config.m_BrFilterPureMap &&
			!(str_comp(m_ppServerlist[i]->m_Info.m_aMap, "dm1") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "dm2") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "dm6") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "dm7") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "dm8") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "dm9") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "ctf1") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "ctf2") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "ctf3") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "ctf4") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "ctf5") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "ctf6") == 0 ||
			str_comp(m_ppServerlist[i]->m_Info.m_aMap, "ctf7") == 0)
		)
		{
			Filtered = 1;
		}
		else if(g_Config.m_BrFilterPing < m_ppServerlist[i]->m_Info.m_Latency)
			Filtered = 1;
		else if(g_Config.m_BrFilterCompatversion && str_comp_num(m_ppServerlist[i]->m_Info.m_aVersion, m_aNetVersion, 3) != 0)
			Filtered = 1;
		else if(g_Config.m_BrFilterServerAddress[0] && !str_find_nocase(m_ppServerlist[i]->m_Info.m_aAddress, g_Config.m_BrFilterServerAddress))
			Filtered = 1;
		else if(g_Config.m_BrFilterGametypeStrict && g_Config.m_BrFilterGametype[0] && str_comp_nocase(m_ppServerlist[i]->m_Info.m_aGameType, g_Config.m_BrFilterGametype))
			Filtered = 1;
		else if(!g_Config.m_BrFilterGametypeStrict && g_Config.m_BrFilterGametype[0] && !str_find_nocase(m_ppServerlist[i]->m_Info.m_aGameType, g_Config.m_BrFilterGametype))
			Filtered = 1;
		else if(g_Config.m_BrFilterVersionStrict && g_Config.m_BrFilterVersion[0] && str_comp_nocase(m_ppServerlist[i]->m_Info.m_aVersion, g_Config.m_BrFilterVersion))
			Filtered = 1;
		else if(!g_Config.m_BrFilterVersionStrict && g_Config.m_BrFilterVersion[0] && !str_find_nocase(m_ppServerlist[i]->m_Info.m_aVersion, g_Config.m_BrFilterVersion))
			Filtered = 1;
		else if(g_Config.m_BrShowDDNet == 1 && g_Config.m_UiBrowserPage != CMenus::PAGE_BROWSER_DDNET && g_Config.m_UiBrowserPage != CMenus::PAGE_BROWSER_FAVORITES && str_find_nocase(m_ppServerlist[i]->m_Info.m_aName, "[DDRaceNetwork]"))
			Filtered = 1;
		else
		{
			if(g_Config.m_BrFilterCountry)
			{
				Filtered = 1;
				// match against player country
				for(p = 0; p < m_ppServerlist[i]->m_Info.m_NumClients; p++)
				{
					if(m_ppServerlist[i]->m_Info.m_aClients[p].m_Country == g_Config.m_BrFilterCountryIndex)
					{
						Filtered = 0;
						break;
					}
				}
			}

			if(!Filtered && g_Config.m_BrFilterString[0] != 0)
			{
				int MatchFound = 0;

				m_ppServerlist[i]->m_Info.m_QuickSearchHit = 0;

				// match against server name
				if(str_find_nocase(m_ppServerlist[i]->m_Info.m_aName, g_Config.m_BrFilterString))
				{
					MatchFound = 1;
					m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_SERVERNAME;
				}

				// match against players
				for(p = 0; p < m_ppServerlist[i]->m_Info.m_NumClients; p++)
				{
					if(str_find_nocase(m_ppServerlist[i]->m_Info.m_aClients[p].m_aName, g_Config.m_BrFilterString) ||
						str_find_nocase(m_ppServerlist[i]->m_Info.m_aClients[p].m_aClan, g_Config.m_BrFilterString))
					{
						MatchFound = 1;
						m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_PLAYER;
						break;
					}
				}

				// match against map
				if(str_find_nocase(m_ppServerlist[i]->m_Info.m_aMap, g_Config.m_BrFilterString))
				{
					MatchFound = 1;
					m_ppServerlist[i]->m_Info.m_QuickSearchHit |= IServerBrowser::QUICK_MAPNAME;
				}

				if(!MatchFound)
					Filtered = 1;
			}

			if(!Filtered && g_Config.m_BrExcludeString[0] != 0)
			{
				int MatchFound = 0;

				// match against server name
				if(str_find_nocase(m_ppServerlist[i]->m_Info.m_aName, g_Config.m_BrExcludeString))
				{
					MatchFound = 1;
				}

				// match against map
				if(str_find_nocase(m_ppServerlist[i]->m_Info.m_aMap, g_Config.m_BrExcludeString))
				{
					MatchFound = 1;
				}

				if(MatchFound)
					Filtered = 1;
			}
		}

		if(Filtered == 0)
		{
			// check for friend
			m_ppServerlist[i]->m_Info.m_FriendState = IFriends::FRIEND_NO;
			for(p = 0; p < m_ppServerlist[i]->m_Info.m_NumClients; p++)
			{
				m_ppServerlist[i]->m_Info.m_aClients[p].m_FriendState = m_pFriends->GetFriendState(
						m_ppServerlist[i]->m_Info.m_aClients[p].m_aName,
						m_ppServerlist[i]->m_Info.m_aClients[p].m_aClan
				);
				m_ppServerlist[i]->m_Info.m_FriendState = max(m_ppServerlist[i]->m_Info.m_FriendState, m_ppServerlist[i]->m_Info.m_aClients[p].m_FriendState);
			}

			if(!g_Config.m_BrFilterFriends || m_ppServerlist[i]->m_Info.m_FriendState != IFriends::FRIEND_NO)
				m_pSortedServerlist[m_NumSortedServers++] = i;
		}
	}
}

int64 CServerBrowser::SortHash() const
{
	int n=4;
	int64 i = g_Config.m_BrSort&0xf;
	i |= g_Config.m_BrFilterEmpty			<< n++;
	i |= g_Config.m_BrFilterNonEmpty		<< n++;
	i |= g_Config.m_BrFilterFull			<< n++;
	i |= g_Config.m_BrFilterSpectators		<< n++;
	i |= g_Config.m_BrFilterFriends			<< n++;
	i |= g_Config.m_BrFilterPw				<< n++;
	i |= g_Config.m_BrSortOrder				<< n++;
	i |= g_Config.m_BrFilterCompatversion	<< n++;
	i |= g_Config.m_BrFilterPure			<< n++;
	i |= g_Config.m_BrFilterPureMap			<< n++;
	i |= g_Config.m_BrFilterGametypeStrict	<< n++;
	i |= g_Config.m_BrFilterVersionStrict	<< n++;
	i |= g_Config.m_BrFilterCountry			<< n++;
	i |= g_Config.m_BrFilterPing			<< n++;
	i |= m_NumServers						<< n++;
	return i;
}

void CServerBrowser::Sort()
{
	int i;

	// create filtered list
	Filter();

	// sort
	if(g_Config.m_BrSort == IServerBrowser::SORT_NAME)
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::SortCompareName));
	else if(g_Config.m_BrSort == IServerBrowser::SORT_PING)
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::SortComparePing));
	else if(g_Config.m_BrSort == IServerBrowser::SORT_MAP)
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::SortCompareMap));
	else if(g_Config.m_BrSort == IServerBrowser::SORT_NUMPLAYERS)
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this,
					g_Config.m_BrFilterSpectators ? &CServerBrowser::SortCompareNumPlayers : &CServerBrowser::SortCompareNumClients));
	else if(g_Config.m_BrSort == IServerBrowser::SORT_GAMETYPE)
		std::stable_sort(m_pSortedServerlist, m_pSortedServerlist+m_NumSortedServers, SortWrap(this, &CServerBrowser::SortCompareGametype));

	// set indexes
	for(i = 0; i < m_NumSortedServers; i++)
		m_ppServerlist[m_pSortedServerlist[i]]->m_Info.m_SortedIndex = i;

	str_copy(m_aFilterGametypeString, g_Config.m_BrFilterGametype, sizeof(m_aFilterGametypeString));
	str_copy(m_aFilterString, g_Config.m_BrFilterString, sizeof(m_aFilterString));
	m_Sorthash = SortHash();
}

void CServerBrowser::RemoveRequest(CServerEntry *pEntry)
{
	if(pEntry->m_pPrevReq || pEntry->m_pNextReq || m_pFirstReqServer == pEntry)
	{
		if(pEntry->m_pPrevReq)
			pEntry->m_pPrevReq->m_pNextReq = pEntry->m_pNextReq;
		else
			m_pFirstReqServer = pEntry->m_pNextReq;

		if(pEntry->m_pNextReq)
			pEntry->m_pNextReq->m_pPrevReq = pEntry->m_pPrevReq;
		else
			m_pLastReqServer = pEntry->m_pPrevReq;

		pEntry->m_pPrevReq = 0;
		pEntry->m_pNextReq = 0;
		m_NumRequests--;
	}
}

CServerBrowser::CServerEntry *CServerBrowser::Find(const NETADDR &Addr)
{
	LOCK_SECTION_LAZY_DBG(m_Lock);
	if(!__SectionLock.TryToLock())
		return (CServerEntry*)0;

	CServerEntry *pEntry = m_aServerlistIp[Addr.ip[0]];

	for(; pEntry; pEntry = pEntry->m_pNextIp)
	{
		if(net_addr_comp(&pEntry->m_Addr, &Addr) == 0)
			return pEntry;
	}
	return (CServerEntry*)0;
}

void CServerBrowser::QueueRequest(CServerEntry *pEntry)
{
	// add it to the list of servers that we should request info from
	pEntry->m_pPrevReq = m_pLastReqServer;
	if(m_pLastReqServer)
		m_pLastReqServer->m_pNextReq = pEntry;
	else
		m_pFirstReqServer = pEntry;
	m_pLastReqServer = pEntry;
	pEntry->m_pNextReq = 0;
	m_NumRequests++;
}

void CServerBrowser::SetInfo(CServerEntry *pEntry, const CServerInfo &Info)
{
	int Fav = pEntry->m_Info.m_Favorite;
	pEntry->m_Info = Info;
	pEntry->m_Info.m_Favorite = Fav;
	pEntry->m_Info.m_NetAddr = pEntry->m_Addr;

	// all these are just for nice compability
	if(pEntry->m_Info.m_aGameType[0] == '0' && pEntry->m_Info.m_aGameType[1] == 0)
		str_copy(pEntry->m_Info.m_aGameType, "DM", sizeof(pEntry->m_Info.m_aGameType));
	else if(pEntry->m_Info.m_aGameType[0] == '1' && pEntry->m_Info.m_aGameType[1] == 0)
		str_copy(pEntry->m_Info.m_aGameType, "TDM", sizeof(pEntry->m_Info.m_aGameType));
	else if(pEntry->m_Info.m_aGameType[0] == '2' && pEntry->m_Info.m_aGameType[1] == 0)
		str_copy(pEntry->m_Info.m_aGameType, "CTF", sizeof(pEntry->m_Info.m_aGameType));

	/*if(!request)
	{
		pEntry->m_Info.latency = (time_get()-pEntry->request_time)*1000/time_freq();
		RemoveRequest(pEntry);
	}*/

	pEntry->m_GotInfo = 1;
}

CServerBrowser::CServerEntry *CServerBrowser::Add(const NETADDR &Addr)
{
	int Hash = Addr.ip[0];
	CServerEntry *pEntry = 0;
	int i;

	// create new pEntry
	pEntry = (CServerEntry *)m_ServerlistHeap.Allocate(sizeof(CServerEntry));
	mem_zero(pEntry, sizeof(CServerEntry));

	// set the info
	pEntry->m_Addr = Addr;
	pEntry->m_ExtraToken = secure_rand() & 0xffff;
	pEntry->m_Info.m_NetAddr = Addr;

	pEntry->m_Info.m_Latency = 999;
	net_addr_str(&Addr, pEntry->m_Info.m_aAddress, sizeof(pEntry->m_Info.m_aAddress), true);
	str_copy(pEntry->m_Info.m_aName, pEntry->m_Info.m_aAddress, sizeof(pEntry->m_Info.m_aName));

	// check if it's a favorite
	for(i = 0; i < m_NumFavoriteServers; i++)
	{
		if(net_addr_comp(&Addr, &m_aFavoriteServers[i]) == 0)
			pEntry->m_Info.m_Favorite = 1;
	}

	// add to the hash list
	pEntry->m_pNextIp = m_aServerlistIp[Hash];
	m_aServerlistIp[Hash] = pEntry;

	if(m_NumServers == m_NumServerCapacity)
	{
		CServerEntry **ppNewlist;
		m_NumServerCapacity += 100;
		ppNewlist = (CServerEntry **)mem_alloc(m_NumServerCapacity*sizeof(CServerEntry*), 1);
		mem_copy(ppNewlist, m_ppServerlist, m_NumServers*sizeof(CServerEntry*));
		mem_free(m_ppServerlist);
		m_ppServerlist = ppNewlist;
	}

	// add to list
	m_ppServerlist[m_NumServers] = pEntry;
	pEntry->m_Info.m_ServerIndex = m_NumServers;
	m_NumServers++;

	return pEntry;
}

void CServerBrowser::Set(const NETADDR &Addr, int Type, int Token, const CServerInfo *pInfo, bool NoSort)
{
	CServerEntry *pEntry = 0;
	if(Type == IServerBrowser::SET_MASTER_ADD)
	{
		if(m_ServerlistType != IServerBrowser::TYPE_INTERNET)
			return;
		m_LastPacketTick = 0;
		if(!Find(Addr))
		{
			pEntry = Add(Addr);
			QueueRequest(pEntry);
		}
	}
	else if(Type == IServerBrowser::SET_FAV_ADD)
	{
		if(m_ServerlistType != IServerBrowser::TYPE_FAVORITES)
			return;

		if(!Find(Addr))
		{
			pEntry = Add(Addr);
			QueueRequest(pEntry);
		}
	}
	else if(Type == IServerBrowser::SET_RECENT)
	{
		if(m_ServerlistType != IServerBrowser::TYPE_RECENT)
			return;

		if(!Find(Addr))
		{
			pEntry = Add(Addr);
			QueueRequest(pEntry);
		}
	}
	else if(Type == IServerBrowser::SET_DDNET_ADD)
	{
		if(m_ServerlistType != IServerBrowser::TYPE_DDNET)
			return;

		if(!Find(Addr))
		{
			pEntry = Add(Addr);
			QueueRequest(pEntry);
		}
	}
	else if(Type == IServerBrowser::SET_TOKEN)
	{
		int CheckToken = Token;
		if(pInfo->m_Type == SERVERINFO_EXTENDED)
		{
			CheckToken = Token & 0xff;
		}

		if(CheckToken != m_CurrentToken)
			return;

		pEntry = Find(Addr);
		if(pEntry && pInfo->m_Type == SERVERINFO_EXTENDED)
		{
			if(((Token & 0xffff00) >> 8) != pEntry->m_ExtraToken)
			{
				return;
			}
		}
		if(!pEntry)
			pEntry = Add(Addr);
		if(pEntry)
		{
			if(m_ServerlistType == IServerBrowser::TYPE_LAN && pInfo->m_Type == SERVERINFO_EXTENDED)
			{
				if(((Token & 0xffff00) >> 8) != m_BroadcastExtraToken)
				{
					return;
				}
			}
			SetInfo(pEntry, *pInfo);
			if (m_ServerlistType == IServerBrowser::TYPE_LAN)
				pEntry->m_Info.m_Latency = min(static_cast<int>((time_get()-m_BroadcastTime)*1000/time_freq()), 999);
			else if (pEntry->m_RequestTime > 0)
			{
				pEntry->m_Info.m_Latency = min(static_cast<int>((time_get()-pEntry->m_RequestTime)*1000/time_freq()), 999);
				pEntry->m_RequestTime = -1; // Request has been answered
			}
			RemoveRequest(pEntry);
		}
	}

	if(!NoSort)
		Sort();
}

void CServerBrowser::AbortRefresh()
{
	CServerEntry *pEntry = m_pFirstReqServer;
	while(pEntry)
	{
		CServerEntry *pNext = pEntry->m_pNextReq;
		RemoveRequest(pEntry); // release request
		pEntry = pNext;
	}

	m_pFirstReqServer = 0;
	m_pLastReqServer = 0;
	m_NumRequests = 0;
	m_CurrentMaxRequests = g_Config.m_BrMaxRequests;

	m_NeedRefresh = false;
}

void CServerBrowser::Refresh(int Type)
{
	if(dbg_assert_strict(Type > TYPE_NONE && Type < TYPE_OUT_OF_BOUNDS, "Invalid Type"))
		return;

	{ // BEGIN LOCKED SECTION
		LOCK_SECTION_LAZY_DBG(m_Lock);

		if(!__SectionLock.TryToLock())
			return;

		// clear out everything
		m_ServerlistHeap.Reset();
		m_NumServers = 0;
		m_NumSortedServers = 0;
		mem_zero(m_aServerlistIp, sizeof(m_aServerlistIp));
		m_pFirstReqServer = 0;
		m_pLastReqServer = 0;
		m_NumRequests = 0;
		m_CurrentMaxRequests = g_Config.m_BrMaxRequests;
		// next token
		m_CurrentToken = secure_rand()%0xFF;

		//
		m_ServerlistType = Type;

	} // END LOCKED SECTION

	if(Type == IServerBrowser::TYPE_LAN)
	{
		unsigned char Buffer[sizeof(SERVERBROWSE_GETINFO)+1];
		CNetChunk Packet;

		mem_copy(Buffer, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
		Buffer[sizeof(SERVERBROWSE_GETINFO)] = (unsigned char)m_CurrentToken;

		/* do the broadcast version */
		Packet.m_ClientID = -1;
		mem_zero(&Packet, sizeof(Packet));
		Packet.m_Address.type = (unsigned int)(m_pNetClient->NetType() | NETTYPE_LINK_BROADCAST);
		Packet.m_Flags = NETSENDFLAG_CONNLESS|NETSENDFLAG_EXTENDED;
		Packet.m_DataSize = sizeof(Buffer);
		Packet.m_pData = Buffer;
		mem_zero(&Packet.m_aExtraData, sizeof(Packet.m_aExtraData));
		m_BroadcastExtraToken = rand() & 0xffff;
		Packet.m_aExtraData[0] = (unsigned char)(m_BroadcastExtraToken >> 8);
		Packet.m_aExtraData[1] = (unsigned char)(m_BroadcastExtraToken & 0xff);
		m_BroadcastTime = time_get();

		for(unsigned short i = g_Config.m_BrLanScanStart; i <= min(65535, g_Config.m_BrLanScanStart+g_Config.m_BrLanScanRange); i++)
		{
			Packet.m_Address.port = i;
			m_pNetClient->Send(&Packet);
		}

		if(g_Config.m_Debug)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", "broadcasting for servers");
	}
	else if(Type == IServerBrowser::TYPE_INTERNET)
	{
		m_NeedRefresh = 1;
	}
	else if(Type == IServerBrowser::TYPE_FAVORITES)
	{
		for(int i = 0; i < m_NumFavoriteServers; i++)
			Set(m_aFavoriteServers[i], IServerBrowser::SET_FAV_ADD, -1, 0, true);
		Sort();
	}
	else if(Type == IServerBrowser::TYPE_RECENT)
	{
		for(int i = 0; i < m_aRecentServers.size(); i++)
			Set(m_aRecentServers[i].m_Addr, IServerBrowser::SET_RECENT, -1, 0, true);
		Sort();
	}
	else if(Type == IServerBrowser::TYPE_DDNET)
	{
		LoadDDNet();

		// remove unknown elements of exclude list
		DDNetCountryFilterClean();
		DDNetTypeFilterClean();

		for(int i = 0; i < m_NumDDNetCountries; i++)
		{
			CDDNetCountry *pCntr = &m_aDDNetCountries[i];

			// check for filter
			if(DDNetFiltered(g_Config.m_BrFilterExcludeCountries, pCntr->m_aName))
				continue;

			for(int g = 0; g < pCntr->m_NumServers; g++)
				if(!DDNetFiltered(g_Config.m_BrFilterExcludeTypes, pCntr->m_aTypes[g]))
					Set(pCntr->m_aServers[g], IServerBrowser::SET_DDNET_ADD, -1, 0, true);
			Sort();
		}
	}
}

void CServerBrowser::RefreshQuick()
{
	LOCK_SECTION_LAZY_DBG(m_Lock);
	if(!__SectionLock.TryToLock())
		return;

	if(IsRefreshing())
		AbortRefresh();

	m_CurrentMaxRequests = g_Config.m_BrMaxRequests;

	const int Length = NumServers();

	// queue request for all existing servers by sorted order
	{
		//dbg_msg("browser", "upgrading part %i/%i", CurrPart+1, NumParts);
		for(int i = 0; i < Length; i++)
		{
			CServerEntry *pEntry = m_ppServerlist[m_pSortedServerlist && m_NumSortedServers == Length ? m_pSortedServerlist[i] : i];
			if(!pEntry)
				continue;
			pEntry->m_RequestTime = /*Now*/0;
			pEntry->m_GotInfo = 0;
			QueueRequest(m_ppServerlist[i]);
		}
	}
}

void CServerBrowser::SaveCache()
{
	LOCK_SECTION_LAZY_DBG(m_Lock);
	if(!__SectionLock.TryToLock())
		return;

	// nothing to save
	if(m_NumServers < 1)
		return;

	IStorageTW *pStorage = Kernel()->RequestInterface<IStorageTW>();

	// open file
	IOHANDLE File = pStorage->OpenFile("tmp/cache/serverlist", IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
	if(!File)
	{
		dbg_msg("browser", "failed to open cache file for writing");
		m_CacheExists = false;
		return;
	}

	// save version of serverlist cache file
	{ char v = CACHE_VERSION; io_write(File, &v, 1); } // save version of cachefile

	// save number of servers
	io_write(File, &m_NumServers, sizeof(m_NumServers)); // save number of servers

	// save array length
	io_write(File, &m_NumServerCapacity, sizeof(m_NumServerCapacity)); // save length of array

	// save all the infos
	CServerInfo *aAllInfos = mem_allocb(CServerInfo, m_NumServers);
	for(int i = 0; i < m_NumServers; i++)
		aAllInfos[i] = m_ppServerlist[i]->m_Info;

	const unsigned int SIZE = sizeof(CServerInfo)*m_NumServers;
	unsigned char *pCompressed = (unsigned char *)mem_alloc(SIZE, 0);
	int CompressedSize = CNetBase::Compress(aAllInfos, SIZE, pCompressed, SIZE);
	io_write(File, &CompressedSize, sizeof(CompressedSize));

	dbg_msg("browser/cache", "compressed %lu -> %i", (unsigned long)(sizeof(CServerInfo)*m_NumServers), CompressedSize);

	if(CompressedSize == -1)
		io_write(File, aAllInfos, SIZE);
	else
		io_write(File, pCompressed, (unsigned int)CompressedSize);

	io_close(File);
	mem_free(pCompressed);
	mem_free(aAllInfos);

	if(g_Config.m_Debug)
		dbg_msg("browser", "successfully saved serverlist with %i entries", m_NumServers);
	m_CacheExists = true;
}

void CServerBrowser::LoadCache()
{
	m_ServerlistType = TYPE_INTERNET;
	void *pThread = thread_init(LoadCacheThread, this);
	thread_detach(pThread);
}

void CServerBrowser::LoadCacheWait()
{
	LoadCache();
	LOCK_SECTION_SMART LockHandler(&m_Lock);
	LockHandler.WaitAndLock();
}

void CServerBrowser::LoadCacheThread(void *pUser)
{
	CServerBrowser *pSelf = (CServerBrowser *)pUser;
	IStorageTW *pStorage = pSelf->Kernel()->RequestInterface<IStorageTW>();

	int64 StartTime = time_get();

	LOCK_SECTION_DBG(pSelf->m_Lock);

	// clear out everything
	pSelf->m_ServerlistHeap.Reset();
	pSelf->m_NumServers = 0;
	pSelf->m_NumSortedServers = 0;
	mem_zero(pSelf->m_aServerlistIp, sizeof(pSelf->m_aServerlistIp));
	pSelf->m_pFirstReqServer = 0;
	pSelf->m_pLastReqServer = 0;
	pSelf->m_NumRequests = 0;
	pSelf->m_CurrentMaxRequests = g_Config.m_BrMaxRequests;
	pSelf->m_CurrentToken = 0;
	pSelf->m_ServerlistType = IServerBrowser::TYPE_INTERNET;

	// open file
	IOHANDLE File = pStorage->OpenFile("tmp/cache/serverlist", IOFLAG_READ, IStorageTW::TYPE_SAVE);
	if(!File)
	{
		dbg_msg("browser", "opening cache file failed.");
		pSelf->m_CacheExists = false;
		return;// false;
	}

	// get version
	{
		char v; io_read(File, &v, 1);
		if(g_Config.m_Debug)
			dbg_msg("browser", "loading serverlist from cache...");
		if(v != CACHE_VERSION)
		{
			dbg_msg("browser", "couldn't load cache: file version doesn't match! (%i != %i)", v, CACHE_VERSION);
			pSelf->m_CacheExists = false;
			return;
		}
	}

	// get number of servers
	int NumServers;
	io_read(File, &NumServers, sizeof(NumServers));
	//dbg_msg("browser", "serverlist cache entries: %i", NumServers);

	// get length of array
	io_read(File, &pSelf->m_NumServerCapacity, sizeof(pSelf->m_NumServerCapacity));

	// get rid of current serverlist and create a new one
	mem_free(pSelf->m_ppServerlist);
	pSelf->m_ppServerlist = (CServerEntry **)mem_alloc(pSelf->m_NumServerCapacity*sizeof(CServerEntry*), 1);

	// read the compressed size data
	int CompressedSize;
	io_read(File, &CompressedSize, sizeof(CompressedSize));

	// read the data from the file into the serverlist
	if(CompressedSize == -1)
	{
		// if the data is not compressed, read the entries as they are
		for(int i = 0; i < NumServers; i++)
		{
			CServerInfo Info;
			io_read(File, &Info, sizeof(CServerInfo));

			NETADDR Addr;
			net_addr_from_str(&Addr, Info.m_aAddress);
			//dbg_msg("browser", "loading %i %s %s", i, Info.m_aAddress, Info.m_aName);
			pSelf->Set(Addr, IServerBrowser::SET_TOKEN, pSelf->m_CurrentToken, &Info, true);
		}
	}
	else
	{
		// allocate some memory
		const unsigned int SIZE = sizeof(CServerInfo)*NumServers;
		CServerInfo *aAllInfos = mem_allocb(CServerInfo, NumServers);
		unsigned char *pCompressed = (unsigned char *)mem_alloc((unsigned int)CompressedSize, 0);

		// read the data from the file and try to decompress it
		io_read(File, pCompressed, (unsigned int)CompressedSize);
		int DecompressedSize = CNetBase::Decompress(pCompressed, CompressedSize, aAllInfos, SIZE);
		mem_free(pCompressed);

		// process the data
		if(DecompressedSize == -1)
		{
			// if decompression failed, return with no serverlist
			dbg_msg("browser", "failed to decompress the serverlist cache");
			io_close(File);
			mem_free(aAllInfos);
			pSelf->m_CacheExists = false;
			return;
		}
		else
		{
			// process all the entries
			dbg_msg("browser", "decompressed serverlist cache %i -> %i", CompressedSize, DecompressedSize);
			for(int i = 0; i < NumServers; i++)
			{
				CServerInfo Info;
				mem_copy(&Info, &aAllInfos[i], sizeof(CServerInfo));
				NETADDR Addr;
				net_addr_from_str(&Addr, Info.m_aAddress);
				//dbg_msg("browser", "loading %i %s %s", i, Info.m_aAddress, Info.m_aName);
				pSelf->Set(Addr, IServerBrowser::SET_TOKEN, pSelf->m_CurrentToken, &Info, true);
			}
		}
		mem_free(aAllInfos);
	}

	io_close(File);

	__SectionLock.Unlock(); // Sort() wants the lock too, so release it manually here

	if(g_Config.m_Debug)
		dbg_msg("browser", "successfully loaded serverlist cache with %i entries (total %i), took %.2fms", pSelf->m_NumServers, NumServers, ((time_get()-StartTime)*1000)/(float)time_freq()); // TODO: check if saving actually succeeded
	pSelf->Sort();
	return;// true;
}

void CServerBrowser::RequestImpl(const NETADDR &Addr, CServerEntry *pEntry) const
{
	unsigned char Buffer[sizeof(SERVERBROWSE_GETINFO)+1];
	CNetChunk Packet;

	if(g_Config.m_Debug)
	{
		char aAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr), true);
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf),"requesting server info from %s", aAddrStr);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", aBuf);
	}

	mem_copy(Buffer, SERVERBROWSE_GETINFO, sizeof(SERVERBROWSE_GETINFO));
	Buffer[sizeof(SERVERBROWSE_GETINFO)] = m_CurrentToken;

	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS|NETSENDFLAG_EXTENDED;
	Packet.m_DataSize = sizeof(Buffer);
	Packet.m_pData = Buffer;
	mem_zero(&Packet.m_aExtraData, sizeof(Packet.m_aExtraData));
	if(pEntry)
	{
		Packet.m_aExtraData[0] = pEntry->m_ExtraToken >> 8;
		Packet.m_aExtraData[1] = pEntry->m_ExtraToken & 0xff;
	}

	m_pNetClient->Send(&Packet);

	if(pEntry)
		pEntry->m_RequestTime = time_get();
}

void CServerBrowser::RequestImpl64(const NETADDR &Addr, CServerEntry *pEntry) const
{
	unsigned char Buffer[sizeof(SERVERBROWSE_GETINFO_64_LEGACY)+1];
	CNetChunk Packet;

	if(g_Config.m_Debug)
	{
		char aAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr), true);
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf),"requesting server info 64 from %s", aAddrStr);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", aBuf);
	}

	mem_copy(Buffer, SERVERBROWSE_GETINFO_64_LEGACY, sizeof(SERVERBROWSE_GETINFO_64_LEGACY));
	Buffer[sizeof(SERVERBROWSE_GETINFO_64_LEGACY)] = m_CurrentToken;

	Packet.m_ClientID = -1;
	Packet.m_Address = Addr;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(Buffer);
	Packet.m_pData = Buffer;

	m_pNetClient->Send(&Packet);

	if(pEntry)
		pEntry->m_RequestTime = time_get();
}


void CServerBrowser::RequestCurrentServer(const NETADDR &Addr) const
{
	RequestImpl(Addr, 0);
}

void CServerBrowser::RequestServerCount()
{
	NETADDR Addr;
	CNetChunk Packet;
	int i = 0;

	m_NeedRefresh = 0;
	m_MasterServerCount = -1;
	mem_zero(&Packet, sizeof(Packet));
	Packet.m_ClientID = -1;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(SERVERBROWSE_GETCOUNT);
	Packet.m_pData = SERVERBROWSE_GETCOUNT;

	for(i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		if(!m_pMasterServer->IsValid(i))
			continue;

		Addr = m_pMasterServer->GetAddr(i);
		m_pMasterServer->SetCount(i, -1);
		Packet.m_Address = Addr;
		m_pNetClient->Send(&Packet);
		if(g_Config.m_Debug)
		{
			dbg_msg("client_srvbrowse", "count-request sent to %d", i);
		}
	}
}

void CServerBrowser::ProcessServerCount()
{
	m_MasterServerCount = 0;
	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		if(!m_pMasterServer->IsValid(i))
			continue;
		int Count = m_pMasterServer->GetCount(i);
		if(Count == -1)
		{
			/* ignore Server
				m_MasterServerCount = -1;
				return;
				// we don't have the required server information
				*/
		}
		else
			m_MasterServerCount += Count;
	}
}

void CServerBrowser::RequestServerList()
{
	// request serverlist
	NETADDR Addr;
	CNetChunk Packet;
	mem_zero(&Packet, sizeof(Packet));
	Packet.m_ClientID = -1;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	Packet.m_DataSize = sizeof(SERVERBROWSE_GETLIST);
	Packet.m_pData = SERVERBROWSE_GETLIST;

	for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
	{
		if(!m_pMasterServer->IsValid(i))
			continue;

		Addr = m_pMasterServer->GetAddr(i);
		Packet.m_Address = Addr;
		m_pNetClient->Send(&Packet);
	}
	if(g_Config.m_Debug)
	{
		dbg_msg("client_srvbrowse", "servercount: %d, requesting server list", m_MasterServerCount);
	}
	m_LastPacketTick = 0;
}

void CServerBrowser::ProcessServerList()
{
	int64 Timeout = time_freq();
	int64 Now = time_get();
	CServerEntry *pEntry;

	pEntry = m_pFirstReqServer;
	int Count = 0;
	while(pEntry) // go through all entries that we currently have and request the infos
	{
		// check if entry timed out
		if(pEntry->m_RequestTime && pEntry->m_RequestTime+Timeout < Now)
		{
			pEntry = pEntry->m_pNextReq;
			continue;
		}

		// no more than X concurrent requests
		if(Count >= m_CurrentMaxRequests)
			break;

		if(pEntry->m_RequestTime == 0)
		{
			if (pEntry->m_Request64Legacy)
				RequestImpl64(pEntry->m_Addr, pEntry);
			else
				RequestImpl(pEntry->m_Addr, pEntry);
		}

		Count++;
		pEntry = pEntry->m_pNextReq;
	}

	// no more current server requests
	if(Count == 0)
	{
		if(m_pFirstReqServer && m_CurrentMaxRequests > g_Config.m_BrCurrRequestsAbortLimit)
		{
			// reset old ones
			pEntry = m_pFirstReqServer;
			while(pEntry)
			{
				pEntry->m_RequestTime = 0;
				pEntry = pEntry->m_pNextReq;
			}

			// update max-requests
			m_CurrentMaxRequests /= 2;
			if(m_CurrentMaxRequests <= g_Config.m_BrCurrRequestsAbortLimit)
			{
				AbortRefresh();
			}
		}
	}
}

void CServerBrowser::Update(bool ForceResort)
{
	// StateMachine part 1: do server list requests
	if(m_NeedRefresh && !m_pMasterServer->IsRefreshing())
	{
		RequestServerCount();
	}

	// StateMachine part 2: check if all server counts arrived & request the serverlist
	if(m_MasterServerCount == -1)
	{
		ProcessServerCount();
		RequestServerList();
	}
	else if(m_MasterServerCount > -1)
	{
		ProcessServerCount();
	}

	if(m_MasterServerCount > m_NumRequests + m_LastPacketTick)
	{
		++m_LastPacketTick;
		return; // wait for more packets
	}

	LOCK_SECTION_LAZY_DBG(m_Lock);
	if(!__SectionLock.TryToLock())
		return;

	ProcessServerList();

	UNLOCK_SECTION();

	// check if we need to resort
//	if(!(g_Config.m_BrLazySorting && IsRefreshing() && LoadingProgression() < 90))
		if(ForceResort || m_Sorthash != SortHash())
			Sort();
}


bool CServerBrowser::IsFavorite(const NETADDR &Addr) const
{
	// search for the address
	int i;
	for(i = 0; i < m_NumFavoriteServers; i++)
	{
		if(net_addr_comp(&Addr, &m_aFavoriteServers[i]) == 0)
			return true;
	}
	return false;
}

void CServerBrowser::AddFavorite(const NETADDR &Addr)
{
	CServerEntry *pEntry;

	if(m_NumFavoriteServers == MAX_FAVORITES)
		return;

	// make sure that we don't already have the server in our list
	for(int i = 0; i < m_NumFavoriteServers; i++)
	{
		if(net_addr_comp(&Addr, &m_aFavoriteServers[i]) == 0)
			return;
	}

	// add the server to the list
	m_aFavoriteServers[m_NumFavoriteServers++] = Addr;
	pEntry = Find(Addr);
	if(pEntry)
		pEntry->m_Info.m_Favorite = 1;

	if(g_Config.m_UiBrowserPage == CMenus::PAGE_BROWSER_FAVORITES)
		Refresh(IServerBrowser::TYPE_FAVORITES);

	if(g_Config.m_Debug)
	{
		char aAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aAddrStr, sizeof(aAddrStr), true);
		m_pConsole->Printf(IConsole::OUTPUT_LEVEL_DEBUG, "client_srvbrowse", "added fav: %s", aAddrStr);
	}
}

void CServerBrowser::RemoveFavorite(const NETADDR &Addr)
{
	int i;
	CServerEntry *pEntry;

	for(i = 0; i < m_NumFavoriteServers; i++)
	{
		if(net_addr_comp(&Addr, &m_aFavoriteServers[i]) == 0)
		{
			mem_move(&m_aFavoriteServers[i], &m_aFavoriteServers[i+1], sizeof(NETADDR)*(m_NumFavoriteServers-(i+1)));
			m_NumFavoriteServers--;

			pEntry = Find(Addr);
			if(pEntry)
				pEntry->m_Info.m_Favorite = 0;

			if(g_Config.m_UiBrowserPage == CMenus::PAGE_BROWSER_FAVORITES)
				Refresh(IServerBrowser::TYPE_FAVORITES);

			return;
		}
	}
}

/*template<class T>
inline void swap(T &a, T &b)
{
	T c = b;
	b = a;
	a = c;
}*/

void CServerBrowser::AddRecent(const NETADDR& Addr)
{
	// add the address into the database
	{
		char aNetAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aNetAddrStr, sizeof(aNetAddrStr), Addr.port);

		char *pQueryBuf = sqlite3_mprintf("INSERT OR REPLACE INTO recent (addr) VALUES ('%q');", aNetAddrStr);
		CQueryRecent *pQuery = new CQueryRecent(pQueryBuf);
		m_pRecentDB->InsertQuery(pQuery);

		m_pConsole->Printf(IConsole::OUTPUT_LEVEL_DEBUG, "srvbrowse", "added recent '%s'", aNetAddrStr);
	}

	// add it to our current session recent cache
	RecentServer e(Addr, m_aRecentServers.size());
	for(int i = 0; i < m_aRecentServers.size(); i++)
		if(m_aRecentServers[i] == e)
			m_aRecentServers.remove_index(i);
	m_aRecentServers.add(e);

	if(g_Config.m_UiBrowserPage == CMenus::PAGE_BROWSER_RECENT)
		Refresh(IServerBrowser::TYPE_RECENT);
}

void CServerBrowser::RemoveRecent(const NETADDR& Addr)
{
	// remove the address into the database
	{
		char aNetAddrStr[NETADDR_MAXSTRSIZE];
		net_addr_str(&Addr, aNetAddrStr, sizeof(aNetAddrStr), Addr.port);

		char *pQueryBuf = sqlite3_mprintf("DELETE FROM recent WHERE addr = '%q';", aNetAddrStr);
		CQueryRecent *pQuery = new CQueryRecent(pQueryBuf);
		m_pRecentDB->InsertQuery(pQuery);

		m_pConsole->Printf(IConsole::OUTPUT_LEVEL_DEBUG, "srvbrowse", "removed recent '%s'", aNetAddrStr);
	}

	// remove it from our current session recent cache
	RecentServer e(Addr, m_aRecentServers.size());
	for(int i = 0; i < m_aRecentServers.size(); i++)
		if(m_aRecentServers[i] == e)
			m_aRecentServers.remove_index(i);

	if(g_Config.m_UiBrowserPage == CMenus::PAGE_BROWSER_RECENT)
		Refresh(IServerBrowser::TYPE_RECENT);
}

void CServerBrowser::ClearRecent()
{
	// remove the addresses from the database
	{
		char *pQueryBuf = sqlite3_mprintf("DROP TABLE IF EXISTS recent;");
		CQueryRecent *pQuery = new CQueryRecent(pQueryBuf);
		m_pRecentDB->InsertQuery(pQuery);
	}

	// remove it from our current session recent cache
	m_pConsole->Printf(IConsole::OUTPUT_LEVEL_DEBUG, "srvbrowse", "cleared recent, removed %i entries", m_aRecentServers.size());
	m_aRecentServers.clear();

	if(g_Config.m_UiBrowserPage == CMenus::PAGE_BROWSER_RECENT)
		Refresh(IServerBrowser::TYPE_RECENT);

}


void CServerBrowser::LoadDDNet()
{
	// reset servers / countries
	m_NumDDNetCountries = 0;
	m_NumDDNetTypes = 0;

	// load ddnet server list
	IStorageTW *pStorage = Kernel()->RequestInterface<IStorageTW>();
	IOHANDLE File = pStorage->OpenFile("tmp/cache/ddnet-servers.json", IOFLAG_READ, IStorageTW::TYPE_ALL);

	if(!File)
		return;

	char aBuf[4096*4];
	mem_zero(aBuf, sizeof(aBuf));

	io_read(File, aBuf, sizeof(aBuf));
	io_close(File);

	// parse JSON
	json_value *pJsonCountries = json_parse(aBuf, (size_t)str_length(aBuf));

	if(pJsonCountries && pJsonCountries->type == json_array)
	{
		for(unsigned int i = 0; i < pJsonCountries->u.array.length && m_NumDDNetCountries < MAX_DDNET_COUNTRIES; i++)
		{
			// pSrv - { name, flagId, servers }
			const json_value &jsonSrv = (*pJsonCountries)[i];
			const json_value &jsonTypes = jsonSrv["servers"];
			const json_value &jsonName = jsonSrv["name"];
			const json_value &jsonFlagID = jsonSrv["flagId"];

			if (jsonSrv.type != json_object || jsonTypes.type != json_object || jsonName.type != json_string || jsonFlagID.type != json_integer)
			{
				dbg_msg("client_srvbrowse", "invalid attributes");
				continue;
			}

			// build structure
			CDDNetCountry *pCntr = &m_aDDNetCountries[m_NumDDNetCountries];

			pCntr->Reset();

			str_copy(pCntr->m_aName, (const char *)jsonName, sizeof(pCntr->m_aName));
			pCntr->m_FlagID = (int)(json_int_t)jsonFlagID;

			// add country
			for (unsigned int t = 0; t < jsonTypes.u.object.length; t++)
			{
				const char *pType = jsonTypes.u.object.values[t].name;
				const json_value &jsonAddrs = *(jsonTypes.u.object.values[t].value);

				// add type
				if(jsonAddrs.u.array.length > 0 && m_NumDDNetTypes < MAX_DDNET_TYPES)
				{
					int pos;
					for(pos = 0; pos < m_NumDDNetTypes; pos++)
					{
						if(!str_comp(m_aDDNetTypes[pos], pType))
							break;
					}
					if(pos == m_NumDDNetTypes)
					{
						str_copy(m_aDDNetTypes[m_NumDDNetTypes], pType, sizeof(m_aDDNetTypes[m_NumDDNetTypes]));
						m_NumDDNetTypes++;
					}
				}

					// add addresses
					for (unsigned int g = 0; g < jsonAddrs.u.array.length; g++, pCntr->m_NumServers++)
					{
						const json_value &jsonAddr = jsonAddrs[ g];

						net_addr_from_str(&pCntr->m_aServers[pCntr->m_NumServers], (const char *)jsonAddr);
						str_copy(pCntr->m_aTypes[pCntr->m_NumServers], pType, sizeof(pCntr->m_aTypes[pCntr->m_NumServers]));
					}
				}

			m_NumDDNetCountries++;
		}
	}

	if(pJsonCountries)
		json_value_free(pJsonCountries);

}

bool CServerBrowser::IsRefreshing() const
{
	return m_pFirstReqServer != 0;
}

bool CServerBrowser::IsRefreshingMasters() const
{
	return m_pMasterServer->IsRefreshing();
}


int CServerBrowser::LoadingProgression() const
{
	if(m_NumServers == 0)
		return 0;

	float Servers = m_NumServers;
	float Loaded = m_NumServers-m_NumRequests;
	return round_to_int(100.0f * Loaded/Servers);
}


void CServerBrowser::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CServerBrowser *pSelf = (CServerBrowser *)pUserData;

	char aAddrStr[128];
	char aBuffer[256];
	for(int i = 0; i < pSelf->m_NumFavoriteServers; i++)
	{
		net_addr_str(&pSelf->m_aFavoriteServers[i], aAddrStr, sizeof(aAddrStr), true);
		str_format(aBuffer, sizeof(aBuffer), "add_favorite %s", aAddrStr);
		pConfig->WriteLine(aBuffer);
	}
}

void CServerBrowser::DDNetFilterAdd(char *pFilter, const char *pName)
{
	if (DDNetFiltered(pFilter, pName))
		return;

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), ",%s", pName);
	str_append(pFilter, aBuf, 128);
}

void CServerBrowser::DDNetFilterRem(char *pFilter, const char *pName)
{
	if (!DDNetFiltered(pFilter, pName))
		return;

	// rewrite exclude/filter list
	char aBuf[128];
	char *p;

	str_copy(aBuf, pFilter, sizeof(aBuf));
	pFilter[0] = '\0';

	p = strtok(aBuf, ",");

	while(p)
	{
		if(str_comp_nocase(pName, p) != 0)
		{
			char aBuf2[128];
			str_format(aBuf2, sizeof(aBuf2), ",%s", p);
			str_append(pFilter, aBuf2, 128);
		}

		p = strtok(NULL, ",");
	}
}

bool CServerBrowser::DDNetFiltered(char *pFilter, const char *pName)
{
	char aBuf[128];
	char *p;

	str_copy(aBuf, pFilter, sizeof(aBuf));

	p = strtok(aBuf, ",");

	while(p)
	{
		if(str_comp_nocase(pName, p) == 0)
			return true; // country excluded

		p = strtok(NULL, ",");
	}

	return false; // contry not excluded
}

void CServerBrowser::DDNetCountryFilterClean()
{
	char aNewList[128];
	aNewList[0] = '\0';

	for(int i = 0; i < m_NumDDNetCountries; i++)
	{
		const char *pName = m_aDDNetCountries[i].m_aName;
		if(DDNetFiltered(g_Config.m_BrFilterExcludeCountries, pName))
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), ",%s", pName);
			str_append(aNewList, aBuf, sizeof(aNewList));
		}
	}

	str_copy(g_Config.m_BrFilterExcludeCountries, aNewList, sizeof(g_Config.m_BrFilterExcludeCountries));
}

void CServerBrowser::DDNetTypeFilterClean()
{
	char aNewList[128];
	aNewList[0] = '\0';

	for(int i = 0; i < m_NumDDNetTypes; i++)
	{
		const char *pName = m_aDDNetTypes[i];
		if(DDNetFiltered(g_Config.m_BrFilterExcludeTypes, pName))
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), ",%s", pName);
			str_append(aNewList, aBuf, sizeof(aNewList));
		}
	}

	str_copy(g_Config.m_BrFilterExcludeTypes, aNewList, sizeof(g_Config.m_BrFilterExcludeTypes));
}

bool CServerBrowser::IsLocked()
{
	LOCK_SECTION_LAZY_DBG(m_Lock);
	bool WasLocked = !__SectionLock.TryToLock();
	return WasLocked;
}
