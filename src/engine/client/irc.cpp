/* (c) unsigned char*. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at https://github.com/CytraL/HClient */
#include <base/math.h>
#include <base/system.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <game/version.h>
#include <game/client/components/menus.h>

#if defined(CONF_FAMILY_UNIX)
    #include <unistd.h>
#elif defined(CONF_FAMILY_WINDOWS)
    #include <windows.h>
#endif

#include <cstring>
#include <ctime>
#include <cstdio> // vsnprintf
#include <cstdarg>
#include <algorithm>

#include "irc.h"

static NETSOCKET invalid_socket = {NETTYPE_INVALID, -1, -1};

CIrc::CIrc()
{
    m_IrcComs.clear();
    m_Hooks.clear();
    m_pGraphics = 0x0;
    m_State = STATE_DISCONNECTED;
    m_Socket = invalid_socket;
    char tmpNick[25]={0};
    str_format(tmpNick, sizeof(tmpNick), "HC-%d", time(NULL));
    m_Nick = tmpNick;
    mem_zero(m_CmdToken, sizeof(m_CmdToken));
    SetActiveCom(-1);
}

void CIrc::RegisterCallback(const char* pMsgID, int (*func)(ReplyData*, void*), void *pUser) // pData, pUser
{
	IrcHook h;
	h.messageID = pMsgID;
	h.function = func;
	h.user = pUser;
	m_Hooks.add(h);
	dbg_msg("engine/irc", "registered callback for '%s'", pMsgID);
}

void CIrc::CallHooks(const char *pMsgID, ReplyData* pReplyData)
{
	for(int i = 0; i < m_Hooks.size(); i++)
	{
		if(m_Hooks[i].messageID == std::string(pMsgID))
			(*(m_Hooks[i].function))(pReplyData, m_Hooks[i].user);
	}
}

void CIrc::Init()
{
    m_pGraphics = Kernel()->RequestInterface<IGraphics>();
    m_pGameClient = Kernel()->RequestInterface<IGameClient>();
    m_pClient = Kernel()->RequestInterface<IClient>();
}

void CIrc::SetActiveCom(int index)
{
    if (index < 0 || index >= (int)m_IrcComs.size())
        index = 0;

    m_ActiveCom = index;
    CIrcCom *pCom = GetCom(index);
    if (pCom)
        pCom->m_NumUnreadMsg = 0;
}

void CIrc::SetActiveCom(CIrcCom *pCom)
{
    if (!pCom)
		return;
	
	std::list<CIrcCom*>::iterator it = std::find(m_IrcComs.begin(), m_IrcComs.end(), pCom);
	if (it != m_IrcComs.end())
	{
        pCom->m_NumUnreadMsg = 0;
		m_ActiveCom = std::distance(m_IrcComs.begin(), it);
	}
}

CIrcCom* CIrc::GetActiveCom()
{
    if (m_ActiveCom < 0 || m_ActiveCom >= (int)m_IrcComs.size())
        return 0x0;

    std::list<CIrcCom*>::iterator it = m_IrcComs.begin();
    std::advance(it, m_ActiveCom);
    return (*it);
}

CIrcCom* CIrc::GetCom(size_t index)
{
    if (index < 0 || index >= m_IrcComs.size())
        return 0x0;

    std::list<CIrcCom*>::iterator it = m_IrcComs.begin();
    std::advance(it, index);
    return (*it);
}
CIrcCom* CIrc::GetCom(std::string name)
{
    std::list<CIrcCom*>::iterator it = m_IrcComs.begin();

    while (it != m_IrcComs.end())
    {
        if ((*it)->GetType() == CIrcCom::TYPE_CHANNEL)
        {
            CComChan *pChan = static_cast<CComChan*>((*it));
            if (str_comp_nocase(name.c_str(), pChan->m_Channel) == 0)
                return (*it);
        }
        else if ((*it)->GetType() == CIrcCom::TYPE_QUERY)
        {
            CComQuery *pQuery = static_cast<CComQuery*>((*it));
            if (str_comp_nocase(name.c_str(), pQuery->m_User) == 0)
                return (*it);
        }

        ++it;
    }

    return 0x0;
}

bool CIrc::CanCloseCom(CIrcCom *pCom)
{
	if(!pCom)
		return false;

	if(GetNumComs() <= 2 || str_comp_nocase(((CComChan*)pCom)->m_Channel, "#AllTheHaxx") == 0 ||
			str_comp_nocase(((CComQuery*)pCom)->m_User, "@status") == 0)
		return false;

	return true;
}

