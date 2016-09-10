#include <base/system.h>

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <cstring>
#include "voting.h"
#include "spoofremote.h"

CSpoofRemote::CSpoofRemote()
{
	Reset();
}

CSpoofRemote::~CSpoofRemote()
{
	if(IsConnected())
		SendCommand("exit");
}

void CSpoofRemote::Reset()
{
	m_pListenerThread = 0;
	m_pWorkerThread = 0;
	m_LastAck = 0;
	m_SpoofRemoteID = -1;
	m_State = 0;
	m_ConnState = 0;
	mem_zero(m_aLastMessage, sizeof(m_aLastMessage));
	mem_zero(m_aLastCommand, sizeof(m_aLastCommand));
	m_LastMessageTime = 0;
	m_Socket.type = NETTYPE_INVALID;
	mem_zero(m_aNetAddr, sizeof(m_aNetAddr));
}

void CSpoofRemote::OnConsoleInit()
{
#if defined(CONF_SPOOFING)
	Console()->Register("spf_connect", "", CFGFLAG_CLIENT, ConConnect, (void *)this, "connect to teh zervor 4 h4xX0r");
	Console()->Register("spf_disconnect", "", CFGFLAG_CLIENT, ConDisconnect, (void *)this, "disconnect from teh zervor 4 h4xX0r");
	Console()->Register("spf_forceclose", "s", CFGFLAG_CLIENT, ConForceClose, (void *)this, "force-close the connection");
	Console()->Register("spf", "r", CFGFLAG_CLIENT, ConCommand, (void *)this, "3X3CUT3 C0MM4ND!");
#endif
}

void CSpoofRemote::OnInit()
{
#if defined(CONF_SPOOFING)
	if(IsConnState(CONNSTATE_DISCONNECTED) && g_Config.m_ClSpoofAutoconnect)
		Connect();
#endif
}

void CSpoofRemote::OnRender()
{
#if defined(CONF_SPOOFING)
	// nevar forgetti moms spaghetti
	if(IsSpfState(STATE_VOTEKICKALL))
	{
		static int64 LastKickTime;
		static int CurClientID = 0;
		static bool Step = false;

//		set_new_tick();
		const int64 Now = time_get();
		if(Now-LastKickTime > time_freq()/4)
		{
			if(Step)
			{
				char aServerAddr[NETADDR_MAXSTRSIZE];
				str_copy(aServerAddr, Client()->GetCurrentServerAddress(), sizeof(aServerAddr));
				net_addr_split(aServerAddr, sizeof(aServerAddr));

				char aCmd[256];
				str_format(aCmd, sizeof(aCmd), "vb %s 48 1", aServerAddr);
				SendCommand(aCmd);

				LastKickTime = Now;
				Step = false;
			}
			else
			{
				SendCommand("dcdum");

				while(CurClientID < MAX_CLIENTS &&
						!m_pClient->m_aClients[CurClientID].m_Active &&
						CurClientID != Client()->m_LocalIDs[0] &&
						CurClientID != Client()->m_LocalIDs[1])
					CurClientID++;

				dbg_msg("dbg", "%i", CurClientID);

				m_pClient->m_pVoting->CallvoteKick(CurClientID, "keck");
				Step = true;

				LastKickTime = Now;
				CurClientID++;
			}
		}

		if(CurClientID > MAX_CLIENTS)
		{
			m_State &= ~STATE_VOTEKICKALL;
			LastKickTime = 0;
			CurClientID = 0;
		}
	}
#endif
}

