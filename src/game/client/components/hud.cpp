/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <engine/client/irc.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <game/layers.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/client/render.h>
#include <game/client/components/scoreboard.h>

#include "binds.h"
#include "camera.h"
#include "chat.h"
#include "controls.h"
#include "voting.h"
#include "hud.h"

CHud::CHud()
{
	// won't work if zero
	m_AverageFPS = 1.0f;
	OnReset();
}

void CHud::OnReset()
{
	m_CheckpointDiff = 0.0f;
	m_DDRaceTime = 0;
	m_LastReceivedTimeTick = 0;
	m_CheckpointTick = 0;
	m_DDRaceTick = 0;
	m_FinishTime = false;
	m_DDRaceTimeReceived = false;
	m_ServerRecord = -1.0f;
	m_PlayerRecord = -1.0f;
	m_Notifications.clear();
	m_Notifications.hint_size(MAX_NOTIFICATIONS>>1);
	m_MaxHealth = 10;
	m_MaxArmor = 10;
	m_MaxAmmo = 10;
}

void CHud::RenderGameTimer()
{
	CALLSTACK_ADD();

	float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;

	if(!(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_SUDDENDEATH))
	{
		char aBuf[32];
		int Time = 0;
		if(m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit && (m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer <= 0))
		{
			Time = m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit*60 - ((Client()->GameTick()-m_pClient->m_Snap.m_pGameInfoObj->m_RoundStartTick)/Client()->GameTickSpeed());

			if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
				Time = 0;
		}
		else if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_RACETIME)
		{
			//The Warmup timer is negative in this case to make sure that incompatible clients will not see a warmup timer
			Time = (Client()->GameTick()+m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer)/Client()->GameTickSpeed();
		}
		else
			Time = (Client()->GameTick()-m_pClient->m_Snap.m_pGameInfoObj->m_RoundStartTick)/Client()->GameTickSpeed();

		CServerInfo Info;
		Client()->GetServerInfo(&Info);

		// render the time in a nice format
		str_clock_secb(aBuf, Time);

		if(g_Config.m_ClShowDecisecs)
		{
			char aTmp[4];
			str_format(aTmp, sizeof(aTmp), ".%d", m_DDRaceTick/10);
			str_append(aBuf, aTmp, sizeof(aBuf));
		}

		float FontSize = 10.0f;
		float w = TextRender()->TextWidth(0, FontSize, aBuf, -1);

		// last 60 sec red, last 10 sec blink
		if(m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit && Time <= 60 && (m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer <= 0))
		{
			float Alpha = Time <= 10 && (2*time_get()/time_freq()) % 2 ? 0.5f : 1.0f;
			TextRender()->TextColor(1.0f, 0.25f, 0.25f, Alpha);
		}
		TextRender()->Text(0, Half-w/2, 2.0f, FontSize, aBuf, -1);

		// render the record
		if(g_Config.m_ClShowhudServerRecord && ((IsRace(&Info) || IsDDRace(&Info)) && m_ServerRecord >= 0/*m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_RACETIME*/))
		{
			FontSize -= 3.0f;
			Time = round_to_int(m_ServerRecord);
			char aRecord[128];
			if(Time >= 60*60*24)
				str_format(aRecord, sizeof(aRecord), "%d %s, %02d:%02d:%02d", Time/60/60/24, Time/60/60/24 == 1 ? Localize("day") : Localize("days"), (Time%86400)/3600, (Time/60)%60, (Time)%60);
			else if(Time >= 60*60)
				str_format(aRecord, sizeof(aRecord), "%02d:%02d:%02d", Time/60/60, (Time/60)%60, Time%60);
			else
				str_format(aRecord, sizeof(aRecord), "%02d:%02d", Time/60, Time%60);

			str_formatb(aBuf, "%s: %s", Localize("Server Record"), aRecord);
			w = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.65f);
			TextRender()->Text(0, Half-w/2, FontSize+  2*3.0f, FontSize, aBuf, -1);
		}

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
}

