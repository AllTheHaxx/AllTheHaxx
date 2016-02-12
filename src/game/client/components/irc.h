#ifndef GAME_CLIENT_COMPONENTS_IRC_H
#define GAME_CLIENT_COMPONENTS_IRC_H

#include <engine/external/cpirc/IRC.h>
#include <game/client/component.h>

class CIRC : public CComponent
{
private:
	IRC m_Connection;

	void *m_pIRCThread;
	static void ListenIRCThread(void *pUser);

public:
	CIRC();

	void SendChat(const char *pMsg);
	void SendRaw(const char *pMsg);

	void SendNickChange(const char *pNewNick);
	void AddLine(const char *pNick, const char *pLine);

	char *CurrentNick() { return m_Connection.current_nick(); }
	bool IsConnected() { return m_Connection.is_connected(); }

	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();

};
#endif