void CSpoofRemote::StartConnection(void *pUserData)
{
#if defined(CONF_SPOOFING)

	CSpoofRemote *pSelf = (CSpoofRemote *)pUserData;

	pSelf->m_ConnState = CONNSTATE_CONNECTING;
	pSelf->Console()->Print(0, "spfrmt", "Connecting to zervor...", false);

	NETADDR BindAddr;
	mem_zero(&pSelf->m_HostAddress, sizeof(pSelf->m_HostAddress));
	mem_zero(&BindAddr, sizeof(BindAddr));

	// lookup
	if(net_host_lookup(g_Config.m_ClSpoofSrvIP, &pSelf->m_HostAddress, NETTYPE_IPV4) != 0)
	{
		pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "spfrmt", "ERROR: Can't resolve %s", g_Config.m_ClSpoofSrvIP);
		pSelf->m_ConnState = CONNSTATE_CONNECTING;
		return;
	}

	pSelf->m_HostAddress.port = (unsigned short)g_Config.m_ClSpoofSrvPort;

	// connect
	BindAddr.type = NETTYPE_IPV4;
	pSelf->m_Socket = net_tcp_create(BindAddr);
	if(net_tcp_connect(pSelf->m_Socket, &pSelf->m_HostAddress) != 0)
	{
		net_tcp_close(pSelf->m_Socket);
		char aBuf[128];
		net_addr_str(&pSelf->m_HostAddress, aBuf, sizeof(aBuf), 0);
		pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "spfrmt", "ERROR: Can't connect to '%s:%d' on type=%i", aBuf, pSelf->m_HostAddress.port, pSelf->m_HostAddress.type);
		pSelf->m_ConnState = CONNSTATE_DISCONNECTED;
		int error = net_errno();
#if defined(CONF_FAMILY_WINDOWS)
		dbg_msg("spfrmt", " : (errorcode=%d)", error);
#else
		dbg_msg("spfrmt", " : (%d '%s')", error, strerror(error));
#endif
		return;
	}

	pSelf->m_LastAck = time_get();

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "spfrmt", "connected, creating threads...", false);
	pSelf->m_ConnState = CONNSTATE_CONNECTED;
	pSelf->m_pWorkerThread = thread_init(CSpoofRemote::Worker, pUserData);
	pSelf->m_pListenerThread = thread_init(CSpoofRemote::Listener, pUserData);

#endif
}

void CSpoofRemote::Disconnect()
{
#if defined(CONF_SPOOFING)
	Console()->Print(0, "spfrmt", "disconnecting from zervor!", false);
	Console()->Print(0, "spfrmt", "requesting threads to terminate...", false);
	Reset();
	Console()->Print(0, "spfrmt", "closing socket...", false);
	net_tcp_close(m_Socket);
#endif
}

void CSpoofRemote::ParseZervorMessage(const char *pMessage)
{
	// ~~~ concerning dummies
	if(!IsSpfState(STATE_DUMMIES) && (
			str_comp_nocase("[Server]: Dummies connected!", pMessage) == 0 ||
			str_comp_nocase("[Server]: Dummies connected (voting...)!", pMessage) == 0))
		m_State |= STATE_DUMMIES;

	if(IsSpfState(STATE_DUMMIES) &&
			str_comp_nocase("[Server]: Dummies disconnected.", pMessage) == 0)
		m_State &= ~STATE_DUMMIES;

	if(!IsSpfState(STATE_DUMMYSPAM) &&
			str_comp_nocase("[Server]: Dummyspam started!", pMessage) == 0)
		m_State |= STATE_DUMMYSPAM;

	if(IsSpfState(STATE_DUMMYSPAM) &&
			str_comp_nocase("[Server]: Dummyspam stopped!", pMessage) == 0)
		m_State &= ~STATE_DUMMYSPAM;
}

void CSpoofRemote::Listener(void *pUserData)
{
#if defined(CONF_SPOOFING)
	CSpoofRemote *pSelf = (CSpoofRemote *)pUserData;

	pSelf->Console()->Print(0, "spfrmt", "started listener thread", false);
	char rBuffer[512];
	while(1)
	{
		if(!pSelf->IsConnected())
		{
			pSelf->Console()->Print(0, "spfrmt", "closed listener thread", false);
			return;
		}

		// receive
		mem_zero(&rBuffer, sizeof(rBuffer));
		int ret = net_tcp_recv(pSelf->m_Socket, rBuffer, sizeof(rBuffer));
		if(ret <= 0 || str_comp(rBuffer, "") == 0)
		{
			dbg_msg("spfrmt", "error while receiving");
			pSelf->Console()->Print(0, "spfrmt", "disconnected due to connection problems", false);
			pSelf->Disconnect();
		}
		else
		{
			if(pSelf->m_SpoofRemoteID < 0)
				pSelf->m_SpoofRemoteID = atoi(rBuffer);

			if(str_comp_nocase(rBuffer, "ping") == 0) // keepalive from server
			{
				pSelf->m_LastAck = time(NULL);
			}
			else if(str_comp_nocase_num(rBuffer, "exit", 4) == 0) // connection ended
			{
				if(str_length(rBuffer) == 4)
					pSelf->Console()->Print(0, "spfrmt", "Disconneted from teh zervor.", true); // maybe leave these message to the server?
				else
					pSelf->Console()->Print(0, "spfrmt", rBuffer, true);

				pSelf->Disconnect();
			}
			else
			{
				pSelf->Console()->Print(0, "spfrmtmsg", rBuffer, true);
				str_copy(pSelf->m_aLastMessage, rBuffer, sizeof(pSelf->m_aLastMessage));
				pSelf->m_LastMessageTime = time_get();

				pSelf->ParseZervorMessage(rBuffer);
			}
		}
	}
#endif
}