void CIrc::StartConnection() // call this from a thread only!
{
	NETADDR BindAddr;
    mem_zero(&m_HostAddress, sizeof(m_HostAddress));
    mem_zero(&BindAddr, sizeof(BindAddr));
    char aNetBuff[2048];

    m_State = STATE_CONNECTING;
    // lookup
	if(net_host_lookup("irc.quakenet.org:6667", &m_HostAddress, NETTYPE_IPV4) != 0)
	{
        dbg_msg("IRC","ERROR: Can't lookup irc.quakenet.org");
        m_State = STATE_DISCONNECTED;
        return;
	}
	m_HostAddress.port = 6667;

    // connect
    BindAddr.type = NETTYPE_IPV4;
    m_Socket = net_tcp_create(BindAddr);
	if(net_tcp_connect(m_Socket, &m_HostAddress) != 0)
	{
	    net_tcp_close(m_Socket);
	    char aBuf[64];
	    net_addr_str(&m_HostAddress, aBuf, sizeof(aBuf), 0);
	    dbg_msg("IRC","ERROR: Can't connect to '%s:%d'...", aBuf, m_HostAddress.port);
	    m_State = STATE_DISCONNECTED;
		return;
	}

	// send request
	SendRaw("CAP LS");
	SendRaw("NICK %s", m_Nick.c_str());
	SendRaw("USER %s 0 * :%s", g_Config.m_ClIRCUser, g_Config.m_ClIRCRealname); // TODO: we have config vars for this

    // status Tab
    CComQuery *pStatus = new CComQuery();
    str_copy(pStatus->m_User, "@Status", sizeof(pStatus->m_User));
    m_IrcComs.push_back(pStatus);
    SetActiveCom(-1);

    m_State = STATE_CONNECTED;

    std::string NetData;
    //int TotalRecv = 0;
    //int TotalBytes = 0;
    int CurrentRecv = 0;
    char LastPong[255]={0};
    while ((CurrentRecv = net_tcp_recv(m_Socket, aNetBuff, sizeof(aNetBuff))) >= 0 && m_State == STATE_CONNECTED)
    {
    	ReplyData reply; char aTime[32];
    	time_t rawtime;
		struct tm *timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		str_format(aTime, sizeof(aTime), "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);


        for (int i=0; i < CurrentRecv; i++)
        {
            if (aNetBuff[i]=='\r' || aNetBuff[i]=='\t')
                 continue;

            if (aNetBuff[i]=='\n')
            {
                size_t del = NetData.find_first_of(":");
                size_t ldel = 0;
                if (del > 0)
                { //NeT Message
                    std::string aMsgID = NetData.substr(0, del-1);
                    std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);
                    if (aMsgID.compare("PING") == 0)
                    {
                        /*char aBuff[255];
                        CIrcCom *pCom = GetCom("@Status");
                        str_format(aBuff, sizeof(aBuff), "PING [%s]", aMsgText.c_str());
                        pCom->m_Buffer.push_back(aBuff);*/
                    	SendRaw("PONG %s :%s", LastPong, aMsgText.c_str());
                    	dbg_msg("engine/irc", "Ping? Pong!");
                        LastPong[0]=0;
                    }
                    else if (aMsgID.compare("PONG") == 0)
                        str_copy(LastPong, aMsgText.c_str(), sizeof(LastPong));
                    else
                    {
                        CIrcCom *pCom = GetCom("@Status");
                        pCom->m_Buffer.push_back(aMsgText);
                    	reply.channel = "@Status";
                        reply.from = "quakenet.org";
                    	reply.params = aMsgText;
                    }
                } else
                { //raw message
                    del = NetData.find_first_of(" ");
                    std::string aMsgFServer = NetData.substr(1, del);
                    ldel = del;
                    del = NetData.find_first_of(" ",del+1);
                    std::string aMsgID = NetData.substr(ldel+1, del-ldel-1);

                    //dbg_msg("IRC", "Raw MSG [%s]: %s",aMsgID.c_str(), aMsgFServer.c_str());
                    //dbg_msg("IRC-RAW", NetData.c_str());

                    if (aMsgID.compare("001") == 0)
                    {
                    	// set the user's wanted modes
                    	SetMode(g_Config.m_ClIRCModes, m_Nick.c_str());

                    	// send Q auth
						if(g_Config.m_ClIRCQAuthName[0] != '\0' && g_Config.m_ClIRCQAuthPass[0] != '\0')
							SendRaw("PRIVMSG Q@CServe.quakenet.org :AUTH %s %s", g_Config.m_ClIRCQAuthName, g_Config.m_ClIRCQAuthPass);

                        // auto-join
                        JoinTo("#AllTheHaxx");

                        reply.channel = "@Status";
                        reply.from = "quakenet.org";
                    }
                    else if (aMsgID.compare("332") == 0) // topic
                    {
                        del = NetData.find_first_of(" ",del+1);
                        ldel = NetData.find_first_of(" ",del+1);
                        std::string aMsgChan = NetData.substr(del+1,ldel-del-1);

                        del = NetData.find_first_of(":", 1);
                        std::string aMsgTopic = NetData.substr(del+1, NetData.length()-del-1);

                        CComChan *pChan = static_cast<CComChan*>(GetCom(aMsgChan));
                        if (pChan)
                            pChan->m_Topic = aMsgTopic;

                        reply.channel = aMsgChan;
                        reply.params = aMsgTopic;
                    }
                    else if (aMsgID.compare("353") == 0) // NAMREPLY
                    {
                        del = NetData.find_first_of("=");
                        ldel = NetData.find_first_of(" ",del+2);

                        std::string aMsgChan = NetData.substr(del+2, ldel-del-2);
                        del = NetData.find_first_of(":",1);
                        std::string aMsgUsers = NetData.substr(del+1, NetData.length()-del-1);

                        CComChan *pChan = static_cast<CComChan*>(GetCom(aMsgChan));
                        if (pChan)
                        {
                            size_t del=0, ldel=0;
                            do{
                                del = aMsgUsers.find_first_of(" ",del+1);
                                pChan->m_Users.push_back(aMsgUsers.substr(ldel, del-ldel));
                                ldel=del+1;
                            } while (del != std::string::npos);
                        }
                        reply.channel = aMsgChan;
                        reply.params = aMsgUsers;
                    }
                    else if (aMsgID.compare("366") == 0) // ENDOFNAMES
                    {
                        del = NetData.find_first_of(" ",del+1);
                        ldel = NetData.find_first_of(" ",del+1);
                        std::string aMsgChan = NetData.substr(del+1,ldel-del-1);

                        CComChan *pChan = static_cast<CComChan*>(GetCom(aMsgChan));
                        if (pChan)
                            pChan->m_Users.sort();

                        reply.channel = aMsgChan;

                    }
                    else if (aMsgID.compare("401") == 0) // NOSUCHNICK
                    {
                        char aBuff[255];
                        del = NetData.find_first_of(" ",del+1);
                        ldel = NetData.find_first_of(" ",del+1);
                        std::string aMsgFrom = NetData.substr(del+1,ldel-del-1);
                        del = NetData.find_first_of(":",1);
                        std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);

                        CIrcCom *pCom = GetCom(aMsgFrom);
                        if (!pCom)
                            pCom = GetCom("@Status");

                        str_format(aBuff, sizeof(aBuff), "*** '%s' %s", aMsgFrom.c_str(), aMsgText.c_str());
                        pCom->m_Buffer.push_back(aBuff);

                        reply.from = aMsgFrom;
                        reply.params = aMsgText;

                    }
                    else if (aMsgID.compare("421") == 0) // UNKNOWNCOMMAND
                    {
                        char aBuff[255];
                        del = NetData.find_first_of(" ",del+1);
                        ldel = NetData.find_first_of(" ",del+1);
                        std::string aMsgCmd = NetData.substr(del+1,ldel-del-1);
                        del = NetData.find_first_of(":",1);
                        std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);

                        CIrcCom *pCom = GetCom("@Status");
                        str_format(aBuff, sizeof(aBuff), "'%s' %s", aMsgCmd.c_str(), aMsgText.c_str());
                        pCom->m_Buffer.push_back(aBuff);

                        reply.from = aMsgCmd;
                        reply.params = aMsgText;

                    }
                    else if (aMsgID.compare("JOIN") == 0)
                    {
                        std::string aMsgChannel = NetData.substr(del+1, NetData.length()-del-1);
                        del = aMsgFServer.find_first_of("!");
                        std::string aMsgFrom = aMsgFServer.substr(0,del);

                        if (aMsgFrom == m_Nick)
                        {
                            CComChan *pNewChan = new CComChan();
                            pNewChan->m_NumUnreadMsg = 0;
                            str_copy(pNewChan->m_Channel, aMsgChannel.c_str(), sizeof(pNewChan->m_Channel));
                            m_IrcComs.push_back(pNewChan);
                            SetActiveCom(m_IrcComs.size()-1);
                        }
                        else
                        {
                            CComChan *pChan = static_cast<CComChan*>(GetCom(aMsgChannel));
                            if (pChan)
                            {
                                pChan->m_Users.push_back(aMsgFrom);
                                pChan->m_Users.sort();
                                char aBuff[255];
                                str_format(aBuff, sizeof(aBuff), "%s*** '%s' has joined %s", aTime, aMsgFrom.c_str(), aMsgChannel.c_str());
                                pChan->m_Buffer.push_back(aBuff);
                            }
                        }

                        reply.channel = aMsgChannel;
                        reply.from = aMsgFrom;
                    }
                    else if (aMsgID.compare("PART") == 0)
                    {
                        std::string aMsgChannel = NetData.substr(del+1, NetData.length()-del-1);
                        del = aMsgFServer.find_first_of("!");
                        std::string aMsgFrom = aMsgFServer.substr(0,del);
                        char aBuff[255];

                        if (aMsgFrom == m_Nick)
                        {
                            CIrcCom *pCom = GetCom(aMsgChannel);
                            if (pCom)
                            {
                                m_IrcComs.remove(pCom);
                                delete pCom;
                                pCom=0x0;
                                SetActiveCom(m_IrcComs.size()-1);
                            }
                        }
                        else
                        {
                            CComChan *pChan = static_cast<CComChan*>(GetCom(aMsgChannel));
                            if (pChan)
                            {
                                pChan->m_Users.remove(aMsgFrom);
                                str_format(aBuff, sizeof(aBuff), "@%s", aMsgFrom.c_str());
                                pChan->m_Users.remove(std::string(aBuff));
                                str_format(aBuff, sizeof(aBuff), "+%s", aMsgFrom.c_str());
                                pChan->m_Users.remove(std::string(aBuff));

                                str_format(aBuff, sizeof(aBuff), "%s*** '%s' part %s", aTime, aMsgFrom.c_str(), aMsgChannel.c_str());
                                pChan->m_Buffer.push_back(aBuff);
                            }
                        }

                       reply.channel = aMsgChannel;
					   reply.from = aMsgFrom;
                    }
                    else if (aMsgID.compare("QUIT") == 0)
                    {
                        del = NetData.find_first_of(":",1);
                        std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);
                        del = aMsgFServer.find_first_of("!");
                        std::string aMsgFrom = aMsgFServer.substr(0,del);
                        char aBuff[255];

                        if (aMsgFrom != m_Nick)
                        {
                            std::list<CIrcCom*>::iterator it = m_IrcComs.begin();
                            while (it != m_IrcComs.end())
                            {
                                if (!(*it) || (*it)->GetType() != CIrcCom::TYPE_CHANNEL)
                                {
                                    ++it;
                                    continue;
                                }

                                CComChan *pChan = static_cast<CComChan*>((*it));
                                if (!pChan)
                                {
                                    ++it;
                                    continue;
                                }

                                pChan->m_Users.remove(aMsgFrom);
                                str_format(aBuff, sizeof(aBuff), "@%s", aMsgFrom.c_str());
                                pChan->m_Users.remove(std::string(aBuff));
                                str_format(aBuff, sizeof(aBuff), "+%s", aMsgFrom.c_str());
                                pChan->m_Users.remove(std::string(aBuff));

                                str_format(aBuff, sizeof(aBuff), "%s*** '%s' quit (%s)", aTime, aMsgFrom.c_str(), aMsgText.c_str());
                                pChan->m_Buffer.push_back(aBuff);

                                ++it;
                            }
                        }

                        reply.channel = "IRC";
						reply.params = aMsgText;
						reply.from = aMsgFrom;
					}
                    else if (aMsgID.compare("TOPIC") == 0)
                    {
                        ldel = NetData.find_first_of(" ",del+1);
                        std::string aMsgChan = NetData.substr(del+1, ldel-del-1);
                        del = NetData.find_first_of(":",1);
                        std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);
                        del = aMsgFServer.find_first_of("!");
                        std::string aMsgFrom = aMsgFServer.substr(0,del);
                        char aBuff[1024];

                        CComChan *pChan = static_cast<CComChan*>(GetCom(aMsgChan));
                        if (pChan)
                        {
                            pChan->m_Topic = aMsgText;
                            str_format(aBuff, sizeof(aBuff), "%s*** '%s' has changed topic to '%s'", aTime, aMsgFrom.c_str(), aMsgText.c_str());
                            pChan->m_Buffer.push_back(aBuff);
						}

						reply.channel = aMsgChan;
						reply.from = aMsgFrom;
						reply.params = aMsgText;
                    }
                    else if (aMsgID.compare("PRIVMSG") == 0)
                    {
                        char aBuff[1024];
                        ldel = NetData.find_first_of(" ", del+1);
                        std::string aMsgChan = NetData.substr(del+1, ldel-del-1);

                        del = NetData.find_first_of(":", 1);
                        std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);
                        int MsgType = GetMsgType(aMsgText.c_str());

						del = aMsgFServer.find_first_of("!");
						std::string aMsgFrom = aMsgFServer.substr(0,del);

                        if (MsgType == MSG_TYPE_TWSERVER)
                        {
                        	if (aMsgChan == m_Nick)
                        	{
								del = aMsgText.find_first_of(" ");
								ldel = aMsgText.find_last_of(" ");
								if (del != std::string::npos && del != ldel)
								{
									char aAddr[32];
									mem_zero(aAddr, sizeof(aAddr));
									std::string CleanMsg = aMsgText.substr(10);
									CleanMsg = CleanMsg.substr(0, CleanMsg.length()-1);
									size_t pc = CleanMsg.find_first_of(" ");
									if (pc != std::string::npos)
									{
										str_copy(aAddr, CleanMsg.substr(pc+1).c_str(), sizeof(aAddr));
										if (m_CmdToken[0] != 0 && str_comp(CleanMsg.substr(0, pc).c_str(), m_CmdToken) == 0 && aAddr[0] != 0)
										{
											if (aAddr[0] != 0 && str_comp_nocase(aAddr, "NONE") != 0)
												m_pClient->Connect(aAddr);
											else
											{
												CIrcCom *pCom = GetActiveCom();
												if (pCom)
												{
													char aBuf[128];
													str_format(aBuf, sizeof(aBuf), "*** '%s' isn't playing on a server!", aMsgFrom.c_str());
													pCom->m_Buffer.push_back(aBuf);
												}
											}

											mem_zero(m_CmdToken, sizeof(m_CmdToken));
										}
									}
								}
                        	}
                        }
                        else if (MsgType == MSG_TYPE_GET_TWSERVER)
                        {
                        	if (aMsgChan == m_Nick)
                        	{
								std::string CleanMsg = aMsgText.substr(13);
								CleanMsg = CleanMsg.substr(0, CleanMsg.length()-1);

								if (!CleanMsg.empty())
									SendServer(aMsgFrom.c_str(), CleanMsg.c_str());
                        	}
                        }
                        else
                        {
							if (aMsgChan == m_Nick)
							{
								CIrcCom *pCom = GetCom(aMsgFrom);
								if (!pCom)
								{
									CComQuery *pNewQuery = new CComQuery();
									pNewQuery->m_NumUnreadMsg = 1;
									str_copy(pNewQuery->m_User, aMsgFrom.c_str(), sizeof(pNewQuery->m_User));
									m_IrcComs.push_back(pNewQuery);

									if (MsgType == MSG_TYPE_ACTION)
										str_format(aBuff, sizeof(aBuff), "%s*** %s: %s", aTime, aMsgFrom.c_str(), aMsgText.substr(8, -1).c_str());
									else
										str_format(aBuff, sizeof(aBuff), "%s<%s> %s", aTime, aMsgFrom.c_str(), aMsgText.c_str());
									pNewQuery->m_Buffer.push_back(aBuff);
								}
								else
								{
									if (pCom != GetActiveCom())
										pCom->m_NumUnreadMsg++;

									if (MsgType == MSG_TYPE_ACTION)
										str_format(aBuff, sizeof(aBuff), "%s*** %s: %s", aTime, aMsgFrom.c_str(), aMsgText.substr(8, -1).c_str());
									else
										str_format(aBuff, sizeof(aBuff), "%s<%s> %s", aTime, aMsgFrom.c_str(), aMsgText.c_str());
									pCom->m_Buffer.push_back(aBuff);
								}

								if (pCom == GetActiveCom())
								{
									aMsgFrom.insert(0,"<"); aMsgFrom.append("> ");
									aMsgFrom.insert(0, aTime);
									m_pGameClient->OnMessageIrc("", aMsgFrom.c_str(), aMsgText.c_str());
								}
							}
							else
							{
								CIrcCom *pCom = GetCom(aMsgChan);
								if (pCom)
								{
									if (pCom != GetActiveCom())
										pCom->m_NumUnreadMsg++;

									if (MsgType == MSG_TYPE_ACTION)
										str_format(aBuff, sizeof(aBuff), "%s*** %s: %s", aTime, aMsgFrom.c_str(), aMsgText.substr(8, -1).c_str());
									else
										str_format(aBuff, sizeof(aBuff), "%s<%s> %s", aTime, aMsgFrom.c_str(), aMsgText.c_str());
									pCom->m_Buffer.push_back(aBuff);
								}

								if (pCom == GetActiveCom())
								{
									aMsgChan.insert(0, "["); aMsgChan.append("] ");
									aMsgFrom.insert(0, "<"); aMsgFrom.append("> ");
									aMsgFrom.insert(0, aTime);
									m_pGameClient->OnMessageIrc(aMsgChan.c_str(), aMsgFrom.c_str(), aMsgText.c_str());
								}
							}
                        }

                        reply.channel = aMsgChan;
                        reply.from = aMsgFrom;
                        reply.params = aMsgText;

                    }
                    else if (aMsgID.compare("NICK") == 0)
                    {
                        char aBuff[255];
                        del = aMsgFServer.find_first_of("!");
                        std::string aMsgOldNick = aMsgFServer.substr(0,del);

                        del = NetData.find_first_of(":", 1);
                        std::string aMsgNewNick = NetData.substr(del+1, NetData.length()-del-1);

                        if (aMsgOldNick == m_Nick)
                            m_Nick = aMsgNewNick;

                        std::list<CIrcCom*>::iterator it = m_IrcComs.begin();
                        while (it != m_IrcComs.end())
                        {
                            if ((*it)->GetType() == CIrcCom::TYPE_QUERY)
                            {
                                CComQuery *pQuery = static_cast<CComQuery*>((*it));
                                if (str_comp_nocase(pQuery->m_User, aMsgOldNick.c_str()) == 0)
                                {
                                	str_copy(pQuery->m_User, aMsgNewNick.c_str(), sizeof(pQuery->m_User));
                                    str_format(aBuff, sizeof(aBuff), "*** '%s' has changed nick to '%s'", aMsgOldNick.c_str(), aMsgNewNick.c_str());
                                    pQuery->m_Buffer.push_back(aBuff);
                                }
                            }
                            else if ((*it)->GetType() == CIrcCom::TYPE_CHANNEL)
                            { //TODO: Rewrite this!! duplicate code :P
                                CComChan *pChan = static_cast<CComChan*>((*it));
                                std::list<std::string>::iterator itU = pChan->m_Users.begin();
                                while (itU != pChan->m_Users.end())
                                {
                                    std::string NickOper = aMsgOldNick; NickOper.insert(0, "@");
                                    std::string NickVoice = aMsgOldNick; NickVoice.insert(0, "+");

                                    if (str_comp_nocase((*itU).c_str(), aMsgOldNick.c_str()) == 0)
                                    {
                                        (*itU) = aMsgNewNick;
                                        str_format(aBuff, sizeof(aBuff), "*** '%s' has changed nick to '%s'", aMsgOldNick.c_str(), aMsgNewNick.c_str());
                                        pChan->m_Buffer.push_back(aBuff);
                                        pChan->m_Users.sort();
                                        break;
                                    }
                                    else if (str_comp_nocase((*itU).c_str(), NickOper.c_str()) == 0)
                                    {
                                        (*itU) = aMsgNewNick;
                                        (*itU).insert(0, "@");
                                        str_format(aBuff, sizeof(aBuff), "*** '%s' has changed nick to '%s'", aMsgOldNick.c_str(), aMsgNewNick.c_str());
                                        pChan->m_Buffer.push_back(aBuff);
                                        pChan->m_Users.sort();
                                        break;
                                    }
                                    else if (str_comp_nocase((*itU).c_str(), NickVoice.c_str()) == 0)
                                    {
                                        (*itU) = aMsgNewNick;
                                        (*itU).insert(0, "+");
                                        str_format(aBuff, sizeof(aBuff), "*** '%s' has changed nick to '%s'", aMsgOldNick.c_str(), aMsgNewNick.c_str());
                                        pChan->m_Buffer.push_back(aBuff);
                                        pChan->m_Users.sort();
                                        break;
                                    }

                                    ++itU;
                                }
                            }

                            ++it;
                        }

						reply.from = aMsgOldNick;
						reply.to = aMsgNewNick;
                    }
                    else if (aMsgID.compare("MODE") == 0)
                    {
                        char aBuff[255];
                        del = aMsgFServer.find_first_of("!");
                        std::string aNickFrom = aMsgFServer.substr(0,del);

                        del = NetData.find_first_of(" ");
                        ldel = NetData.find_first_of(" ", del+1);
                        del = NetData.find_first_of(" ", ldel+1);
                        std::string aChannel = NetData.substr(ldel+1, (del)-(ldel+1));

                        ldel = NetData.find_first_of(" ", del+1);
                        std::string aMode = NetData.substr(del+1, ldel-(del+1));

                        ldel = NetData.find_first_of(" ", del+1);
                        del = NetData.find_first_of(" ", ldel+1);
                        std::string aNickTo = NetData.substr(ldel+1, del-(ldel+1));


                        CIrcCom *pCom = GetCom(aChannel);
                        if (pCom && pCom->GetType() == CIrcCom::TYPE_CHANNEL)
                        {
                            CComChan *pChan = static_cast<CComChan*>(pCom);
                            if (pChan)
                            {
                                str_format(aBuff, sizeof(aBuff), "*** '%s' sets mode '%s' on '%s'", aNickFrom.c_str(), aMode.c_str(), aNickTo.c_str());
                                pChan->m_Buffer.push_back(aBuff);

                                std::string aNewNick = aNickTo;
                                std::string aNickToVoz = aNickTo; aNickToVoz.insert(0, "+");

                                if (aMode[0] == '+')
                                {
                                    if (aMode[1] == 'o')
                                        aNewNick.insert(0, "@");
                                    else if (aMode[1] == 'v')
                                        aNewNick.insert(0, "+");
                                }
                                else if (aMode[0] == '-')
                                {
                                    if (aMode[1] == 'o')
                                        aNickTo.insert(0, "@");
                                }

                                std::list<std::string>::iterator it = pChan->m_Users.begin();
                                while (it != pChan->m_Users.end())
                                {
                                    if ((*it).compare(aNickTo) == 0 || (*it).compare(aNickToVoz) == 0)
                                    {
                                        (*it) = aNewNick;
                                        break;
                                    }
                                    ++it;
                                }

                                pChan->m_Users.sort();
                            }
                        }
                        reply.channel = aChannel;
						reply.from = aNickFrom;
						reply.to = aNickTo;
						reply.params = aMode;
                    }
                    else if (aMsgID.compare("KICK") == 0)
                    {
                        char aBuff[255];
                        del = aMsgFServer.find_first_of("!");
                        std::string aNickFrom = aMsgFServer.substr(0,del);

                        del = NetData.find_first_of(" ");
                        ldel = NetData.find_first_of(" ", del+1);
                        del = NetData.find_first_of(" ", ldel+1);
                        std::string aChannel = NetData.substr(ldel+1, (del)-(ldel+1));

                        ldel = NetData.find_first_of(" ", del+1);
                        std::string aNickTo = NetData.substr(del+1, ldel-(del+1));

                        ldel = NetData.find_first_of(":", 1);
                        std::string aKickReason = NetData.substr(ldel+1);


                        CIrcCom *pCom = GetCom(aChannel);
                        if (pCom && pCom->GetType() == CIrcCom::TYPE_CHANNEL)
                        {
                            if (aNickTo == m_Nick)
                            {
                                m_IrcComs.remove(pCom);
                                delete pCom;
                                pCom=0x0;
                                SetActiveCom(0);
                            }
                            else
                            {
                                CComChan *pChan = static_cast<CComChan*>(pCom);

                                str_format(aBuff, sizeof(aBuff), "*** '%s' kick '%s' (%s)", aNickFrom.c_str(), aNickTo.c_str(), aKickReason.c_str());
                                pChan->m_Buffer.push_back(aBuff);

                                pChan->m_Users.remove(aNickTo);
                                aNickTo.insert(0, "@");
                                pChan->m_Users.remove(aNickTo);
                                aNickTo[0]='+';
                                pChan->m_Users.remove(aNickTo);
                            }
                        }
                        reply.channel = aChannel;
						reply.from = aNickFrom;
						reply.to = aNickTo;
						reply.params = aKickReason;
                    }
                    else
                    {
                        char aBuff[1024];
                        ldel = NetData.find_first_of(" ", del+1);
                        del = ldel;
                        ldel = NetData.find_first_of(" ", del+1);
                        std::string aMsgData = NetData.substr(del+1, ldel-del-1);
                        del = NetData.find_first_of(":", 1);
                        std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);

                        if (ldel < del && ldel != std::string::npos)
                            str_format(aBuff, sizeof(aBuff), "%s %s", aMsgData.c_str(), aMsgText.c_str());
                        else
                            str_format(aBuff, sizeof(aBuff), "%s", aMsgText.c_str());

                        CIrcCom *pCom = GetCom("@Status");
                        pCom->m_Buffer.push_back(aBuff);
                    }

                    CallHooks(aMsgID.c_str(), &reply);
                }

                NetData.clear();
                continue;
            }
            NetData+=aNetBuff[i];
        }
    }

    // finish
    net_tcp_close(m_Socket);
    m_State = STATE_DISCONNECTED;

    Disconnect();
}

