#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/textrender.h>
#include <engine/storage.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>

#include "chat.h"
#include "menus.h"
#include "skins.h"
#include "identity.h"

void CMenus::ConKeyToggleHotbar(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CMenus *pSelf = (CMenus *)pUserData;
	if(pSelf->Client()->State() == IClient::STATE_ONLINE)
	{
		if(pSelf->m_HotbarActive ^= 1)
			pSelf->Input()->SetIMEState(true);
		else
			pSelf->Input()->SetIMEState(false);
	}
}

void CMenus::RenderIdents(CUIRect MainView)
{
	CALLSTACK_ADD();

	const int NumIdentities = m_pClient->m_pIdentity->NumIdents();
	if(NumIdentities == 0)
		return;

	CUIRect Button, Label, Temp;
	static CButtonContainer s_Scrollbar;
	static float s_ScrollValue = 0.0f;

	RenderTools()->DrawUIRect(&MainView, vec4(0.65f, 0.68f, 0.9f, 0.64f), CUI::CORNER_T, 10.0f);

	MainView.HSplitTop(8.0f, &Button, &Temp);
	Button.VSplitMid(&Button, &Label);

	// scrollbar
	Temp.HSplitTop(16.0f, &Button, &Temp);
	Button.VMargin(4.0f, &Button);
	if(NumIdentities > 4)
		s_ScrollValue = DoScrollbarH(&s_Scrollbar, &Button, s_ScrollValue);
	else
		s_ScrollValue = 0.0f;

	Temp.HSplitTop(68.0f, &Button, &Temp);
	UI()->ClipEnable(&Button);
	Button.HMargin(5.0f, &Button);
	Button.VSplitLeft(7.5f, 0, &Button);

	const int ListWidth = NumIdentities * (15.0f+80.0f) - 4 * (15.0f+80.0f);
	Button.x -= ListWidth * s_ScrollValue;

	for(int i = 0; i < NumIdentities; i++)
	{
		CIdentity::CIdentEntry *pIdent = m_pClient->m_pIdentity->GetIdent(i);
		if(!pIdent)
			continue;

		const CSkins::CSkin *pSkin = NULL;
		CTeeRenderInfo SkinInfo;

		pSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(pIdent->m_aSkin));

		if(pIdent->m_UseCustomColor)
		{
			SkinInfo.m_Texture = pSkin->GetColorTexture();
			SkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pIdent->m_ColorBody);
			SkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pIdent->m_ColorFeet);
		}
		else
		{
			SkinInfo.m_Texture = pSkin->GetOrgTexture();
			SkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			SkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		Button.VSplitLeft(80.0f, &Label, &Button);
		const vec3& BloodColor = pSkin->GetBloodColor();
		static CButtonContainer s_Button[512];
		if(DoButton_Menu(&s_Button[i], "", 0, &Label, 0, CUI::CORNER_ALL, pIdent->m_UseCustomColor ? mix(SkinInfo.m_ColorBody, SkinInfo.m_ColorFeet, 0.4f)*0.7f :
				vec4(BloodColor.r, BloodColor.g, BloodColor.b, 0.7f)))
		{
			GameClient()->m_pIdentity->ApplyIdent(i, (bool)g_Config.m_ClDummy);
		}

		SkinInfo.m_Size = 50.0f*UI()->Scale();
		RenderTools()->RenderTee(CAnimState::GetIdle(), &SkinInfo, 0, vec2(1, 0), vec2(Label.x + Label.w * 0.5f, Label.y + Label.h * 0.5f + 8.0f));

		Label.HSplitBottom(15.0f, 0, &Label);
		Label.y = Button.y;
		// awesome rainbow
		const float Hue = (sinf(Client()->LocalTime()/3.1415f)/2.0f)+0.5f;
		vec3 rgb = HslToRgb(vec3(Hue, 1.0f, 0.5f));
		RenderTools()->DrawUIRect(&Label, GameClient()->m_pIdentity->UsingIdent(i, (bool)g_Config.m_ClDummy) ? vec4(rgb.r, rgb.g, rgb.b, 0.71f) :
				vec4(BloodColor.r * 0.3f, BloodColor.g * 0.3f, BloodColor.b * 0.3f, 0.95f), CUI::CORNER_T, 4.0f);
		UI()->DoLabelScaled(&Label, pIdent->m_aName, 10.0f, 0);
		Button.VSplitLeft(15.0f, 0, &Button);
	}

	UI()->ClipDisable();
}

