/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/keys.h>
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/render.h>

#include "spectator.h"


void CSpectator::ConKeySpectator(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CSpectator *pSelf = (CSpectator *)pUserData;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
		pSelf->m_Active = pResult->GetInteger(0) != 0;
	else
		pSelf->m_Active = false;
}

void CSpectator::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	((CSpectator *)pUserData)->Spectate(pResult->GetInteger(0));
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	int CurPos = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] && pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID == pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID)
		CurPos = i;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] || pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID;
			GotNewSpectatorID = true;
			break;
		}
	}
	else
	{
		for(int i = CurPos + 1; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] || pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID;
			GotNewSpectatorID = true;
			break;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = 0; i < CurPos; i++)
			{
				if(!pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] || pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID = pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID;
				GotNewSpectatorID = true;
				break;
			}
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	int CurPos = -1;
	for (int i = 0; i < MAX_CLIENTS; i++)
		if (pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] && pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID == pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID)
		CurPos = i;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		for(int i = MAX_CLIENTS -1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] || pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID;
			GotNewSpectatorID = true;
			break;
		}
	}
	else
	{
		for(int i = CurPos - 1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] || pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID;
			GotNewSpectatorID = true;
			break;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = MAX_CLIENTS - 1; i > CurPos; i--)
			{
				if(!pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i] || pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_Team == TEAM_SPECTATORS)
					continue;

			NewSpectatorID = pSelf->m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_ClientID;
			GotNewSpectatorID = true;
				break;
			}
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
}

CSpectator::CSpectator()
{
	OnReset();
	m_OldMouseX = m_OldMouseY = 0.0f;
	m_Sortation = SORT_BY_SCORE;
}

void CSpectator::OnConsoleInit()
{
	CALLSTACK_ADD();

	Console()->Register("+spectate", "", CFGFLAG_CLIENT, ConKeySpectator, this, "Open spectator mode selector");
	Console()->Register("spectate", "i[spectator-id]", CFGFLAG_CLIENT, ConSpectate, this, "Switch spectator mode");
	Console()->Register("spectate_next", "", CFGFLAG_CLIENT, ConSpectateNext, this, "Spectate the next player");
	Console()->Register("spectate_previous", "", CFGFLAG_CLIENT, ConSpectatePrevious, this, "Spectate the previous player");
}

bool CSpectator::OnMouseMove(float x, float y)
{
	CALLSTACK_ADD();

	if(!m_Active)
		return false;

#if defined(__ANDROID__) // No relative mouse on Android
	m_SelectorMouse = vec2(x,y);
	if( m_OldMouseX != x || m_OldMouseY != y )
	{
		m_OldMouseX = x;
		m_OldMouseY = y;
		m_SelectorMouse = vec2((x - Graphics()->ScreenWidth()/2), (y - Graphics()->ScreenHeight()/2));
	}
#else
	UI()->ConvertMouseMove(&x, &y);
	m_SelectorMouse += vec2(x,y);
#endif
	return true;
}

void CSpectator::OnRelease()
{
	CALLSTACK_ADD();

	OnReset();
}

