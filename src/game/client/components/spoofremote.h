#ifndef GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H
#define GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H

#include <base/system.h>
#include <game/client/component.h>

#if defined(CONF_FAMILY_UNIX)
#include <netinet/in.h>
#endif

class CSpoofRemote : public CComponent
{
public:
	enum
		{
			SPOOF_STATE_CONNECTED = 0x1,
			SPOOF_STATE_DUMMIES = 0x2,
			SPOOF_STATE_DUMMYSPAM = 0x4,
		};

private:

	// engine variables
	void *m_pListenerThread;
	void *m_pWorkerThread;
	time_t m_LastAck;
#if defined(CONF_FAMILY_UNIX)
	int m_Socket;
	struct sockaddr_in m_Info;
#endif
#if defined(CONF_FAMILY_WINDOWS)
	SOCKET m_Socket;
	SOCKADDR_IN info;
#endif

	// control variables
	int m_State; // current state, see above
	int m_SpoofRemoteID; // our id at teh zervor
	char m_aLastMessage[256]; // the last message from zervor
	float m_LastMessageTime; // when teh l4st
	char m_aLastCommand[256]; // last message we've sent


	void Reset();

	void ParseZervorMessage(const char *pMessage);
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

	inline bool IsConnected() const { return m_State&SPOOF_STATE_CONNECTED; }
	inline int IsState(int state) const { return m_State&state; }
	inline const char *LastMessage() const { return m_aLastMessage; }
	inline float LastMessageTime() const { return m_LastMessageTime; }
	inline const char *LastCommand() const { return m_aLastCommand; }

	void Connect(const char *pAddr, int Port);
	void Disconnect();

	void OnConsoleInit();
	void SendCommand(const char *pCommand);
};

#endif