void CHud::RenderPauseNotification()
{
	CALLSTACK_ADD();

	if((m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED) &&
		!(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
	{
		const char *pText = Localize("Game paused");
		float FontSize = 20.0f;
		float w = TextRender()->TextWidth(0, FontSize,pText, -1);
		TextRender()->Text(0, 150.0f*Graphics()->ScreenAspect()+-w/2.0f, 50.0f, FontSize, pText, -1);
	}
}

void CHud::RenderSuddenDeath()
{
	CALLSTACK_ADD();

	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_SUDDENDEATH)
	{
		float Half = 300.0f*Graphics()->ScreenAspect()/2.0f;
		const char *pText = Localize("Sudden Death");
		float FontSize = 12.0f;
		float w = TextRender()->TextWidth(0, FontSize, pText, -1);
		TextRender()->Text(0, Half-w/2, 2, FontSize, pText, -1);
	}
}

void CHud::RenderScoreHud()
{
	CALLSTACK_ADD();

	// render small score hud
	if(!(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
	{
		int GameFlags = m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags;
		float Whole = 300*Graphics()->ScreenAspect();
		float StartY = 220.0f;

		if(GameFlags&GAMEFLAG_TEAMS && m_pClient->m_Snap.m_pGameDataObj)
		{
			char aScoreTeam[2][32];
			str_format(aScoreTeam[TEAM_RED], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed);
			str_format(aScoreTeam[TEAM_BLUE], sizeof(aScoreTeam)/2, "%d", m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue);
			float aScoreTeamWidth[2] = { TextRender()->TextWidth(0, 14.0f, aScoreTeam[TEAM_RED], -1), TextRender()->TextWidth(0, 14.0f, aScoreTeam[TEAM_BLUE], -1) };
			int FlagCarrier[2] = { m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed, m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue };
			float ScoreWidthMax = max(max(aScoreTeamWidth[TEAM_RED], aScoreTeamWidth[TEAM_BLUE]), TextRender()->TextWidth(0, 14.0f, "100", -1));
			float Split = 3.0f;
			float ImageSize = GameFlags&GAMEFLAG_FLAGS ? 16.0f : Split;

			for(int t = 0; t < 2; t++)
			{
				// draw box
				Graphics()->BlendNormal();
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				if(t == 0)
					Graphics()->SetColor(1.0f, 0.0f, 0.0f, 0.25f);
				else
					Graphics()->SetColor(0.0f, 0.0f, 1.0f, 0.25f);
				RenderTools()->DrawRoundRectExt(Whole-ScoreWidthMax-ImageSize-2*Split, StartY+t*20, ScoreWidthMax+ImageSize+2*Split, 18.0f, 5.0f, CUI::CORNER_L);
				Graphics()->QuadsEnd();

				// draw score
				TextRender()->Text(0, Whole-ScoreWidthMax+(ScoreWidthMax-aScoreTeamWidth[t])/2-Split, StartY+t*20, 14.0f, aScoreTeam[t], -1);

				if(GameFlags&GAMEFLAG_FLAGS)
				{
					int BlinkTimer = (m_pClient->m_FlagDropTick[t] != 0 &&
										(Client()->GameTick()-m_pClient->m_FlagDropTick[t])/Client()->GameTickSpeed() >= 25) ? 10 : 20;
					if(FlagCarrier[t] == FLAG_ATSTAND || (FlagCarrier[t] == FLAG_TAKEN && ((Client()->GameTick()/BlinkTimer)&1)))
					{
						// draw flag
						Graphics()->BlendNormal();
						Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
						Graphics()->QuadsBegin();
						RenderTools()->SelectSprite(t==0?SPRITE_FLAG_RED:SPRITE_FLAG_BLUE);
						IGraphics::CQuadItem QuadItem(Whole-ScoreWidthMax-ImageSize, StartY+1.0f+t*20, ImageSize/2, ImageSize);
						Graphics()->QuadsDrawTL(&QuadItem, 1);
						Graphics()->QuadsEnd();
					}
					else if(FlagCarrier[t] >= 0)
					{
						// draw name of the flag holder
						int ID = FlagCarrier[t]%MAX_CLIENTS;
						const char *pName = m_pClient->m_aClients[ID].m_aName;
						float w = TextRender()->TextWidth(0, 8.0f, pName, -1);
						TextRender()->Text(0, min(Whole-w-1.0f, Whole-ScoreWidthMax-ImageSize-2*Split), StartY+(t+1)*20.0f-3.0f, 8.0f, pName, -1);

						// draw tee of the flag holder
						CTeeRenderInfo Info = m_pClient->m_aClients[ID].m_RenderInfo;
						Info.m_Size = 18.0f;
						RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0),
							vec2(Whole-ScoreWidthMax-Info.m_Size/2-Split, StartY+1.0f+Info.m_Size/2+t*20));
					}
				}
				StartY += 8.0f;
			}
		}
		else
		{
			int Local = -1;
			int aPos[2] = { 1, 2 };
			const CNetObj_PlayerInfo *apPlayerInfo[2] = { 0, 0 };
			int i = 0;
			for(int t = 0; t < 2 && i < MAX_CLIENTS && m_pClient->m_Snap.m_paInfoByScore[i]; ++i)
			{
				if(m_pClient->m_Snap.m_paInfoByScore[i]->m_Team != TEAM_SPECTATORS)
				{
					apPlayerInfo[t] = m_pClient->m_Snap.m_paInfoByScore[i];
					if(apPlayerInfo[t]->m_ClientID == m_pClient->m_Snap.m_LocalClientID)
						Local = t;
					++t;
				}
			}
			// search local player info if not a spectator, nor within top2 scores
			if(Local == -1 && m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
			{
				for(; i < MAX_CLIENTS && m_pClient->m_Snap.m_paInfoByScore[i]; ++i)
				{
					if(m_pClient->m_Snap.m_paInfoByScore[i]->m_Team != TEAM_SPECTATORS)
						++aPos[1];
					if(m_pClient->m_Snap.m_paInfoByScore[i]->m_ClientID == m_pClient->m_Snap.m_LocalClientID)
					{
						apPlayerInfo[1] = m_pClient->m_Snap.m_paInfoByScore[i];
						Local = 1;
						break;
					}
				}
			}
			char aScore[2][32];
			for(int t = 0; t < 2; ++t)
			{
				if(apPlayerInfo[t])
				{
					CServerInfo Info;
					Client()->GetServerInfo(&Info);
					if(IsRace(&Info) && g_Config.m_ClDDRaceScoreBoard)
					{
						if (apPlayerInfo[t]->m_Score != -9999)
							str_format(aScore[t], sizeof(aScore[t]), "%02d:%02d", abs(apPlayerInfo[t]->m_Score)/60, abs(apPlayerInfo[t]->m_Score)%60);
						else
							aScore[t][0] = 0;
					}
					else
						str_format(aScore[t], sizeof(aScore)/2, "%d", apPlayerInfo[t]->m_Score);
				}
				else
					aScore[t][0] = 0;
			}
			float aScoreWidth[2] = {TextRender()->TextWidth(0, 14.0f, aScore[0], -1), TextRender()->TextWidth(0, 14.0f, aScore[1], -1)};
			float ScoreWidthMax = max(max(aScoreWidth[0], aScoreWidth[1]), TextRender()->TextWidth(0, 14.0f, "10", -1));
			float Split = 3.0f, ImageSize = 16.0f, PosSize = 16.0f;

			for(int t = 0; t < 2; t++)
			{
				// draw box
				Graphics()->BlendNormal();
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				if(t == Local)
					Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
				else
					Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
				RenderTools()->DrawRoundRectExt(Whole-ScoreWidthMax-ImageSize-2*Split-PosSize, StartY+t*20, ScoreWidthMax+ImageSize+2*Split+PosSize, 18.0f, 5.0f, CUI::CORNER_L);
				Graphics()->QuadsEnd();

				// draw score
				TextRender()->Text(0, Whole-ScoreWidthMax+(ScoreWidthMax-aScoreWidth[t])/2-Split, StartY+t*20, 14.0f, aScore[t], -1);

				if(apPlayerInfo[t])
				{
					// draw name
					int ID = apPlayerInfo[t]->m_ClientID;
					if(ID >= 0 && ID < MAX_CLIENTS)
					{
						const char *pName = m_pClient->m_aClients[ID].m_aName;
						float w = TextRender()->TextWidth(0, 8.0f, pName, -1);
						TextRender()->Text(0, min(Whole-w-1.0f, Whole-ScoreWidthMax-ImageSize-2*Split-PosSize), StartY+(t+1)*20.0f-3.0f, 8.0f, pName, -1);

						// draw tee
						CTeeRenderInfo Info = m_pClient->m_aClients[ID].m_RenderInfo;
						Info.m_Size = 18.0f;
						RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0),
							vec2(Whole-ScoreWidthMax-Info.m_Size/2-Split, StartY+1.0f+Info.m_Size/2+t*20));
					}
				}

				// draw position
				char aBuf[32];
				str_format(aBuf, sizeof(aBuf), "%d.", aPos[t]);
				TextRender()->Text(0, Whole-ScoreWidthMax-ImageSize-Split-PosSize, StartY+2.0f+t*20, 10.0f, aBuf, -1);

				StartY += 8.0f;
			}
		}
	}
}

