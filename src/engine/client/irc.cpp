/* (c) unsigned char*. See licence.txt in the root of the distribution for more information. */
/* edited by The AllTheHaxx Team */
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
#include "luabinding.h"

static NETSOCKET invalid_socket = {NETTYPE_INVALID, -1, -1};

CIRC::CIRC()
{
	m_apIRCComs.clear();
	m_apIRCComs.hint_size(4);
	m_Hooks.clear();
	m_pGraphics = 0x0;
	m_State = STATE_DISCONNECTED;
	m_Socket = invalid_socket;
	m_Nick = "";
	m_StartTime = 0;
	m_ActiveCom = 0;
	mem_zero(m_CmdToken, sizeof(m_CmdToken));

	m_Debug = false;
}

void CIRC::RegisterCallback(const char* pMsgID, int (*func)(ReplyData*, void*, void*), void *pUser) // pData, pUser
{
	IRCHook h;
	h.messageID = pMsgID;
	h.function = func;
	h.user = pUser;
	m_Hooks.add(h);
	dbg_msg("engine/IRC", "registered callback for '%s'", pMsgID);
}

void CIRC::CallHooks(const char *pMsgID, ReplyData* pReplyData)
{
	for(int i = 0; i < m_Hooks.size(); i++)
	{
		if(m_Hooks[i].messageID == std::string(pMsgID))
		{
			int ret = (*(m_Hooks[i].function))(pReplyData, m_Hooks[i].user, this);
			if(ret != 0)
			{
				char aError[128];
				str_format(aError, sizeof(aError), "IRC callback returned != 0 (ret=%i, i=%i, callback=%s, func=%p)",
						ret, i, m_Hooks[i].messageID.c_str(), m_Hooks[i].function);
				dbg_assert(ret == 0, aError);
			}
		}
	}
}

void CIRC::Init()
{
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();
	m_pGameClient = Kernel()->RequestInterface<IGameClient>();
	m_pClient = Kernel()->RequestInterface<IClient>();
}

void CIRC::SetActiveCom(unsigned index)
{
	dbg_assert(index < (unsigned)m_apIRCComs.size(), "CIRC::SetActiveCom called with index out of range");

	m_ActiveCom = index;
	GetCom(index)->m_NumUnreadMsg = 0;
}

void CIRC::SetActiveCom(CIRCCom *pCom)
{
	dbg_assert(pCom != NULL, "CIRC::SetActiveCom called with nullptr");

	for(unsigned i = 0; i < (unsigned)m_apIRCComs.size(); i++)
		if(m_apIRCComs[i] == pCom)
		{
			SetActiveCom(i);
			break;
		}
}

CIRCCom* CIRC::GetActiveCom()
{
	if(m_State == STATE_DISCONNECTED)
		return 0;

	dbg_assert(m_ActiveCom < (unsigned)m_apIRCComs.size(), "CIRC::m_ActiveCom out of range");
	dbg_assert(m_apIRCComs[m_ActiveCom] != NULL, "invalid pointer in CIRC::m_apIRCComs");

	return m_apIRCComs[m_ActiveCom];
}

CIRCCom* CIRC::GetCom(unsigned index)
{
	dbg_assert(index < (unsigned)m_apIRCComs.size(), "CIRC::GetCom called with index out of range");
	dbg_assert(m_apIRCComs[index] != NULL, "invalid pointer in CIRC::m_apIRCComs");

	return m_apIRCComs[index];
}

CIRCCom* CIRC::GetCom(const std::string& Name)
{
	for(int i = 0; i < m_apIRCComs.size(); i++)
	{
		CIRCCom *pCom = m_apIRCComs[i];
		if(str_comp_nocase(Name.c_str(), pCom->m_aName) == 0)
			return pCom;
	}

	return 0x0;
}

bool CIRC::CanCloseCom(CIRCCom *pCom)
{
	if(!pCom)
		return false;

	if(GetNumComs() <= 2)
		return false;

	if(pCom->GetType() == CIRCCom::TYPE_CHANNEL)
		if(str_comp_nocase(((CComChan*)pCom)->Channel(), "#AllTheHaxx") == 0)
			return false;

	if(pCom->GetType() == CIRCCom::TYPE_QUERY)
		if(str_comp_nocase(((CComQuery*)pCom)->User(), "@status") == 0)
			return false;

	return true;
}

