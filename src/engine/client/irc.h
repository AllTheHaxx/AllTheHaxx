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
	CIRCCom* GetCom(const std::string& Name);
	void CloseCom(unsigned index);
	void CloseCom(CIRCCom *pCom);
	unsigned GetNumComs() { return (unsigned)m_apIRCComs.size(); }
	bool CanCloseCom(CIRCCom *pCom);

	CIRCCom* OpenCom(int Type, const char *pName, bool SwitchTo = true, int UnreadMessages = 0);
	CComQuery* OpenComQuery(const char *pName, bool SwitchTo = true, int UnreadMessages = 0) { return static_cast<CComQuery *>(OpenCom(CIRCCom::TYPE_QUERY, pName, SwitchTo, UnreadMessages)); }
	CComChan* OpenComChan(const char *pName, bool SwitchTo = true, int UnreadMessages = 0) { return static_cast<CComChan *>(OpenCom(CIRCCom::TYPE_CHANNEL, pName, SwitchTo, UnreadMessages)); }
	void OpenQuery(const char *to);
	void JoinTo(const char *to, const char *pass = "");
	void SetTopic(const char *topic);
	void Part(const char *pReason = 0, CIRCCom *pCom = 0);

	void SetMode(const char *mode, const char *to);
	void SetNick(const char *pNick);
	const char* GetNick() { return m_Nick.c_str(); }
	const std::string &GetNickStd() const { return m_Nick; }
	luabridge::LuaRef LuaGetUserlist(const char *pChannel, lua_State *L);
	int NumUnreadMessages(int *pArray = 0);
	int GetMsgType(const char *msg);

	void SendMsg(const char *to, const char *msg, int type = MSG_TYPE_NORMAL);
	void SendMsgLua(const char *to, const char *msg);
	void SendRaw(const char *fmt, ...)
	#if defined(DO_NOT_COMPILE_THIS_CODE) && (defined(__GNUC__) || defined(__clang__))
	__attribute__ ((format (printf, 1, 2))) /* Warn if you specify wrong arguments in printf format string */
	#endif
	;

	void SendGetServer(const char *to);
	void SendVersion(const char *to);

	void StartConnection();
	void Disconnect(const char *pReason = 0);

	void SetAway(bool state, const char *msg = 0x0);

	void ExecuteCommand(const char *cmd, char *params);


protected:
	class IGraphics *m_pGraphics;
	class IGameClient *m_pGameClient;
	class IClient *m_pClient;

	int m_State;
	unsigned m_ActiveCom;
	NETSOCKET m_Socket;
	NETADDR m_HostAddress;

	std::string m_Nick;
	bool m_IsAway;
	char m_CmdToken[12];

	array<CIRCCom*> m_apIRCComs;
	array<IRCHook> m_Hooks;

private:
	void CallHooks(const char* pMsgID, ReplyData* pReplyData);

	void SendServer(const char *to, const char *Token);
};

#endif

