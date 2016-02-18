#ifndef GAME_CLIENT_COMPONENTS_IRC_H
#define GAME_CLIENT_COMPONENTS_IRC_H

#include <stdlib.h>
#include <string>

#include <base/tl/sorted_array.h>
#include <engine/client/irc.h>
#include <game/client/component.h>

class CIrcBind : public CComponent
{
private:

	void *m_pIRCThread;
	static void ListenIRCThread(void *pUser);

public:
	CIrcBind();

	void SendChat(const char *pMsg);
	void SendCommand(const char *pCmd);

	void Connect();
	void Disconnect(char *pReason);

	void SendNickChange(const char *pNewNick);
	void AddLine(int Type, const char *pNick, const char *pLine); // chat
	void AddLine(const char *pLine); // system

	const char *CurrentNick() { return m_pClient->Irc()->GetNick(); } // XXX this is depreciated and only for compatibility
	bool IsConnected() { return m_pClient->Irc()->GetState() == IIrc::STATE_CONNECTED; }

	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnShutdown();

};
#endif