void CIRC::StartConnection() // call this from a thread only!
{
	m_apIRCComs.delete_all();
	m_apIRCComs.clear();
	m_IsAway = false;

	NETADDR BindAddr;
	mem_zero(&m_HostAddress, sizeof(m_HostAddress));
	mem_zero(&BindAddr, sizeof(BindAddr));
	char aNetBuff[2048];

	m_State = STATE_CONNECTING;
	// lookup
	unsigned int connectionType = NETTYPE_IPV6;
	//if(net_host_lookup("irc.ipv6.quakenet.org:6667", &m_HostAddress, connectionType) != 0)
	{
		connectionType = NETTYPE_IPV4;
		if(net_host_lookup("irc.quakenet.org:6667", &m_HostAddress, connectionType) != 0)
		{
			dbg_msg("IRC","ERROR: Can't lookup irc.quakenet.org");
			m_State = STATE_DISCONNECTED;
			return;
		}
	}

	m_HostAddress.port = 6667;

	// connect
	BindAddr.type = connectionType;
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

	// set nickname
	{
		char aSanitizedNick[32];
		str_copy(aSanitizedNick, g_Config.m_ClIRCNick, sizeof(aSanitizedNick));
		str_irc_sanitize(aSanitizedNick);

		bool OnlyUnderscores = true;
		for(int i = 0; i < str_length(aSanitizedNick); i++)
		{
			if(aSanitizedNick[i] != '_')
			{
				OnlyUnderscores = false;
				break;
			}
		}
		if(str_length(aSanitizedNick) == 0 || str_comp(aSanitizedNick, "haxxless tee") == 0 || OnlyUnderscores)
			str_copy(aSanitizedNick, g_Config.m_PlayerName, sizeof(aSanitizedNick));

		// don't overexaggerate it with the underscores
		const char *pNickStart = aSanitizedNick;
		if(!OnlyUnderscores)
			while(*pNickStart == '_')
				pNickStart++;
		SetNick(aSanitizedNick);
	}

	// send request
	SendRaw("CAP LS");
	SendRaw("NICK %s", m_Nick.c_str());
	SendRaw("USER %s 0 * :%s", "allthehax", g_Config.m_PlayerName);

	// status tab
	OpenComQuery("@Status");

	m_StartTime = time_get();
	m_State = STATE_CONNECTED;

	std::string NetData;
	//int TotalRecv = 0;
	//int TotalBytes = 0;
	long CurrentRecv = 0;
	char LastPong[255]={0};
	while ((CurrentRecv = net_tcp_recv(m_Socket, aNetBuff, sizeof(aNetBuff))) >= 0 && m_State == STATE_CONNECTED)
	{
		ReplyData reply;

		if(m_Debug)
			dbg_msg("irc/dbg", "%s", aNetBuff);

		for (int i=0; i < CurrentRecv; i++)
		{
			if (aNetBuff[i]=='\r' || aNetBuff[i]=='\t')
				 continue;

			if (aNetBuff[i]=='\n')
			{
				size_t del = NetData.find_first_of(':');
				size_t ldel = 0;
				if (del > 0)
				{ //NeT Message
					std::string aMsgID = NetData.substr(0, del-1);
					std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);
					if (aMsgID == "PING")
					{
						SendRaw("PONG %s :%s", LastPong, aMsgText.c_str());
						if(g_Config.m_Debug)
							dbg_msg("engine/IRC", "Ping? Pong!");
						LastPong[0]=0;
					}
					else if (aMsgID == "PONG")
						str_copy(LastPong, aMsgText.c_str(), sizeof(LastPong));
					else
					{
						CIRCCom *pCom = GetCom("@Status");
						pCom->m_Buffer.push_back(aMsgText);
						reply.channel = "@Status";
						reply.from = "quakenet.org";
						reply.params = aMsgText;
					}
				} else
				{ //raw message
					del = NetData.find_first_of(' ');
					std::string MsgFServer = NetData.substr(1, del);
					ldel = del;
					del = NetData.find_first_of(' ',del+1);
					std::string MsgID = NetData.substr(ldel+1, del-ldel-1);

					//dbg_msg("IRC", "Raw MSG [%s]: %s",MsgID.c_str(), MsgFServer.c_str());
					//dbg_msg("IRC-RAW", NetData.c_str());

					if (MsgID == "001")
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
					else if (MsgID == "332") // topic
					{
						del = NetData.find_first_of(' ',del+1);
						ldel = NetData.find_first_of(' ',del+1);
						std::string aMsgChan = NetData.substr(del+1,ldel-del-1);

						del = NetData.find_first_of(':', 1);
						std::string aMsgTopic = NetData.substr(del+1, NetData.length()-del-1);

						CComChan *pChan = dynamic_cast<CComChan*>(GetCom(aMsgChan));
						if (pChan)
							pChan->m_Topic = aMsgTopic;

						reply.channel = aMsgChan;
						reply.params = aMsgTopic;
					}
					else if (MsgID == "353") // NAMREPLY
					{
						del = NetData.find_first_of('=');
						ldel = NetData.find_first_of(' ',del+2);

						std::string aMsgChan = NetData.substr(del+2, ldel-del-2);
						del = NetData.find_first_of(':',1);
						std::string aMsgUsers = NetData.substr(del+1, NetData.length()-del-1);

						CComChan *pChan = dynamic_cast<CComChan*>(GetCom(aMsgChan));
						if(pChan)
						{
							size_t del=0, ldel=0;
							do
							{
								del = aMsgUsers.find_first_of(' ', del+1);
								std::string name = aMsgUsers.substr(ldel, del-ldel);
								int level = 0;
								if(name.c_str()[0] == '@')
									level |= CComChan::CUser::LEVEL_ADMIN;

								if(name.c_str()[0] == '+')
									level |= CComChan::CUser::LEVEL_VOICE;

								if(level > 0)
									name = name.c_str()+1;

								CComChan::CUser User(name);
								User.m_Level = level;
								CComChan::CUser *pFound = pChan->m_Users.find(User);
								if(pFound)
									*pFound = User; // update the entry
								else
									pChan->m_Users.add_unsorted(User);

								ldel=del+1;
							} while (del != std::string::npos);
						}
						reply.channel = aMsgChan;
						reply.params = aMsgUsers;
					}
					else if (MsgID == "366") // ENDOFNAMES
					{
						del = NetData.find_first_of(' ',del+1);
						ldel = NetData.find_first_of(' ',del+1);
						std::string aMsgChan = NetData.substr(del+1,ldel-del-1);

						CComChan *pChan = dynamic_cast<CComChan*>(GetCom(aMsgChan));
						if(pChan)
							pChan->m_Users.sort_range();

						reply.channel = aMsgChan;

					}
					else if (MsgID == "401") // NOSUCHNICK
					{
						del = NetData.find_first_of(' ',del+1);
						ldel = NetData.find_first_of(' ',del+1);
						std::string aMsgFrom = NetData.substr(del+1,ldel-del-1);
						del = NetData.find_first_of(':',1);
						std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);

						CIRCCom *pCom = GetCom(aMsgFrom);
						if(!pCom)
							pCom = GetCom(0);

						pCom->AddMessage("*** '%s' %s", aMsgFrom.c_str(), aMsgText.c_str());

						reply.from = aMsgFrom;
						reply.params = aMsgText;

					}
					else if (MsgID == "433") // NICKNAME-ALREADY-IN-USE
					{

					}
					else if (MsgID == "421") // UNKNOWNCOMMAND
					{
						del = NetData.find_first_of(' ',del+1);
						ldel = NetData.find_first_of(' ',del+1);
						std::string aMsgCmd = NetData.substr(del+1,ldel-del-1);
						del = NetData.find_first_of(':',1);
						std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);

						CIRCCom *pCom = GetCom(0);
						pCom->AddMessage("'%s' %s", aMsgCmd.c_str(), aMsgText.c_str());

						reply.from = aMsgCmd;
						reply.params = aMsgText;

					}
					else if (MsgID == "JOIN")
					{
						std::string MsgChannel = NetData.substr(del+1, NetData.length()-del-1);
						del = MsgFServer.find_first_of('!');
						std::string MsgFrom = MsgFServer.substr(0,del);

						if (MsgFrom == m_Nick) // we (were) joined a channel
						{
							OpenComChan(MsgChannel.c_str());
						}
						else if(MsgFrom != "circleci-bot") // ignore the ci bot
						{
							CComChan *pChan = dynamic_cast<CComChan*>(GetCom(MsgChannel));
							if(pChan)
							{
								pChan->m_Users.add(CComChan::CUser(MsgFrom));
								if(g_Config.m_ClIRCShowJoins)
									pChan->AddMessage("*** '%s' has joined %s", MsgFrom.c_str(), MsgChannel.c_str());
							}
						}

						reply.channel = MsgChannel;
						reply.from = MsgFrom;
					}
					else if (MsgID == "PART")
					{
						std::string MsgChannel = NetData.substr(del+1, NetData.length()-del-1);
						del = MsgFServer.find_first_of('!');
						std::string MsgFrom = MsgFServer.substr(0,del);

						if (MsgFrom == m_Nick) // we (were) left a channel
						{
							CIRCCom *pCom = GetCom(MsgChannel);
							if(pCom)
							{
								CloseCom(pCom);
							}
						}
						else if(MsgFrom != "circleci-bot") // ignore the ci bot
						{
							char aChanName[128];
							str_copy(aChanName, MsgChannel.c_str(), sizeof(aChanName));
							const char *pReason = str_find(aChanName, ":")+1;
							if(str_replace_char(aChanName, ':', '\0'))
								aChanName[str_length(aChanName)-1] = '\0';
							CComChan *pChan = dynamic_cast<CComChan*>(GetCom(std::string(aChanName)));
							if(pChan)
							{
								pChan->RemoveUserFromList(MsgFrom.c_str());

								if(g_Config.m_ClIRCShowJoins)
								{
									if(pReason)
										pChan->AddMessage("*** '%s' left %s (%s)", MsgFrom.c_str(), aChanName, pReason);
									else
										pChan->AddMessage("*** '%s' left %s", MsgFrom.c_str(), aChanName);
								}
							}
							else
								dbg_msg("IRC", "WARNING: Got a PART message but no channel for it! { '%s' > '%s' }", MsgFrom.c_str(), MsgChannel.c_str());
						}

					   reply.channel = MsgChannel;
					   reply.from = MsgFrom;
					}
					else if (MsgID == "QUIT")
					{
						del = NetData.find_first_of(':',1);
						std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);
						del = MsgFServer.find_first_of('!');
						std::string aMsgFrom = MsgFServer.substr(0,del);

						if(aMsgFrom != m_Nick && aMsgFrom != "circleci-bot") // ignore the ci bot) // this applies only for channels, not for queries
						{
							for(int c = 0; c < m_apIRCComs.size(); c++) // iterate though all coms and ignore queries
							{
								CIRCCom *pCom = m_apIRCComs[c];
								dbg_assert(pCom != NULL, "CIRC::m_apIRCComs holds an invalid item");

								if(pCom->GetType() != CIRCCom::TYPE_CHANNEL)
									continue;

								CComChan *pChan = dynamic_cast<CComChan*>(pCom);
								dbg_assert(pChan != NULL, "failed casting pCom to CComChan*");

								pChan->RemoveUserFromList(aMsgFrom.c_str());
								if(g_Config.m_ClIRCShowJoins)
									pChan->AddMessage("*** '%s' has quit (%s)", aMsgFrom.c_str(), aMsgText.c_str());

							}
						}

						reply.channel = "IRC";
						reply.params = aMsgText;
						reply.from = aMsgFrom;
					}
					else if (MsgID == "TOPIC")
					{
						ldel = NetData.find_first_of(' ',del+1);
						std::string aMsgChan = NetData.substr(del+1, ldel-del-1);
						del = NetData.find_first_of(':',1);
						std::string aMsgText = NetData.substr(del+1, NetData.length()-del-1);
						del = MsgFServer.find_first_of('!');
						std::string aMsgFrom = MsgFServer.substr(0,del);

						CComChan *pChan = dynamic_cast<CComChan*>(GetCom(aMsgChan));
						if (pChan)
						{
							pChan->m_Topic = aMsgText;
							pChan->AddMessage("*** '%s' changed topic to '%s'", aMsgFrom.c_str(), aMsgText.c_str());
						}

						reply.channel = aMsgChan;
						reply.from = aMsgFrom;
						reply.params = aMsgText;
					}
					else if (MsgID == "PRIVMSG")
					{
						char aBuff[1024];
						ldel = NetData.find_first_of(' ', del + 1);
						std::string aMsgChan = NetData.substr(del + 1, ldel - del - 1);

						del = NetData.find_first_of(':', 1);
						std::string aMsgText = NetData.substr(del + 1, NetData.length() - del - 1);
						int MsgType = GetMsgType(aMsgText.c_str());

						del = MsgFServer.find_first_of('!');
						std::string MsgFrom = MsgFServer.substr(0, del);

						if(MsgType == MSG_TYPE_TWSERVER) // somebody sends us his server
						{
							if(aMsgChan == m_Nick)
							{
								del = aMsgText.find_first_of(' ');
								ldel = aMsgText.find_last_of(' ');
								if(del != std::string::npos && del != ldel)
								{
									char aAddr[NETADDR_MAXSTRSIZE];
									mem_zero(aAddr, sizeof(aAddr));
									std::string CleanMsg = aMsgText.substr(10);
									CleanMsg = CleanMsg.substr(0, CleanMsg.length() - 1);
									size_t pc = CleanMsg.find_first_of(' ');
									if(pc != std::string::npos)
									{
										str_copy(aAddr, CleanMsg.substr(pc + 1).c_str(), sizeof(aAddr));
										if(m_CmdToken[0] != 0
												&& str_comp(CleanMsg.substr(0, pc).c_str(), m_CmdToken) == 0
												&& aAddr[0] != 0)
										{
											if(aAddr[0] != 0 && str_comp_nocase(aAddr, "NONE") != 0 && str_comp_nocase(aAddr, "FORBIDDEN") != 0)
											{
												m_pClient->Connect(aAddr);
											}
											else
											{
												CIRCCom *pCom = GetActiveCom();
												if(pCom)
												{
													char aBuf[128];
													if(str_comp_nocase(aAddr, "FORBIDDEN") == 0)
														str_format(aBuf, sizeof(aBuf),
																"*** '%s' forbids joining his server!",
																MsgFrom.c_str());
													else if(str_comp_nocase(aAddr, "NONE") == 0)
														str_format(aBuf, sizeof(aBuf),
																"*** '%s' isn't playing on a server!",
																MsgFrom.c_str());
													else
														str_format(aBuf, sizeof(aBuf),
																"*** '%s' sent invalid information!", MsgFrom.c_str());
													pCom->m_Buffer.push_back(std::string(aBuf));
												}
											}

											mem_zero(m_CmdToken, sizeof(m_CmdToken));
										}
									}
								}
							}
						}
						else if(MsgType == MSG_TYPE_GET_TWSERVER) // somebody wants to know our server
						{
							if(aMsgChan == m_Nick)
							{
								std::string CleanMsg = aMsgText.substr(13);
								CleanMsg = CleanMsg.substr(0, CleanMsg.length() - 1);

								if(!CleanMsg.empty())
									SendServer(MsgFrom.c_str(), CleanMsg.c_str());
							}
						}
						else if(MsgType == MSG_TYPE_CTCP) // custom ctcp message
						{
							array<std::string> CmdListParams;
							char aBuf[512]; char *Ptr;
							str_copy(aBuf, aMsgText.c_str(), sizeof(aBuf));
							Ptr = aBuf+1;
							str_replace_char_rev_num(Ptr, 1, '\1', '\0');

							if(g_Config.m_Debug == 3 && m_Debug)
								dbg_msg("IRC", "got a CTCP '%s' from '%s'", Ptr, MsgFrom.c_str());

							for (char *p = strtok(Ptr, " "); p != NULL; p = strtok(NULL, " "))
								CmdListParams.add(p);

							if(str_comp_nocase(CmdListParams[0].c_str(), "version") == 0)
								SendVersion(MsgFrom.c_str());
							else if(str_comp_nocase(CmdListParams[0].c_str(), "time") == 0)
							{
								char aLTime[64];
								str_timestampb(aLTime);
								char aCTime[64];
								str_clock_secb(aCTime, round_to_int(m_pClient->SteadyTimer()));
								SendRaw("NOTICE %s :TIME %s +%s", MsgFrom.c_str(), aLTime, aCTime);
							}
							else if(str_comp_nocase(CmdListParams[0].c_str(), "playerinfo") == 0)
							{
								str_format(aBuf, sizeof(aBuf), "NOTICE %s :PLAYERINFO %i,%i,%i", MsgFrom.c_str(), ((CGameClient*)m_pGameClient)->m_pMenus->m_Nalf[0]^GAME_ATH_VERSION_NUMERIC, ((CGameClient*)m_pGameClient)->m_pMenus->m_Nalf[(g_Config.m_ClDummy&(0xEF%2))|1]^GAME_ATH_VERSION_NUMERIC,GAME_ATH_VERSION_NUMERIC);
								CmdListParams.remove_index(0);
								while(!CmdListParams.empty())
								{
									str_append(aBuf, " '", sizeof(aBuf));
									str_append(aBuf, CmdListParams[0].c_str(), sizeof(aBuf));
									str_append(aBuf, "=", sizeof(aBuf));
									if(str_comp_nocase("name", CmdListParams[0].c_str()) == 0)
										str_append(aBuf, g_Config.m_PlayerName, sizeof(aBuf));
									else if(str_comp_nocase("clan", CmdListParams[0].c_str()) == 0)
										str_append(aBuf, g_Config.m_PlayerClan, sizeof(aBuf));
									else if(str_comp_nocase("skin", CmdListParams[0].c_str()) == 0)
										str_append(aBuf, g_Config.m_ClPlayerSkin, sizeof(aBuf));
									else if(str_comp_nocase("dummy", CmdListParams[0].c_str()) == 0)
										str_append(aBuf, g_Config.m_ClDummyName, sizeof(aBuf));
									else
										str_append(aBuf, "\\\\¶¶_error:arg\\\\", sizeof(aBuf));
									str_append(aBuf, "' ", sizeof(aBuf));

									CmdListParams.remove_index(0);
								}
								SendRaw(aBuf);
							}
						}
						else // normal message
						{
							reply.channel = aMsgChan;
							reply.from = MsgFrom;
							reply.params = aMsgText;


							{
								CIRCCom *pCom;
								if(aMsgChan == m_Nick) // this is the case for private chats ("Query"s)
								{
									pCom = GetCom(MsgFrom);
									if(!pCom)
										pCom = OpenComQuery(MsgFrom.c_str(), false);
								}
								else
								{
									pCom = GetCom(aMsgChan);
									if(!pCom)
										pCom = OpenComChan(aMsgChan.c_str(), false);
								}

								if(pCom)
								{
									if(pCom != GetActiveCom())
										pCom->m_NumUnreadMsg++;

									if(MsgType == MSG_TYPE_ACTION) // the "/me" thingy
									{
										str_format(aBuff, sizeof(aBuff), "* %s ", MsgFrom.c_str());
										str_append(aBuff, aMsgText.substr(8, -1).c_str(), sizeof(aBuff));
										str_replace_char_rev_num(aBuff, 1, '\1', '\0');
									}
									else
									{
										str_format(aBuff, sizeof(aBuff), "<%s> ", MsgFrom.c_str());
										str_append(aBuff, aMsgText.c_str(), sizeof(aBuff));
									}

									pCom->AddMessage_nofmt(aBuff);
								}

								if(pCom == GetActiveCom())
								{
									aMsgChan.insert(0, "[");
									aMsgChan.append("] ");
									MsgFrom.insert(0, "<");
									MsgFrom.append("> ");
									//MsgFrom.insert(0, aTime);
								}
								m_pGameClient->OnMessageIRC(aMsgChan, MsgFrom, aMsgText);
							}
						}
					}
					else if (MsgID == "NOTICE")
					{
						char aBuff[1024];
						ldel = NetData.find_first_of(' ', del + 1);
						std::string MsgChan = NetData.substr(del + 1, ldel - del - 1);

						del = NetData.find_first_of(':', 1);
						std::string MsgText = NetData.substr(del + 1, NetData.length() - del - 1);
						int MsgType = GetMsgType(MsgText.c_str());

						del = MsgFServer.find_first_of('!');
						std::string MsgFrom = MsgFServer.substr(0, del);
						{
							reply.channel = MsgChan;
							reply.from = MsgFrom;
							reply.params = MsgText;

							if(MsgChan == m_Nick)
							{
								CIRCCom *pCom = GetCom(MsgFrom);
								std::replace(MsgText.begin(), MsgText.end(), '\1', ' ');
								if(!pCom)
								{
									// search through all users
									std::vector<const char *> s_AvailableQueries;
									{
										s_AvailableQueries.clear();
										for(unsigned c = 0; c < GetNumComs(); c++)
										{
											if(GetCom(c)->GetType() == CIRCCom::TYPE_CHANNEL)
											{
												CComChan *pChanCom = ((CComChan *)GetCom(c));
												for(int u = 0; u < pChanCom->m_Users.size(); u++)
												{
													s_AvailableQueries.push_back(pChanCom->m_Users[u].c_str());
												}
											}
											else if(str_comp_nocase(((CComQuery *)GetCom(c))->m_aName, "@status") != 0)
												s_AvailableQueries.push_back(((CComQuery *)GetCom(c))->m_aName);

										}
									}

									for(std::vector<const char *>::iterator it = s_AvailableQueries.begin(); it != s_AvailableQueries.end(); it++)
									{
										if(str_comp(*it, MsgFrom.c_str()) == 0)
										{
											pCom = OpenComQuery(MsgFrom.c_str(), false); // only open a com for seeable users
											break;
										}
									}

									if(!pCom)
										pCom = GetCom(0); // otherwise print it to the Status com
								}

								if(pCom)
								{
									if(pCom != GetActiveCom())
										pCom->m_NumUnreadMsg++;

									if(MsgType == MSG_TYPE_ACTION)
										pCom->AddMessage(MsgText.substr(8, -1).c_str());
									else
										pCom->AddMessage_nofmt((MsgFrom + ": " + MsgText).c_str());
								}
							}
							else
							{
								CIRCCom *pCom = GetCom(MsgChan);
								if(pCom)
								{
									if(pCom != GetActiveCom())
										pCom->m_NumUnreadMsg++;

									if(MsgType == MSG_TYPE_ACTION)
									{
										str_format(aBuff, sizeof(aBuff), "* %s ", MsgFrom.c_str());
										str_append(aBuff, MsgText.substr(8, -1).c_str(), sizeof(aBuff));
									}
									else
									{
										str_format(aBuff, sizeof(aBuff), "<%s> ", MsgFrom.c_str());
										str_append(aBuff, MsgText.c_str(), sizeof(aBuff));
									}
									pCom->AddMessage_nofmt(aBuff);
								}

								if(pCom == GetActiveCom())
								{
									MsgChan.insert(0, "[");
									MsgChan.append("] ");
									MsgFrom.insert(0, "<");
									MsgFrom.append("> ");
									//MsgFrom.insert(0, aTime);
								}
								m_pGameClient->OnMessageIRC(MsgChan, MsgFrom, MsgText);
							}
						}
					}
					else if (MsgID == "NICK")
					{
						del = MsgFServer.find_first_of('!');
						std::string aMsgOldNick = MsgFServer.substr(0,del);

						del = NetData.find_first_of(':', 1);
						std::string aMsgNewNick = NetData.substr(del+1, NetData.length()-del-1);

						if(aMsgOldNick == m_Nick) // our name has changed
							m_Nick = aMsgNewNick;

						for(int c = 0; c < m_apIRCComs.size(); c++)
						{
							CIRCCom *pCom = m_apIRCComs[c];
							if (pCom->GetType() == CIRCCom::TYPE_QUERY)
							{
								CComQuery *pQuery = dynamic_cast<CComQuery*>(pCom);
								if (str_comp_nocase(pQuery->m_aName, aMsgOldNick.c_str()) == 0)
								{
									str_copy(pQuery->m_aName, aMsgNewNick.c_str(), sizeof(pQuery->m_aName));
									pQuery->AddMessage( "*** '%s' changed their nick to '%s'", aMsgOldNick.c_str(), aMsgNewNick.c_str());
								}
							}
							else if (pCom->GetType() == CIRCCom::TYPE_CHANNEL)
							{
								CComChan *pChan = dynamic_cast<CComChan*>(pCom);
								for(int u = 0; u < pChan->m_Users.size(); u++)
								{
									std::string& User = pChan->m_Users[u].m_Nick;

									if (str_comp_nocase(User.c_str(), aMsgOldNick.c_str()) == 0)
									{
										User = aMsgNewNick;
										pChan->AddMessage("*** '%s' changed their nick to '%s'", aMsgOldNick.c_str(), aMsgNewNick.c_str());
										pChan->m_Users.sort_range();
										break;
									}
								}
							}
						}

						reply.from = aMsgOldNick;
						reply.to = aMsgNewNick;
					}
					else if (MsgID == "MODE")
					{
						del = MsgFServer.find_first_of('!');
						std::string aNickFrom = MsgFServer.substr(0,del);

						del = NetData.find_first_of(' ');
						ldel = NetData.find_first_of(' ', del+1);
						del = NetData.find_first_of(' ', ldel+1);
						std::string aChannel = NetData.substr(ldel+1, (del)-(ldel+1));

						ldel = NetData.find_first_of(' ', del+1);
						std::string aMode = NetData.substr(del+1, ldel-(del+1));

						ldel = NetData.find_first_of(' ', del+1);
						del = NetData.find_first_of(' ', ldel+1);
						std::string aNickTo = NetData.substr(ldel+1, del-(ldel+1));


						CIRCCom *pCom = GetCom(aChannel);
						if (pCom && pCom->GetType() == CIRCCom::TYPE_CHANNEL)
						{
							CComChan *pChan = dynamic_cast<CComChan*>(pCom);
							if (pChan)
							{
								char aGenericTerm[32] = {0};
								str_format(aGenericTerm, sizeof(aGenericTerm), "%s %s %s",
										aMode.c_str()[0] == '+' ? "gives" : "removes",
											aMode.c_str()[1] == 'o' ? "operator" :
											aMode.c_str()[1] == 'v' ? "guest status" :
											aMode.c_str()[1] == 'b' ? "a ban" : "unknown",
										aMode.c_str()[0] == '+' ? "to" : "from");
								pChan->AddMessage("*** '%s' %s '%s' (mode%s)", aNickFrom.c_str(), aGenericTerm, aNickTo.c_str(), aMode.c_str());

								CComChan::CUser *pUser = pChan->GetUser(aNickTo);
								if (aMode[0] == '+')
								{
									if(str_find_nocase(aMode.c_str(), "o"))
										pUser->m_Level |= CComChan::CUser::LEVEL_ADMIN;
									if(str_find_nocase(aMode.c_str(), "v"))
										pUser->m_Level |= CComChan::CUser::LEVEL_VOICE;
								}
								else if (aMode[0] == '-')
								{
									if(str_find_nocase(aMode.c_str(), "o"))
										pUser->m_Level &= ~CComChan::CUser::LEVEL_ADMIN;
									if(str_find_nocase(aMode.c_str(), "v"))
										pUser->m_Level &= ~CComChan::CUser::LEVEL_VOICE;
								}

								pChan->m_Users.sort_range();
							}
						}
						reply.channel = aChannel;
						reply.from = aNickFrom;
						reply.to = aNickTo;
						reply.params = aMode;
					}
					else if (MsgID == "KICK")
					{
						del = MsgFServer.find_first_of('!');
						std::string aNickFrom = MsgFServer.substr(0,del);

						del = NetData.find_first_of(' ');
						ldel = NetData.find_first_of(' ', del+1);
						del = NetData.find_first_of(' ', ldel+1);
						std::string aChannel = NetData.substr(ldel+1, (del)-(ldel+1));

						ldel = NetData.find_first_of(' ', del+1);
						std::string aNickTo = NetData.substr(del+1, ldel-(del+1));

						ldel = NetData.find_first_of(':', 1);
						std::string aKickReason = NetData.substr(ldel+1);


						CIRCCom *pCom = GetCom(aChannel);
						if (pCom && pCom->GetType() == CIRCCom::TYPE_CHANNEL)
						{
							if (aNickTo == m_Nick) // we got kicked
							{
								CloseCom(dynamic_cast<CComQuery*>(pCom));
								GetCom(0)->AddMessage("You got kicked from channel '%s', Reason: %s", aChannel.c_str(), aKickReason.c_str());
							}
							else
							{
								CComChan *pChan = dynamic_cast<CComChan*>(pCom);

								if(g_Config.m_ClIRCShowJoins && aKickReason != "Deprecated version")
									pChan->AddMessage("*** '%s' kicked '%s' (Reason: %s)", aNickFrom.c_str(), aNickTo.c_str(), aKickReason.c_str());

								pChan->RemoveUserFromList(aNickTo.c_str());
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
						ldel = NetData.find_first_of(' ', del+1);
						del = ldel;
						ldel = NetData.find_first_of(' ', del+1);
						std::string MsgData = NetData.substr(del+1, ldel-del-1);
						del = NetData.find_first_of(':', 1);
						std::string MsgText = NetData.substr(del+1, NetData.length()-del-1);

						if (ldel < del && ldel != std::string::npos)
							str_format(aBuff, sizeof(aBuff), "%s %s", MsgData.c_str(), MsgText.c_str());
						else
							str_format(aBuff, sizeof(aBuff), "%s", MsgText.c_str());

						GetCom(0)->AddMessage_nofmt(aBuff);
					}

					CallHooks(MsgID.c_str(), &reply);
				}

				NetData.clear();
				continue;
			}
			NetData += aNetBuff[i];
		}
	}

	dbg_msg("irc", "closing socket");
	net_tcp_close(m_Socket);

	mem_zero(m_CmdToken, sizeof(m_CmdToken));
	m_State = STATE_DISCONNECTED;

}

