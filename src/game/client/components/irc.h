#ifndef GAME_CLIENT_COMPONENTS_IRC_H
#define GAME_CLIENT_COMPONENTS_IRC_H

#include <stdlib.h>
#include <string>

#include <base/tl/sorted_array.h>

#include <game/client/component.h>

class CIRC : public CComponent
{
public:
	struct IRCUser
	{
		std::string m_User;
		std::string m_Domain;
		std::string m_Server;
		std::string m_Nick;
		std::string m_Modes;
		//std::string pNoIdeaHowToNameIt; // what is this? (:3 or :0)
		std::string m_Realname;

		bool operator <(const CIRC::IRCUser& other) { return m_User[0] < other.m_User[0]; }
	};

	enum
	{
		IRC_LINETYPE_CHAT=0,
		IRC_LINETYPE_NOTICE,
		//IRC_LINETYPE_SYSTEM, // this one not (yet)
	};

private:

	void *m_pIRCThread;
	static void ListenIRCThread(void *pUser);

public:
	CIRC();

	void SendChat(const char *pMsg);
	void SendRaw(const char *pMsg);

	void Connect();
	void Disconnect(char *pReason);

	void SendRequestUserList();
	void SendNickChange(const char *pNewNick);
	void AddLine(int Type, const char *pNick, const char *pLine); // chat
	void AddLine(const char *pLine); // system

	char *CurrentNick() { return (char*)"noname"; } // TODO
	bool IsConnected() { return false; } // TODO

	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnShutdown();

	sorted_array<IRCUser> m_UserList;

};
#endif