void CIrc::NextRoom()
{
	/* -- dunno if i misunderstood this...? -Henritees
    if (m_ActiveCom >= (int)m_IrcComs.size()-1)
        SetActiveCom(((int)m_IrcComs.size()>1)?1:0);
    else if (m_ActiveCom <= 0)
        SetActiveCom((int)m_IrcComs.size()-1);
    else
        SetActiveCom(m_ActiveCom+1);*/
	if (m_ActiveCom >= (int)m_IrcComs.size()-1)
		SetActiveCom(0);
	else
		SetActiveCom(m_ActiveCom+1);
}

void CIrc::PrevRoom()
{
    if (m_ActiveCom <= 0)
        SetActiveCom((int)m_IrcComs.size()-1);
    else
        SetActiveCom(m_ActiveCom-1);
}

void CIrc::OpenQuery(const char *to)
{
    char SanNick[25] = {0};
    str_copy(SanNick, (to[0] == '@' || to[0] == '+')?to+1:to, sizeof(SanNick));
	
	CIrcCom *pCom = GetCom(SanNick);
	if (pCom)
		SetActiveCom(pCom);
	else
	{
		CComQuery *pQuery = new CComQuery();
		str_copy(pQuery->m_User, (to[0] == '@' || to[0] == '+')?to+1:to, sizeof(pQuery->m_User));
		m_IrcComs.push_back(pQuery);
		SetActiveCom(m_IrcComs.size()-1);
	}
}