void CIRC::NextRoom()
{
	if(m_ActiveCom >= (unsigned)m_apIRCComs.size()-1)
		SetActiveCom(0U);
	else
		SetActiveCom(m_ActiveCom+1);
}

void CIRC::PrevRoom()
{
	if (m_ActiveCom <= 0)
		SetActiveCom((unsigned)m_apIRCComs.size()-1);
	else
		SetActiveCom(m_ActiveCom-1);
}

void CIRC::OpenQuery(const char *to)
{
	char aSanNick[25];
	str_copy(aSanNick, to, sizeof(aSanNick));

	CIRCCom *pCom = GetCom(aSanNick);
	if(pCom)
		SetActiveCom(pCom);
	else
		OpenCom(CIRCCom::TYPE_QUERY, aSanNick);
}

CIRCCom* CIRC::OpenCom(int Type, const char *pName, bool SwitchTo, int UnreadMessages)
{
	if(dbg_assert_strict(Type == CIRCCom::TYPE_CHANNEL || Type == CIRCCom::TYPE_QUERY, "invalid type"))
		return NULL;

	CIRCCom *pNewCom = Type == CIRCCom::TYPE_CHANNEL
					   ? static_cast<CIRCCom *>(new CComChan())
					   : static_cast<CIRCCom *>(new CComQuery());

	pNewCom->m_NumUnreadMsg = UnreadMessages;
	str_copy(pNewCom->m_aName, pName, sizeof(pNewCom->m_aName));
	m_apIRCComs.add(pNewCom);

	if(SwitchTo)
		SetActiveCom(m_apIRCComs.size()-1U);

	return pNewCom;
}

