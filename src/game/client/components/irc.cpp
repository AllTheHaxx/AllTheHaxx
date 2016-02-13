#include <base/system.h>
#include <base/math.h>
#include <engine/shared/config.h>

#if defined(CONF_FAMILY_WINDOWS)
	#include <windows.h>
#endif

#include "console.h"
#include "hud.h"
#include "irc.h"

CIRC::CIRC()
{
	m_pIRCThread = 0;
	OnReset();
}

void CIRC::OnRender()
{
}

void CIRC::ListenIRCThread(void *pUser)
{
	//CIRC *pData = (CIRC *)pUser;
	return;
}

void CIRC::AddLine(int Type, const char *pNick, const char *pLine)
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

void CIRC::AddLine(const char *pLine)
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


void CIRC::SendChat(const char* pMsg)
{
	char aBuf[510];
	str_format(aBuf, sizeof(aBuf), "PRIVMSG #AllTheHaxx :%s", pMsg);
	// TODO!
}

void CIRC::SendRaw(const char* pMsg)
{
	char aBuf[510];
	str_format(aBuf, sizeof(aBuf), "%s", pMsg+1);
	// TODO!
}

void CIRC::Connect()
{
	if(IsConnected())
		return;

	// TODO!
}

void CIRC::Disconnect(char *pReason)
{
	if(!IsConnected())
		return;

	// TODO!
}

void CIRC::SendRequestUserList()
{
	m_UserList.clear();
	// XXX!
}

void CIRC::SendNickChange(const char *pNewNick)
{
	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%s", pNewNick);
	// TODO!
}

void CIRC::OnConsoleInit()
{
	//Connect();
}

void CIRC::OnReset()
{
}

void CIRC::OnShutdown()
{
	Disconnect(g_Config.m_ClIRCLeaveMsg);
}
