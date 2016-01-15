#ifndef GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H
#define GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H

#include <base/system.h>
#include <game/client/component.h>

#if defined(CONF_FAMILY_UNIX)
#include <netinet/in.h>
#endif

class CSpoofRemote : public CComponent
{
	bool m_IsConnected; // whether we are connected to teh zervor
	char m_LastMessage[256]; // the last message from zervor
	float m_LastMessageTime;
	int m_SpoofRemoteID; // our id at teh zervor
	void *m_pListenerThread;
	void *m_pWorkerThread;
	time_t m_LastAck, m_ErrorTime;
#if defined(CONF_FAMILY_UNIX)
	int m_Socket;
	struct sockaddr_in m_Info;
#endif
#if defined(CONF_FAMILY_WINDOWS)
	SOCKET m_Socket;
	SOCKADDR_IN info;
#endif

	void Reset();
	void Connect(const char *pAddr, int Port);
	void Disconnect();
	static void CreateThreads(void *pUserData);
	static void Listener(void *pUserData);
	static void Worker(void *pUserData);

	static void ConConnect(IConsole::IResult *pResult, void *pUserData);
	static void ConDisconnect(IConsole::IResult *pResult, void *pUserData);
	static void ConForceClose(IConsole::IResult *pResult, void *pUserData);
	static void ConCommand(IConsole::IResult *pResult, void *pUserData);

public:
	CSpoofRemote();
	~CSpoofRemote();

	inline bool IsConnected() const { return m_IsConnected; }
	inline const char *LastMessage() const { return m_LastMessage; }
	inline float LastMessageTime() const { return m_LastMessageTime; }
	void OnConsoleInit();
	void SendCommand(const char *pCommand);
};

#endif