void CIrc::JoinTo(const char *to, const char *pass)
{
	SendRaw("JOIN %s %s", to, pass);
}

void CIrc::SetMode(const char *mode, const char *to)
{
    CIrcCom *pCom = GetActiveCom();
    if (!pCom || pCom->GetType() == CIrcCom::TYPE_QUERY)
        return;

    CComChan *pChan = static_cast<CComChan*>(pCom);
    if (!pChan)
        return;

    if (!to || to[0] == 0)
        SendRaw("MODE %s %s %s", pChan->m_Channel, mode, m_Nick.c_str());
    else
    	SendRaw("MODE %s %s %s", pChan->m_Channel, mode, to);
}

void CIrc::SetTopic(const char *topic)
{
    CIrcCom *pCom = GetActiveCom();
    if (!pCom || pCom->GetType() != CIrcCom::TYPE_CHANNEL)
        return;

    CComChan *pChan = static_cast<CComChan*>(pCom);
    SendRaw("TOPIC %s :%s", pChan->m_Channel, topic);
}

void CIrc::Part(const char *pReason, CIrcCom *pCom)
{
    if(!pCom)
    	pCom = GetCom(m_ActiveCom);

    if (!pCom)
        return;

    if (pCom->GetType() == CIrcCom::TYPE_CHANNEL)
    {
        CComChan *pChan = static_cast<CComChan*>(pCom);
        if(pReason && pReason[0])
        	SendRaw("PART %s :%s", pChan->m_Channel, pReason);
        else
        	SendRaw("PART %s %", pChan->m_Channel);

        m_IrcComs.remove(pCom);
        delete pCom;
        pCom=0x0;
        SetActiveCom(m_IrcComs.size()-1);
    }
    else if (pCom->GetType() == CIrcCom::TYPE_QUERY)
    {
        CComQuery *pQuery = static_cast<CComQuery*>(pCom);
        if (str_comp_nocase(pQuery->m_User, "@Status") == 0)
            return;

        m_IrcComs.remove(pCom);
        delete pCom;
        pCom=0x0;
        SetActiveCom(m_IrcComs.size()-1);
    }
}

