/* (c) Redix and Sushi */

#include <stdio.h>

#include <base/system.h>
#include <engine/shared/config.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>

#include "menus.h"
#include "race_demo.h"

CRaceDemo::CRaceDemo()
{
	m_RaceState = RACE_NONE;
	m_RecordStopTime = 0;
	m_Time = 0;
	m_DemoStartTick = 0;
}

void CRaceDemo::Stop()
{
	if(Client()->RaceRecordIsRecording())
		Client()->RaceRecordStop();

	char aFilename[512];
	str_format(aFilename, sizeof(aFilename), "demos/%s_tmp_%d.demo", m_pMap, pid());
	Storage()->RemoveFile(aFilename, IStorageTW::TYPE_SAVE);

	m_Time = 0;
	m_RaceState = RACE_NONE;
	m_RecordStopTime = 0;
	m_DemoStartTick = 0;
}

void CRaceDemo::OnStateChange(int NewState, int OldState)
{
	if(OldState == IClient::STATE_ONLINE)
		Stop();
}

void CRaceDemo::OnRender()
{
	if(!g_Config.m_ClAutoRaceRecord || !m_pClient->m_Snap.m_pGameInfoObj || m_pClient->m_Snap.m_SpecInfo.m_Active || Client()->State() != IClient::STATE_ONLINE)
		return;

	// start the demo
	if(m_DemoStartTick < Client()->GameTick())
	{
		bool start = false;
		std::list < int > Indices = m_pClient->Collision()->GetMapIndices(m_pClient->m_PredictedPrevChar.m_Pos, m_pClient->m_LocalCharacterPos);
		if(!Indices.empty())
			for(std::list < int >::iterator i = Indices.begin(); i != Indices.end(); i++)
			{
				if(m_pClient->Collision()->GetTileIndex(*i) == TILE_BEGIN) start = true;
				if(m_pClient->Collision()->GetFTileIndex(*i) == TILE_BEGIN) start = true;
			}
		else
		{
			if(m_pClient->Collision()->GetTileIndex(m_pClient->Collision()->GetPureMapIndex(m_pClient->m_LocalCharacterPos)) == TILE_BEGIN) start = true;
			if(m_pClient->Collision()->GetFTileIndex(m_pClient->Collision()->GetPureMapIndex(m_pClient->m_LocalCharacterPos)) == TILE_BEGIN) start = true;
		}

		if(start)
		{
			OnReset();
			char aBuf[512];
			str_format(aBuf, sizeof(aBuf), "tmp_%d", pid());
			m_pMap = Client()->RaceRecordStart(aBuf);
			m_DemoStartTick = Client()->GameTick() + Client()->GameTickSpeed();
			m_RaceState = RACE_STARTED;
		}
	}

	// stop the demo
	if(m_RaceState == RACE_FINISHED && m_RecordStopTime < Client()->GameTick() && m_Time > 0)
	{
		CheckDemo();
		OnReset();
	}
}

void CRaceDemo::OnReset()
{
	if(Client()->State() == IClient::STATE_ONLINE)
		Stop();
}

void CRaceDemo::OnShutdown()
{
	Stop();
}

void CRaceDemo::OnMessage(int MsgType, void *pRawMsg)
{
	if(!g_Config.m_ClAutoRaceRecord || Client()->State() != IClient::STATE_ONLINE || m_pClient->m_Snap.m_SpecInfo.m_Active)
		return;

	// check for messages from server
	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalClientID && m_RaceState == RACE_FINISHED)
		{
			// check for new record
			CheckDemo();
			OnReset();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(pMsg->m_ClientID == -1 && m_RaceState == RACE_STARTED)
		{
			char aName[MAX_NAME_LENGTH];
			const char *pFinished = str_find(pMsg->m_pMessage, " finished in: ");
			int FinishedPos = pFinished - pMsg->m_pMessage;
			if (!pFinished || FinishedPos == 0 || FinishedPos >= (int)sizeof(aName))
				return;
			
			// store the name
			str_copy(aName, pMsg->m_pMessage, FinishedPos + 1);

			// prepare values and state for saving
			int Minutes;
			float Seconds;
			if(!str_comp(aName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName) && sscanf(pFinished, " finished in: %d minute(s) %f", &Minutes, &Seconds) == 2)
			{
				m_RaceState = RACE_FINISHED;
				m_RecordStopTime = Client()->GameTick() + Client()->GameTickSpeed();
				m_Time = Minutes*60 + Seconds;
			}
		}
	}
}

void CRaceDemo::CheckDemo()
{
	// stop the demo recording
	Client()->RaceRecordStop();

	char aTmpDemoName[128];
	str_format(aTmpDemoName, sizeof(aTmpDemoName), "%s_tmp_%d", m_pMap, pid());

	// loop through demo files
	m_pClient->m_pMenus->DemolistPopulate();
	for(int i = 0; i < m_pClient->m_pMenus->m_lDemos.size(); i++)
	{
		if(!str_comp_num(m_pClient->m_pMenus->m_lDemos[i].m_aName, m_pMap, str_length(m_pMap)) && str_comp_num(m_pClient->m_pMenus->m_lDemos[i].m_aName, aTmpDemoName, str_length(aTmpDemoName)) && str_length(m_pClient->m_pMenus->m_lDemos[i].m_aName) > str_length(m_pMap) && m_pClient->m_pMenus->m_lDemos[i].m_aName[str_length(m_pMap)] == '_')
		{
			const char *pDemo = m_pClient->m_pMenus->m_lDemos[i].m_aName;

			// set cursor
			pDemo += str_length(m_pMap)+1;
			float DemoTime = str_tofloat(pDemo);
			if(m_Time < DemoTime)
			{
				// save new record
				SaveDemo(m_pMap);

				// delete old demo
				char aFilename[512];
				str_format(aFilename, sizeof(aFilename), "demos/%s.demo", m_pClient->m_pMenus->m_lDemos[i].m_aName);
				Storage()->RemoveFile(aFilename, IStorageTW::TYPE_SAVE);
			}

			m_Time = 0;

			return;
		}
	}

	// save demo if there is none
	SaveDemo(m_pMap);

	m_Time = 0;
}

void CRaceDemo::SaveDemo(const char* pDemo)
{
	char aNewFilename[512];
	char aOldFilename[512];
	if(g_Config.m_ClDemoName)
	{
		char aPlayerName[MAX_NAME_LENGTH];
		str_copy(aPlayerName, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName, sizeof(aPlayerName));

		// check the player name
		for(int i = 0; i < MAX_NAME_LENGTH; i++)
		{
			if(!aPlayerName[i])
				break;

			if(aPlayerName[i] == '\\' || aPlayerName[i] == '/' || aPlayerName[i] == '|' || aPlayerName[i] == ':' || aPlayerName[i] == '*' || aPlayerName[i] == '?' || aPlayerName[i] == '<' || aPlayerName[i] == '>' || aPlayerName[i] == '"')
				aPlayerName[i] = '%';

			str_format(aNewFilename, sizeof(aNewFilename), "demos/%s_%5.2f_%s.demo", pDemo, m_Time, aPlayerName);
		}
	}
	else
		str_format(aNewFilename, sizeof(aNewFilename), "demos/%s_%5.2f.demo", pDemo, m_Time);

	str_format(aOldFilename, sizeof(aOldFilename), "demos/%s_tmp_%d.demo", m_pMap, pid());

	Storage()->RenameFile(aOldFilename, aNewFilename, IStorageTW::TYPE_SAVE);

	dbg_msg("racedemo", "saved better demo");
}
