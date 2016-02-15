#include <base/system.h>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/client/irc.h>

#include "console.h"
#include "hud.h"
#include "irc.h"


int irchook_join(IIrc::ReplyData* hostd, void* user)
{
	CIrcBind *pData = (CIrcBind *)user;

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "[%s] %s joined the chat", hostd->channel.c_str(), hostd->from.c_str());
	pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
	return 0;
}

int irchook_leave(IIrc::ReplyData* hostd, void* user) // serves for both QUIT and PART
{
	CIrcBind *pData = (CIrcBind *)user;

	char aBuf[64];
	if(hostd->params.c_str()[0])
		str_format(aBuf, sizeof(aBuf), "[%s] %s left the chat (%s)", hostd->channel.c_str(), hostd->from.c_str(), hostd->params.c_str());
	else
		str_format(aBuf, sizeof(aBuf), "[%s] %s left the chat", hostd->channel.c_str(), hostd->from.c_str());
	pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
	return 0;
}

int irchook_privmsg(IIrc::ReplyData* hostd, void* user)
{
	CIrcBind *pData = (CIrcBind *)user;

	if(str_find_nocase(hostd->params.c_str(), pData->GameClient()->Irc()->GetNick()))
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "[%s] You were mentioned by %s", hostd->channel.c_str(), hostd->from.c_str());
		pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
	}
	return 0;
}

CIrcBind::CIrcBind()
{
	m_pIRCThread = 0;
	OnReset();
}

void CIrcBind::OnRender()
{
}

void CIrcBind::ListenIRCThread(void *pUser)
{
	CIrcBind *pIrc = (CIrcBind*)pUser;
	pIrc->m_pClient->Irc()->StartConnection();
	return;
}

void CIrcBind::AddLine(int Type, const char *pNick, const char *pLine) // TODO: reimplement!
{
/*	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char aBuf[530];
	if(Type == IRC_LINETYPE_CHAT)
		str_format(aBuf, sizeof(aBuf), "[%02d:%02d:%02d][%s]: %s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, pNick, pLine);
	else if(Type == IRC_LINETYPE_NOTICE)
	{
		if(pNick)
			str_format(aBuf, sizeof(aBuf), "[%02d:%02d:%02d] -%s-: %s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, pNick, pLine);
		else
			str_format(aBuf, sizeof(aBuf), "[%02d:%02d:%02d] %s", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, pLine+5);
	}
	GameClient()->m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_IRC, aBuf);*/
}

void CIrcBind::AddLine(const char *pLine) // TODO: reimplement!
{
/*	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char aBuf[530];
	str_format(aBuf, sizeof(aBuf), "[%02d:%02d:%02d] *** %s",
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, pLine);
	GameClient()->m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_IRC, aBuf);*/
}


void CIrcBind::SendChat(const char* pMsg) // XXX this is depreciated and only for compatibility
{
	m_pClient->Irc()->SendMsg("#AllTheHaxx", pMsg, IIrc::MSG_TYPE_NORMAL);
}

void CIrcBind::SendCommand(const char* pCmd)
{
	m_pClient->Irc()->SendRaw(++pCmd);
}

void CIrcBind::Connect() // XXX this is depreciated and only for compatibility
{
	if(IsConnected())
		return;

    m_pClient->Irc()->SetNick(g_Config.m_ClIRCNick);
	thread_init(ListenIRCThread, this);
}

void CIrcBind::Disconnect(char *pReason) // XXX this is depreciated and only for compatibility
{
	if(!IsConnected())
		return;

	m_pClient->Irc()->Disconnect(pReason);
}

void CIrcBind::SendNickChange(const char *pNewNick) // XXX this is depreciated and only for compatibility
{
	m_pClient->Irc()->SetNick(pNewNick);
}

void CIrcBind::OnConsoleInit()
{
	// TODO: register hooks here!
	m_pClient->Irc()->RegisterCallback("JOIN", irchook_join, this);
	m_pClient->Irc()->RegisterCallback("PART", irchook_leave, this);
	m_pClient->Irc()->RegisterCallback("QUIT", irchook_leave, this);
	m_pClient->Irc()->RegisterCallback("PRIVMSG", irchook_privmsg, this);

	if(g_Config.m_ClIRCAutoconnect)
		Connect();
}

void CIrcBind::OnReset()
{
}

void CIrcBind::OnShutdown()
{
	Disconnect(g_Config.m_ClIRCLeaveMsg);
}