void CIrc::Disconnect(const char *pReason)
{
    if (m_State != STATE_DISCONNECTED)
    {
		if(pReason && pReason[0])
			SendRaw("QUIT :%s", pReason);
		else
			SendRaw("QUIT");
        m_State = STATE_DISCONNECTED;
    }

    std::list<CIrcCom*>::iterator it = m_IrcComs.begin();
    while (it != m_IrcComs.end())
    {
        delete (*it);
        it = m_IrcComs.erase(it);
    }

    mem_zero(m_CmdToken, sizeof(m_CmdToken));
}

void CIrc::SendMsg(const char *to, const char *msg, int type)
{ //TODO: Rework this! duplicate code :P
    if (GetState() == STATE_DISCONNECTED || !msg || msg[0] == 0)
        return;

    char aBuff[1024];
    char aDest[25];

    // Search Destination
    if (!to || to[0] == 0)
    {
        if (m_ActiveCom == -1)
            return;

        std::list<CIrcCom*>::iterator it = m_IrcComs.begin();
        std::advance(it, m_ActiveCom);
        if ((*it)->GetType() == CIrcCom::TYPE_CHANNEL)
        {
            CComChan *pChan = static_cast<CComChan*>((*it));
            str_copy(aDest, pChan->m_Channel, sizeof(aDest));
        }
        else if ((*it)->GetType() == CIrcCom::TYPE_QUERY)
        {
            CComQuery *pQuery = static_cast<CComQuery*>((*it));
            if (str_comp_nocase(pQuery->m_User, "@Status") == 0)
            {
                str_format(aBuff, sizeof(aBuff),"** You can't send messages to '@Status'!", GetNick(), msg);
                pQuery->m_Buffer.push_back(aBuff);
                return;
            }

            str_copy(aDest, pQuery->m_User, sizeof(aDest));
        }
        else
            return;
    }
    else
    	str_copy(aDest, to, sizeof(aDest));


    // Send
    if (type == MSG_TYPE_ACTION)
    	str_format(aBuff, sizeof(aBuff), "ACTION %s", msg);
    else
    	str_copy(aBuff, msg, sizeof(aBuff));

    SendRaw("PRIVMSG %s :%s", aDest, aBuff);
    CIrcCom *pCom = GetCom(aDest);
    if (pCom)
    {
    	if (type == MSG_TYPE_ACTION)
    		str_format(aBuff, sizeof(aBuff),"*** %s: %s", GetNick(), msg);
    	else
    		str_format(aBuff, sizeof(aBuff),"<%s> %s", GetNick(), msg);
        pCom->m_Buffer.push_back(aBuff);
    }
}

