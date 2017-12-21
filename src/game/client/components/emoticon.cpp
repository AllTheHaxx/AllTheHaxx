/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/gamecore.h> // get_angle
#include <game/client/animstate.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include "chat.h"
#include "emoticon.h"

CEmoticon::CEmoticon()
{
	OnReset();
}

void CEmoticon::ConKeyEmoticon(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CEmoticon *pSelf = (CEmoticon *)pUserData;
	if(!pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active && pSelf->Client()->State() == IClient::STATE_ONLINE)
		pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CEmoticon::ConEmote(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	((CEmoticon *)pUserData)->Emote(pResult->GetInteger(0));
}

void CEmoticon::OnConsoleInit()
{
	CALLSTACK_ADD();

	Console()->Register("+emote", "", CFGFLAG_CLIENT, ConKeyEmoticon, this, "Open emote selector");
	Console()->Register("emote", "i[emote-id]", CFGFLAG_CLIENT, ConEmote, this, "Use emote");
}

void CEmoticon::OnReset()
{
	m_WasActive = false;
	m_Active = false;
	m_SelectedEmote = -1;
	m_SelectedEyeEmote = -1;
}

void CEmoticon::OnRelease()
{
	CALLSTACK_ADD();

	m_Active = false;
}

bool CEmoticon::OnMouseMove(float x, float y)
{
	CALLSTACK_ADD();

	if(!m_Active)
		return false;

#if defined(__ANDROID__) // No relative mouse on Android
	m_SelectorMouse = vec2(x,y);
#else
	UI()->ConvertMouseMove(&x, &y);
	m_SelectorMouse += vec2(x,y);
#endif
	return true;
}

void CEmoticon::DrawCircle(float x, float y, float r, int Segments)
{
	CALLSTACK_ADD();

	RenderTools()->DrawCircle(x, y, r, Segments);
}


void CEmoticon::OnRender()
{
	CALLSTACK_ADD();

	static float s_Val = 0.0;
	static float s_aBubbleVal[NUM_EMOTICONS];

	if(!m_Active)
	{
		if(m_WasActive)
		{
			 if(m_SelectedEmote != -1)
				 Emote(m_SelectedEmote);
			 if(m_SelectedEyeEmote != -1)
				 EyeEmote(m_SelectedEyeEmote);
			m_WasActive = false;
			mem_zero(s_aBubbleVal, sizeof(s_aBubbleVal));
		}
		if(g_Config.m_ClSmoothEmoteWheel)
			smooth_set(&s_Val, 0.0f, (float)g_Config.m_ClSmoothEmoteWheelDelay, Client()->RenderFrameTime());
		if(s_Val < 0.005f || !g_Config.m_ClSmoothEmoteWheel)
		{
			//s_Val = 0.0f;
			return;
		}
	}
	else
	{
		if(g_Config.m_ClSmoothEmoteWheel)
			smooth_set(&s_Val, 1.0f, (float)g_Config.m_ClSmoothEmoteWheelDelay, Client()->RenderFrameTime());
		else
			s_Val = 1.0f;
	}


	if(m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		m_WasActive = false;
		s_Val = 0.0f;
		mem_zero(s_aBubbleVal, sizeof(s_aBubbleVal));
		return;
	}

	CUIRect Screen = *UI()->Screen();

	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	Graphics()->BlendNormal();

	// draw the big circle
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.3f*s_Val);
	DrawCircle(Screen.w/2, Screen.h/2, s_Val*190.0f, 64);
	Graphics()->QuadsEnd();

	if(m_Active)
		m_WasActive = true;

	if (length(m_SelectorMouse) > 170.0f)
		m_SelectorMouse = normalize(m_SelectorMouse) * 170.0f;

	float SelectedAngle = GetAngle(m_SelectorMouse) + 2*pi/24;
	if (SelectedAngle < 0)
		SelectedAngle += 2*pi;

	m_SelectedEmote = -1;
	m_SelectedEyeEmote = -1;
	if (length(m_SelectorMouse) > 110.0f)
		m_SelectedEmote = (int)(SelectedAngle / (2*pi) * NUM_EMOTICONS);
	else if(length(m_SelectorMouse) > 40.0f)
		m_SelectedEyeEmote = (int)(SelectedAngle / (2*pi) * NUM_EMOTES);

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
	Graphics()->QuadsBegin();

	// draw the emoticons in a circle
	static float s_PopVal = 0.0f;
	if(s_Val > 0.5f)
	{
		if(g_Config.m_ClSmoothEmoteWheel)
			smooth_set(&s_PopVal, 1.0f, (float)g_Config.m_ClSmoothEmoteWheelDelay+10.0f, Client()->RenderFrameTime());
		else
			s_PopVal = 1.0f;
	}
	else
	{
		if(g_Config.m_ClSmoothEmoteWheel)
			smooth_set(&s_PopVal, 0.0f, (float)g_Config.m_ClSmoothEmoteWheelDelay+10.0f, Client()->RenderFrameTime());
		else
			s_PopVal = 0.0f;
	}
	if(s_PopVal < 0.001f)
		s_PopVal = 0.0f;

	for(int i = 0; i < NUM_EMOTICONS; i++)
	{
		float Angle = 2*pi*i/NUM_EMOTICONS;
		if (Angle > pi)
			Angle -= 2*pi;

		bool Selected = m_SelectedEmote == i;

		smooth_set(&s_aBubbleVal[i], Selected ? 35.0f : 0.0f, 25.0f, Client()->RenderFrameTime());
		float WantedSize = s_aBubbleVal[i] + 50.0f;

		float NudgeX = s_PopVal*150.0f * cosf(Angle);
		float NudgeY = s_PopVal*150.0f * sinf(Angle);
		RenderTools()->SelectSprite(SPRITE_OOP + i);
		Graphics()->SetColor(1.0,1.0,1.0,s_Val);
		IGraphics::CQuadItem QuadItem(Screen.w/2 + NudgeX, Screen.h/2 + NudgeY, WantedSize, WantedSize);
		Graphics()->QuadsDraw(&QuadItem, 1);
	}

	Graphics()->QuadsEnd();

	const CServerInfo *pServerInfo = Client()->GetServerInfo();
	if((IsDDRace(pServerInfo) || IsDDNet(pServerInfo) || IsBWMod(pServerInfo) || IsPlus(pServerInfo)) && g_Config.m_ClEyeWheel)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, s_PopVal*0.3f);
		DrawCircle(Screen.w/2.0f, Screen.h/2.0f, min(s_Val*190.0f, 100.0f), 64);
		Graphics()->QuadsEnd();

		CTeeRenderInfo TeeInfo;
		TeeInfo = m_pClient->m_aClients[m_pClient->Client()->m_LocalIDs[g_Config.m_ClDummy]].m_RenderInfo;
		Graphics()->TextureSet(TeeInfo.m_Texture);

		// draw the eyeemotes in a circle
		for (int i = 0; i < NUM_EMOTES; i++)
		{
			float Angle = 2*pi*(float)i/(float)NUM_EMOTES;
			if (Angle > pi)
				Angle -= 2*pi;

			bool Selected = m_SelectedEyeEmote == i;

			TeeInfo.m_Size = Selected ? 64.0f : 48.0f;
			TeeInfo.m_Size *= s_PopVal;

			float NudgeX = s_PopVal*70.0f * cosf(Angle);
			float NudgeY = s_PopVal*70.0f * sinf(Angle);
			TeeInfo.m_ColorBody.a = s_PopVal;
			TeeInfo.m_ColorFeet.a = s_PopVal;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, i, vec2(-1,0), vec2(Screen.w/2 + NudgeX, Screen.h/2 + NudgeY), true);
		}

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,s_PopVal*0.3f);
		DrawCircle(Screen.w/2.0f, Screen.h/2.0f, min(s_Val*190.0f, 30.0f), 64);
		Graphics()->QuadsEnd();
	}
	else
		m_SelectedEyeEmote = -1;

	if(!m_Active)
		return;

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_HUDCURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1, 1, 1, s_Val*1);
	IGraphics::CQuadItem QuadItem(s_Val*m_SelectorMouse.x + Screen.w / 2, s_Val*m_SelectorMouse.y + Screen.h / 2, 24, 24);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CEmoticon::Emote(int Emoticon)
{
	CALLSTACK_ADD();

	CNetMsg_Cl_Emoticon Msg;
	Msg.m_Emoticon = Emoticon;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

	if(g_Config.m_ClDummyCopyMoves)
	{
		CMsgPacker Msg(NETMSGTYPE_CL_EMOTICON);
		Msg.AddInt(Emoticon);
		Client()->SendMsgExY(&Msg, MSGFLAG_VITAL, false, !g_Config.m_ClDummy);
	}
}

void CEmoticon::EyeEmote(int Emote)
{
	CALLSTACK_ADD();
	if(Emote >= NUM_EMOTES)
		return;

	static const char *s_apEmoteMapping[NUM_EMOTES] = {
			"normal",
			"pain",
			"happy",
			"surprise",
			"angry",
			"blink"
	};

	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "/emote %s %d", s_apEmoteMapping[Emote], g_Config.m_ClEyeDuration);
	GameClient()->m_pChat->Say(0, aBuf);
}
