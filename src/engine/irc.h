/* (c) unsigned char*. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at https://github.com/CytraL/HClient */
#ifndef ENGINE_IRC_H
#define ENGINE_IRC_H

#include "kernel.h"
#include <vector>
#include <string>
#include <list>
#include <base/tl/sorted_array.h>

class CIRCCom
{
	MACRO_ALLOC_HEAP();

	unsigned int m_Type;
public:
	enum
	{
		TYPE_CHANNEL = 0,
		TYPE_QUERY
	};

	CIRCCom(unsigned int type)
	{
		m_Type = type;
		m_NumUnreadMsg = 0;
	}

	std::vector<std::string> m_Buffer;
	int m_NumUnreadMsg;
	char m_aName[25]; // channel/user

	unsigned int GetType() const { return m_Type; }

	void AddMessage(const char *fmt, ...);
	void AddMessage_nofmt(const char *msg);
};


class CComChan : public CIRCCom
{
public:
	struct CUser
	{
		enum
		{
			LEVEL_USER=0,
			LEVEL_VOICE=1 << 0,
			LEVEL_ADMIN=1 << 1,
		};

		int m_Level;
		std::string m_Nick;

		CUser(){}

		CUser(const std::string& Nick) : m_Nick(Nick)
		{
			m_Level = 0;
		}

	/*	CUser(const CUser& other)
		{
			m_Level = other.m_Level;
			m_Nick = other.m_Nick;
		}*/

		bool IsAdmin() const { return (m_Level&LEVEL_ADMIN) != 0; }
		bool IsVoice() const { return (m_Level&LEVEL_VOICE) != 0; }

		const char *c_str() const { return m_Nick.c_str(); }

		bool operator<(const CUser& other)
		{
			if(m_Level != other.m_Level)
				return m_Level > other.m_Level;
			return m_Nick < other.m_Nick;
		}
	};

	CComChan() : CIRCCom(CIRCCom::TYPE_CHANNEL) { }

	sorted_array<CUser> m_Users;
	std::string m_Topic;
	const char *Channel() const { return m_aName; }

	void RemoveUserFromList(const char *pName)
	{
		for(int u = 0; u < m_Users.size(); u++)
			if(str_comp(m_Users[u].c_str(), pName) == 0)
			{
				m_Users.remove_index(u);
				break;
			}
	}

	CUser* GetUser(const std::string& Nick)
	{
		for(int u = 0; u < m_Users.size(); u++)
			if(m_Users[u].m_Nick == Nick)
				return &(m_Users[u]);
		return 0;
	}
};


class CComQuery : public CIRCCom
{
public:
	CComQuery() : CIRCCom(CIRCCom::TYPE_QUERY) { }
	const char *User() const { return m_aName; }
};


class IIRC : public IInterface
{
	MACRO_INTERFACE("IRC", 0)
public:
	struct ReplyData
	{
		std::string channel;
		std::string from;
		std::string to;
		std::string params;
	};

    enum
    {
        STATE_DISCONNECTED=0,
		STATE_CONNECTING,
        STATE_CONNECTED,
        STATE_AWAY,

		// for CTCP messages
		MSG_TYPE_NORMAL = 0,
		MSG_TYPE_ACTION,
		MSG_TYPE_TWSERVER,
		MSG_TYPE_GET_TWSERVER,
		MSG_TYPE_CTCP // any unknown ctcp
    };


    virtual void Init() = 0;

    virtual void RegisterCallback(const char* pMsgID, int (*func)(ReplyData*, void*, void*), void *pUser) = 0; // pData, pUser
    virtual void CallHooks(const char* pMsgID, ReplyData* pReplyData) = 0;

    virtual int GetState() = 0;
    virtual void NextRoom() = 0;
    virtual void PrevRoom() = 0;

    virtual void SetActiveCom(unsigned index) = 0;
	virtual void SetActiveCom(CIRCCom *pCom) = 0;
    virtual CIRCCom* GetActiveCom() = 0;
    virtual CIRCCom* GetCom(unsigned index) = 0;
    virtual CIRCCom* GetCom(std::string name) = 0;
	virtual void CloseCom(unsigned index) = 0;
	virtual void CloseCom(CIRCCom *pCom) = 0;
    virtual unsigned GetNumComs() = 0;
    virtual bool CanCloseCom(CIRCCom *pCom) = 0;

    virtual void OpenQuery(const char *to) = 0;
    virtual void JoinTo(const char *to, const char *pass = "") = 0;
    virtual void SetTopic(const char *topic) = 0;
    virtual void Part(const char *pReason = 0, CIRCCom *pCom = 0) = 0;

    virtual void SetMode(const char *mode, const char *to) = 0;
    virtual void SetNick(const char *nick) = 0;
    virtual const char* GetNick() = 0;
    virtual int NumUnreadMessages(int *pArray = 0) = 0;
    virtual int GetMsgType(const char *msg) = 0;

    virtual void SendMsg(const char *to, const char *msg, int type = MSG_TYPE_NORMAL) = 0;
    virtual void SendRaw(const char *fmt, ...) = 0;
    virtual void SendGetServer(const char *to) = 0;
    virtual void SendVersion(const char *to) = 0;

    virtual void StartConnection() = 0;
    virtual void Disconnect(const char *pReason = 0) = 0;

    virtual void SetAway(bool state, const char *msg = 0x0) = 0;

    virtual void ExecuteCommand(const char *cmd, char *params) = 0;
};

#endif
