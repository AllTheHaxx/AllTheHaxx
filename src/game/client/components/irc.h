#ifndef GAME_CLIENT_COMPONENTS_IRC_H
#define GAME_CLIENT_COMPONENTS_IRC_H

#include <engine/client/irc.h>
#include <game/client/component.h>

class CIRCBind : public CComponent
{
private:

	void *m_pIRCThread;
	static void ListenIRCThread(void *pUser);

public:
	CIRCBind();

	void Connect();

	void SendCommand(const char *pCmd);
	void OnNickChange(const char *pNewNick);

	const char *CurrentNick() { return m_pClient->IRC()->GetNick(); } // XXX this is depreciated and only for compatibility
	bool IsConnected() { return (m_pClient->IRC()->GetState() != IIRC::STATE_DISCONNECTED); }

	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnShutdown();

};
#endif