void CIRC::CloseCom(unsigned index)
{
	dbg_assert(index >= 1U && index < (unsigned)m_apIRCComs.size(), "CIRC::CloseCom called with index out of range");
	dbg_assert(m_apIRCComs[index] != NULL, "invalid pointer in CIRC::m_apIRCComs");

	delete m_apIRCComs[index];
	m_apIRCComs.remove_index_fast(index);

	SetActiveCom(index-1U);
}

void CIRC::CloseCom(CIRCCom *pCom)
{
	dbg_assert(pCom != NULL, "CIRC::CloseCom called with nullptr");

	for(int i = 0; i < m_apIRCComs.size(); i++)
	{
		if(m_apIRCComs[i] == pCom)
		{
			CloseCom((unsigned)i);
			return;
		}
	}

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "CIRC::CloseCom called with invalid pointer %p (got a total of %i coms)", pCom, m_apIRCComs.size());
	dbg_assert(false, aBuf);
}

void CIRCCom::AddMessage_nofmt(const char *msg)
{
	AddMessage("%s", msg);
}

void CIRCCom::AddMessage(const char *fmt, ...)
{
	if(!fmt || fmt[0] == 0)
		return;

	char aTime[32];
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	str_format(aTime, sizeof(aTime), "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

	va_list args;
	char aMsg[768];

	va_start(args, fmt);
	#if defined(CONF_FAMILY_WINDOWS)
		_vsnprintf(aMsg, sizeof(aMsg), fmt, args);
	#else
		vsnprintf(aMsg, sizeof(aMsg), fmt, args);
	#endif
	va_end(args);

	m_Buffer.push_back(std::string(aTime) + std::string(aMsg));
}

void CIRC::JoinTo(const char *to, const char *pass)
{
	SendRaw("JOIN %s %s", to, pass);
}

void CIRC::SetMode(const char *mode, const char *to)
{
	CIRCCom *pCom = GetActiveCom();
	if (!pCom || pCom->GetType() == CIRCCom::TYPE_QUERY)
		return;

	CComChan *pChan = dynamic_cast<CComChan*>(pCom);
	if (!pChan)
		return;

	if (!to || to[0] == 0)
		SendRaw("MODE %s %s %s", pChan->m_aName, mode, m_Nick.c_str());
	else
		SendRaw("MODE %s %s %s", pChan->m_aName, mode, to);
}

void CIRC::SetTopic(const char *topic)
{
	CIRCCom *pCom = GetActiveCom();
	if (!pCom || pCom->GetType() != CIRCCom::TYPE_CHANNEL)
		return;

	CComChan *pChan = dynamic_cast<CComChan*>(pCom);
	SendRaw("TOPIC %s :%s", pChan->m_aName, topic);
}

void CIRC::Part(const char *pReason, CIRCCom *pCom)
{
	if(!pCom)
		pCom = GetCom(m_ActiveCom);

	dbg_assert(pCom != NULL, "called CIRC::Part with pCom==nullptr");

	if(!CanCloseCom(pCom))
	{
		dbg_msg("IRC", "cannot part from '%s'", pCom->m_aName);
		return;
	}

	if(pCom->GetType() == CIRCCom::TYPE_CHANNEL)
	{
		if(pReason && pReason[0])
			SendRaw("PART %s :%s", pCom->m_aName, pReason);
		else
			SendRaw("PART %s :Left", pCom->m_aName);
	}

	CloseCom(pCom);
}

void CIRC::Disconnect(const char *pReason)
{
	if (m_State == STATE_DISCONNECTED)
	{
		dbg_msg("IRC", "WARNING: tried to disconnect while already disconnected!");
		return;
	}

	if (m_State == STATE_CONNECTED)
	{
		if(pReason && pReason[0])
		{
			SendRaw("QUIT :%s", pReason);
			m_apIRCComs.delete_all();
		}
		else
			SendRaw("QUIT");
	}

	m_State = STATE_DISCONNECTED;
}

void CIRC::SendMsg(const char *to, const char *msg, int type)
{ //TODO: Rework this! duplicate code :P
	if (GetState() == STATE_DISCONNECTED || !msg || msg[0] == 0)
		return;

	char aBuff[1024];
	char aDest[25];

	// Search Destination
	if (!to || to[0] == 0)
	{
		CIRCCom *pCom = GetActiveCom();
		if (pCom->GetType() == CIRCCom::TYPE_CHANNEL)
		{
			CComChan *pChan = dynamic_cast<CComChan*>(pCom);
			str_copy(aDest, pChan->m_aName, sizeof(aDest));
		}
		else if (pCom->GetType() == CIRCCom::TYPE_QUERY)
		{
			CComQuery *pQuery = dynamic_cast<CComQuery*>(pCom);
			if (str_comp_nocase(pQuery->m_aName, "@Status") == 0)
			{
				pQuery->AddMessage_nofmt("*** You can't send messages to '@Status'!");
				return;
			}

			str_copy(aDest, pQuery->m_aName, sizeof(aDest));
		}
		else
			return;
	}
	else
		str_copy(aDest, to, sizeof(aDest));


	// Send
	if (type == MSG_TYPE_ACTION)
		str_format(aBuff, sizeof(aBuff), "\1ACTION %s\1", msg);
	else
		str_copy(aBuff, msg, sizeof(aBuff));

	SendRaw("PRIVMSG %s :%s", aDest, aBuff);
	CIRCCom *pCom = GetCom(aDest);
	if(pCom)
	{
		if (type == MSG_TYPE_ACTION)
			str_format(aBuff, sizeof(aBuff),"* %s %s", GetNick(), msg);
		else
			str_format(aBuff, sizeof(aBuff),"<%s> %s", GetNick(), msg);
		pCom->AddMessage_nofmt(aBuff);
	}
}

void CIRC::SendMsgLua(const char *to, const char *msg)
{
	int Type = MSG_TYPE_NORMAL;
	if(str_comp_nocase_num(msg, "/me", 3) == 0)
	{
		Type = MSG_TYPE_ACTION;
		if(str_length(msg) == 3)
			msg = " ";
		else
			msg += 4;
	}
	SendMsg(to, msg, Type);
}


void CIRC::SendRaw(const char *fmt, ...)
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
	net_tcp_send(m_Socket, msg, str_length(msg)); // HERE MIGHT APPEAR A SIGPIPE >.<
}

