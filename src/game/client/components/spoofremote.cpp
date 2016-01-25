#include <base/system.h>
#if defined(CONF_FAMILY_UNIX)

#include <stdio.h> //perror
#include <string.h>    //strlen
#include <ctime> // time
#include <unistd.h>
#include <sys/socket.h>    //socket
#include <arpa/inet.h> //inet_addr
#include <netdb.h> //hostent

#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include "voting.h"
#include "spoofremote.h"

#define RAISE_ERROR(msg) printf("At %s(%i) occurred error '%s'\n", __FILE__, __LINE__, msg); perror(":");


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
	m_Socket = -1;
	m_SpoofRemoteID = -1;
	m_LastAck = 0;
	m_State = 0;
	mem_zero(m_aLastMessage, sizeof(m_aLastMessage));
	mem_zero(m_aLastCommand, sizeof(m_aLastCommand));
	m_LastMessageTime = -1.0f;
}

void CSpoofRemote::OnConsoleInit()
{
	Console()->Register("spf_connect", "", CFGFLAG_CLIENT, ConConnect, (void *)this, "connect to teh zervor 4 h4xX0r");
	Console()->Register("spf_disconnect", "", CFGFLAG_CLIENT, ConDisconnect, (void *)this, "disconnect from teh zervor 4 h4xX0r");
	Console()->Register("spf_forceclose", "s", CFGFLAG_CLIENT, ConForceClose, (void *)this, "force-close the connection");
	Console()->Register("spf", "s", CFGFLAG_CLIENT, ConCommand, (void *)this, "3X3CUT3 C0MM4ND!");
}

void CSpoofRemote::OnRender()
{
	// nevar forgetti moms spaghetti
	if(IsState(SPOOF_STATE_VOTEKICKALL))
	{
		static int64 LastKickTime;
		static int CurClientID = 0;
		static bool Step = false;

		if(time_get()-LastKickTime > time_freq()/4)
		{
			if(Step)
			{
				CServerInfo CurrentServerInfo;
				char aServerAddr[64];
				Client()->GetServerInfo(&CurrentServerInfo);
				str_copy(aServerAddr, CurrentServerInfo.m_aAddress, sizeof(aServerAddr));
				net_addr_split(aServerAddr, sizeof(aServerAddr));

				char aCmd[256];
				str_format(aCmd, sizeof(aCmd), "vb %s 48 1", aServerAddr);
				SendCommand(aCmd);

				LastKickTime = time_get();
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

				LastKickTime = time_get();
				CurClientID++;
			}
		}

		if(CurClientID > MAX_CLIENTS)
		{
			m_State &= ~SPOOF_STATE_VOTEKICKALL;
			LastKickTime = 0;
			CurClientID = 0;
		}
	}
}

void CSpoofRemote::Connect(const char *pAddr, int Port)
{
	// Info
	m_Info.sin_addr.s_addr = inet_addr(pAddr);
	m_Info.sin_family = AF_INET;
	m_Info.sin_port = htons(Port);

	// Socket
	Console()->Print(0, "spfrmt", "opening socket...", false);
	m_Socket = socket(AF_INET , SOCK_STREAM , 0);
	if (m_Socket == -1)
	{
		RAISE_ERROR("socket");
		Console()->Print(0, "spfrmt", "error while creating socket", false);
		return;
	}

	// Connect in a thread so that the game doesn't get hung
	m_LastAck = time(NULL);
	thread_init(CSpoofRemote::CreateThreads, (void *)this);
}

void CSpoofRemote::Disconnect()
{
	Console()->Print(0, "spfrmt", "disconnecting from zervor!", false);
	Console()->Print(0, "spfrmt", "requesting threads to terminate...", false);
	Reset();
	Console()->Print(0, "spfrmt", "closing socket...", false);
	close(m_Socket);
	m_Socket = -1;
}

void CSpoofRemote::CreateThreads(void *pUserData)
{
	CSpoofRemote *pSelf = (CSpoofRemote *)pUserData;

	pSelf->Console()->Print(0, "spfrmt", "Connecting to zervor...", false);
	if (connect(pSelf->m_Socket, (struct sockaddr*)&pSelf->m_Info, sizeof(m_Info)) < 0)
	{
		RAISE_ERROR("connect");
		pSelf->Console()->Print(0, "spfrmt", "error while connecting", false);
		return;
	}
	pSelf->m_State |= SPOOF_STATE_CONNECTED;

	pSelf->Console()->Print(0, "spfrmt", "connected, creating threads...", false);

	pSelf->m_pWorkerThread = thread_init(CSpoofRemote::Worker, pUserData);
	pSelf->m_pListenerThread = thread_init(CSpoofRemote::Listener, pUserData);
	return;
}

void CSpoofRemote::ParseZervorMessage(const char *pMessage)
{
	// ~~~ concerning dummies
	if(!IsState(SPOOF_STATE_DUMMIES) && (
			str_comp_nocase("[Server]: Dummies connected!", pMessage) == 0 ||
			str_comp_nocase("[Server]: Dummies connected (voting...)!", pMessage) == 0))
		m_State |= SPOOF_STATE_DUMMIES;

	if(IsState(SPOOF_STATE_DUMMIES) &&
			str_comp_nocase("[Server]: Dummies disconnected.", pMessage) == 0)
		m_State &= ~SPOOF_STATE_DUMMIES;

	if(!IsState(SPOOF_STATE_DUMMYSPAM) &&
			str_comp_nocase("[Server]: Dummyspam started!", pMessage) == 0)
		m_State |= SPOOF_STATE_DUMMYSPAM;

	if(IsState(SPOOF_STATE_DUMMYSPAM) &&
			str_comp_nocase("[Server]: Dummyspam stopped!", pMessage) == 0)
		m_State &= ~SPOOF_STATE_DUMMYSPAM;
}

