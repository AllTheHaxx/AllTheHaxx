#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/components/sounds.h>
#include <game/client/ui.h>
#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include <game/localization.h>

#include "binds.h"
#include "countryflags.h"
#include "menus.h"
#include "skins.h"
#include "identity.h"

void CMenus::RenderSettingsIdent(CUIRect MainView)
{
	// render background
	CUIRect Temp, TabBar, Button, Label, View;
	static int Page = 0;
		
	int numID = m_pClient->m_pIdentity->NumIdents();

	MainView.VSplitLeft(240.0f, &TabBar, &MainView);
	TabBar.VSplitRight(2.0f, &TabBar, &Button);
	RenderTools()->DrawUIRect(&Button, vec4(0.0f, 0.8f, 0.6f, 0.5f), 0, 0);
	
	int s_aUpIDs[numID];
	int s_aDownIDs[numID];
	for(int i = 0; i < numID; i++)
	{
		CIdentity::CIdentEntry *pEntry = m_pClient->m_pIdentity->GetIdent(i);
		TabBar.HSplitTop(24.0f, &Button, &TabBar);
		if(DoButton_MenuTab(pEntry->m_aName, "", Page == i, &Button, 0))
			Page = i;

		Button.VSplitRight(Button.h, 0, &Temp);
		Temp.Margin(4.0f, &Temp);
		if(DoButton_Menu(pEntry, Localize("X"), 0, &Temp))
			m_pClient->m_pIdentity->DeleteIdent(i);

		if(i < numID-1)
		{
			Button.VSplitRight(Button.h, 0, &Temp);
			Temp.Margin(4.0f, &Temp);
			Temp.x -= 16.0f;
			if(DoButton_Menu(&s_aDownIDs[i], Localize("↓"), 0, &Temp))
			{
				m_pClient->m_pIdentity->SwapIdent(i, 1);
			}
		}

		if(i > 0)
		{
			Button.VSplitRight(Button.h, 0, &Temp);
			Temp.Margin(4.0f, &Temp);
			Temp.x -= 32.0f;
			if(DoButton_Menu(&s_aUpIDs[i], Localize("↑"), 0, &Temp))
			{
				m_pClient->m_pIdentity->SwapIdent(i, -1);
			}
		}

		Button.HSplitTop(Button.h*0.25f, 0, &Label);

		const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(pEntry->m_aSkin));
		CTeeRenderInfo OwnSkinInfo;
		if(pEntry->m_UseCustomColor)
		{
			OwnSkinInfo.m_Texture = pOwnSkin->m_ColorTexture;
			OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorBody);
			OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorFeet);
		}
		else
		{
			OwnSkinInfo.m_Texture = pOwnSkin->m_OrgTexture;
			OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		OwnSkinInfo.m_Size = 26.0f*UI()->Scale();
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Button.x + OwnSkinInfo.m_Size, Button.y + Button.h *0.6f));
		Button.HMargin(2.0f, &Button);
		Button.HSplitBottom(16.0f, 0, &Button);
		UI()->DoLabelScaled(&Button, pEntry->m_aName, 14.0f, 0);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	
	static int s_ButtonAdd = 0;

	TabBar.HSplitTop(24.0f, &Button, &TabBar);
	Button.VSplitRight(240.0f, 0, &Temp);
	Temp.Margin(4.0f, &Temp);
	if(DoButton_Menu(&s_ButtonAdd, Localize("Add Identity"), 0, &Temp))
	{
		CIdentity::CIdentEntry Entry;
		mem_zero(&Entry, sizeof(Entry));
		str_format(Entry.m_aName, sizeof(Entry.m_aName), "melon tee");
		str_format(Entry.m_aClan, sizeof(Entry.m_aClan), "Team Green");
		str_format(Entry.m_aSkin, sizeof(Entry.m_aSkin), "toptri");
		m_pClient->m_pIdentity->AddIdent(Entry);
	}

	MainView.Margin(10.0f, &MainView);

	MainView.Margin(10.0f, &MainView);
	MainView.HSplitTop(10.0f, 0, &View);

	View.HSplitTop(20.0f, 0, &View);
	View.HSplitTop(20.0f, &Button, &View);
	Button.VSplitLeft(230.0f, &Button, 0);

	CIdentity::CIdentEntry *pEntry = m_pClient->m_pIdentity->GetIdent(Page);
	if(!m_pClient->m_pIdentity->NumIdents() || !pEntry)
		return;

	// skin info
	const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(pEntry->m_aSkin));
	CTeeRenderInfo OwnSkinInfo;
	if(pEntry->m_UseCustomColor)
	{
		OwnSkinInfo.m_Texture = pOwnSkin->m_ColorTexture;
		OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorBody);
		OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorFeet);
	}
	else
	{
		OwnSkinInfo.m_Texture = pOwnSkin->m_OrgTexture;
		OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	}
	OwnSkinInfo.m_Size = 50.0f*UI()->Scale();

	char aBuf[128];

	// player name
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetName = 0.0f;
	if(DoEditBox(&pEntry->m_aName, &Button, pEntry->m_aName, sizeof(g_Config.m_PlayerName), 14.0f, &s_OffsetName))
		m_NeedSendinfo = true;

	// player clan
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetClan = 0.0f;
	if(DoEditBox(&pEntry->m_aClan, &Button, pEntry->m_aClan, sizeof(g_Config.m_PlayerClan), 14.0f, &s_OffsetClan))
		m_NeedSendinfo = true;

	// custom colour selector
	View.HSplitTop(20.0f, 0, &View);
	View.HSplitTop(20.0f, &Button, &View);
	Button.VSplitLeft(230.0f, &Button, 0);
	if(DoButton_CheckBox(&pEntry->m_UseCustomColor, Localize("Custom colors"), pEntry->m_UseCustomColor, &Button))
	{
		pEntry->m_UseCustomColor ^= 1;
		m_NeedSendinfo = true;
	}

	View.HSplitTop(5.0f, 0, &View);
	//View.HSplitTop(82.5f, &Label, &View);
	if(pEntry->m_UseCustomColor)
	{
		CUIRect aRects[2];
		Label.VSplitMid(&aRects[0], &aRects[1]);
		View.VSplitMid(&aRects[0], 0);
		aRects[0].HSplitMid(&aRects[0], &aRects[1]);

		aRects[0].VSplitRight(10.0f, &aRects[0], 0);
		aRects[1].VSplitRight(10.0f, &aRects[1], 0);

		int *paColors[2];
		paColors[0] = &pEntry->m_ColorBody;
		paColors[1] = &pEntry->m_ColorFeet;

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
			// things are getting even more hacky... i hope noone will ever read this
			if(i)
			{
				aRects[i].x += 305.0f;
				aRects[i].y -= 158.0f;
				Label.x += 305.0f;
				Label.y -= 158.0f;
			}

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
				k = DoScrollbarH(&s_aColorSlider[i][s], &Button, k);
				Color <<= 8;
				Color += clamp((int)(k*255), 0, 255);
				UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
			}

			if(PrevColor != Color)
				m_NeedSendinfo = true;

			*paColors[i] = Color;
		}
	}

	// skin selector
	MainView.VSplitMid(0, &MainView);

	MainView.y -= 48.0f;
	MainView.HSplitTop(50.0f, &Label, &View);
	Label.VSplitLeft(230.0f, &Label, 0);
	RenderTools()->DrawUIRect(&Label, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
	RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Label.x+30.0f, Label.y+28.0f));
	Label.HSplitTop(15.0f, 0, &Label);
	Label.VSplitLeft(70.0f, 0, &Label);
	UI()->DoLabelScaled(&Label, pEntry->m_aSkin, 14.0f, -1, 150.0f);

	MainView.HSplitTop(60.0f, 0, &MainView);
	static bool s_InitSkinlist = false;
	static sorted_array<const CSkins::CSkin *> s_paSkinList;
	static float s_ScrollValue = {0.0f};

	if(!s_InitSkinlist)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_pSkins->Num(); ++i)
		{
			const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);
			// no special skins
			if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
				continue;
			s_paSkinList.add(s);
		}
		s_InitSkinlist = true;
	}

	// shit's so hacky.
	MainView.x -= 300.0f;
	MainView.y += 150.0f;
	MainView.w *= 2.0f;
	MainView.h /= 1.5f;

	int OldSelected = -1;
	UiDoListboxStart(&s_InitSkinlist, &MainView, 50.0f, Localize("Skins"), "", s_paSkinList.size(), 8, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); i++)
	{
		
		const CSkins::CSkin *s = s_paSkinList[i];

		if(s == 0)
			continue;

		if(str_comp(s->m_aName, pEntry->m_aSkin) == 0)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			if(pEntry->m_UseCustomColor)
			{
				Info.m_Texture = s->m_ColorTexture;
				Info.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorBody);
				Info.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pEntry->m_ColorFeet);
			}
			else
			{
				Info.m_Texture = s->m_OrgTexture;
				Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}

			Info.m_Size = UI()->Scale()*50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));

			if(g_Config.m_Debug)
			{
				vec3 BloodColor = pEntry->m_UseCustomColor ? m_pClient->m_pSkins->GetColorV3(pEntry->m_ColorBody) : s->m_BloodColor;
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
		str_format(pEntry->m_aSkin, sizeof(s_paSkinList[NewSelected]->m_aName), s_paSkinList[NewSelected]->m_aName);
		m_NeedSendinfo = true;
	}
}
