/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/textrender.h>
#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "nameplates.h"
#include "controls.h"

void CNamePlates::RenderNameplate(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	const CNetObj_PlayerInfo *pPlayerInfo
	)
{
	float IntraTick = Client()->IntraGameTick();

	vec2 Position = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), IntraTick);

	bool OtherTeam;

	if (m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_Team == TEAM_SPECTATORS && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
		OtherTeam = false;
	else if (m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW)
		OtherTeam = m_pClient->m_Teams.Team(pPlayerInfo->m_ClientID) != m_pClient->m_Teams.Team(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID);
	else
		OtherTeam = m_pClient->m_Teams.Team(pPlayerInfo->m_ClientID) != m_pClient->m_Teams.Team(m_pClient->m_Snap.m_LocalClientID);

	const float FontSizeName = 18.0f + 20.0f * g_Config.m_ClNameplatesSize / 100.0f;
	const float FontSizeClan = 18.0f + 20.0f * g_Config.m_ClNameplatesClanSize / 100.0f;

	// render nameplates
	if(!pPlayerInfo->m_Local)
	{
		float a = 1;
		if(g_Config.m_ClNameplatesAlways == 0)
			a = clamp(1-powf(distance(m_pClient->m_pControls->m_TargetPos[g_Config.m_ClDummy], Position)/200.0f,16.0f), 0.0f, 1.0f);

		char aName[MAX_NAME_LENGTH+4];
		if(g_Config.m_Debug)
			str_format(aName, sizeof(aName), "%i: %s", pPlayerInfo->m_ClientID, m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aName);
		else
			str_copy(aName, m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aName, sizeof(aName));

		const char *pClan = m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aClan;

		char aScore[8];
		str_format(aScore, sizeof(aScore), "%i", pPlayerInfo->m_Score);

		const float twName = TextRender()->TextWidth(0, FontSizeName, aName, -1);
		const float twScore = TextRender()->TextWidth(0, FontSizeName*0.6f+2, aScore, -1);

		vec3 rgb = vec3(1.0f, 1.0f, 1.0f);
		if(g_Config.m_ClNameplatesTeamcolors && m_pClient->m_Teams.Team(pPlayerInfo->m_ClientID))
			rgb = HslToRgb(vec3(m_pClient->m_Teams.Team(pPlayerInfo->m_ClientID) / 64.0f, 1.0f, 0.75f));

		if (OtherTeam)
		{
			TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.2f);
			TextRender()->TextColor(rgb.r, rgb.g, rgb.b, g_Config.m_ClShowOthersAlpha / 100.0f);
		}
		else
		{
			TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.5f*a);
			TextRender()->TextColor(rgb.r, rgb.g, rgb.b, a);
		}
		if(g_Config.m_ClNameplatesTeamcolors && m_pClient->m_Snap.m_pGameInfoObj && (m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS))
		{
			if(pPlayerInfo->m_Team == TEAM_RED)
				TextRender()->TextColor(1.0f, 0.5f, 0.5f, a);
			else if(pPlayerInfo->m_Team == TEAM_BLUE)
				TextRender()->TextColor(0.7f, 0.7f, 1.0f, a);
		}

		if (str_length(pClan) > 0 && g_Config.m_ClNameplatesClan) // name + clan
		{
			// name
			TextRender()->Text(0, Position.x - twName / 2.0f, Position.y - FontSizeName - 38.0f - FontSizeClan - 3.0f, FontSizeName, aName, -1);

			// clan
			if(g_Config.m_ClNameplatesClancolors && str_comp(pClan, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aClan) == 0)
				TextRender()->TextColor(0, .7f, 0, a);
			const float tw = TextRender()->TextWidth(0, FontSizeClan, pClan, -1);
			TextRender()->Text(0, Position.x - tw / 2.0f, Position.y - FontSizeClan - 38.0f, FontSizeClan, pClan, -1);
		}
		else
		{
			// only name
			TextRender()->Text(0, Position.x - twName / 2.0f, Position.y - FontSizeName - 38.0f, FontSizeName, aName, -1);
		}

		// render score
		if(g_Config.m_ClNamePlatesScore
				&& !IsRace(Client()->GetServerInfo()) && !IsDDNet(Client()->GetServerInfo()) && !IsBWMod(Client()->GetServerInfo()))
		{
			CUIRect Bg;
			Bg.x = Position.x - twScore / 2.0f        -4.0f;
			Bg.y = Position.y - FontSizeName*0.6f+2+28.0f +1.2f;
			Bg.w = twScore                            +10.0f;
			Bg.h = FontSizeName*0.6f+2                    +4.0f;
			RenderTools()->DrawUIRect(&Bg, vec4(0.0f, 0.0f, 0.0f, a / 1.5f), CUI::CORNER_ALL, 10.0f);

			TextRender()->TextColor(0.6f, 1.0f, 0.4f, a - 0.1f);
			TextRender()->Text(0, Position.x - twScore / 2.0f, Position.y - FontSizeName*0.6f+2 + 28.0f, FontSizeName*0.6f+2, aScore, -1);
		}

		TextRender()->TextColor(1,1,1,1);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);

		// render ATH sign
		if(g_Config.m_ClNamePlatesATH &&
				(pPlayerChar->m_PlayerFlags&PLAYERFLAG_ATH1) && (pPlayerChar->m_PlayerFlags&PLAYERFLAG_ATH2))
		{
			const float Alpha = clamp((sinf((float)Client()->LocalTime()*3.141592f*((float)g_Config.m_ClNamePlatesATHBlinkTime/100.0f))+0.5f), 0.0f, 1.0f);
			float PosY = Position.y - 65.0f;

			PosY -= FontSizeName;  // take name into account
			if(str_length(pClan) > 0 && g_Config.m_ClNameplatesClan)
				PosY -= FontSizeClan-5.0f; // take clan into account

			// do the actual thing
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_ATH].m_Id);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
			RenderTools()->DrawRoundRect(Position.x-30.f, PosY, 55.f, 25.f, 0.f);
			Graphics()->QuadsEnd();
		}
	}
}

void CNamePlates::OnRender()
{
	if (!g_Config.m_ClNameplates || m_pClient->AntiPingPlayers())
		return;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// only render active characters
		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		const void *pInfo = Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);

		if(pInfo)
		{
			RenderNameplate(
				&m_pClient->m_Snap.m_aCharacters[i].m_Prev,
				&m_pClient->m_Snap.m_aCharacters[i].m_Cur,
				(const CNetObj_PlayerInfo *)pInfo);
		}
	}
}