void CHud::RenderWarmupTimer()
{
	CALLSTACK_ADD();

	// render warmup timer
	if(m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer > 0 && !(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_RACETIME))
	{
		char Buf[256];
		float FontSize = 20.0f;
		float w = TextRender()->TextWidth(0, FontSize, Localize("Warmup"), -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()+-w/2, 50, FontSize, Localize("Warmup"), -1);

		int Seconds = m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer/SERVER_TICK_SPEED;
		if(Seconds < 5)
			str_format(Buf, sizeof(Buf), "%d.%d", Seconds, (m_pClient->m_Snap.m_pGameInfoObj->m_WarmupTimer*10/SERVER_TICK_SPEED)%10);
		else
			str_format(Buf, sizeof(Buf), "%d", Seconds);
		w = TextRender()->TextWidth(0, FontSize, Buf, -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()+-w/2, 75, FontSize, Buf, -1);
	}
}

void CHud::MapscreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup)
{
	CALLSTACK_ADD();

	float Points[4];
	RenderTools()->MapscreenToWorld(CenterX, CenterY, pGroup->m_ParallaxX/100.0f, pGroup->m_ParallaxY/100.0f,
		pGroup->m_OffsetX, pGroup->m_OffsetY, Graphics()->ScreenAspect(), 1.0f, Points);
	Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);
}

void CHud::RenderTextInfo()
{
	CALLSTACK_ADD();

	// render fps counter
	if(g_Config.m_ClShowfps)
	{
		// calculate avg. fps
		float FPS = 1.0f / Client()->RenderFrameTime();
		m_AverageFPS = (m_AverageFPS*(1.0f-(1.0f/m_AverageFPS))) + (FPS*(1.0f/m_AverageFPS));
		if(m_AverageFPS <= 0)
			m_AverageFPS = FPS;
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%d", (int)m_AverageFPS);

		float x = m_Width-5-TextRender()->TextWidth(0, 9, aBuf, -1);
		float y = m_Height-14*UI()->Scale();
		switch(g_Config.m_ClShowfpsPos)
		{
			case 0: // BR
				// default; set above
				break;
			case 1: // BL
				x = 5;
				// y remains the same
				break;
			case 2: // TL
				x = 5;
				y = 5 + (g_Config.m_ClShowhud && g_Config.m_ClShowhudHealthAmmo ? 30 : 0);
				break;
			case 3: // TR
				// x remains the same
				y = 5;
				break;
		}
		TextRender()->Text(0, x, y, 9, aBuf, -1);
	}

	// render prediction time
	if(g_Config.m_ClShowpred)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d", Client()->GetPredictionTime());
		TextRender()->TextColor(0.65f, 0.65f, 0.65f, 1);
		bool MoveText = g_Config.m_ClShowfps && g_Config.m_ClShowfpsPos == g_Config.m_ClShowpredPos;
		float x = m_Width-5-TextRender()->TextWidth(0, 9, aBuf, -1);
		float y = MoveText ? m_Height-25*UI()->Scale() : m_Height-14*UI()->Scale();
		switch(g_Config.m_ClShowpredPos)
		{
			case 0: // BR
				// default; set above
				break;
			case 1: // BL
				x = 5;
				// y remains the same
				break;
			case 2: // TL
				x = 5;
				y = (MoveText ? 16 : 5) + (g_Config.m_ClShowhud && g_Config.m_ClShowhudHealthAmmo ? 30 : 0); // move it down for the health bars
				break;
			case 3: // TR
				// x remains the same
				y = MoveText ? 16 : 5;
				break;
		}
		TextRender()->Text(0, x, y, 9, aBuf, -1);
		TextRender()->TextColor(1,1,1,1);
	}
}

void CHud::RenderConnectionWarning()
{
	CALLSTACK_ADD();

	if(Client()->ConnectionProblems())
	{
		const char *pText = Localize("Connection Problems...");
		float w = TextRender()->TextWidth(0, 24, pText, -1);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()-w/2, 50, 24, pText, -1);
	}
}

void CHud::RenderTeambalanceWarning()
{
	CALLSTACK_ADD();

	// render prompt about team-balance
	if(!m_pClient->m_Snap.m_pGameInfoObj)
	{
		float FlashVal = (float)(0.3f * (sin(Client()->LocalTime() * 2.35f) / 2.0f + 0.4f));
		TextRender()->TextColor(0.7f+FlashVal,0.1f+FlashVal,0.0f+FlashVal,1.0f);
		TextRender()->Text(0x0, 5, 50, 6, "Error: m_pClient->m_Snap.m_pGameInfoObj is null ", -1);
		TextRender()->TextColor(1,1,1,1);
	}
	else if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		int TeamDiff = m_pClient->m_Snap.m_aTeamSize[TEAM_RED]-m_pClient->m_Snap.m_aTeamSize[TEAM_BLUE];
		if (g_Config.m_ClWarningTeambalance && abs(TeamDiff) >= 2)
		{
			const char *pText = Localize("Please balance teams!");
			static const float w = TextRender()->TextWidth(0, 6, pText, -1);
			float FlashVal = (float)(0.3f * (sin(Client()->LocalTime() * 2.35f) / 2.0f + 0.4f));
			if(g_Config.m_ClColorfulClient)
			{
				vec3 color = vec3(0.7f, 0.2f, 0.2f); // red
				if(TeamDiff < 0)
					color = vec3(0.2f, 0.2f, 0.7f); // blue
				const CUIRect Rect(0.0f, 47.0f, w + 10.0f, 15.0f);
				RenderTools()->DrawUIRect(&Rect, vec4(color, 0.3f + FlashVal), CUI::CORNER_R, 3.5f);
			}
			TextRender()->TextColor(0.7f+FlashVal,0.7f+FlashVal,0.2f+FlashVal,1.0f);
			TextRender()->Text(0x0, 5, 50, 6, pText, -1);
			TextRender()->TextColor(1,1,1,1);
		}
	}
}