void CIRC::SetNick(const char *pNick)
{
	char aBuf[64] = {0};
	str_copy(aBuf, pNick, sizeof(aBuf));
	str_irc_sanitize(aBuf);
	pNick = aBuf;
	if(*pNick == '\0')
		pNick = "nameless dennis";

	if (m_State == STATE_CONNECTED)
	{
		SendRaw("NICK %s", pNick);
	}

	m_Nick = pNick;
}

void CIRC::SetAway(bool state, const char *msg)
{
	if (state)
		SendRaw("AWAY :%s", msg);
	else
		SendRaw("AWAY");
}

int CIRC::GetMsgType(const char *msg)
{
	int len = str_length(msg);
	if (len > 0 && msg[0] == '\1' && msg[len-1] == '\1')
	{
		char aCmd[12];
		mem_zero(aCmd, sizeof(aCmd));
		for(int i = 1, e = 0; i < len && msg[i] != ' ' && e < (int)sizeof(aCmd); i++, e++)
			aCmd[e] = msg[i];

		if (str_comp_nocase(aCmd, "ACTION") == 0)
			return MSG_TYPE_ACTION;
		else if (str_comp_nocase(aCmd, "TWSERVER") == 0)
			return MSG_TYPE_TWSERVER;
		else if (str_comp_nocase(aCmd, "GETTWSERVER") == 0)
			return MSG_TYPE_GET_TWSERVER;

		return MSG_TYPE_CTCP;
	}

	return MSG_TYPE_NORMAL;
}

