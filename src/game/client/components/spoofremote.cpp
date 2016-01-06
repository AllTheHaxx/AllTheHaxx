#include <stdio.h> //perror
#include <string.h>    //strlen
#include <base/system.h>

#if defined(CONF_FAMILY_UNIX)
	#include <unistd.h>
	#include <sys/socket.h>    //socket
	#include <arpa/inet.h> //inet_addr
	#include <netdb.h> //hostent
	#define RAISE_ERROR(msg) printf("At %s(%i) occurred error '%s':\n", __FILE__, __LINE__, msg); perror(msg)
#endif

#if defined(CONF_FAMILY_WINDOWS)
	#include "stdafx.h"
	#pragma comment(lib, "ws2_32.lib")
	#include <WinSock2.h>
	#include <Windows.h>
	#define RAISE_ERROR(msg) printf("At %s(%i) occurred error '%s':\n", __FILE__, __LINE__, msg); WSAGetLastError()
#endif

#include <engine/shared/config.h>
#include "spoofremote.h"


CSpoofRemote::CSpoofRemote()
{
	m_pListenerThread = 0;
}

CSpoofRemote::~CSpoofRemote()
{
	thread_destroy(m_pListenerThread);
#if defined(CONF_FAMILY_UNIX)
	close(m_Socket);
#endif
#if defined(CONF_FAMILY_WINDOWS)
	closesocket(m_Socket);
	WSACleanup();
#endif
}

void CSpoofRemote::OnConsoleInit()
{
	Console()->Register("spf_connect", "si", CFGFLAG_CLIENT, ConConnect, (void *)this, "connect to teh zervor 4 h4xX0r");
	Console()->Register("spf", "s", CFGFLAG_CLIENT, ConCommand, (void *)this, "3X3CUT3 C0MM4ND!");
}

void CSpoofRemote::Init(const char *pAddr, int Port)
{
#if defined(CONF_FAMILY_WINDOWS)
	// WSA
	WSADATA data;
	if(WSAStartup(MAKEWORD(2, 0), &data) != 0)
	{
		RAISE_ERROR("WSAStartup");
		Console()->Print(0, "spoofremote", "error while starting WSA", false);
		WSAGetLastError();
		return;
	}
#endif

	// Socket
	m_Socket = socket(AF_INET , SOCK_STREAM , 0);
	if (m_Socket == -1)
	{
		RAISE_ERROR("socket");
		Console()->Print(0, "spoofremote", "error while creating socket", false);
		return;
	}

	// Info
	m_Info.sin_addr.s_addr = inet_addr(pAddr);
	m_Info.sin_family = AF_INET;
	m_Info.sin_port = htons(Port);

	Console()->Print(0, "spoofremote", "Connecting to zervor...", false);

	// Connect
	if (connect(m_Socket, (struct sockaddr*)&m_Info, sizeof(m_Info)) < 0)
	{
		RAISE_ERROR("connect");
		Console()->Print(0, "spoofremote", "error while connecting", false);
		return;
	}

	CreateThread((void *)this);
}

void CSpoofRemote::CreateThread(void *pUser)
{
	m_pListenerThread = thread_init(CSpoofRemote::Listener, pUser);
}

void CSpoofRemote::Listener(void *pUserData)
{
	CSpoofRemote *pSelf = (CSpoofRemote *)pUserData;
	int sock = pSelf->m_Socket;
	char rBuffer[256];
	while(1)
	{
		memset(&rBuffer, 0, sizeof(rBuffer));

		if(recv(sock, rBuffer, sizeof(rBuffer), 0) < 0)
		{
			pSelf->Console()->Print(0, "spoofremote", "error in recv", false);
			perror("Error in recv()");
		}
		else
			pSelf->Console()->Print(0, "spoofremote][server", rBuffer, true);
	}
}

void CSpoofRemote::SendCommand(const char *pCommand)
{
	if(!pCommand)
		return;

	if(send(m_Socket, pCommand, strlen(pCommand), 0) < 0)
	{
		Console()->Print(0, "spoofremote", "error while sending", false);
		perror("Error in send()");
	}
}


void CSpoofRemote::ConConnect(IConsole::IResult *pResult, void *pUserData)
{
	((CSpoofRemote *)pUserData)->Init(pResult->GetString(0), pResult->GetInteger(1));
}

void CSpoofRemote::ConCommand(IConsole::IResult *pResult, void *pUserData)
{
	((CSpoofRemote *)pUserData)->SendCommand(pResult->GetString(0));
}
