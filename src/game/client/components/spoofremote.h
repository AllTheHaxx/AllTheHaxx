#ifndef GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H
#define GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H

#include <base/system.h>

#include <game/client/component.h>

class CSpoofRemote : public CComponent
{
public:
	enum
		{
			CONNSTATE_DISCONNECTED = 0,
			CONNSTATE_CONNECTING,
			CONNSTATE_CONNECTED,
			STATE_DUMMIES     = 1 << 0,
			STATE_DUMMYSPAM   = 1 << 1,
			STATE_VOTEKICKALL = 1 << 2,
		};

private:
	// engine variables
	void *m_pListenerThread;
	void *m_pWorkerThread;
	int64 m_LastAck;
	NETADDR m_HostAddress;
	NETSOCKET m_Socket;
	char m_aNetAddr[NETADDR_MAXSTRSIZE];

	// control variables
	int m_ConnState; // current state, see above
	int m_State; // current state, see above
	int m_SpoofRemoteID; // our id at teh zervor
	char m_aLastMessage[256]; // the last message from zervor
	int64 m_LastMessageTime; // when teh l4st
	char m_aLastCommand[256]; // last message we've sent


	void Reset();

	void ParseZervorMessage(const char *pMessage);
	static void StartConnection(void *pUserData);
	static void Listener(void *pUserData);
	static void Worker(void *pUserData);

	static void ConConnect(IConsole::IResult *pResult, void *pUserData);
	static void ConDisconnect(IConsole::IResult *pResult, void *pUserData);
	static void ConForceClose(IConsole::IResult *pResult, void *pUserData);
	static void ConCommand(IConsole::IResult *pResult, void *pUserData);

public:
	CSpoofRemote();
	~CSpoofRemote();

	inline bool IsConnected() const { return m_ConnState == CONNSTATE_CONNECTED; }
	inline int IsConnState(int State) const { return m_ConnState == State; }
	inline int IsSpfState(int State) const { return m_State&State; }
	inline const char *LastMessage() const { return m_aLastMessage; }
	inline int64 LastMessageTime() const { return m_LastMessageTime; }
	inline const char *LastCommand() const { return m_aLastCommand; }

	inline void VotekickAll() { m_State |= STATE_VOTEKICKALL; }

	void Connect() { thread_init(StartConnection, this); };
	void Disconnect();

	void OnConsoleInit();
	void OnInit();
	void OnRender();
	void SendCommand(const char *pCommand);
};

#endif