void CHud::PushNotification(const char *pMsg, vec4 Color)
{
	CALLSTACK_ADD();

	if(!g_Config.m_ClNotifications)
		return;

	// make sure we do not exceed limit
	while(m_Notifications.size() >= MAX_NOTIFICATIONS)
		m_Notifications.remove_index(m_Notifications.size()-1);

	// setup a new notification
	CNotification n;
	str_copy(n.m_aMsg, pMsg, sizeof(n.m_aMsg));
	n.m_SpawnTime = Client()->LocalTime();
	n.m_xOffset = 10.0f;
	n.m_Color = Color;

	// push it onto our stack
	m_Notifications.add(n);
	m_Notifications.sort_range();
}

const char* CHud::GetNotification(int index)
{
	CALLSTACK_ADD();

	if(index < m_Notifications.size())
		return m_Notifications[index].m_aMsg;
	return 0;
}

void CHud::RenderNotifications()
{
	CALLSTACK_ADD();

	if(!g_Config.m_ClNotifications)
		return;

	const float NOTIFICATION_LIFETIME = (float)g_Config.m_ClNotificationsLifetime; // in seconds
	const float TEXT_SIZE = 6.0f;
	const float Y_BOTTOM = m_Height/1.7f;

	// render background
	if(!m_Notifications.empty())
	{
		// check for the required number of lines
		int NumLines = 0;
		for(int i = 0; i < m_Notifications.size(); i++)
			NumLines += TextRender()->TextLineCount(0, TEXT_SIZE, m_Notifications[i].m_aMsg, m_Width/4.3f);

		float ybottom = Y_BOTTOM + TEXT_SIZE/2;
		float ytop_wanted = ybottom - NumLines*TEXT_SIZE - TEXT_SIZE/2;
		static float ytop = ytop_wanted;
		smooth_set(&ytop, ytop_wanted, 15.0f, Client()->RenderFrameTime());
		float height = ybottom-ytop;
		CUIRect r;
		r.x = m_Width-m_Width/4.3f-2.5f;
		r.y = ytop;
		r.w = m_Width/4.3f+2.5f;
		r.h = height;
		RenderTools()->DrawUIRect(
				&r,
				vec4(0,0,0,
					 m_Notifications.size() > 1 ? 0.5f
												: min(0.5f, (m_Notifications[0].m_SpawnTime + NOTIFICATION_LIFETIME-Client()->LocalTime()) / NOTIFICATION_LIFETIME)),
				CUI::CORNER_L, 3.5f);
	}

	// render all the notifications
	float yOffset = 0.0f;
	for(int i = 0; i < m_Notifications.size(); i++)
	{
		CNotification *n = &m_Notifications[i];
		yOffset += (TextRender()->TextLineCount(0, TEXT_SIZE, n->m_aMsg, m_Width/4.3f)-1)*TEXT_SIZE;

		float FadeVal = (n->m_SpawnTime + NOTIFICATION_LIFETIME-Client()->LocalTime()) / NOTIFICATION_LIFETIME;

		// remove if faded out
		if(FadeVal < 0.03f)
		{
			m_Notifications.remove_index(i);
			continue;
		}

		if(n->m_xOffset > 0.08f)
		{
			float x = (10.0f-n->m_xOffset)/10.0f;
			float val = -1.0f*powf(x,7.0f)+1.0f; // <-- nice formula
			vec4 Color = mix(n->m_Color, vec4(1,0,0,1), val);
			TextRender()->TextColor(Color.r, Color.g, Color.b, 1);
			//TextRender()->TextColor(1,n->m_Color.g-n->m_xOffset/0.8,n->m_Color.b-n->m_xOffset/0.8,1); // <-- that's the old one
		}
		else
		{
			n->m_xOffset = 0.0f;
			TextRender()->TextColor(n->m_Color.r, n->m_Color.g, n->m_Color.b, min(FadeVal, n->m_Color.a));
		}
		TextRender()->Text(0, m_Width-m_Width/4.3f+2.5f-(n->m_xOffset-=n->m_xOffset/15), Y_BOTTOM-(yOffset+=TEXT_SIZE), TEXT_SIZE, n->m_aMsg, m_Width/4.3f);
		//if(TextRender()->TextLineCount(0, 6.4f, n->m_aMsg, m_Width/4.3f) > 1)
		//yOffset += (TextRender()->TextLineCount(0, 6.4f, n->m_aMsg, m_Width/4.3f)-1)*6.3f;
	}

	TextRender()->TextColor(1,1,1,1);
}

void CHud::RenderIRCNotifications(CUIRect Rect)
{
	CALLSTACK_ADD();

	Rect.HMargin(Rect.h/3.0f, &Rect);

/*	// hack - why?
	{
		static bool True = false;
		if(!True) m_pClient->IRC()->SetActiveCom(0);
		True = true;
	}
*/
	static float Offset = -Rect.w-1;
	if(m_pClient->IRC()->NumUnreadMessages())
	{
		smooth_set(&Offset, 0.0f, 40.0f, Client()->RenderFrameTime());
		Rect.x += Offset;

		char aBuf[19];
		int Num, pNum[2];
		Num = m_pClient->IRC()->NumUnreadMessages(pNum);
		str_format(aBuf, sizeof(aBuf), "Chat: %i (%i + %i)", Num, pNum[0], pNum[1]); // total, channel, query

		Rect.w = TextRender()->TextWidth(0, 6.0f, aBuf, str_length(aBuf)) + 7.5f;
		RenderTools()->DrawUIRect(&Rect, vec4(0,0,0,0.40f), CUI::CORNER_R, 5.0f);

		Rect.x += 2.5f;
		TextRender()->Text(0, Rect.x, Rect.y+3.0f, 6.0f, aBuf, -1);
	}
	else
		Offset = -Rect.w - 1;
}

void CHud::RenderChatBox()
{
	CALLSTACK_ADD();

	if (!g_Config.m_ClShowhudChatbox || !m_pClient->m_pChat->Blend())
		return;

	float LineWidth = m_pClient->m_pScoreboard->Active() ? 100.0f : 210.0f;
	float HeightLimit = m_pClient->m_pScoreboard->Active() ? 55.0f : m_pClient->m_pChat->IsShown() ? 165.0f : 85.0f;
	float HeightStart = 280.0f - HeightLimit;

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, min(m_pClient->m_pChat->Blend(), (float)g_Config.m_ClShowhudChatbox/100.0f));
	RenderTools()->DrawRoundRectExt(0.0f, HeightStart, LineWidth, HeightLimit, 5.0f, 
										m_pClient->m_pChat->IsActive() ? CUI::CORNER_TR : CUI::CORNER_R);

	if(m_pClient->m_pChat->IsActive())
	{
		Graphics()->SetColor(0.0f, 0.0f, 0.0f, min(m_pClient->m_pChat->Blend(), (float)g_Config.m_ClShowhudChatbox/65.0f));
		RenderTools()->DrawRoundRectExt(0.0f, HeightStart + HeightLimit, LineWidth, 12.0f, 5.0f, CUI::CORNER_BR);
	}
	Graphics()->QuadsEnd();
}