void CMenus::RenderTrans(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, Label, Rect;

	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.5f, 0.0f, 0.64f), CUI::CORNER_L, 10.0f);

	MainView.Margin(5.0f, &MainView);

	// incoming messages
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_TransIn;
	if(DoButton_CheckBox(&s_TransIn, Localize("Translate incoming messages"), g_Config.m_ClTransIn, &Button))
		g_Config.m_ClTransIn ^= 1;

	static float s_InGlideVal = 0.0f;
	if(g_Config.m_ClTransIn)
		smooth_set(&s_InGlideVal, 1.0f, 27.0f, Client()->RenderFrameTime());
	else
		smooth_set(&s_InGlideVal, 0.0f, 27.0f, Client()->RenderFrameTime());

	if(s_InGlideVal > 0.01f)
	{
		MainView.HSplitTop(20.0f*s_InGlideVal, &Rect, &MainView);
		Rect.VMargin(3.0f, &Rect);
		Rect.HSplitTop(3.0f, 0, &Rect);

		// clip the view
		const CUIRect ClippingRect = Rect;
		UI()->ClipEnable(&ClippingRect);

		Rect.VSplitLeft(Rect.w*0.4f, &Button, &Rect);

		// do the source label + editbox
		Button.VSplitLeft(Button.w*0.55f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Source:"), 14.0, -1);
		static float s_OffsetInSrc = 0.0f;
		static CButtonContainer s_TransInSrc;
		DoEditBox(&s_TransInSrc, &Button, g_Config.m_ClTransInSrc, sizeof(g_Config.m_ClTransInSrc), 14.0f, &s_OffsetInSrc);
		UI()->ClipDisable();

		// do the same for destination
		UI()->ClipEnable(&ClippingRect);
		Rect.VSplitLeft(13.0f, 0, &Button);
		Button.VSplitLeft(Button.w*0.65f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Destination:"), 14.0, -1);
		static float s_OffsetInDst = 0.0f;
		static CButtonContainer s_TransInDst;
		DoEditBox(&s_TransInDst, &Button, g_Config.m_ClTransInDst, sizeof(g_Config.m_ClTransInDst), 14.0f, &s_OffsetInDst);
		UI()->ClipDisable();
	}

	// outgoing messages
	MainView.HSplitTop(5.0f+10.0f*s_InGlideVal, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_TransOut;
	if(DoButton_CheckBox(&s_TransOut, Localize("Translate outgoing messages"), g_Config.m_ClTransOut, &Button))
		g_Config.m_ClTransOut ^= 1;

	static float s_OutGlideVal = 0.0f;
	if(g_Config.m_ClTransOut)
		smooth_set(&s_OutGlideVal, 1.0f, 27.0f, Client()->RenderFrameTime());
	else
		smooth_set(&s_OutGlideVal, 0.0f, 27.0f, Client()->RenderFrameTime());

	if(s_OutGlideVal > 0.01f)
	{
		MainView.HSplitTop(20.0f*s_OutGlideVal, &Rect, &MainView);
		Rect.VMargin(3.0f, &Rect);
		Rect.HSplitTop(3.0f, 0, &Rect);

		// clip the view
		const CUIRect ClippingRect = Rect;
		UI()->ClipEnable(&ClippingRect);

		Rect.VSplitLeft(Rect.w*0.4f, &Button, &Rect);

		// do the source label + editbox
		Button.VSplitLeft(Button.w*0.55f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Source:"), 14.0, -1);
		static float s_OffsetOutSrc = 0.0f;
		static CButtonContainer s_TransOutSrc;
		DoEditBox(&s_TransOutSrc, &Button, g_Config.m_ClTransOutSrc, sizeof(g_Config.m_ClTransOutSrc), 14.0f, &s_OffsetOutSrc);
		UI()->ClipDisable();

		// do the same for destination
		UI()->ClipEnable(&ClippingRect);
		Rect.VSplitLeft(13.0f, 0, &Button);
		Button.VSplitLeft(Button.w*0.65f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Destination:"), 14.0, -1);
		static float s_OffsetOutDst = 0.0f;
		static CButtonContainer s_TransOutDst;
		DoEditBox(&s_TransOutDst, &Button, g_Config.m_ClTransOutDst, sizeof(g_Config.m_ClTransOutDst), 14.0f, &s_OffsetOutDst);
		UI()->ClipDisable();
	}
}

void CMenus::RenderCrypt(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button;
	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.5f, 0.0f, 0.64f), CUI::CORNER_R, 10.0f);

	MainView.Margin(5.0f, &MainView);

	// flagchat enable checkbox
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_FlagChat;
	if(DoButton_CheckBox(&s_FlagChat, Localize("Receive hidden chat"), g_Config.m_ClFlagChat, &Button, Localize("Hidden-chat messages are displayed in yellow")))
		g_Config.m_ClFlagChat ^= 1;

	// label
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	UI()->DoLabelScaled(&Button, Localize("Crypted Chat:"), 13.0f, CUI::ALIGN_LEFT);

	// 'enter password' box
	MainView.HSplitTop(4.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static char s_aPlainTextPassword[62] = {0};
	{
		if(m_pClient->m_pChat->GotKey())
			RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.0f, 0.7f, 0.5f), CUI::CORNER_ALL, 3.0f);

		static float s_Offset = 0;
		static CButtonContainer s_PasswordBox;
		DoEditBox(&s_PasswordBox, &Button, s_aPlainTextPassword, sizeof(s_aPlainTextPassword), 12.0f, &s_Offset, true, CUI::CORNER_ALL, !m_pClient->m_pChat->GotKey() ? Localize("Enter Password") : Localize("Enter new Password"), CUI::ALIGN_LEFT, m_pClient->m_pChat->GotKey() ? Localize("This will overwrite the currently active key!") : 0);
	}

	// 'password repeat' box if 'enter password' box has been filled in
	static char s_aPlainTextPasswordRepeat[62] = {0};
	if(str_length(s_aPlainTextPassword) > 0)
	{
		MainView.HSplitTop(4.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		if(str_comp(s_aPlainTextPassword, s_aPlainTextPasswordRepeat) != 0)
			RenderTools()->DrawUIRect(&Button, vec4(0.7f, 0, 0, 0.5f), CUI::CORNER_ALL, 3.0f);

		static float s_Offset = 0;
		static CButtonContainer s_PasswordRepeatBox;
		DoEditBox(&s_PasswordRepeatBox, &Button, s_aPlainTextPasswordRepeat, sizeof(s_aPlainTextPasswordRepeat), 12.0f, &s_Offset, true, CUI::CORNER_ALL, Localize("Repeat Password"), CUI::ALIGN_LEFT);
	}

	// apply/clear button
	MainView.HSplitTop(4.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_ApplyButton;
	if((str_length(s_aPlainTextPassword) > 0 && str_comp(s_aPlainTextPassword, s_aPlainTextPasswordRepeat) == 0)
	   || (str_length(s_aPlainTextPassword) == 0 && m_pClient->m_pChat->GotKey()))
	{
		if(DoButton_Menu(&s_ApplyButton, str_length(s_aPlainTextPassword) > 0 ? Localize("Apply") : Localize("Clear"), 0, &Button, str_length(s_aPlainTextPassword) > 0 ? Localize("Set the key as the one used for crypt chat") : Localize("Clear the current key and disable crypt chat")))
		{
			m_pClient->m_pChat->SetKey(s_aPlainTextPassword);

			mem_zerob(s_aPlainTextPassword);
			mem_zerob(s_aPlainTextPasswordRepeat);
		}
	}
	else if(str_length(s_aPlainTextPassword) > 0)
	{
		TextRender()->TextColor(0.7f, 0.1f, 0.1f, 1.0f);
		UI()->DoLabelScaled(&Button, Localize("Passwords do not match!"), 12.0f, CUI::ALIGN_CENTER);
		TextRender()->TextColor(1,1,1,1);
	}

}

void CMenus::RenderHotbar(CUIRect MainView)
{
	CALLSTACK_ADD();

	if(!m_HotbarActive)
	{
		m_HotbarWasActive = false;
		return;
	}

	m_HotbarWasActive = true;

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	Graphics()->BlendNormal();

	CUIRect r;
	MainView.HSplitTop(115.0f, &r, 0);
	r.w = 380.0f;
	r.x = MainView.w / 2.0f - r.w / 2.0f;
	r.y = MainView.h - r.h;
	RenderIdents(r);

	if(m_pClient->m_pChat->TranslatorAvailable())
	{
		CUIRect t;
		MainView.HSplitTop(115.0f, &t, 0);
		t.w = 340.0f;
		t.x = MainView.w - t.w;
		t.y = MainView.h / 2.0f - t.h / 2.0f;
		RenderTrans(t);
	}

	CUIRect c;
	MainView.HSplitTop(125.0f, &c, 0);
	c.w = 342.0f;
	c.x = 0.0f;
	c.y = MainView.h / 2.0f - c.h / 2.0f;
	RenderCrypt(c);
}
