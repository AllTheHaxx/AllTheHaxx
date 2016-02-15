/* (c) unsigned char*. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at https://github.com/CytraL/HClient */
#ifndef ENGINE_CLIENT_IRC_H
#define ENGINE_CLIENT_IRC_H

#include <base/system.h>
#include <base/tl/array.h>
#include <engine/irc.h>
#include <list>
#include <string>

class CIrc : public IIrc
{
	struct IrcHook
	{
		std::string messageID;
		int (*function)(IIrc::ReplyData*, void*);
		void* user;
	};

public:
    CIrc();

    void Init();

    virtual void RegisterCallback(const char* pMsgID, int (*func)(ReplyData*, void*), void *pUser); // pData, pUser

    int GetState() { return m_State; }
    void NextRoom();
    void PrevRoom();

    void SetActiveCom(int index);
	void SetActiveCom(CIrcCom *pCom);
    CIrcCom* GetActiveCom();
    CIrcCom* GetCom(size_t index);
    CIrcCom* GetCom(std::string name);
    int GetNumComs() { return m_IrcComs.size(); }
    bool CanCloseCom(CIrcCom *pCom);

    void OpenQuery(const char *to);
    void JoinTo(const char *to, const char *pass = "");
    void SetTopic(const char *topic);
    void Part(const char *pReason = 0, CIrcCom *pCom = 0);

    void SetMode(const char *mode, const char *to);
    void SetNick(const char *nick);
    const char* GetNick() { return m_Nick.c_str(); }
    int NumUnreadMessages(int *pArray = 0);

    void SendMsg(const char *to, const char *msg, int type = MSG_TYPE_NORMAL);
    void SendRaw(const char *fmt, ...);
    void SendGetServer(const char *to);

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
    int m_ActiveCom;
    NETSOCKET m_Socket;
    NETADDR m_HostAddress;

    char m_CmdToken[12];

    std::list<CIrcCom*> m_IrcComs;
    array<IrcHook> m_Hooks;

private:
    void CallHooks(const char* pMsgID, ReplyData* pReplyData);

    int GetMsgType(const char *msg);
    void SendServer(const char *to, const char *Token);
};
#endif