void CIRC::SendServer(const char *to, const char *Token)
{
	const char *curAddr = m_pClient->GetCurrentServerAddress();
	SendRaw("PRIVMSG %s :\1TWSERVER %s %s\1", to, Token, g_Config.m_ClIRCAllowJoin ? ((curAddr&&curAddr[0]!=0)?curAddr:"NONE") : "FORBIDDEN");
}

void CIRC::SendGetServer(const char *to)
{
	str_format(m_CmdToken, sizeof(m_CmdToken), "%i", rand());
	SendRaw("PRIVMSG %s :\1GETTWSERVER %s\1", to, m_CmdToken);
}

void CIRC::SendVersion(const char *to)
{
	SendRaw("NOTICE %s :VERSION AllTheHaxx '%s%s' on %s-%s-%s; DDNet v%i; Teeworlds %s (%s); %s built on %s", to,
			ALLTHEHAXX_VERSION, "",
			CONF_FAMILY_STRING, CONF_PLATFORM_STRING, CONF_ARCH_STRING,
			CLIENT_VERSIONNR, GAME_VERSION, GAME_NETVERSION,
	#if defined(CONF_DEBUG)
			"debug"
	#else
			"release"
	#endif
			,BUILD_DATE
	);
}

void CIRC::ExecuteCommand(const char *cmd, char *params)
{
	array<std::string> CmdListParams;
	for (char *p = strtok(params, " "); p != NULL; p = strtok(NULL, " "))
		CmdListParams.add(p);

	if (str_comp_nocase(cmd, "join") == 0 || str_comp_nocase(cmd, "j") == 0)
	{
		if (CmdListParams.empty())
			return;

		JoinTo(CmdListParams[0].c_str(), (CmdListParams.size() > 1)?CmdListParams[1].c_str():"");
	}
	else if (str_comp_nocase(cmd, "query") == 0 || str_comp_nocase(cmd, "q") == 0)
	{
		if (CmdListParams.empty())
			return;

		OpenQuery(CmdListParams[0].c_str());
	}
	else if (str_comp_nocase(cmd, "squery") == 0 || str_comp_nocase(cmd, "sq") == 0)
	{
		if (CmdListParams.empty())
			return;

		SendGetServer(CmdListParams[0].c_str());
	}
	else if (str_comp_nocase(cmd, "topic") == 0 || str_comp_nocase(cmd, "t") == 0)
	{
		if (CmdListParams.empty())
			return;

		SetTopic(params);
	}
	else if (str_comp_nocase(cmd, "part") == 0 || str_comp_nocase(cmd, "p") == 0)
	{
		Part();
	}
	else if (str_comp_nocase(cmd, "nick") == 0)
	{
		if (CmdListParams.empty())
			return;

		SetNick(CmdListParams[0].c_str());
	}
	else if (str_comp_nocase(cmd, "op") == 0)
	{
		if (!CmdListParams.empty())
			SetMode("+o", CmdListParams[0].c_str());
		else
			SetMode("+o", 0x0);
	}
	else if (str_comp_nocase(cmd, "deop") == 0)
	{
		if (!CmdListParams.empty())
			SetMode("-o", CmdListParams[0].c_str());
		else
			SetMode("-o", 0x0);
	}
	else if (str_comp_nocase(cmd, "voz") == 0)
	{
		if (!CmdListParams.empty())
			SetMode("+v", CmdListParams[0].c_str());
		else
			SetMode("+v", 0x0);
	}
	else if (str_comp_nocase(cmd, "devoz") == 0)
	{
		if (!CmdListParams.empty())
			SetMode("-v", CmdListParams[0].c_str());
		else
			SetMode("-v", 0x0);
	}
	else if (str_comp_nocase(cmd, "clear") == 0)
	{
		CIRCCom *pCom = GetActiveCom();
		if (pCom)
			pCom->m_Buffer.clear();
	}
	else if (str_comp_nocase(cmd, "away") == 0)
	{
		m_IsAway ^= true;
		if (!CmdListParams.empty())
			SetAway(m_IsAway, CmdListParams[0].c_str());
		else
			SetAway(m_IsAway);
	}
	else if (str_comp_nocase(cmd, "msg") == 0)
	{
		if (CmdListParams.size() >= 2)
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "PRIVMSG %s :%s",
					CmdListParams[0].c_str(), CmdListParams[1].c_str()); // to & what
			CmdListParams.remove_index(0); // pop twice
			CmdListParams.remove_index(0); //   -> the first two arguments
			while(!CmdListParams.empty()) // add all other arguments
			{
				str_append(aBuf, " ", sizeof(aBuf));
				str_append(aBuf, CmdListParams[0].c_str(), sizeof(aBuf));
				CmdListParams.remove_index(0);
			}
			SendRaw(aBuf);
		}
	}
	else if (str_comp_nocase(cmd, "ctcp") == 0)
	{
		if (CmdListParams.size() >= 2)
		{
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "PRIVMSG %s :\1%s",
					CmdListParams[0].c_str(), CmdListParams[1].c_str()); // to & what
			CmdListParams.remove_index(0); // pop twice
			CmdListParams.remove_index(0); //   -> the first two arguments
			while(!CmdListParams.empty()) // add all other arguments
			{
				str_append(aBuf, " ", sizeof(aBuf));
				str_append(aBuf, CmdListParams[0].c_str(), sizeof(aBuf));
				CmdListParams.remove_index(0);
			}
			aBuf[str_length(aBuf)] = '\1';
			SendRaw(aBuf);
		}
	}
	else if(str_comp_nocase(cmd, "me") == 0)
	{
		if (!CmdListParams.empty())
		{
			char aMsg[768];
			str_format(aMsg, sizeof(aMsg), "%s", CmdListParams[0].c_str()); // first word
			CmdListParams.remove_index(0); // pop
			while(!CmdListParams.empty()) // add all other arguments to the message
			{
				str_append(aMsg, " ", sizeof(aMsg));
				str_append(aMsg, CmdListParams[0].c_str(), sizeof(aMsg));
				CmdListParams.remove_index(0);
			}

			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "PRIVMSG %s :\1ACTION %s",
					GetActiveCom()->GetType() == CIRCCom::TYPE_QUERY ?
							((CComQuery*)GetActiveCom())->m_aName : ((CComChan*)GetActiveCom())->m_aName,
					aMsg); // message text


			aBuf[str_length(aBuf)] = '\1';
			SendRaw(aBuf);
			GetActiveCom()->AddMessage("* %s %s", m_Nick.c_str(), aMsg);
		}
	}
	else if(str_comp_nocase(cmd, "/debug") == 0)
	{
		m_Debug ^= true;
		GetActiveCom()->AddMessage("[[ SYSTEM ]] %sing debug mode", m_Debug ? "Enter" : "Leave");
	}
	else
		SendRaw("%s %s", cmd, params);
}

