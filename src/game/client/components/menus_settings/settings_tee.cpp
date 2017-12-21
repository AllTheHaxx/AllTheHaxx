#include "../menus.h"

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/generated/client_data.h>
#include <game/client/animstate.h>
#include "../identity.h"

void CMenus::RenderSettingsTee(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, Label, Button2, Dummy, DummyLabel, SkinList, QuickSearch, QuickSearchClearButton;

	MainView.HSplitTop(10.0f, 0, &MainView);

	char *Skin = g_Config.m_ClPlayerSkin;
	int *UseCustomColor = &g_Config.m_ClPlayerUseCustomColor;
	int *ColorBody = &g_Config.m_ClPlayerColorBody;
	int *ColorFeet = &g_Config.m_ClPlayerColorFeet;

	if(m_Dummy)
	{
		Skin = g_Config.m_ClDummySkin;
		UseCustomColor = &g_Config.m_ClDummyUseCustomColor;
		ColorBody = &g_Config.m_ClDummyColorBody;
		ColorFeet = &g_Config.m_ClDummyColorFeet;
	}

	// skin info
	const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(Skin));
	CTeeRenderInfo OwnSkinInfo;
	if(*UseCustomColor)
	{
		OwnSkinInfo.m_Texture = pOwnSkin->GetColorTexture();
		OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(*ColorBody);
		OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(*ColorFeet);
	}
	else
	{
		OwnSkinInfo.m_Texture = pOwnSkin->GetOrgTexture();
		OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	OwnSkinInfo.m_Size = 50.0f*UI()->Scale();

	MainView.HSplitTop(20.0f, &Label, &MainView);
	Label.VSplitLeft(280.0f, &Label, &Dummy);
	Label.VSplitLeft(230.0f, &Label, 0);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Your skin"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, -1);

	CUIRect Right;
	Dummy.HSplitTop(20.0f, 0, &Dummy);
	Dummy.VSplitLeft(Dummy.w/2.3f, &Dummy, &Right);

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);
	static int s_SkinFilter;
	static CButtonContainer s_ButtonSkinFilter;
	char aFilterLabel[32];
	str_format(aFilterLabel, sizeof(aFilterLabel), "Filter: %s", s_SkinFilter == 0 ? Localize("All Skins") : s_SkinFilter == 1 ? Localize("Vanilla Skins only") : s_SkinFilter == 2 ? Localize("Non-Vanilla Skins only") : "");
	if(DoButton_CheckBox_Number(&s_ButtonSkinFilter, aFilterLabel, s_SkinFilter, &DummyLabel))
	{
		if(++s_SkinFilter > 2) s_SkinFilter = 0;
		m_InitSkinlist = true;
	}

	Right.VSplitLeft(10.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Right, 0);
	static CButtonContainer s_VanillaSkinsOnly;
	if(DoButton_CheckBox(&s_VanillaSkinsOnly, Localize("Allow Vanilla Skins only"), g_Config.m_ClVanillaSkinsOnly, &Right))
	{
		g_Config.m_ClVanillaSkinsOnly ^= 1;
		GameClient()->m_pSkins->RefreshSkinList();
		m_InitSkinlist = true;
	}

	Dummy.HSplitTop(5.0f, 0, &Dummy);
	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);
	static CButtonContainer s_DummySettings;
	if(DoButton_CheckBox(&s_DummySettings, Localize("Dummy settings"), m_Dummy, &DummyLabel))
	{
		m_Dummy ^= 1;
	}

	Dummy.HSplitTop(5.0f, 0, &Dummy);
	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);
	static int s_SkinSaveAsIdentClicked = 0;
	if(s_SkinSaveAsIdentClicked == 0)
	{
		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, Localize("Save Skin as new Identity"), 0, &DummyLabel))
			s_SkinSaveAsIdentClicked = 1;
	}
	else if(s_SkinSaveAsIdentClicked == 1)
	{
		CUIRect DummyLabelRight;
		DummyLabel.VSplitRight(30.0f, &DummyLabel, &DummyLabelRight);

		static float s_NewIdentName = 0.0f;
		static char aName[16];
		static CButtonContainer s_NewIdentNameEditbox;
		DoEditBox(&s_NewIdentNameEditbox, &DummyLabel, aName, sizeof(aName), 10, &s_NewIdentName, false, CUI::CORNER_L, g_Config.m_PlayerName);

		static CButtonContainer s_OkButton;
		if(DoButton_Menu(&s_OkButton, Localize("Ok"), 0, &DummyLabelRight, 0, CUI::CORNER_R))
		{
			CIdentity::CIdentEntry Entry;
			mem_zero(&Entry, sizeof(Entry));
			str_copyb(Entry.m_aName, str_comp(aName, "") != 0 ? aName : g_Config.m_PlayerName);
			str_copyb(Entry.m_aClan, g_Config.m_PlayerClan);
			str_copyb(Entry.m_aSkin, g_Config.m_ClPlayerSkin);
			Entry.m_UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
			Entry.m_ColorBody = g_Config.m_ClPlayerColorBody;
			Entry.m_ColorFeet = g_Config.m_ClPlayerColorFeet;
			m_pClient->m_pIdentity->AddIdent(Entry);
			s_SkinSaveAsIdentClicked = 2;
		}
	}
	else if(s_SkinSaveAsIdentClicked == 2)
	{
		char aBuf[64];
		static float s_StartTime = -1.0f;
		if(s_StartTime < 0.0f)
			s_StartTime = Client()->LocalTime();
		str_format(aBuf, sizeof(aBuf), Localize("New Identity '%s' created!"), m_pClient->m_pIdentity->GetIdent(m_pClient->m_pIdentity->NumIdents()-1)->m_aName);
		TextRender()->TextColor(0.0f, 1.0f,
								1.0f - ((s_StartTime + 4.0f) - Client()->LocalTime()) / 4.0f,
								((s_StartTime + 3.5f) - Client()->LocalTime()) / 3.5f);
		TextRender()->Text(0, DummyLabel.x, DummyLabel.y, 15, aBuf, 800);
		TextRender()->TextColor(1,1,1,1);
		if(Client()->LocalTime() > s_StartTime + 4.0f)
		{
			s_StartTime = -1.0f;
			s_SkinSaveAsIdentClicked = 0;
		}
	}

	Dummy.HSplitTop(20.0f, &DummyLabel, &Dummy);

	MainView.HSplitTop(50.0f, &Label, &MainView);
	Label.VSplitLeft(230.0f, &Label, 0);
	RenderTools()->DrawUIRect(&Label, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
	RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Label.x+30.0f, Label.y+28.0f));
	Label.HSplitTop(15.0f, 0, &Label);
	Label.VSplitLeft(70.0f, 0, &Label);
	UI()->DoLabelScaled(&Label, Skin, 14.0f, -1, 150);

	// custom color selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitMid(&Button, &Button2);
	{
		CUIRect bt = Button;
		bt.w /= 2.0f;
		static CButtonContainer s_ColorBody;
		if(DoButton_CheckBox(&s_ColorBody, Localize("Custom colors"), *UseCustomColor, &bt))
		{
			*UseCustomColor = *UseCustomColor?0:1;
			if(m_Dummy)
				m_NeedSendDummyinfo = true;
			else
				m_NeedSendinfo = true;
		}
	}

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(82.5f, &Label, &MainView);
	if(*UseCustomColor)
	{
		CUIRect aRects[2];
		Label.VSplitMid(&aRects[0], &aRects[1]);
		aRects[0].VSplitRight(10.0f, &aRects[0], 0);
		aRects[1].VSplitLeft(10.0f, 0, &aRects[1]);

		int *paColors[2];
		paColors[0] = ColorBody;
		paColors[1] = ColorFeet;

		const char *paParts[] = {
			Localize("Body"),
			Localize("Feet")};
		const char *paLabels[] = {
			Localize("Hue"),
			Localize("Sat."),
			Localize("Lht.")};
		static int s_aColorSlider[2][3];
		STATIC_INIT_ZERO(s_aColorSlider)

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
				k = DoScrollbarH(&Container, &Button, k, 0, k*255.0f);
				Color <<= 8;
				Color += clamp((int)(k*255), 0, 255);
				UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
			}

			if(PrevColor != Color)
			{
				if(m_Dummy)
					m_NeedSendDummyinfo = true;
				else
					m_NeedSendinfo = true;
			}

			*paColors[i] = Color;
		}
	}

	// skin selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(230.0f, &SkinList, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	static CButtonContainer s_Listbox;
	const int NumSkins = GetSkinList().size();
	UiDoListboxStart(&s_Listbox, &SkinList, 50.0f, Localize("Skins"), "", NumSkins, 4, OldSelected, s_ScrollValue);
	for(int i = 0; i < NumSkins; ++i)
	{
		const CSkins::CSkin *s = GetSkinList()[i];
		if(s == 0)
			continue;

		if(str_comp(s->GetName(), Skin) == 0)
			OldSelected = i;

		CPointerContainer Container(&GetSkinList()[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		char aBuf[128];
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			if(*UseCustomColor)
			{
				Info.m_Texture = s->GetColorTexture();
				Info.m_ColorBody = m_pClient->m_pSkins->GetColorV4(*ColorBody);
				Info.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(*ColorFeet);
			}
			else
			{
				Info.m_Texture = s->GetOrgTexture();
				Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}

			Info.m_Size = UI()->Scale()*50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+30, Item.m_Rect.y+Item.m_Rect.h/2));


			Item.m_Rect.VSplitLeft(60.0f, 0, &Item.m_Rect);
			Item.m_Rect.HSplitTop(10.0f, 0, &Item.m_Rect);
			str_format(aBuf, sizeof(aBuf), "%s", s->GetName());
			RenderTools()->UI()->DoLabelScaled(&Item.m_Rect, aBuf, 12.0f, -1, Item.m_Rect.w, g_Config.m_ClSkinFilterString);
			if(g_Config.m_Debug)
			{
				vec3 BloodColor = *UseCustomColor ? m_pClient->m_pSkins->GetColorV3(*ColorBody) : s->GetBloodColor();
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
		mem_copy(Skin, GetSkinList()[NewSelected]->GetName(), sizeof(g_Config.m_ClPlayerSkin));
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}

	// render quick search and refresh (bottom bar)
	{
		CUIRect Refresh;
		MainView.HSplitBottom(ms_ButtonHeight, &MainView, &QuickSearch);
		QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
		QuickSearch.VSplitLeft(240.0f, &QuickSearch, &Refresh);
		UI()->DoLabelScaled(&QuickSearch, "⚲", 14.0f, -1);
		float wSearch = TextRender()->TextWidth(0, 14.0f, "⚲", -1);
		QuickSearch.VSplitLeft(wSearch, 0, &QuickSearch);
		QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
		QuickSearch.VSplitRight(15.0f, &QuickSearch, &QuickSearchClearButton);
		static float Offset = 0.0f;
		static CButtonContainer s_SkinFilterString;
		if(DoEditBox(&s_SkinFilterString, &QuickSearch, g_Config.m_ClSkinFilterString, sizeof(g_Config.m_ClSkinFilterString), 14.0f, &Offset, false, CUI::CORNER_L, Localize("Search")))
			m_InitSkinlist = true;

		// clear button
		{
			CPointerContainer s_ClearButton(&g_Config.m_ClSkinFilterString);
			if(DoButton_Menu(&s_ClearButton, "×", 0, &QuickSearchClearButton, Localize("clear"), CUI::CORNER_R, vec4(1,1,1,0.33f)))
			{
				g_Config.m_ClSkinFilterString[0] = 0;
				UI()->SetActiveItem(&g_Config.m_ClSkinFilterString);
				m_InitSkinlist = true;
			}
		}

		Refresh.VSplitLeft(5.0f, 0, &Refresh);
		Refresh.VSplitLeft(150.0f, &Refresh, 0);
		static CButtonContainer s_RefreshButton;
		if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &Refresh))
		{
			GameClient()->m_pSkins->RefreshSkinList();
			m_InitSkinlist = true;
		}
	}
}
