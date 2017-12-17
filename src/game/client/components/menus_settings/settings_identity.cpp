#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>

#include <game/client/components/sounds.h>
#include <game/client/animstate.h>
#include <game/localization.h>

#include "game/client/components/countryflags.h"
#include "game/client/components/menus.h"
#include "game/client/components/identity.h"

int CMenus::RenderSettingsIdentLegacy(CUIRect MainView)
{
	CUIRect Button, Bar;

	enum
	{
		PAGE_PLAYER = 0,
		PAGE_TEE = 1
	};
	static int s_Page = PAGE_PLAYER;

	MainView.HSplitTop(20.0f, &Bar, &MainView);
	Bar.VSplitLeft(MainView.w/5.0f, &Button, &Bar);
	static CButtonContainer s_BackButton;
	if(DoButton_MenuTab(&s_BackButton, Localize("< back"), 0, &Button, CUI::CORNER_L))
		return 0;

	Bar.VSplitMid(&Button, &Bar);
	static CButtonContainer s_ButtonPlayer;
	if(DoButton_MenuTab(&s_ButtonPlayer, Localize("Player"), 0, &Button, 0))
		s_Page = PAGE_PLAYER;

	static CButtonContainer s_ButtonTee;
	if(DoButton_MenuTab(&s_ButtonTee, Localize("Tee"), 0, &Bar, CUI::CORNER_R))
		s_Page = PAGE_TEE;

	if(s_Page == PAGE_PLAYER)
		RenderSettingsPlayer(MainView);
	else if(s_Page == PAGE_TEE)
		RenderSettingsTee(MainView);
	else
		s_Page = PAGE_PLAYER;

	return 1;
}

