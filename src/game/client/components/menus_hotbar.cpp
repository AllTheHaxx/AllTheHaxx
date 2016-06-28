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

void CMenus::ConKeyShortcut(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CMenus *pSelf = (CMenus *)pUserData;
	if(pSelf->Client()->State() == IClient::STATE_ONLINE)
	{
		if(pResult->GetInteger(0) != 0)
			pSelf->m_HotbarActive ^= 1;
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
			SkinInfo.m_Texture = pSkin->m_ColorTexture;
			SkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pIdent->m_ColorBody);
			SkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pIdent->m_ColorFeet);
		}
		else
		{
			SkinInfo.m_Texture = pSkin->m_OrgTexture;
			SkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			SkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		Button.VSplitLeft(80.0f, &Label, &Button);
		static CButtonContainer s_Button[512];
		if(DoButton_Menu(&s_Button[i], "", 0, &Label, 0, CUI::CORNER_ALL, pIdent->m_UseCustomColor ? mix(SkinInfo.m_ColorBody, SkinInfo.m_ColorFeet, 0.4f)*0.7f :
				vec4(pSkin->m_BloodColor.r, pSkin->m_BloodColor.g, pSkin->m_BloodColor.b, 0.7f)))
		{
			CIdentity::CIdentEntry *pIdent = m_pClient->m_pIdentity->GetIdent(i);
			str_format(g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), pIdent->m_aName);
			str_format(g_Config.m_PlayerClan, sizeof(g_Config.m_PlayerClan), pIdent->m_aClan);
			str_format(g_Config.m_ClPlayerSkin, sizeof(g_Config.m_ClPlayerSkin), pIdent->m_aSkin);
			g_Config.m_ClPlayerUseCustomColor = pIdent->m_UseCustomColor;
			g_Config.m_ClPlayerColorBody = pIdent->m_ColorBody;
			g_Config.m_ClPlayerColorFeet = pIdent->m_ColorFeet;
			m_pClient->SendInfo(false);
		}

		SkinInfo.m_Size = 50.0f*UI()->Scale();
		RenderTools()->RenderTee(CAnimState::GetIdle(), &SkinInfo, 0, vec2(1, 0), vec2(Label.x + Label.w * 0.5f, Label.y + Label.h * 0.5f + 8.0f));

		Label.HSplitBottom(15.0f, 0, &Label);
		Label.y = Button.y;
		// some h4XoRinG right here to get awesome R41NB0W!!
		static float s_Hue = 1000.0f;
		if(s_Hue > 1.0f) s_Hue = RgbToHsl(vec3(1.0f, 0.0f, 0.0f)).h;
		s_Hue += Client()->RenderFrameTime()/255.0f;
		vec3 rgb = HslToRgb(vec3(s_Hue, 1.0f, 0.5f));
		RenderTools()->DrawUIRect(&Label, GameClient()->m_pIdentity->UsingIdent(i) ? vec4(rgb.r, rgb.g, rgb.b, 0.71f) :
				vec4(pSkin->m_BloodColor.r * 0.3f, pSkin->m_BloodColor.g * 0.3f, pSkin->m_BloodColor.b * 0.3f, 0.95f), CUI::CORNER_T, 4.0f);
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
		smooth_set(&s_InGlideVal, 1.0f, 27.0f*(0.005f/Client()->RenderFrameTime()));
	else
		smooth_set(&s_InGlideVal, 0.0f, 27.0f*(0.005f/Client()->RenderFrameTime()));

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
		smooth_set(&s_OutGlideVal, 1.0f, 27.0f*(0.005f/Client()->RenderFrameTime()));
	else
		smooth_set(&s_OutGlideVal, 0.0f, 27.0f*(0.005f/Client()->RenderFrameTime()));

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

int CMenus::ListdirCallback(const char *name, int is_dir, int dir_type, void *user)
{
	CALLSTACK_ADD();

	if(is_dir || str_length(name) < 9)
		return 0;

	// only count pubkeys and check for the corrosponding privkey afterwards
	if(str_comp_nocase_num(name+str_length(name)-8, "_pub.key", 8))
		return 0;

	char aKeyName[64], aPrivKeyPath[128];
	str_copy(aKeyName, name, sizeof(aKeyName));
	aKeyName[str_length(aKeyName)-8] = '\0';

	str_format(aPrivKeyPath, sizeof(aPrivKeyPath), "rsa/%s_priv.key", aKeyName);
	IOHANDLE f = io_open(aPrivKeyPath, IOFLAG_READ);
	if(!f)
	{
		dbg_msg("rsa", "missing private key of keypair '%s'", aKeyName);
		return 0;
	}
	io_close(f);

	array<std::string> *pRSAKeyList = (array<std::string>*)user;
	pRSAKeyList->add(std::string(aKeyName));
	return 0;
}

