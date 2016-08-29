#include <base/system.h>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/client/irc.h>

#include "console.h"
#include "hud.h"
#include <engine/shared/jobs.h> // sounds.h needs this
#include "sounds.h"
#include "irc.h"


int IRChook_join(IIRC::ReplyData* hostd, void* user, void* engine)
{
	CIRCBind *pData = (CIRCBind *)user;

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "[%s] %s joined the chat", hostd->channel.c_str(), hostd->from.c_str());
	pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
	return 0;
}

int IRChook_leave(IIRC::ReplyData* hostd, void* user, void* engine) // serves for both QUIT and PART
{
	CIRCBind *pData = (CIRCBind *)user;

	char aBuf[64];
	if(hostd->params.c_str()[0])
		str_format(aBuf, sizeof(aBuf), "[%s] %s left the chat (%s)", hostd->channel.c_str(), hostd->from.c_str(), hostd->params.c_str());
	else
		str_format(aBuf, sizeof(aBuf), "[%s] %s left the chat", hostd->channel.c_str(), hostd->from.c_str());
	pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
	return 0;
}

int IRChook_privmsg(IIRC::ReplyData* hostd, void* user, void* engine)
{
	CIRCBind *pData = (CIRCBind *)user;
	CIRC *pIRC = (CIRC *)engine;

	// nothing to do for control messages
	if(pIRC->GetMsgType(hostd->params.c_str()) != IIRC::MSG_TYPE_NORMAL ||
			hostd->from.length() == 0 || hostd->channel.length() == 0)
		return 0;

	// play a sound
	if(g_Config.m_SndIRC)
		pData->GameClient()->m_pSounds->Play(CSounds::CHN_GUI, SOUND_IRC_MESSAGE, 1.0f);

	// print chat message....
	if(g_Config.m_ClIRCPrintChat)
	{
		char aBuf[256], aTime[32];
		time_t rawtime;
		struct tm *timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		str_format(aTime, sizeof(aTime), "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		// ...to console
		if(hostd->channel == pIRC->m_Nick)
			str_format(aBuf, sizeof(aBuf), "[private chat]: %s<%s> %s", aTime, hostd->from.c_str(), hostd->params.c_str());
		else
			str_format(aBuf, sizeof(aBuf), "[chat]: [%s]: %s<%s> %s", hostd->channel.c_str(), aTime, hostd->from.c_str(), hostd->params.c_str());
		pData->GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "IRC", aBuf, false);

		// ...to notifications
		if(g_Config.m_ClNotifications)
		{
			if(hostd->channel == pIRC->m_Nick)
				str_format(aBuf, sizeof(aBuf), "[%s]: %s", hostd->from.c_str(), hostd->params.c_str());
			else
				str_format(aBuf, sizeof(aBuf), "[%s]: <%s> %s", hostd->channel.c_str(), hostd->from.c_str(), hostd->params.c_str());
			pData->GameClient()->m_pHud->PushNotification(aBuf, str_find_nocase(hostd->params.c_str(), pIRC->m_Nick.c_str()) ?
					vec4(0.2f, 1, 0.5f, 1) :
					vec4(0.2f, 0.5f, 1, 1));
		}
	}
	else if(str_find_nocase(hostd->params.c_str(), pIRC->m_Nick.c_str()))
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "[%s] You were mentioned by %s", hostd->channel.c_str(), hostd->from.c_str());
		pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
	}

	return 0;
}

CIRCBind::CIRCBind()
{
	m_pIRCThread = 0;
}

void CIRCBind::OnRender()
{
	static int64 LastConnTry = 0;
	if (g_Config.m_ClIRCAutoconnect && !IsConnected())
	{
		if(time_get() > LastConnTry + 10*time_freq())
		{
//			set_new_tick();
			LastConnTry = time_get();
			Connect();
		}
	}
}

void CIRCBind::ListenIRCThread(void *pUser)
{
	CIRCBind *pIRC = (CIRCBind*)pUser;
	pIRC->m_pClient->IRC()->StartConnection();
	return;
}

void CIRCBind::SendCommand(const char* pCmd)
{
	m_pClient->IRC()->SendRaw(++pCmd);
}

void CIRCBind::Connect() // XXX this is deprecated and only for compatibility
{
	if(IsConnected())
		return;

	m_pClient->IRC()->SetNick(g_Config.m_ClIRCNick);
	thread_init(ListenIRCThread, this);
}

void CIRCBind::Disconnect(char *pReason) // XXX this is deprecated and only for compatibility
{
	if(!IsConnected())
		return;

	m_pClient->IRC()->Disconnect(pReason);
}

void CIRCBind::OnNickChange(const char *pNewNick) // XXX this is deprecated and only for compatibility
{
	m_pClient->IRC()->SetNick(pNewNick);
}

void CIRCBind::OnConsoleInit()
{
	// TODO: register hooks here!
	m_pClient->IRC()->RegisterCallback("JOIN", IRChook_join, this);
	m_pClient->IRC()->RegisterCallback("PART", IRChook_leave, this);
	m_pClient->IRC()->RegisterCallback("QUIT", IRChook_leave, this);
	m_pClient->IRC()->RegisterCallback("PRIVMSG", IRChook_privmsg, this);
}

void CIRCBind::OnReset()
{
}

void CIRCBind::OnShutdown()
{
	Disconnect(g_Config.m_ClIRCLeaveMsg);
}