void CSpoofRemote::Worker(void *pUserData)
{
#if defined(CONF_SPOOFING)
	CSpoofRemote *pSelf = (CSpoofRemote *)pUserData;

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "spfrmt", "started worker thread", false);
	int64 LastAck = time_get();
	while(1)
	{
		thread_sleep(1); // be nice

		if(!pSelf->IsConnected())
		{
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "spfrmt", "closed worker thread", false);
			return;
		}

		if(pSelf->m_SpoofRemoteID < 0)
			continue;

		// keep alive every 15 seconds
		if(time_get() < LastAck + 15*time_freq())
		{
			thread_sleep((int)((LastAck + 15 * time_freq() - time_get()) / 2));
			continue;
		}

		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "\x16 %d", pSelf->m_SpoofRemoteID);
		pSelf->SendCommand(aBuf);
		LastAck = time_get();
	}
#endif
}

void CSpoofRemote::SendCommand(const char *pCommand)
{
#if defined(CONF_SPOOFING)
	if(!pCommand || str_length(pCommand) <= 0)
		return;

	// save the command, but no control messages
	if(str_length(pCommand) > 4)
		str_copy(m_aLastCommand, pCommand, sizeof(m_aLastCommand));

	if(!IsConnected())
	{
		Console()->Print(0, "spfrmt", "not connected. Use spf_connect first!", false);
		str_copy(m_aLastMessage, "[Local]: not connected. Use spf_connect first!", sizeof(m_aLastMessage));
		return;
	}

	if(net_tcp_send(m_Socket, pCommand, str_length(pCommand)) < 0)
	{
		Console()->Print(0, "spfrmt", "error while sending, disconnecting!", false);
		dbg_msg("spfrmt", "error while sending");
		Disconnect();
	}
#endif
}


void CSpoofRemote::ConConnect(IConsole::IResult *pResult, void *pUserData)
{
#if defined(CONF_SPOOFING)
	CSpoofRemote *pSelf = ((CSpoofRemote *)pUserData);
	if(!pSelf->IsConnState(CONNSTATE_DISCONNECTED))
		pSelf->Console()->Print(0, "spfrmt", "Disconnect first before opening a new connection!", false);
	else
		pSelf->Connect();
#endif
}

void CSpoofRemote::ConDisconnect(IConsole::IResult *pResult, void *pUserData)
{
	CSpoofRemote *pSelf = ((CSpoofRemote *)pUserData);
	if(!pSelf->IsConnected())
		pSelf->Console()->Print(0, "spfrmt", "No need to disconnect, you are not connected!", false);
	else
		pSelf->SendCommand("exit");
}

void CSpoofRemote::ConForceClose(IConsole::IResult *pResult, void *pUserData)
{
	CSpoofRemote *pSelf = ((CSpoofRemote *)pUserData);
	pSelf->Console()->Print(0, "spfrmt", "Force-closing connection, resetting spfrmt!", false);
	pSelf->Disconnect();
}

void CSpoofRemote::ConCommand(IConsole::IResult *pResult, void *pUserData)
{
	CSpoofRemote *pSelf = ((CSpoofRemote *)pUserData);
	pSelf->SendCommand(pResult->GetString(0));
}