void CMenus::RenderCrypt(CUIRect MainView)
{
	CALLSTACK_ADD();

	static array<std::string> s_RSAKeyList;
	static bool s_RSAKeyListInited = false;
	if(!s_RSAKeyListInited)
	{
		s_RSAKeyList.clear();
		Storage()->ListDirectory(IStorageTW::TYPE_ALL, "rsa", ListdirCallback, &s_RSAKeyList);
		s_RSAKeyListInited = true;
	}

	CUIRect Button, Label, Rect;

	RenderTools()->DrawUIRect(&MainView, vec4(0.0f, 0.5f, 0.0f, 0.64f), CUI::CORNER_R, 10.0f);

	MainView.Margin(5.0f, &MainView);

	MainView.HSplitTop(15.0f, &Button, &MainView);
	static CButtonContainer s_FlagChat;
	if(DoButton_CheckBox(&s_FlagChat, Localize("Receive hidden chat"), g_Config.m_ClFlagChat, &Button))
		g_Config.m_ClFlagChat ^= 1;

	MainView.HSplitTop(4.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(60.0f, &Label, &Button);
	Button.VSplitLeft(264.0f, &Button, 0);
	Label.x += 4.0f;
	Button.x += 10.0f;
	UI()->DoLabelScaled(&Label, "RSA key:", 14.0, -1);
	static float s_OffsetKeyName = 0.0f;
	static char aKeyName[64] = {0};
	Button.VSplitRight(Button.h, &Button, &Label);
	static int s_ListboxActive = false;
	static CButtonContainer s_ListboxActiveButton;
	if(DoButton_Menu(&s_ListboxActiveButton, ">", s_ListboxActive, &Label, 0, CUI::CORNER_R))
		s_ListboxActive ^= 1;

	static CButtonContainer s_Editbox;
	DoEditBox(&s_Editbox, &Button, aKeyName, 32, 14.0f, &s_OffsetKeyName, false, CUI::CORNER_ALL, Localize("RSA Key name"));
	// do the key selector
	if(s_ListboxActive)
	{
		CUIRect KeyList = MainView;
		KeyList.x += KeyList.w + 25.0f;
		KeyList.w *= 0.83f;
		KeyList.HMargin(-80.0f, &KeyList);
		KeyList.Margin(-7.0f, &KeyList);
		RenderTools()->DrawUIRect(&KeyList, vec4(0.0f, 0.5f, 0.0f, 0.64f), CUI::CORNER_ALL, 6.0f);
		KeyList.Margin(7.0f, &KeyList);

		static CButtonContainer s_Listbox;
		static float s_ListboxScrollVal = 0.0f;
		UiDoListboxStart(&s_Listbox, &KeyList, 15.0f, Localize("List of your RSA keys"), "", s_RSAKeyList.size(), 1, -1, s_ListboxScrollVal);
		for(int i = 0; i < s_RSAKeyList.size(); i++)
		{
			CPointerContainer Container(&s_RSAKeyList[i]);
			CListboxItem Item = UiDoListboxNextItem(&Container);
			if(!Item.m_Visible)
				continue;

			if(UI()->MouseInside(&Item.m_Rect))
				RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1, 1, 1, 0.3f), 0, 0);
	
			UI()->DoLabelScaled(&Item.m_Rect, s_RSAKeyList[i].c_str(), 12.0f, -1, -1, 0);
		}

		int SelectedItem = UiDoListboxEnd(&s_ListboxScrollVal, 0);
		if(SelectedItem > -1)
		{
			str_copy(aKeyName, s_RSAKeyList[SelectedItem].c_str(), sizeof(aKeyName));
			s_ListboxActive = false;
		}
	}

	MainView.HSplitTop(4.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(330.0f, &Button, 0);
	Label.x += 4.0f;
	Button.x += 5.0f;
	static CButtonContainer s_GenButton;
	if(DoButton_Menu(&s_GenButton, Localize("1. Generate RSA key"), 0, &Button, Localize("Generates a new (256 b) RSA key you can then save and share")))
	{
		m_pClient->m_pChat->GenerateKeyPair(256, 3);
	}

	MainView.HSplitTop(4.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(330.0f, &Button, 0);
	Button.x += 5.0f;
	static CButtonContainer s_SaveButton;
	if(DoButton_Menu(&s_SaveButton, Localize("2. Save RSA key"), 0, &Button, Localize("Save key with the name you entered above")))
	{
		m_pClient->m_pChat->SaveKeys(m_pClient->m_pChat->m_pKeyPair, aKeyName);
		s_RSAKeyListInited = false;
	}

	MainView.HSplitTop(4.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(330.0f, &Button, 0);
	Button.x += 5.0f;
	static CButtonContainer s_LoadButton;
	if(DoButton_Menu(&s_LoadButton, Localize("Load RSA key"), 0, &Button, Localize("Load key with the name you entered above")))
	{
		m_pClient->m_pChat->LoadKeys(aKeyName);
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

	CUIRect t;
	MainView.HSplitTop(115.0f, &t, 0);
	t.w = 340.0f;
	t.x = MainView.w - t.w;
	t.y = MainView.h / 2.0f - t.h / 2.0f;
	RenderTrans(t);

	CUIRect c;
	MainView.HSplitTop(125.0f, &c, 0);
	c.w = 342.0f;
	c.x = 0.0f;
	c.y = MainView.h / 2.0f - c.h / 2.0f;
	RenderCrypt(c);
}