void CIrc::SendRaw(const char *fmt, ...)
{
    if (!fmt || fmt[0] == 0)
        return;

	va_list args;
	char msg[1024*4];

	va_start(args, fmt);
	#if defined(CONF_FAMILY_WINDOWS)
		_vsnprintf(msg, sizeof(msg), fmt, args);
	#else
		vsnprintf(msg, sizeof(msg), fmt, args);
	#endif
	va_end(args);

	str_append(msg, "\r\n", sizeof(msg));
    net_tcp_send(m_Socket, msg, strlen(msg));
}

void CIrc::SetNick(const char *nick)
{
    if (m_State == STATE_CONNECTED)
        SendRaw("Nick %s", nick);

    m_Nick = nick;
}

void CIrc::SetAway(bool state, const char *msg)
{
	if (state)
		SendRaw("AWAY :%s", msg);
	else
		SendRaw("AWAY");
}

int CIrc::GetMsgType(const char *msg)
{
	int len = str_length(msg);
	if (len > 0 && msg[0] == 0x01 && msg[len-1] == 0x01)
	{
		char aCmd[12];
		mem_zero(aCmd, sizeof(aCmd));
		for (int i=1, e=0; i<len&&msg[i]!=32&&e<(int)sizeof(aCmd); i++, e++) aCmd[e] = msg[i];

		if (str_comp_nocase(aCmd, "ACTION") == 0)
			return MSG_TYPE_ACTION;
		else if (str_comp_nocase(aCmd, "TWSERVER") == 0)
			return MSG_TYPE_TWSERVER;
		else if (str_comp_nocase(aCmd, "GETTWSERVER") == 0)
			return MSG_TYPE_GET_TWSERVER;
	}

	return MSG_TYPE_NORMAL;
}

