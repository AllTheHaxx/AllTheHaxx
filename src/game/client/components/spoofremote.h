#ifndef GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H
#define GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H

#include <base/system.h>
#include <game/client/component.h>

#if defined(CONF_FAMILY_UNIX)
#include <netinet/in.h>
#endif

class CSpoofRemote : public CComponent
{
	void *m_pListenerThread;
#if defined(CONF_FAMILY_UNIX)
	int m_Socket;
	struct sockaddr_in m_Info;
#endif
#if defined(CONF_FAMILY_WINDOWS)
	SOCKET m_Socket;
	SOCKADDR_IN info;
#endif

	void Init(const char *pAddr, int Port);
	void CreateThread(void* pUser);
	static void Listener(void *pUserData);

	static void ConConnect(IConsole::IResult *pResult, void *pUserData);
	static void ConDisconnect(IConsole::IResult *pResult, void *pUserData);
	static void ConCommand(IConsole::IResult *pResult, void *pUserData);

public:
	CSpoofRemote();
	~CSpoofRemote();

	void OnConsoleInit();
	void SendCommand(const char *pCommand);
};

#endif