void CHud::RenderVoting()
{
	CALLSTACK_ADD();

	CUIRect Rect;
	Rect.x = 0;
	Rect.y = 60-2;
	Rect.w = 100+4+5;
	Rect.h = 46;

	static float RectOffset = Rect.w;
	static float VBarsInset = 0.0f;
	const float VBarsMargin = Rect.w*0.25f+20.0f;
	static bool ShouldRender = false;
	if((!g_Config.m_ClShowVotesAfterVoting && !m_pClient->m_pScoreboard->Active() && m_pClient->m_pVoting->TakenChoice())
			|| !m_pClient->m_pVoting->IsVoting()
			|| Client()->State() == IClient::STATE_DEMOPLAYBACK)
		ShouldRender = false;
	else
		ShouldRender = true;


	if(ShouldRender)
	{
		// 100% visible
		smooth_set(&RectOffset, 0.0f, 30.0f, Client()->RenderFrameTime());
		smooth_set(&VBarsInset, 0.0f, 7.0f, Client()->RenderFrameTime());
	}
	else if(!g_Config.m_ClShowVotesAfterVoting && !m_pClient->m_pScoreboard->Active() && m_pClient->m_pVoting->TakenChoice() && m_pClient->m_pVoting->IsVoting())
	{
		// 75% visible
		smooth_set(&RectOffset, Rect.w*0.75f, 20.0f, Client()->RenderFrameTime());
		smooth_set(&VBarsInset, VBarsMargin,  40.0f, Client()->RenderFrameTime());
	}
	else
	{
		// invisible
		smooth_set(&RectOffset, Rect.w, 20.0f, Client()->RenderFrameTime());
		smooth_set(&VBarsInset, 0, 10.0f, Client()->RenderFrameTime());
	}

	// draw irc notification attached to the right of the voting window
	{
		CUIRect IRCRect = Rect;
		IRCRect.x += Rect.w;
		IRCRect.x -= RectOffset;
		RenderIRCNotifications(IRCRect);
	}

	m_pClient->m_pVoting->CalculateBars();

	// completely invisible, nothing to render
	if(RectOffset >= Rect.w && !ShouldRender)
		return;

	// draw the rect
#if defined(__ANDROID__)
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.40f);
	static const float TextX = 265;
	static const float TextY = 1;
	static const float TextW = 200;
	static const float TextH = 42;
	RenderTools()->DrawRoundRect(TextX-5, TextY, TextW+15, TextH, 5.0f);
	RenderTools()->DrawRoundRect(TextX-5, TextY+TextH+2, TextW/2-10, 20, 5.0f);
	RenderTools()->DrawRoundRect(TextX+TextW/2+20, TextY+TextH+2, TextW/2-10, 20, 5.0f);
	Graphics()->QuadsEnd();
#else
	Rect.x -= RectOffset;
	RenderTools()->DrawUIRect(&Rect, vec4(0,0,0,0.40f), CUI::CORNER_R, 5.0f);

	// HACK FOR CLIPPING: the clipping does not take into account the screen mapping!
	// 600 seems to be some kind of magical value concerning screen mapping
	// we are using 300, so we have to multiply by 2 to get the values for clipping
	{
		CUIRect ClippingRect(Rect.x*2, Rect.y*2, Rect.w*2, Rect.h*2);
		UI()->ClipEnable(&ClippingRect);
	}
	// END HACK

	Rect.x += RectOffset;
#endif

	// render vertical bars
	{
		float yPos = 60.0f;
		float VerticalBarsX = Rect.w-RectOffset - VBarsInset + (Rect.w*0.25f) + 10.0f;

		CTextCursor Cursor;
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), Localize("%ds"), m_pClient->m_pVoting->SecondsLeft());
		float tw = TextRender()->TextWidth(0x0, 6, aBuf, -1);
		TextRender()->SetCursor(&Cursor, VerticalBarsX+4 - tw, yPos, 6.0f, TEXTFLAG_RENDER);
		TextRender()->TextEx(&Cursor, aBuf, -1);

		yPos += 10.0f; // space to the seconds-text
		CUIRect Base(VerticalBarsX, yPos, 4, Rect.h - (yPos-Rect.y) - 5.0f);
		m_pClient->m_pVoting->RenderBarsVertical(Base, true);
	}
	UI()->ClipDisable();

	float TotalOffset = RectOffset + VBarsInset*1.2f;
	TextRender()->TextColor(1,1,1,1);

	CTextCursor Cursor;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), Localize("%ds left"), m_pClient->m_pVoting->SecondsLeft());
#if defined(__ANDROID__)
	float tw = TextRender()->TextWidth(0x0, 10, aBuf, -1);
	TextRender()->SetCursor(&Cursor, TextX+TextW-tw, 0.0f, 10.0f, TEXTFLAG_RENDER);
#else
	float tw = TextRender()->TextWidth(0x0, 6, aBuf, -1);
	TextRender()->SetCursor(&Cursor, 5.0f+100.0f-tw-TotalOffset, 60.0f, 6.0f, TEXTFLAG_RENDER);
#endif
	TextRender()->TextEx(&Cursor, aBuf, -1);

