#ifndef GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H
#define GAME_CLIENT_COMPONENTS_SPOOFREMOTE_H

#include <base/system.h>
#if defined(CONF_FAMILY_UNIX)
#include <netinet/in.h>
#include <game/client/component.h>

class CSpoofRemote : public CComponent
{
public:
	enum
		{
			SPOOF_STATE_CONNECTING  = 1 << 0,
			SPOOF_STATE_CONNECTED   = 1 << 1,
			SPOOF_STATE_DUMMIES     = 1 << 2,
			SPOOF_STATE_DUMMYSPAM   = 1 << 3,
			SPOOF_STATE_VOTEKICKALL = 1 << 4,
		};

private:

	// engine variables
	void *m_pListenerThread;
	void *m_pWorkerThread;
	time_t m_LastAck;
	int m_Socket;
	struct sockaddr_in m_Info;

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

	inline void VotekickAll() { m_State |= SPOOF_STATE_VOTEKICKALL; }

	void Connect(const char *pAddr, int Port);
	void Disconnect();

	void OnConsoleInit();
	void OnInit();
	void OnRender();
	void SendCommand(const char *pCommand);
};

#endif
#endif
