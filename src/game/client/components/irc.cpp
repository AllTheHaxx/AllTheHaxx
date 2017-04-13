#include <base/system.h>
#include <base/math.h>
#include <engine/shared/config.h>
#include <engine/client/irc.h>
#include <engine/shared/jobs.h>

#include "console.h"
#include "hud.h"
#include "sounds.h"
#include "irc.h"


int IRChook_join(IIRC::ReplyData* hostd, void* user, void* engine)
{
	CIRCBind *pData = (CIRCBind *)user;

	if(g_Config.m_ClIRCShowJoins != 2 || str_comp(hostd->from.c_str(), pData->GameClient()->IRC()->GetNick()) == 0)
		return 0;

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "[%s] %s joined the chat", hostd->channel.c_str(), hostd->from.c_str());
	pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
	return 0;
}

int IRChook_leave(IIRC::ReplyData* hostd, void* user, void* engine) // serves for both QUIT and PART
{
	CIRCBind *pData = (CIRCBind *)user;
	if(g_Config.m_ClIRCShowJoins != 2)
		return 0;

	char aBuf[64];
	if(hostd->params.c_str()[0])
		str_format(aBuf, sizeof(aBuf), "[%s] %s left the chat (%s)", hostd->channel.c_str(), hostd->from.c_str(), hostd->params.c_str());
	else
		str_format(aBuf, sizeof(aBuf), "[%s] %s left the chat", hostd->channel.c_str(), hostd->from.c_str());
	pData->GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
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

void CIRCBind::OnMessageIRC(const char *pChan, const char *pUser, const char *pText)
{
	// play a sound
	if(g_Config.m_SndIRC)
		GameClient()->m_pSounds->Play(CSounds::CHN_GUI, SOUND_IRC_MESSAGE, 1.0f);

	LUA_FIRE_EVENT("OnMessageIRC", std::string(pChan), std::string(pUser), std::string(pText));

	// print chat message....
//	if(g_Config.m_ClIRCPrintChat)
	{
		char aBuf[256], aTime[32];
		time_t rawtime;
		struct tm *timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		str_format(aTime, sizeof(aTime), "[%02d:%02d:%02d] ", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);

		// ...to console
		if(str_comp(pChan, GameClient()->IRC()->GetNick()) == 0)
			str_format(aBuf, sizeof(aBuf), "[private chat]: %s<%s> %s", aTime, pUser, pText);
		else
			str_format(aBuf, sizeof(aBuf), "[chat]: [%s]: %s<%s> %s", pChan, aTime, pUser, pText);
		GameClient()->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "IRC", aBuf, false);

		// ...to notifications
		if(g_Config.m_ClNotifications)
		{
			if(str_comp(pChan, GameClient()->IRC()->GetNick()) == 0) // private chat
				str_format(aBuf, sizeof(aBuf), "[%s]: %s", pUser, pText);
			else
				str_format(aBuf, sizeof(aBuf), "[%s]: <%s> %s", pChan, pUser, pText);
			GameClient()->m_pHud->PushNotification(aBuf, str_find_nocase(pText, GameClient()->IRC()->GetNick()) ?
																vec4(0.2f, 1, 0.5f, 1) :
																vec4(0.2f, 0.5f, 1, 1));
		}
		else if(str_find_nocase(pText, GameClient()->IRC()->GetNick()))
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "[%s] You were mentioned by %s", pChan, pUser);
			GameClient()->m_pHud->PushNotification(aBuf, vec4(0.2f, 1, 0.2f, 1));
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

void CIRCBind::Connect()
{
	if(IsConnected())
		return;

	m_pClient->IRC()->SetNick(g_Config.m_ClIRCNick);
	thread_init_named(ListenIRCThread, this, "IRC listener");
}

void CIRCBind::OnNickChange(const char *pNewNick)
{
	m_pClient->IRC()->SetNick(pNewNick);
}

void CIRCBind::OnConsoleInit()
{
	// TODO: register hooks here!
	m_pClient->IRC()->RegisterCallback("JOIN", IRChook_join, this);
	m_pClient->IRC()->RegisterCallback("PART", IRChook_leave, this);
	m_pClient->IRC()->RegisterCallback("QUIT", IRChook_leave, this);
}

void CIRCBind::OnShutdown()
{
	if(IsConnected())
		m_pClient->IRC()->Disconnect(g_Config.m_ClIRCLeaveMsg);
}