#if defined(__ANDROID__)
	TextRender()->SetCursor(&Cursor, TextX, 0.0f, 10.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = TextW-tw;
#else
	TextRender()->SetCursor(&Cursor, 5.0f-TotalOffset, 60.0f, 6.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = 100.0f-tw;
#endif
	Cursor.m_MaxLines = 3;
	TextRender()->TextEx(&Cursor, m_pClient->m_pVoting->VoteDescription(), -1);

	// reason
	str_format(aBuf, sizeof(aBuf), "%s %s", Localize("Reason:"), m_pClient->m_pVoting->VoteReason());
#if defined(__ANDROID__)
	TextRender()->SetCursor(&Cursor, TextX, 23.0f, 10.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
#else
	TextRender()->SetCursor(&Cursor, 5.0f-TotalOffset, 79.0f, 6.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
#endif
	Cursor.m_LineWidth = 100.0f;
	TextRender()->TextEx(&Cursor, aBuf, -1);

#if defined(__ANDROID__)
	CUIRect Base(TextX, TextH - 8, TextW, 4);
#else
	CUIRect Base(5-TotalOffset, 88, 100, 4);
#endif
	m_pClient->m_pVoting->RenderBars(Base, false);

#if defined(__ANDROID__)
	Base.y += Base.h+6;
	UI()->DoLabel(&Base, Localize("Vote yes"), 16.0f, -1);
	UI()->DoLabel(&Base, Localize("Vote no"), 16.0f, 1);
	if( Input()->KeyPress(KEY_MOUSE_1) )
	{
		float mx, my;
		Input()->MouseRelative(&mx, &my);
		mx *= m_Width / Graphics()->ScreenWidth();
		my *= m_Height / Graphics()->ScreenHeight();
		if( my > TextY+TextH-40 && my < TextY+TextH+20 ) {
			if( mx > TextX-5 && mx < TextX-5+TextW/2-10 )
				m_pClient->m_pVoting->Vote(1);
			if( mx > TextX+TextW/2+20 && mx < TextX+TextW/2+20+TextW/2-10 )
				m_pClient->m_pVoting->Vote(-1);
		}
	}
#else
	const char *pYesKey = m_pClient->m_pBinds->GetKey("vote yes");
	const char *pNoKey = m_pClient->m_pBinds->GetKey("vote no");
	str_format(aBuf, sizeof(aBuf), "%s - %s", pYesKey, Localize("Vote yes"));
	Base.y += Base.h+1;
	if(g_Config.m_ClColorfulClient && m_pClient->m_pVoting->TakenChoice() == 1)
		TextRender()->TextColor(0,0.756f,0,1);
	UI()->DoLabel(&Base, aBuf, 6.0f, -1);
	TextRender()->TextColor(1,1,1,1);

	str_format(aBuf, sizeof(aBuf), "%s - %s", Localize("Vote no"), pNoKey);
	if(g_Config.m_ClColorfulClient && m_pClient->m_pVoting->TakenChoice() == -1)
		TextRender()->TextColor(0.756f,0,0,1);
	UI()->DoLabel(&Base, aBuf, 6.0f, 1);
	TextRender()->TextColor(1,1,1,1);
#endif
}

void CHud::RenderCursor()
{
	CALLSTACK_ADD();

	if(!m_pClient->m_Snap.m_pLocalCharacter || Client()->State() == IClient::STATE_DEMOPLAYBACK || !g_Config.m_ClShowhudCursor)
		return;

	MapscreenToGroup(m_pClient->m_pCamera->m_Center.x, m_pClient->m_pCamera->m_Center.y, Layers()->GameGroup());
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();

	// render cursor
	RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[m_pClient->m_Snap.m_pLocalCharacter->m_Weapon%NUM_WEAPONS].m_pSpriteCursor);
	Graphics()->QuadsSetRotation((float)g_Config.m_ClMouseRotation*((2.0f*3.1415926f)/360.0f));
	float CursorSize = 64;
	RenderTools()->DrawSprite(m_pClient->m_pControls->m_TargetPos[g_Config.m_ClDummy].x, m_pClient->m_pControls->m_TargetPos[g_Config.m_ClDummy].y, CursorSize);
	Graphics()->QuadsEnd();
}

void CHud::RenderHealthAndAmmo(const CNetObj_Character *pCharacter)
{
	CALLSTACK_ADD();

	if(!pCharacter)
		return;

	//mapscreen_to_group(gacenter_x, center_y, layers_game_group());

	float x = 5;
	float y = 5;

	// render ammo count
	// render gui stuff


	const bool ShowNinjaTimer = /*g_Config.m_ClShowhudHealthAmmoBars && */IsVanilla(Client()->GetServerInfo(0)) && pCharacter->m_Weapon == WEAPON_NINJA;
	IGraphics::CQuadItem aArray[10];
	int i;

	// render ammo on non-ddnet or modded ddnet
	if(!IsDDNet(Client()->GetServerInfo(0)) || pCharacter->m_AmmoCount > 0)
	{

		for(i = 0; i < (g_Config.m_ClShowhudMode != 0 ? 1 : min(pCharacter->m_AmmoCount, 10)); i++)
			aArray[i] = IGraphics::CQuadItem(x + i * 12, y + 24, 10, 10);
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(g_pData->m_Weapons.m_aId[pCharacter->m_Weapon % NUM_WEAPONS].m_pSpriteProj);
		Graphics()->QuadsDrawTL(aArray, i);
		Graphics()->QuadsEnd();

		static int64 NinjaStartTime = 0;
		if(ShowNinjaTimer)
		{
			if(NinjaStartTime == 0)
				NinjaStartTime = time_get();
		} else
			NinjaStartTime = 0;

		if(pCharacter->m_Weapon != WEAPON_HAMMER && (pCharacter->m_Weapon != WEAPON_NINJA || (ShowNinjaTimer && NinjaStartTime > 0)))
		{
			if(g_Config.m_ClShowhudMode != 0) // bars/new
			{
				m_MaxAmmo = max(m_MaxAmmo, pCharacter->m_AmmoCount);
				CUIRect r;

				// background
				if(g_Config.m_ClShowhudMode == 1) // bars
				{
					r.x = x + 12;
					r.y = y + 24;
					r.h = 10;
					r.w = 12 * 10;
					RenderTools()->DrawUIRect(&r, vec4(0, 0, 0, 0.4f), CUI::CORNER_R, 3.0f);
				}

				// bar
				static float Width = 0.0f;
				static int LastWeapon = -1;
				if(pCharacter->m_Weapon != LastWeapon) // detect weapon switch
				{
					if(!ShowNinjaTimer)
						Width = ((float)pCharacter->m_AmmoCount / (float)m_MaxAmmo) * 120.0f; // instantly set the value
					else
						Width = 120.0f;
					LastWeapon = pCharacter->m_Weapon;
				}

				float WantedWidth = !ShowNinjaTimer ? ((float)pCharacter->m_AmmoCount / (float)m_MaxAmmo) :
									((float)round_to_int((((NinjaStartTime + ((int64)g_pData->m_Weapons.m_Ninja.m_Duration / 1000LL + 1LL) * time_freq()) - time_get()) & 0x1F00000) >> 5 * 4) / 15.0f);
				smooth_set(&Width, WantedWidth * 120.0f, 20.0f, Client()->RenderFrameTime());

				if(Width > 5 && g_Config.m_ClShowhudMode == 1)
				{
					r.w = min(m_Width / 3, Width);
					RenderTools()->DrawUIRect(&r, vec4(0.7f, (!ShowNinjaTimer) * 0.7f, (!ShowNinjaTimer) * 0.7f, 0.8f), CUI::CORNER_R, 3.0f);
				}

				char aBuf[16];
				str_format(aBuf, sizeof(aBuf), "%i", ShowNinjaTimer ? round_to_int((Width / 120.0f) * 15.0f) : pCharacter->m_AmmoCount);
				TextRender()->Text(0, x + 13, y + 24, 6, aBuf, 100);
			}
		}
	}

	// render health
	int h = 0;
	for(; h < (g_Config.m_ClShowhudMode != 0 ? 1 : min(pCharacter->m_Health, 10)); h++)
		aArray[h] = IGraphics::CQuadItem(x+h*12,y,10,10);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_HEALTH_FULL);
	Graphics()->QuadsDrawTL(aArray, g_Config.m_ClShowhudMode != 0 ? 1 : h);
	Graphics()->QuadsEnd();
	if(g_Config.m_ClShowhudMode)
	{
		if(g_Config.m_ClShowhudMode == 1) // bars
		{
			m_MaxHealth = max(m_MaxHealth, pCharacter->m_Health);
			CUIRect r;
			// background
			r.x = x + 12;
			r.y = y;
			r.h = 10;
			r.w = 12 * 10;
			RenderTools()->DrawUIRect(&r, vec4(0, 0, 0, 0.4f), CUI::CORNER_R, 3.0f);

			// bar
			static float Width = 0.0f;
			if(smooth_set(&Width, ((float)pCharacter->m_Health / (float)m_MaxHealth) * 120.0f, 20.0f, Client()->RenderFrameTime()) > 5)
			{
				r.w = min(m_Width / 3, Width);
				RenderTools()->DrawUIRect(&r, vec4(g_Config.m_ClColorfulClient ? 1.0f - Width / 120.0f : 0.7f, g_Config.m_ClColorfulClient ? Width / 120.0f : 0.0f, 0, 0.8f), CUI::CORNER_R, 3.0f);
			}
		}
		char aBuf[16];
		str_format(aBuf, sizeof(aBuf), "%i", pCharacter->m_Health);
		TextRender()->Text(0, x+13, y, 6, aBuf, 100);
	}


	if(g_Config.m_ClShowhudMode == 0) // vanilla
	{
		i = 0;

		for(; h < 10; h++)
			aArray[i++] = IGraphics::CQuadItem(x+h*12,y,10,10);
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(SPRITE_HEALTH_EMPTY);
		Graphics()->QuadsDrawTL(aArray, i);
		Graphics()->QuadsEnd();
	}

	// render armor meter
	h = 0;
	for(; h < (g_Config.m_ClShowhudMode != 0 ? 1 : min(pCharacter->m_Armor, 10)); h++)
		aArray[h] = IGraphics::CQuadItem(x+h*12,y+12,10,10);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(g_Config.m_ClShowhudMode == 2 && pCharacter->m_Armor == 0 ? SPRITE_ARMOR_EMPTY : SPRITE_ARMOR_FULL);
	Graphics()->QuadsDrawTL(aArray, /*g_Config.m_ClShowhudHealthAmmoBars ? 1 :*/ h);
	Graphics()->QuadsEnd();
	if(g_Config.m_ClShowhudMode != 0) // bars
	{
		if(g_Config.m_ClShowhudMode == 1)
		{
			m_MaxArmor = max(m_MaxArmor, pCharacter->m_Armor);
			CUIRect r;
			// background
			r.x = x + 12;
			r.y = y + 12;
			r.h = 10;
			r.w = 12 * 10;
			RenderTools()->DrawUIRect(&r, vec4(0, 0, 0, 0.4f), CUI::CORNER_R, 3.0f);

			// bar
			static float Width = 0.0f;
			if(smooth_set(&Width, ((float)pCharacter->m_Armor / (float)m_MaxArmor) * 120.0f, 20.0f, Client()->RenderFrameTime()) > 5)
			{
				r.w = min(m_Width / 3, Width);
				RenderTools()->DrawUIRect(&r, vec4(0.7f, 0.8f, 0, 0.8f), CUI::CORNER_R, 3.0f);
			}
		}
		char aBuf[16];
		str_format(aBuf, sizeof(aBuf), "%i", pCharacter->m_Armor);
		TextRender()->Text(0, x+13, y+12, 6, aBuf, 100);
	}

	if(g_Config.m_ClShowhudMode == 0)
	{
		i = 0;
		for(; h < 10; h++)
			aArray[i++] = IGraphics::CQuadItem(x + h*12.0f, y+12.0f, 10.0f, 10.0f);
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();
		RenderTools()->SelectSprite(SPRITE_ARMOR_EMPTY);
		Graphics()->QuadsDrawTL(aArray, i);
		Graphics()->QuadsEnd();
	}
}

void CHud::RenderSpectatorHud()
{
	CALLSTACK_ADD();

	// draw the box
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.4f);
	RenderTools()->DrawRoundRectExt(m_Width-180.0f, m_Height-15.0f, 180.0f, 15.0f, 5.0f, CUI::CORNER_TL);
	Graphics()->QuadsEnd();

	// draw the text
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Spectate"), m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW ?
																   m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_aName : Localize("Free-View"));
	TextRender()->Text(0, m_Width-174.0f, m_Height-13.0f, 8.0f, aBuf, -1);
}

