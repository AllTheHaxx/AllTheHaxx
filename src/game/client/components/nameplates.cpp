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

	float FontSize = 18.0f + 20.0f * g_Config.m_ClNameplatesSize / 100.0f;
	float FontSizeClan = 18.0f + 20.0f * g_Config.m_ClNameplatesClanSize / 100.0f;
	// render name plate
	if(!pPlayerInfo->m_Local)
	{
		float a = 1;
		if(g_Config.m_ClNameplatesAlways == 0)
			a = clamp(1-powf(distance(m_pClient->m_pControls->m_TargetPos[g_Config.m_ClDummy], Position)/200.0f,16.0f), 0.0f, 1.0f);

		char aName[MAX_NAME_LENGTH+3];
		if(g_Config.m_Debug)
			str_format(aName, sizeof(aName), "%i: %s", pPlayerInfo->m_ClientID, m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aName);
		else
			str_copy(aName, m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aName, sizeof(aName));

		//char aClan[MAX_CLAN_LENGTH];
		//str_copy(aClan, m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aClan, sizeof(aClan));
		const char *pClan = m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aClan;

		char aScore[128];
		str_format(aScore, sizeof(aScore), "%i", pPlayerInfo->m_Score);

		float tw = TextRender()->TextWidth(0, FontSize, aName, -1);
		float tw2 = TextRender()->TextWidth(0, FontSize*0.6f+2, aScore, -1);

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

		if (str_comp(pClan, "") && g_Config.m_ClNameplatesClan) // name + clan
		{
			TextRender()->Text(0, Position.x - tw / 2.0f, Position.y - FontSize - 38.0f - FontSize, FontSize, aName, -1); // name

			//FontSize = round_to_int(FontSize * 3 / 4);
			tw = TextRender()->TextWidth(0, FontSizeClan, pClan, -1);

			TextRender()->Text(0, Position.x - tw / 2.0f, Position.y - FontSizeClan - 38.0f, FontSizeClan, pClan, -1); // clan
		}
		else
		{
			TextRender()->Text(0, Position.x - tw / 2.0f, Position.y - FontSize - 38.0f, FontSize, aName, -1); // just name
		}

		if(g_Config.m_ClNamePlatesScore)
		{
			CUIRect Bg;
			Bg.x = Position.x - tw2 / 2.0f            -1.0f;
			Bg.y = Position.y - FontSize*0.6f+2+28.0f +1.2f;
			Bg.w = tw2                                +4.0f;
			Bg.h = FontSize*0.6f+2                    +4.0f;
			RenderTools()->DrawUIRect(&Bg, vec4(0.0f, 0.0f, 0.0f, a / 1.5f), CUI::CORNER_ALL, 10.0f);

			TextRender()->TextColor(0.6f, 1.0f, 0.4f, a - 0.1f);
			TextRender()->Text(0, Position.x - tw2 / 2.0f, Position.y - FontSize*0.6f+2 + 28.0f, FontSize*0.6f+2, aScore, -1);
		}

		/*if(g_Config.m_Debug) // render client id when in debug aswell
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf),"%d", pPlayerInfo->m_ClientID);
			float Offset = g_Config.m_ClNameplatesClan ? (FontSize * 2 + FontSizeClan) : (FontSize * 2);
			float tw_id = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->Text(0, Position.x-tw_id/2.0f, Position.y-Offset-38.0f, 28.0f, aBuf, -1);
		}*/

		TextRender()->TextColor(1,1,1,1);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
		
		if(pPlayerChar->m_PlayerFlags&PLAYERFLAG_ATH)
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_ATH].m_Id);
			Graphics()->QuadsBegin();
			float Alpha = sinf(Client()->GameTick()*0.025f);
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, Alpha);
			
			float PosY = !g_Config.m_ClNameplatesClan ? Position.y - 3.f*FontSize : Position.y - 3*FontSize -38.f;
			
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