void CSpoofRemote::Listener(void *pUserData)
{
	CSpoofRemote *pSelf = (CSpoofRemote *)pUserData;

	pSelf->Console()->Print(0, "spfrmt", "started listener thread", false);
	while(1)
	{
		if(!pSelf->IsConnected())
		{
			pSelf->Console()->Print(0, "spfrmt", "closed listener thread", false);
			return;
		}

		// receive
		char rBuffer[256];
		memset(&rBuffer, 0, sizeof(rBuffer));
		int ret = recv(pSelf->m_Socket, rBuffer, sizeof(rBuffer), 0);
		if(ret <= 0 || str_comp(rBuffer, "") == 0)
		{
			RAISE_ERROR("Error while receiving");
			pSelf->Console()->Print(0, "spfrmt", "disconnected due to connection problems", false);
			pSelf->Disconnect();
		}
		else
		{
			if(pSelf->m_SpoofRemoteID < 0)
				pSelf->m_SpoofRemoteID = atoi(rBuffer);

			if(rBuffer[0] == '\x16') // keepalive from server
			{
				pSelf->m_LastAck = time(NULL);
			}
			else if(rBuffer[0] == '\x04') // EOT
			{
				pSelf->Console()->Print(0, "spfrmt", "End of transmission: ", true);

				if(rBuffer[1] == '\x06')
					pSelf->Console()->Print(0, "spfrmt", "Disconneted from teh zervor.", true); // maybe leave these message to the server?
				else if(rBuffer[1] == '\x15')
					pSelf->Console()->Print(0, "spfrmt", "Ack timeout.", true);
				else
					pSelf->Console()->Print(0, "spfrmt", "No reason given.", true);

				pSelf->Disconnect();
			}
			else
			{
				pSelf->Console()->Print(0, "spfrmtmsg", rBuffer, true);
				str_copy(pSelf->m_aLastMessage, rBuffer, sizeof(pSelf->m_aLastMessage));
				pSelf->m_LastMessageTime = pSelf->Client()->LocalTime();

				pSelf->ParseZervorMessage(rBuffer);
			}
		}
	}
}

void CSpoofRemote::Worker(void *pUserData)
{
	CSpoofRemote *pSelf = (CSpoofRemote *)pUserData;

	pSelf->Console()->Print(0, "spfrmt", "started worker thread", false);
	while(1)
	{
		if(!pSelf->IsConnected())
		{
			pSelf->Console()->Print(0, "spfrmt", "closed worker thread", false);
			return;
		}

		if(pSelf->m_SpoofRemoteID < 0)
			continue;

	/*	static bool HasWarned = false;
		if(!HasWarned && time(0) > pSelf->m_LastAck + 1*60) // after one minute: warning
		{
			pSelf->Console()->Print(0, "spfrmt", "Warning: zervor hasn't responded for a minute!", true);
			pSelf->Console()->Print(0, "spfrmt", "Warning: disconnecting 60 seconds!", true);
			HasWarned = true;
		}
		else if(HasWarned && time(0) < pSelf->m_LastAck + 2)
		{
			pSelf->Console()->Print(0, "spfrmt", "Yey, teh zervor has just came back alive :P", true);
			HasWarned = false;
		}
		if(time(0) > pSelf->m_LastAck + 2*60) // after two minutes: disconenct
		{
			pSelf->Console()->Print(0, "spfrmt", "Warning: zervor hasn't responded for two minutes!", true);
			pSelf->Console()->Print(0, "spfrmt", "Warning: it most likely won't come back, disconnecting!", true);
			pSelf->Disconnect();
		}
	*/
		static time_t LastAck = time(NULL);
		if(time(NULL) < LastAck + 15)
			continue;

		// keep alive every 15 seconds
		char aBuf[32];
		snprintf(aBuf, sizeof(aBuf), "\x16 %d", pSelf->m_SpoofRemoteID);
		pSelf->SendCommand(aBuf);
		LastAck = time(NULL);
		thread_sleep(1);
	}

}

void CSpoofRemote::SendCommand(const char *pCommand)
{
	if(!pCommand)
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

	if(send(m_Socket, pCommand, strlen(pCommand), 0) < 0)
	{
		Console()->Print(0, "spfrmt", "error while sending", false);
		RAISE_ERROR("Error while sending");
		Disconnect();
	}
}


void CSpoofRemote::ConConnect(IConsole::IResult *pResult, void *pUserData)
{
	CSpoofRemote *pSelf = ((CSpoofRemote *)pUserData);
	if(pSelf->IsConnected())
		pSelf->Console()->Print(0, "spfrmt", "Disconnect first before opening a new connection!", false);
	else
		pSelf->Connect(g_Config.m_ClSpoofSrvIP, g_Config.m_ClSpoofSrvPort);
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

#endif