void CIrc::SendServer(const char *to, const char *Token)
{
	const char *curAddr = m_pClient->GetCurrentServerAddress();
	SendRaw("PRIVMSG %s :TWSERVER %s %s", to, Token, (curAddr&&curAddr[0]!=0)?curAddr:"NONE");
}

void CIrc::SendGetServer(const char *to)
{
	str_format(m_CmdToken, sizeof(m_CmdToken), "%ld", time_get());
	SendRaw("PRIVMSG %s :GETTWSERVER %s", to, m_CmdToken);
}

void CIrc::ExecuteCommand(const char *cmd, char *params)
{
	array<std::string> CmdListParams;
	for (char *p = strtok(params, " "); p != NULL; p = strtok(NULL, " "))
		CmdListParams.add(p);

    if (str_comp_nocase(cmd, "join") == 0 || str_comp_nocase(cmd, "j") == 0)
    {
        if (CmdListParams.size() == 0)
            return;

        JoinTo(CmdListParams[0].c_str(), (CmdListParams.size() > 1)?CmdListParams[1].c_str():"");
    }
    else if (str_comp_nocase(cmd, "query") == 0 || str_comp_nocase(cmd, "q") == 0)
    {
    	if (CmdListParams.size() == 0)
            return;

        OpenQuery(CmdListParams[0].c_str());
    }
    else if (str_comp_nocase(cmd, "squery") == 0 || str_comp_nocase(cmd, "sq") == 0)
    {
    	if (CmdListParams.size() == 0)
            return;

        SendGetServer(CmdListParams[0].c_str());
    }
    else if (str_comp_nocase(cmd, "topic") == 0 || str_comp_nocase(cmd, "t") == 0)
    {
    	if (CmdListParams.size() == 0)
            return;

        SetTopic(params);
    }
    else if (str_comp_nocase(cmd, "part") == 0 || str_comp_nocase(cmd, "p") == 0)
    {
        Part();
    }
    else if (str_comp_nocase(cmd, "nick") == 0)
    {
    	if (CmdListParams.size() == 0)
            return;

        SetNick(CmdListParams[0].c_str());
    }
    else if (str_comp_nocase(cmd, "op") == 0)
    {
    	if (CmdListParams.size() > 0)
            SetMode("+o", CmdListParams[0].c_str());
        else
            SetMode("+o", 0x0);
    }
    else if (str_comp_nocase(cmd, "deop") == 0)
    {
    	if (CmdListParams.size() > 0)
            SetMode("-o", CmdListParams[0].c_str());
        else
            SetMode("-o", 0x0);
    }
    else if (str_comp_nocase(cmd, "voz") == 0)
    {
    	if (CmdListParams.size() > 0)
            SetMode("+v", CmdListParams[0].c_str());
        else
            SetMode("+v", 0x0);
    }
    else if (str_comp_nocase(cmd, "devoz") == 0)
    {
    	if (CmdListParams.size() > 0)
            SetMode("-v", CmdListParams[0].c_str());
        else
            SetMode("-v", 0x0);
    }
    else if (str_comp_nocase(cmd, "clear") == 0)
    {
    	CIrcCom *pCom = GetActiveCom();
    	if (pCom)
    		pCom->m_Buffer.clear();
    }
    else
        SendRaw("%s %s", cmd, params);
}