void CMenus::RenderSettingsIdent(CUIRect MainView)
{
	CALLSTACK_ADD();

	static int s_LegacyIsOpen = 0;
	if(s_LegacyIsOpen)
	{
		s_LegacyIsOpen = RenderSettingsIdentLegacy(MainView);
		return;
	}

	// render background
	CUIRect Temp, TabBar, Button, Label, View;
	static int Page = 0;

	int numID = m_pClient->m_pIdentity->NumIdents();

	// add a new identity from current settings as we need at least one
	if(numID == 0)
		m_pClient->m_pIdentity->AddIdentCurr();

	// render the vertical dividing bar
	MainView.VSplitLeft(240.0f, &TabBar, &MainView);
	TabBar.VSplitRight(2.0f, &TabBar, &Button);
	RenderTools()->DrawUIRect(&Button, vec4((vec3)ms_GuiColor, 0.5f), 0, 0);

	// render legacy view button above ident list
	TabBar.HSplitTop(20.0f, &Button, &TabBar);
	static CButtonContainer s_ButtonOpenLegacy;
	if(DoButton_Menu(&s_ButtonOpenLegacy, Localize("Open legacy view"), 0, &Button, 0, CUI::CORNER_TL))
		s_LegacyIsOpen = 1;

	// render filter box below ident list
	TabBar.HSplitBottom(20.0f, &TabBar, &Temp);
	Temp.VSplitRight(15.0f, &Temp, &Button);
	static CButtonContainer s_EditboxFilter;
	static float s_Offset = 0.0f;
	DoEditBox(&s_EditboxFilter, &Temp, m_aIdentFilterString, sizeof(m_aIdentFilterString), 12.0f, &s_Offset, false, CUI::CORNER_BL, Localize("Search..."));
	// clear button
	static CButtonContainer s_ClearButton;
	if(DoButton_Menu(&s_ClearButton, "×", 0, &Button, 0, CUI::CORNER_NONE))
	{
		m_aIdentFilterString[0] = '\0';
		UI()->SetActiveItem(s_EditboxFilter.GetID());
	}

	static CButtonContainer s_aDeleteIDs[512];
	static CButtonContainer s_aUpIDs[512];
	static CButtonContainer s_aDownIDs[512];
	static CButtonContainer s_aPageIDs[512];
	static CButtonContainer s_aNewIdentIDs[512];
	static CButtonContainer s_aPasteIDs[512];
	static CButtonContainer s_aItemIDs[512];
	static CButtonContainer s_LeftListbox;
	static int s_NumIdentsShown = numID;
	char aBottomLine[64];
	str_formatb(aBottomLine, "%i/%i shown", s_NumIdentsShown, numID);
	static float s_LeftListboxScrollVal = 0.0f;
	bool AllShown = s_NumIdentsShown == numID;
	UiDoListboxStart(&s_LeftListbox, &TabBar, 24.0f, Localize("Identities"), aBottomLine, s_NumIdentsShown+1, 1, -1, s_LeftListboxScrollVal, CUI::CORNER_NONE, CUI::CORNER_NONE);
	s_NumIdentsShown = 0;
	for(int i = 0; i < numID+1; i++)
	{
		if(i >= 512)
			break;

		CIdentity::CIdentEntry *pEntry = m_pClient->m_pIdentity->GetIdent(i);

		if(i < numID)
		{
			if(m_aIdentFilterString[0] != '\0' &&
			   !str_find_nocase(pEntry->m_aTitle, m_aIdentFilterString) && !str_find_nocase(pEntry->m_aName, m_aIdentFilterString))
				continue;
		}

		if(i < numID)
			s_NumIdentsShown++;

		CListboxItem Item = UiDoListboxNextItem(&s_aItemIDs[i], false/*Page == i*/);

		if(!Item.m_Visible)
			continue;

		Button = Item.m_Rect;

		if(i == numID)
		{
			CUIRect RightButton;
			Button.HSplitBottom(4.0f, &Button, 0);
			Button.VSplitRight(240.0f, 0, &Temp);
			Button.VSplitRight(Button.w/3.0f, &Button, &RightButton);
			if(DoButton_MenuTab(&s_aNewIdentIDs[i], Localize("New Identity"), false, &Button, CUI::CORNER_BL, vec4(0.7f, 0.7f, 0.2f, ms_ColorTabbarActive.a), vec4(0.7f, 0.7f, 0.2f, ms_ColorTabbarInactive.a)))
			{
				m_pClient->m_pIdentity->AddIdentNew();
				// select it right away so we can edit it immediately
				Page = numID;
				numID++;
			}
			if(DoButton_MenuTab(&s_aPasteIDs[i], Localize("Paste"), false, &RightButton, CUI::CORNER_BR, vec4(0.7f, 0.7f, 0.2f, ms_ColorTabbarActive.a), vec4(0.7f, 0.7f, 0.2f, ms_ColorTabbarInactive.a),
								Localize("Double-click an identity to copy it to the clipboard, you can then send the resulting text to someone. If someone sends you an identity-code, copy the text and click this button to add it.")))
				m_pClient->m_pIdentity->AddIdentFromJson(Input()->GetClipboardText());
			break;
		}

		// select identity
		if(DoButton_MenuTab(&s_aPageIDs[i], "", Page == i, &Button, i == 0 ? CUI::CORNER_T : 0, vec4(ms_GuiColor, ms_ColorTabbarActive.a), vec4(ms_GuiColor, ms_ColorTabbarInactive.a)))
			Page = i;

		// copy identity
		if(UI()->MouseInside(&Button) && Input()->MouseDoubleClick())
		{
			Input()->MouseDoubleClickReset();
			char *pIdentJson = m_pClient->m_pIdentity->GetIdentAsJsonStr(i);
			Input()->SetClipboardText(pIdentJson);
			mem_free(pIdentJson);
		}

		const bool IsLastEntry = i == numID-1;

		CUIRect ButtonBar;
		Button.VSplitRight(Button.h * 3.0f, 0, &ButtonBar);
		ButtonBar.Margin(4.0f, &ButtonBar);

		ButtonBar.VSplitRight(15.0f, &ButtonBar, &Temp);
		if(m_pClient->m_pIdentity->NumIdents() > 1)
		{
			if(DoButton_Menu(&s_aDeleteIDs[i], "×", 0, &Temp, 0, CUI::CORNER_R | (AllShown ? CUI::CORNER_NONE : CUI::CORNER_L), vec4(0.7f, 0.2f, 0.2f, 0.9f)))
			{
				m_pClient->m_pIdentity->DeleteIdent(i);
				if(i < Page)
					Page--;
			}
		}

		if(AllShown)
		{
			if(!IsLastEntry)
			{
				ButtonBar.VSplitRight(15.0f, &ButtonBar, &Temp);
				if(DoButton_Menu(&s_aDownIDs[i], "↓", 0, &Temp, 0, i >= 1 ? 0 : CUI::CORNER_L))
				{
					m_pClient->m_pIdentity->SwapIdent(i, 1);
					m_MousePos.y += 36.0f * UI()->Scale();
					if(Page == i)
						Page++;
					else if(i == Page-1)
						Page--;
				}
			}

			if(i >= 1)
			{
				ButtonBar.VSplitRight(15.0f, &ButtonBar, &Temp);
				if(DoButton_Menu(&s_aUpIDs[i], "↑", 0, &Temp, 0, CUI::CORNER_L))
				{
					m_pClient->m_pIdentity->SwapIdent(i, -1);
					m_MousePos.y -= 36.0f * UI()->Scale();
					if(i == Page)
						Page--;
					else if(i == Page+1)
						Page++;
				}
			}
		}

		Button.HSplitTop(Button.h*0.25f, 0, &Label);

		static int DefaultSkin = -1;
		if(DefaultSkin < 0)
			 DefaultSkin = m_pClient->m_pSkins->Find("default");

		int SkinID = m_pClient->m_pSkins->Find(pEntry->m_aSkin);
		if(SkinID < 0)
			SkinID = DefaultSkin;

		const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(SkinID);
		CTeeRenderInfo OwnSkinInfo;
		if(pEntry->m_UseCustomColor)
		{
			OwnSkinInfo.m_Texture = pOwnSkin->GetColorTexture();
			OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorBody);
			OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorFeet);
		}
		else
		{
			OwnSkinInfo.m_Texture = pOwnSkin->GetOrgTexture();
			OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		OwnSkinInfo.m_Size = 26.0f * UI()->Scale();

		CUIRect Icons = Button;
		// tee
		Icons.VSplitLeft(OwnSkinInfo.m_Size/UI()->Scale(), &Temp, &Icons);
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Temp.x, Temp.y), false, 1.0f, true);

		// flag
		Icons.VSplitLeft(25.0f, &Temp, &Icons);
		const CCountryFlags::CCountryFlag *pFlag = m_pClient->m_pCountryFlags->GetByCountryCode(pEntry->m_Country);
		m_pClient->m_pCountryFlags->Render(pFlag->m_CountryCode, vec4(1), Temp);

		Button.HMargin(2.0f, &Button);
		Button.HSplitBottom(16.0f, 0, &Button);
		const bool IsMain = m_pClient->m_pIdentity->UsingIdent(i, false);
		const bool IsDummy = m_pClient->m_pIdentity->UsingIdent(i, true);
		vec3 rgb = IsMain && !IsDummy ? vec3(0.7f, 0.7f, 0.2f) : !IsMain && IsDummy ? vec3(0.2f, 0.7f, 0.7f) : IsMain && IsDummy ? vec3(0.2f, 0.7f, 0.2f) : vec3(1,1,1);
		TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
		if(str_length(pEntry->m_aTitle) > 0)
			UI()->DoLabelScaled(&Button, pEntry->m_aTitle, 14.0f, CUI::ALIGN_CENTER, -1.0f, m_aIdentFilterString);
		else
			UI()->DoLabelScaled(&Button, pEntry->m_aName, 14.0f, CUI::ALIGN_CENTER, -1.0f, m_aIdentFilterString);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	UiDoListboxEnd(&s_LeftListboxScrollVal, 0);


	MainView.HSplitTop(20.0f, &Temp, &View);
	Temp.VMargin(5.0f, &Temp);
	Temp.VSplitMid(&Button, &Temp);
	MainView.Margin(10.0f, &MainView);

	static int s_ControlPage = 0;
	CButtonContainer s_ButtonPlayer;
	if(DoButton_MenuTab(&s_ButtonPlayer, Localize("Player"), s_ControlPage == 0, &Button, CUI::CORNER_L))
		s_ControlPage = 0;
	CButtonContainer s_ButtonTee;
	if(DoButton_MenuTab(&s_ButtonTee, Localize("Tee"), s_ControlPage == 1, &Temp, CUI::CORNER_R))
		s_ControlPage = 1;

	if(s_ControlPage == 0)
		RenderSettingsIdentPlayer(View, Page);
	if(s_ControlPage == 1)
		RenderSettingsIdentTee(View, Page);
}