int CIRC::NumUnreadMessages(int *pArray)
{
	int NumChan = 0, NumQuery = 0;
	for(unsigned i = 0; i < GetNumComs(); i++)
	{
		CIRCCom *pCom = GetCom(i);
		if(pCom->GetType() == CIRCCom::TYPE_CHANNEL)
			NumChan += ((CComChan *)pCom)->m_NumUnreadMsg;
		else if(pCom->GetType() == CIRCCom::TYPE_QUERY)
			NumQuery += ((CComQuery *)pCom)->m_NumUnreadMsg;
	}

	if(pArray)
	{
		pArray[0] = NumChan;
		pArray[1] = NumQuery;
	}

	return NumChan + NumQuery;
}

LuaRef CIRC::LuaGetUserlist(const char *pChannel, lua_State *L)
{
	CIRCCom *pCom = CLua::m_pCGameClient->m_pIRC->GetCom(std::string(pChannel));
	LuaRef Result(L); // initialises with nil
	if(pCom)
	{
		if(pCom->GetType() == CIRCCom::TYPE_CHANNEL)
		{
			CComChan *pChan = static_cast<CComChan*>(pCom);
			const sorted_array<CComChan::CUser>& Userlist = pChan->m_Users;
			Result = luabridge::newTable(L);
			for(int u = 0; u < Userlist.size(); u++)
				Result.append(Userlist[u].m_Nick);
		}
		else
			Result = pChannel;
	}

	return Result;
}