void CHud::RenderLocalTime(float x)
{
	CALLSTACK_ADD();

	if(!g_Config.m_ClShowLocalTimeAlways && !m_pClient->m_pScoreboard->Active())
		return;

	//draw the box
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.4f);
	RenderTools()->DrawRoundRectExt(x-30.0f, 0.0f, 25.0f, 12.5f, 3.75f, CUI::CORNER_B);
	Graphics()->QuadsEnd();

	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	//draw the text
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "%02d:%02d", timeinfo->tm_hour, timeinfo->tm_min);
	TextRender()->Text(0, x-25.0f, 2.5f, 5.0f, aBuf, -1);
}

void CHud::OnRender()
{
	CALLSTACK_ADD();

	if(!m_pClient->m_Snap.m_pGameInfoObj)
		return;

	m_Width = 300.0f*Graphics()->ScreenAspect();
	m_Height = 300.0f;
	Graphics()->MapScreen(0.0f, 0.0f, m_Width, m_Height);

	if(g_Config.m_ClShowhud)
	{
		if(m_pClient->m_Snap.m_pLocalCharacter && !(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
		{
			if (g_Config.m_ClShowhudHealthAmmo)
				RenderHealthAndAmmo(m_pClient->m_Snap.m_pLocalCharacter);
			RenderDDRaceEffects();
		}
		else if(m_pClient->m_Snap.m_SpecInfo.m_Active)
		{
			if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW && g_Config.m_ClShowhudHealthAmmo)
				RenderHealthAndAmmo(&m_pClient->m_Snap.m_aCharacters[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_Cur);
			RenderSpectatorHud();
		}

		RenderGameTimer();
		RenderPauseNotification();
		RenderSuddenDeath();
		if (g_Config.m_ClShowhudScore)
			RenderScoreHud();
		RenderWarmupTimer();
		RenderTextInfo();
		RenderLocalTime((m_Width/7)*3);
		if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
			RenderConnectionWarning();
		RenderTeambalanceWarning();
		RenderVoting();
		RenderNotifications();
		if (g_Config.m_ClShowRecord)
			RenderRecord();
	}
	RenderChatBox();
	RenderCursor();
}

void CHud::OnMessage(int MsgType, void *pRawMsg)
{
	CALLSTACK_ADD();

	if(MsgType == NETMSGTYPE_SV_DDRACETIME)
	{
		m_DDRaceTimeReceived = true;

		CNetMsg_Sv_DDRaceTime *pMsg = (CNetMsg_Sv_DDRaceTime *)pRawMsg;

		m_DDRaceTime = pMsg->m_Time;
		m_DDRaceTick = 0;

		m_LastReceivedTimeTick = Client()->GameTick();

		m_FinishTime = pMsg->m_Finish ? true : false;

		if(pMsg->m_Check)
		{
			m_CheckpointDiff = (float)pMsg->m_Check/100;
			m_CheckpointTick = Client()->GameTick();
		}
	}
	else if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;
		if(pMsg->m_Victim == m_pClient->m_Snap.m_LocalClientID)
		{
			m_CheckpointTick = 0;
			m_DDRaceTime = 0;
		}
	}
	else if(MsgType == NETMSGTYPE_SV_RECORD)
	{
		CServerInfo Info;
		Client()->GetServerInfo(&Info);

		CNetMsg_Sv_Record *pMsg = (CNetMsg_Sv_Record *)pRawMsg;

		// NETMSGTYPE_SV_RACETIME on old race servers
		if(!IsDDRace(&Info) && IsRace(&Info))
		{
			m_DDRaceTimeReceived = true;

			m_DDRaceTime = pMsg->m_ServerTimeBest; // First value: m_Time
			m_DDRaceTick = 0;

			m_LastReceivedTimeTick = Client()->GameTick();

			if(pMsg->m_PlayerTimeBest) // Second value: m_Check
			{
				m_CheckpointDiff = (float)pMsg->m_PlayerTimeBest/100;
				m_CheckpointTick = Client()->GameTick();
			}
		}
		else
		{
			m_ServerRecord = (float)pMsg->m_ServerTimeBest/100;
			m_PlayerRecord = (float)pMsg->m_PlayerTimeBest/100;
		}
	}
}

void CHud::RenderDDRaceEffects()
{
	CALLSTACK_ADD();

	// check racestate
	if(m_FinishTime && m_LastReceivedTimeTick + Client()->GameTickSpeed()*2 < Client()->GameTick())
	{
		m_FinishTime = false;
		m_DDRaceTimeReceived = false;
		return;
	}

	if(m_DDRaceTime)
	{
		char aBuf[64];
		if(m_FinishTime)
		{
			str_format(aBuf, sizeof(aBuf), "Finish time: %02d:%02d.%02d", m_DDRaceTime/6000, m_DDRaceTime/100-m_DDRaceTime/6000 * 60, m_DDRaceTime % 100);
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0,12,aBuf,-1)/2, 20, 12, aBuf, -1);
		}
		else if(m_CheckpointTick + Client()->GameTickSpeed()*6 > Client()->GameTick())
		{
			str_format(aBuf, sizeof(aBuf), "%+5.2f", m_CheckpointDiff);

			// calculate alpha (4 sec 1 than get lower the next 2 sec)
			float a = 1.0f;
			if(m_CheckpointTick+Client()->GameTickSpeed()*4 < Client()->GameTick() && m_CheckpointTick+Client()->GameTickSpeed()*6 > Client()->GameTick())
			{
				// lower the alpha slowly to blend text out
				a = ((float)(m_CheckpointTick+Client()->GameTickSpeed()*6) - (float)Client()->GameTick()) / (float)(Client()->GameTickSpeed()*2);
			}

			if(m_CheckpointDiff > 0)
				TextRender()->TextColor(1.0f,0.5f,0.5f,a); // red
			else if(m_CheckpointDiff < 0)
				TextRender()->TextColor(0.5f,1.0f,0.5f,a); // green
			else if(!m_CheckpointDiff)
				TextRender()->TextColor(1,1,1,a); // white
			TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0, 10, aBuf, -1)/2, 20, 10, aBuf, -1);

			TextRender()->TextColor(1,1,1,1);
		}
	}
	/*else if(m_DDRaceTimeReceived)
	{
		str_format(aBuf, sizeof(aBuf), "%02d:%02d.%d", m_DDRaceTime/60, m_DDRaceTime%60, m_DDRaceTick/10);
		TextRender()->Text(0, 150*Graphics()->ScreenAspect()-TextRender()->TextWidth(0, 12,"00:00.0",-1)/2, 20, 12, aBuf, -1); // use fixed value for text width so its not shaky
	}*/



	static int LastChangeTick = 0;
	if(LastChangeTick != Client()->PredGameTick())
	{
		m_DDRaceTick += 100/Client()->GameTickSpeed();
		LastChangeTick = Client()->PredGameTick();
	}

	if(m_DDRaceTick >= 100)
		m_DDRaceTick = 0;
}

void CHud::RenderRecord()
{
	CALLSTACK_ADD();

	if(m_ServerRecord > 0 )
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Server best:");
		TextRender()->Text(0, 5, 40, 6, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "%02d:%05.2f", (int)m_ServerRecord/60, m_ServerRecord-((int)m_ServerRecord/60*60));
		TextRender()->Text(0, 53, 40, 6, aBuf, -1);
	}

	if(m_PlayerRecord > 0 )
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Personal best:");
		TextRender()->Text(0, 5, 47, 6, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "%02d:%05.2f", (int)m_PlayerRecord/60, m_PlayerRecord-((int)m_PlayerRecord/60*60));
		TextRender()->Text(0, 53, 47, 6, aBuf, -1);
	}
}
