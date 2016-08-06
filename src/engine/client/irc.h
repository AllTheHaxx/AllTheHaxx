/* (c) unsigned char*. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at https://github.com/CytraL/HClient */
#ifndef ENGINE_CLIENT_IRC_H
#define ENGINE_CLIENT_IRC_H

#include <base/system.h>
#include <base/tl/array.h>
#include <base/tl/sorted_array.h>
#include <engine/irc.h>
#include <list>
#include <string>

class CIRC : public IIRC
{
	struct IRCHook
	{
		std::string messageID;
		int (*function)(IIRC::ReplyData*, void*, void*);
		void* user;
	};

	int64 m_StartTime;
	bool m_Debug;

public:
	CIRC();

	void Init();

	virtual void RegisterCallback(const char* pMsgID, int (*func)(ReplyData*, void*, void*), void *pUser); // pData, pUser, this

	int GetState() { return m_State; }
	void NextRoom();
	void PrevRoom();

	void SetActiveCom(unsigned index);
	void SetActiveCom(CIRCCom *pCom);
	CIRCCom* GetActiveCom();
	CIRCCom* GetCom(unsigned index);
	CIRCCom* GetCom(std::string name);
	void CloseCom(unsigned index);
	void CloseCom(CIRCCom *pCom);
	unsigned GetNumComs() { return (unsigned)m_apIRCComs.size(); }
	bool CanCloseCom(CIRCCom *pCom);

	template<class TCOM>
	TCOM* OpenCom(const char *pName, bool SwitchTo = true, int UnreadMessages = 0);
	void OpenQuery(const char *to);
	void JoinTo(const char *to, const char *pass = "");
	void SetTopic(const char *topic);
	void Part(const char *pReason = 0, CIRCCom *pCom = 0);

	void SetMode(const char *mode, const char *to);
	void SetNick(const char *nick);
	const char* GetNick() { return m_Nick.c_str(); }
	int NumUnreadMessages(int *pArray = 0);
	int GetMsgType(const char *msg);

	void SendMsg(const char *to, const char *msg, int type = MSG_TYPE_NORMAL);
	void SendRaw(const char *fmt, ...);
	void SendGetServer(const char *to);
	void SendVersion(const char *to);

	void StartConnection();
	void Disconnect(const char *pReason = 0);

	void SetAway(bool state, const char *msg = 0x0);

	void ExecuteCommand(const char *cmd, char *params);

	std::string m_Nick;

protected:
	class IGraphics *m_pGraphics;
	class IGameClient *m_pGameClient;
	class IClient *m_pClient;

	int m_State;
	unsigned m_ActiveCom;
	NETSOCKET m_Socket;
	NETADDR m_HostAddress;

	char m_CmdToken[12];

	array<CIRCCom*> m_apIRCComs;
	array<IRCHook> m_Hooks;

private:
	void CallHooks(const char* pMsgID, ReplyData* pReplyData);

	void SendServer(const char *to, const char *Token);
};

#endif