void CSpectator::OnRender()
{
	CALLSTACK_ADD();

	if(!m_Active)
	{
		if(m_WasActive)
		{
			if(m_SelectedSpectatorID != NO_SELECTION)
				Spectate(m_SelectedSpectatorID);
			m_WasActive = false;
		}
		return;
	}

	if(!m_pClient->m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		m_Active = false;
		m_WasActive = false;
		return;
	}

	m_WasActive = true;
	m_SelectedSpectatorID = NO_SELECTION;

	// draw background
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	float ObjWidth = 300.0f;
	float FontSize = 20.0f;
	float BigFontSize = 20.0f;
	float StartY = -190.0f;
	float LineHeight = 60.0f;
	float TeeSizeMod = 1.0f;
	float RoundRadius = 30.0f;
	bool Selected = false;
	int TotalPlayers = 0;
	int PerLine = 8;
	float BoxMove = -10.0f;

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_paInfoByDDTeam[i] || m_pClient->m_Snap.m_paInfoByDDTeam[i]->m_Team == TEAM_SPECTATORS)
			continue;

		++TotalPlayers;
	}

	if (TotalPlayers > 32)
	{
		FontSize = 18.0f;
		LineHeight = 30.0f;
		TeeSizeMod = 0.7f;
		PerLine = 16;
		RoundRadius = 10.0f;
		BoxMove = 3.0f;
	}
	if (TotalPlayers > 16)
	{
		ObjWidth = 600.0f;
	}

	Graphics()->MapScreen(0, 0, Width, Height);

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.43f);
	RenderTools()->DrawRoundRect(Width/2.0f-ObjWidth, Height/2.0f-300.0f, ObjWidth*2, 600.0f, 20.0f);
	Graphics()->QuadsEnd();

	// clamp mouse position to selector area
	m_SelectorMouse.x = clamp(m_SelectorMouse.x, -(ObjWidth - 20.0f), ObjWidth - 20.0f);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, -280.0f, 280.0f);

	// draw selections
	vec4 freeviewRect = vec4(
		Width/2.0f-(ObjWidth - 20.0f),
		Height/2.0f-280.0f,
		270.0f,
		60.0f
	);
	if((Client()->State() == IClient::STATE_DEMOPLAYBACK && m_pClient->m_DemoSpecID == SPEC_FREEVIEW) ||
		 m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
		RenderTools()->DrawRoundRect(freeviewRect.x, freeviewRect.y, freeviewRect.u, freeviewRect.v, 20.0f);
		Graphics()->QuadsEnd();
	}

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK && m_pClient->m_DemoSpecID == SPEC_FOLLOW)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.3f);
		RenderTools()->DrawRoundRect(Width/2.0f-(ObjWidth - 310.0f), Height/2.0f-280.0f, 270.0f, 60.0f, 20.0f);
		Graphics()->QuadsEnd();
	}

	// free-view button logic
	if(m_SelectorMouse.x >= freeviewRect.x-Width/2.0f && m_SelectorMouse.x <= -(ObjWidth-290+10.0f) &&
		m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
	{
		m_SelectedSpectatorID = SPEC_FREEVIEW;
		Selected = true;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
	TextRender()->Text(0, Width/2.0f-(ObjWidth-60.0f), Height/2.0f-265.0f, BigFontSize, Localize("Free-View"), -1);

	// sortation rect
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.69f, 0.69f, 1.0f, 0.3f);

	// sortation toggle logic
	vec4 sortationRect = freeviewRect;
	sortationRect.x += sortationRect.u+10;
	bool MouseInsideSortationRect = false;
	if(	m_SelectorMouse.x >= sortationRect.x-Width/2.0f /*&& m_SelectorMouse.x <= -(ObjWidth-290+10.0f)*/ &&
		m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
	{
		if(Input()->KeyPress(KEY_MOUSE_1))
			m_Sortation ^= 1;
		Graphics()->SetColor(1.0f, 0.69f, 0.69f, 0.3f);
		MouseInsideSortationRect = true;
	}
	RenderTools()->DrawRoundRect(sortationRect.x, sortationRect.y, sortationRect.u, sortationRect.v, 20.0f);
	Graphics()->QuadsEnd();
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, MouseInsideSortationRect?1.0f:0.5f);
	TextRender()->Text(0, (Width/2.0f-(ObjWidth - 310.0f))+5, Height/2.0f-265.0f, BigFontSize,
			m_Sortation == SORT_BY_SCORE ? Localize("Sorted by score") : m_Sortation == SORT_BY_NAME ? Localize("Sorted by name") : "bug!", -1);

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		Selected = false;
		if(m_SelectorMouse.x > -(ObjWidth-290.0f) && m_SelectorMouse.x <= -(ObjWidth-590.0f) &&
			m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
		{
			m_SelectedSpectatorID = SPEC_FOLLOW;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		TextRender()->Text(0, Width/2.0f-(ObjWidth-350.0f), Height/2.0f-265.0f, BigFontSize, Localize("Follow"), -1);
	}

	float x = -(ObjWidth - 30.0f), y = StartY;

	int OldDDTeam = -1;

	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pSortedInfo = m_Sortation == SORT_BY_NAME ? m_pClient->m_Snap.m_paInfoByName[i] : m_pClient->m_Snap.m_paInfoByDDTeam[i];
		if(!pSortedInfo || pSortedInfo->m_Team == TEAM_SPECTATORS)
			continue;

		++Count;

		if(Count == PerLine + 1 || (Count > PerLine + 1 && (Count-1)%PerLine == 0))
		{
			x += 290.0f;
			y = StartY;
		}

		const CNetObj_PlayerInfo *pInfo = pSortedInfo;
		int DDTeam = ((CGameClient *) m_pClient)->m_Teams.Team(pInfo->m_ClientID);
		int NextDDTeam = 0;

		for(int j = i + 1; j < MAX_CLIENTS; j++)
		{
			const CNetObj_PlayerInfo *pInfo2 = m_Sortation == SORT_BY_NAME ? m_pClient->m_Snap.m_paInfoByName[j] : m_pClient->m_Snap.m_paInfoByDDTeam[j];

			if(!pInfo2 || pInfo2->m_Team == TEAM_SPECTATORS)
				continue;

			NextDDTeam = ((CGameClient *) m_pClient)->m_Teams.Team(pInfo2->m_ClientID);
			break;
		}

		if (OldDDTeam == -1)
		{
			for (int j = i - 1; j >= 0; j--)
			{
				const CNetObj_PlayerInfo *pInfo2 = m_Sortation == SORT_BY_NAME ? m_pClient->m_Snap.m_paInfoByName[j] : m_pClient->m_Snap.m_paInfoByDDTeam[j];

				if(!pInfo2 || pInfo2->m_Team == TEAM_SPECTATORS)
					continue;

				OldDDTeam = ((CGameClient *) m_pClient)->m_Teams.Team(pInfo2->m_ClientID);
				break;
			}
		}

		if (DDTeam != TEAM_FLOCK)
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			vec3 rgb = HslToRgb(vec3(DDTeam / 64.0f, 1.0f, 0.5f));
			Graphics()->SetColor(rgb.r, rgb.g, rgb.b, 0.5f);

			int Corners = 0;

			if (OldDDTeam != DDTeam)
				Corners |= CUI::CORNER_TL | CUI::CORNER_TR;
			if (NextDDTeam != DDTeam)
				Corners |= CUI::CORNER_BL | CUI::CORNER_BR;

			RenderTools()->DrawRoundRectExt(Width/2.0f+x-10.0f, Height/2.0f+y+BoxMove, 270.0f, LineHeight, RoundRadius, Corners);

			Graphics()->QuadsEnd();
		}

		OldDDTeam = DDTeam;

		if((Client()->State() == IClient::STATE_DEMOPLAYBACK && m_pClient->m_DemoSpecID == pSortedInfo->m_ClientID)
			|| (Client()->State() != IClient::STATE_DEMOPLAYBACK && m_pClient ->m_Snap.m_SpecInfo.m_SpectatorID == pSortedInfo->m_ClientID))
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(Width/2.0f+x-10.0f, Height/2.0f+y+BoxMove, 270.0f, LineHeight, RoundRadius);
			Graphics()->QuadsEnd();
		}

		Selected = false;
		if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x < x+260.0f &&
			m_SelectorMouse.y >= y-(LineHeight/6.0f) && m_SelectorMouse.y < y+(LineHeight*5.0f/6.0f))
		{
			m_SelectedSpectatorID = pSortedInfo->m_ClientID;
			Selected = true;
		}
		float TeeAlpha;
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK &&
			!m_pClient->m_Snap.m_aCharacters[pSortedInfo->m_ClientID].m_Active)
		{
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.25f);
			TeeAlpha = 0.5f;
		}
		else
		{
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
			TeeAlpha = 1.0f;
		}
		TextRender()->Text(0, Width/2.0f+x+50.0f, Height/2.0f+y+5.0f, FontSize, m_pClient->m_aClients[pSortedInfo->m_ClientID].m_aName, 220.0f);

		// flag
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == pSortedInfo->m_ClientID ||
			m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == pSortedInfo->m_ClientID))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(pSortedInfo->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(Width/2.0f+x-LineHeight/5.0f, Height/2.0f+y-LineHeight/3.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pSortedInfo->m_ClientID].m_RenderInfo;
		TeeInfo.m_ColorBody.a = TeeAlpha;
		TeeInfo.m_ColorFeet.a = TeeAlpha;
		TeeInfo.m_Size *= TeeSizeMod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Width/2.0f+x+20.0f, Height/2.0f+y+20.0f), true);

		y += LineHeight;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// render cursor
	float Scale = UI()->Scale();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_HUDCURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem QuadItem(m_SelectorMouse.x+(Width/2.0f), m_SelectorMouse.y+(Height/2.0f), 48.0f*Scale, 48.0f*Scale);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CSpectator::OnReset()
{
	m_WasActive = false;
	m_Active = false;
	m_SelectedSpectatorID = NO_SELECTION;
}

void CSpectator::Spectate(int SpectatorID)
{
	CALLSTACK_ADD();

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->m_DemoSpecID = clamp(SpectatorID, (int)SPEC_FOLLOW, MAX_CLIENTS-1);
		return;
	}

	if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SpectatorID)
		return;

	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpectatorID = SpectatorID;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
