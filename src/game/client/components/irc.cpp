#include <base/system.h>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/client/irc.h>

#include "console.h"
#include "hud.h"
#include "irc.h"

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

void CIrcBind::AddLine(int Type, const char *pNick, const char *pLine)
{
	time_t rawtime;
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
	GameClient()->m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_IRC, aBuf);
}

void CIrcBind::AddLine(const char *pLine)
{
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char aBuf[530];
	str_format(aBuf, sizeof(aBuf), "[%02d:%02d:%02d] *** %s",
			timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec, pLine);
	GameClient()->m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_IRC, aBuf);
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