void CMenus::RenderSettingsIdentPlayer(CUIRect MainView, int Page)
{
	CIdentity::CIdentEntry *pEntry = m_pClient->m_pIdentity->GetIdent(Page);
	if(!m_pClient->m_pIdentity->NumIdents() || !pEntry)
		return;

	CUIRect Button, Label;

	// skin info
	const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(pEntry->m_aSkin));
	CTeeRenderInfo OwnSkinInfo;
	if(pEntry->m_UseCustomColor)
	{
		OwnSkinInfo.m_Texture = pOwnSkin->GetColorTexture();
		OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorBody);
		OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorFeet);
	}
	else
	{
		OwnSkinInfo.m_Texture = pOwnSkin->GetOrgTexture();
		OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	OwnSkinInfo.m_Size = 50.0f*UI()->Scale();

	char aBuf[128];
	MainView.Margin(10.0f, &MainView);
	MainView.HSplitTop(50.0f, &Label, &MainView);
	RenderTools()->DrawUIRect(&Label, vec4(1,1,1,0.2f), CUI::CORNER_ALL, 25.0f);
	Label.VSplitLeft(15.0f, 0, &Label);
	Label.HSplitTop(3.0f, 0, &Label);
	const bool IsMain = m_pClient->m_pIdentity->UsingIdent(Page, false);
	const bool IsDummy = m_pClient->m_pIdentity->UsingIdent(Page, true);
	str_format(aBuf, sizeof(aBuf), "%s", IsMain && !IsDummy ? Localize("Main Identity: ") : !IsMain && IsDummy ? Localize("Dummy's Identity: ") : IsMain && IsDummy ? Localize("Both's Identity: ") : "");
	if(str_length(pEntry->m_aTitle) > 0)
		str_append(aBuf, pEntry->m_aTitle, sizeof(aBuf));
	else
		str_append(aBuf, pEntry->m_aName, sizeof(aBuf));
	UI()->DoLabelScaled(&Label, aBuf, 35.0f, -1, (int)Label.w);
	MainView.HSplitTop(10.0f, 0, &MainView);

	// skin view
	MainView.HSplitTop(50.0f, &Label, 0);
	Label.VSplitMid(0, &Label);
	RenderTools()->DrawUIRect(&Label, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
	RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Label.x, Label.y)+vec2(30.0f, 28.0f)*UI()->Scale());
	Label.HSplitTop(15.0f, 0, &Label);
	Label.VSplitLeft(70.0f, 0, &Label);
	UI()->DoLabelScaled(&Label, pEntry->m_aSkin, 14.0f, -1, 150);

	// ident title
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Title"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetTitle[512] = {0.0f};
	CPointerContainer ContainerTitle(&s_OffsetTitle[Page]);
	if(DoEditBox(&ContainerTitle, &Button, pEntry->m_aTitle, sizeof(pEntry->m_aTitle), 14.0f, &s_OffsetTitle[Page], false, CUI::CORNER_ALL, "", 0, Localize("a custom title to display this identity")))
		m_NeedSendinfo = true;

	// player name
	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetName[512] = {0.0f};
	CPointerContainer ContainerName(&s_OffsetName[Page]);
	if(DoEditBox(&ContainerName, &Button, pEntry->m_aName, sizeof(g_Config.m_PlayerName), 14.0f, &s_OffsetName[Page]))
		m_NeedSendinfo = true;

	// player clan
	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetClan[512] = {0.0f};
	CPointerContainer ContainerClan(&s_OffsetClan[Page]);
	if(DoEditBox(&ContainerClan, &Button, pEntry->m_aClan, sizeof(g_Config.m_PlayerClan), 14.0f, &s_OffsetClan[Page]))
		m_NeedSendinfo = true;

	// apply identity
	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Apply as"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	Button.VSplitMid(&Button, &Label);
	static CButtonContainer s_ApplyButtonMain[512], s_ApplyButtonDummy[512];
	if(!IsMain)
	{
		if(DoButton_Menu(&s_ApplyButtonMain[Page], Localize("Main"), 0, &Button, "", CUI::CORNER_L|CUI::CORNER_R*IsDummy, vec4(0.7f, 0.7f, 0.2f, 1.0f)))
			GameClient()->m_pIdentity->ApplyIdent(Page, false);
	}
	if(!IsDummy)
	{
		if(DoButton_Menu(&s_ApplyButtonDummy[Page], Localize("Dummy"), 0, &Label, "", CUI::CORNER_R|CUI::CORNER_L*IsMain, vec4(0.2f, 0.7f, 0.7f, 1.0f)))
			GameClient()->m_pIdentity->ApplyIdent(Page, true);
	}

	// country flag selector
	int *pCountry = &pEntry->m_Country;
	MainView.HSplitTop(20.0f, 0, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	static CButtonContainer s_Listbox;
	UiDoListboxStart(&s_Listbox, &MainView, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

	for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
	{
		const CCountryFlags::CCountryFlag *pFlag = m_pClient->m_pCountryFlags->GetByIndex(i);
		if(pFlag->m_CountryCode == *pCountry)
			OldSelected = i;
		CPointerContainer Container(&pFlag->m_CountryCode);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect FlagLabel;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &FlagLabel);
			float OldWidth = Item.m_Rect.w;
			Item.m_Rect.w = Item.m_Rect.h*2;
			Item.m_Rect.x += (OldWidth-Item.m_Rect.w)/ 2.0f;
			m_pClient->m_pCountryFlags->Render(pFlag->m_CountryCode, vec4(1), Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
			if(pFlag->m_Texture != -1)
				UI()->DoLabelScaled(&FlagLabel, pFlag->m_aCountryCodeString, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		*pCountry = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}
}

void CMenus::RenderSettingsIdentTee(CUIRect MainView, int Page)
{
	CIdentity::CIdentEntry *pEntry = m_pClient->m_pIdentity->GetIdent(Page);
	if(!m_pClient->m_pIdentity->NumIdents() || !pEntry)
		return;

	CUIRect Button, Label, View;

	// skin info
	const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(pEntry->m_aSkin));
	CTeeRenderInfo OwnSkinInfo;
	if(pEntry->m_UseCustomColor)
	{
		OwnSkinInfo.m_Texture = pOwnSkin->GetColorTexture();
		OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorBody);
		OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorFeet);
	}
	else
	{
		OwnSkinInfo.m_Texture = pOwnSkin->GetOrgTexture();
		OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	OwnSkinInfo.m_Size = 50.0f*UI()->Scale();

	MainView.Margin(10.0f, &MainView);
	MainView.HSplitTop(50.0f, &Label, &MainView);
	Label.VSplitMid(&View, &Label);

	// skin view
	RenderTools()->DrawUIRect(&Label, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
	RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Label.x+30.0f, Label.y+28.0f));
	Label.HSplitTop(15.0f, 0, &Label);
	Label.VSplitLeft(70.0f, 0, &Label);
	UI()->DoLabelScaled(&Label, pEntry->m_aSkin, 14.0f, -1, 150);

	// player name
	View.HSplitTop(20.0f, &Button, &View);
	Button.VSplitLeft(230.0f, &Button, 0);
	static CButtonContainer s_VanillaSkinsOnly;
	if(DoButton_CheckBox(&s_VanillaSkinsOnly, Localize("Allow Vanilla Skins only"), g_Config.m_ClVanillaSkinsOnly, &Button))
	{
		g_Config.m_ClVanillaSkinsOnly ^= 1;
		GameClient()->m_pSkins->RefreshSkinList();
		m_InitSkinlist = true;
	}

	// player clan
	View.HSplitTop(5.0f, 0, &View);
	View.HSplitTop(20.0f, &Button, &View);
	Button.VSplitLeft(230.0f, &Button, 0);
	static CButtonContainer s_CheckboxUseCustomColor;
	if(DoButton_CheckBox(&s_CheckboxUseCustomColor, Localize("Custom colors"), pEntry->m_UseCustomColor, &Button))
	{
		pEntry->m_UseCustomColor ^= 1;
		m_NeedSendinfo = true;
	}

	// apply identity
	MainView.HSplitTop(5.0f, 0, &MainView);

	// vanilla skins only
	MainView.HSplitTop(10.0f, 0, &View);
	if(pEntry->m_UseCustomColor)
	{
		CUIRect aRects[2];
		MainView.VSplitMid(&aRects[0], &aRects[1]);

		aRects[0].VSplitRight(10.0f, &aRects[0], 0);
		aRects[1].VSplitRight(10.0f, &aRects[1], 0);

		int *paColors[2] = {
				&pEntry->m_ColorBody,
				&pEntry->m_ColorFeet
		};

		const char *paParts[] = {
				Localize("Body"),
				Localize("Feet")};
		const char *paLabels[] = {
				Localize("Hue"),
				Localize("Sat."),
				Localize("Lht.")};
		static int s_aColorSlider[2][3] = { { 0 } };

		for(int i = 0; i < 2; i++)
		{
			aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
			UI()->DoLabelScaled(&Label, paParts[i], 14.0f, -1);
			aRects[i].VSplitLeft(20.0f, 0, &aRects[i]);
			aRects[i].HSplitTop(2.5f, 0, &aRects[i]);

			int PrevColor = *paColors[i];
			int Color = 0;
			for(int s = 0; s < 3; s++)
			{
				aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
				Label.VSplitLeft(100.0f, &Label, &Button);
				Button.HMargin(2.0f, &Button);

				float k = ((PrevColor>>((2-s)*8))&0xff) / 255.0f;
				CPointerContainer Container(&s_aColorSlider[i][s]);
				k = DoScrollbarH(&Container, &Button, k, 0, (int)(k * 100.0f));
				Color <<= 8;
				Color += clamp((int)(k*255), 0, 255);
				UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
			}

			if(PrevColor != Color)
				m_NeedSendinfo = true;

			*paColors[i] = Color;
		}
	}

	View = MainView;
	View.HSplitTop(100.0f, 0, &View); /// another hack because of all the haxx above D:
	static float s_ScrollValue = {0.0f};
	int OldSelected = -1;
	static CButtonContainer s_Listbox;
	View.HSplitBottom(25.0f, &Button, &View);

	// do skinlist
	const int NumSkins = GetSkinList().size();
	UiDoListboxStart(&s_Listbox, &Button, 50.0f, Localize("Skins"), "", NumSkins, 8, OldSelected, s_ScrollValue);
	for(int i = 0; i < NumSkins; i++)
	{
		const CSkins::CSkin *s = GetSkinList()[i];
		if(!s)
			continue;

		if(str_comp(s->GetName(), pEntry->m_aSkin) == 0)
			OldSelected = i;

		CPointerContainer Container(&GetSkinList()[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		if(Item.m_Visible)
		{
			if(UI()->MouseInside(&Item.m_HitRect))
			{
				m_pClient->m_pTooltip->SetTooltip(s->GetName());
				if(i != OldSelected)
					RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1,1,1,0.2f), CUI::CORNER_ALL, 3.5f);
			}

			CTeeRenderInfo Info;
			if(pEntry->m_UseCustomColor)
			{
				Info.m_Texture = s->GetColorTexture();
				Info.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorBody);
				Info.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorFeet);
			}
			else
			{
				Info.m_Texture = s->GetOrgTexture();
				Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}

			Info.m_Size = UI()->Scale()*50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));

			if(g_Config.m_Debug)
			{
				vec3 BloodColor = pEntry->m_UseCustomColor ? m_pClient->m_pSkins->GetColorV3(pEntry->m_ColorBody) : s->GetBloodColor();
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(BloodColor.r, BloodColor.g, BloodColor.b, 1.0f);
				IGraphics::CQuadItem QuadItem(Item.m_Rect.x, Item.m_Rect.y, 12.0f, 12.0f);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		str_copyb(pEntry->m_aSkin, GetSkinList()[NewSelected]->GetName());
		m_NeedSendinfo = true;
	}

	//View.HSplitTop(5.0f, 0, &View);
	// render quick search and refresh (bottom bar)
	View.HSplitTop(5.0f, 0, &View);
	View.HSplitTop(ms_ButtonHeight, &View, 0);
	{
		View.VSplitLeft(240.0f, &Button, &View);
		//QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
		UI()->DoLabelScaled(&Button, "⚲", 14.0f, -1);
		float wSearch = TextRender()->TextWidth(0, 14.0f, "⚲", -1);
		Button.VSplitLeft(wSearch, 0, &Button);
		Button.VSplitLeft(5.0f, 0, &Button);
		static float Offset = 0.0f;
		static CButtonContainer s_SkinFilterString;
		if(DoEditBox(&s_SkinFilterString, &Button, g_Config.m_ClSkinFilterString, sizeof(g_Config.m_ClSkinFilterString), 14.0f, &Offset, false, CUI::CORNER_L, Localize("Search")))
			m_InitSkinlist = true;

		// clear button
		{
			CPointerContainer s_ClearButton(&g_Config.m_ClSkinFilterString);
			View.VSplitLeft(15.0f, &Button, &View);
			if(DoButton_Menu(&s_ClearButton, "×", 0, &Button, Localize("clear"), CUI::CORNER_R, vec4(1,1,1,0.33f)))
			{
				g_Config.m_ClSkinFilterString[0] = 0;
				UI()->SetActiveItem(&g_Config.m_ClSkinFilterString);
				m_InitSkinlist = true;
			}
		}

		View.VSplitLeft(5.0f, 0, &View);
		View.VSplitLeft(150.0f, &Button, &View);
		static CButtonContainer s_RefreshButton;
		if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Button))
		{
			GameClient()->m_pSkins->RefreshSkinList();
			m_InitSkinlist = true;
		}
	}

	// render skin filters (also bottom bar)
	{
		View.VSplitLeft(5.0f, 0, &View);
		View.VSplitLeft(180.0f, &Button, &View);
		char aFilterLabel[32];
		str_format(aFilterLabel, sizeof(aFilterLabel), "%s", g_Config.m_ClSkinFilterAdvanced == 0 ? Localize("All Skins") : g_Config.m_ClSkinFilterAdvanced == 1 ? Localize("Vanilla Skins only") : g_Config.m_ClSkinFilterAdvanced == 2 ? Localize("Non-Vanilla Skins only") : "");
		static CButtonContainer s_ButtonSkinFilter;
		int PrevFilter = g_Config.m_ClSkinFilterAdvanced;
		if(DoButton_CheckBox_Number(&s_ButtonSkinFilter, aFilterLabel, g_Config.m_ClSkinFilterAdvanced, &Button) == 1)
			if(++g_Config.m_ClSkinFilterAdvanced > 2) g_Config.m_ClSkinFilterAdvanced = 0;
		if(DoButton_CheckBox_Number(&s_ButtonSkinFilter, aFilterLabel, g_Config.m_ClSkinFilterAdvanced, &Button) == 2)
			if(--g_Config.m_ClSkinFilterAdvanced < 0) g_Config.m_ClSkinFilterAdvanced = 2;
		if(g_Config.m_ClSkinFilterAdvanced != PrevFilter)
			m_InitSkinlist = true;
	}
}

