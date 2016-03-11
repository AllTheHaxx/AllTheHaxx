/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <engine/graphics.h>
#include <engine/shared/config.h>
#include <game/generated/client_data.h>
#include <game/client/animstate.h>
#include <game/localization.h>

#include "menus.h"
#include "skins.h"
#include "gskins.h"
#include "pskins.h"
#include "eskins.h"
#include "cskins.h"

void CMenus::RenderSettingsTexture(CUIRect MainView)
{
	static int s_ControlPage = 0;

	CUIRect TabBar, Button;
	MainView.HSplitTop(20.0f, &TabBar, &MainView);
	MainView.Margin(10.0f, &MainView);

	// tab bar
	{
		TabBar.VSplitLeft(TabBar.w/4, &Button, &TabBar);
		static int s_Button0 = 0;
		if(DoButton_MenuTab(&s_Button0, Localize("Gameskin"), s_ControlPage == 0, &Button, 0))
			s_ControlPage = 0;

		TabBar.VSplitLeft(TabBar.w/3, &Button, &TabBar);
		static int s_Button1 = 0;
		if(DoButton_MenuTab(&s_Button1, Localize("Particles"), s_ControlPage == 1, &Button, 0))
			s_ControlPage = 1;

		TabBar.VSplitMid(&Button, &TabBar);
		static int s_Button2 = 0;
		if(DoButton_MenuTab(&s_Button2, Localize("Emoticons"), s_ControlPage == 2, &Button, 0))
			s_ControlPage = 2;

		static int s_Button3 = 0;
		if(DoButton_MenuTab(&s_Button3, Localize("Cursor"), s_ControlPage == 3, &TabBar, 0))
			s_ControlPage = 3;
	}

	// render page
	if(s_ControlPage == 0)
		RenderSettingsGameskin(MainView);
	else if(s_ControlPage == 1)
		RenderSettingsParticles(MainView);
	else if(s_ControlPage == 2)
		RenderSettingsEmoticons(MainView);
	else if(s_ControlPage == 3)
		RenderSettingsCursor(MainView);
}

void CMenus::RenderSettingsGameskin(CUIRect MainView)
{
	CUIRect Button, Label;
	MainView.HSplitTop(10.0f, 0, &MainView);

	// skin selector
	static bool s_InitSkinlist = true;
	static sorted_array<const CgSkins::CgSkin *> s_paSkinList;
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinlist)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_pgSkins->Num(); ++i)
		{
			const CgSkins::CgSkin *s = m_pClient->m_pgSkins->Get(i);
			// no special skins
			if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
				continue;
			s_paSkinList.add(s);
		}
		s_InitSkinlist = false;
	}

	int OldSelected = -1;
	UiDoListboxStart(&s_InitSkinlist, &MainView, 160.0f, Localize("Texture"), "", s_paSkinList.size(), 2, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CgSkins::CgSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;

		if(str_comp(s->m_aName, g_Config.m_GameTexture) == 0)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);

			int gTexture = s->m_Texture;
			char gName[512];
			str_format(gName, sizeof(gName), "%s", s->m_aName);;
			Graphics()->TextureSet(gTexture);
			Graphics()->QuadsBegin();
			IGraphics::CQuadItem QuadItem(Item.m_Rect.x+Item.m_Rect.w/2 - 120.0f, Item.m_Rect.y+Item.m_Rect.h/2 - 60.0f, 240.0f, 120.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			UI()->DoLabel(&Label, gName, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		mem_copy(g_Config.m_GameTexture, s_paSkinList[NewSelected]->m_aName, sizeof(g_Config.m_GameTexture));
		g_pData->m_aImages[IMAGE_GAME].m_Id = s_paSkinList[NewSelected]->m_Texture;
	}
}
	
        
void CMenus::RenderSettingsParticles(CUIRect MainView)
{
	CUIRect Button, Label;
	MainView.HSplitTop(10.0f, 0, &MainView);

	// skin selector
	static bool s_InitSkinlist = true;
	static sorted_array<const CpSkins::CpSkin *> s_paSkinList;
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinlist)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_ppSkins->Num(); ++i)
		{
			const CpSkins::CpSkin *s = m_pClient->m_ppSkins->Get(i);
			// no special skins
			if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
				continue;
			s_paSkinList.add(s);
		}
		s_InitSkinlist = false;
	}

	int OldSelected = -1;
	UiDoListboxStart(&s_InitSkinlist, &MainView, 160.0f, Localize("Particles"), "", s_paSkinList.size(), 3, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CpSkins::CpSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;

		if(str_comp(s->m_aName, g_Config.m_GameParticles) == 0)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);

			int gTexture = s->m_Texture;
			char gName[512];
			str_format(gName, sizeof(gName), "%s", s->m_aName);;
			Graphics()->TextureSet(gTexture);
			Graphics()->QuadsBegin();
			IGraphics::CQuadItem QuadItem(Item.m_Rect.x+Item.m_Rect.w/2 - 60.0f, Item.m_Rect.y+Item.m_Rect.h/2 - 60.0f, 120.0f, 120.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			UI()->DoLabel(&Label, gName, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		mem_copy(g_Config.m_GameParticles, s_paSkinList[NewSelected]->m_aName, sizeof(g_Config.m_GameParticles));
		g_pData->m_aImages[IMAGE_PARTICLES].m_Id = s_paSkinList[NewSelected]->m_Texture;
	}
}

void CMenus::RenderSettingsEmoticons(CUIRect MainView)
{
	CUIRect Button, Label;
	MainView.HSplitTop(10.0f, 0, &MainView);

	// skin selector
	static bool s_InitSkinlist = true;
	static sorted_array<const CeSkins::CeSkin *> s_paSkinList;
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinlist)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_peSkins->Num(); ++i)
		{
			const CeSkins::CeSkin *s = m_pClient->m_peSkins->Get(i);
			// no special skins
			if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
				continue;
			s_paSkinList.add(s);
		}
		s_InitSkinlist = false;
	}

	int OldSelected = -1;
	UiDoListboxStart(&s_InitSkinlist, &MainView, 160.0f, Localize("Emoticons"), "", s_paSkinList.size(), 3, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CeSkins::CeSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;

		if(str_comp(s->m_aName, g_Config.m_GameEmoticons) == 0)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);

			int gTexture = s->m_Texture;
			char gName[512];
			str_format(gName, sizeof(gName), "%s", s->m_aName);;
			Graphics()->TextureSet(gTexture);
			Graphics()->QuadsBegin();
			IGraphics::CQuadItem QuadItem(Item.m_Rect.x+Item.m_Rect.w/2 - 60.0f, Item.m_Rect.y+Item.m_Rect.h/2 - 60.0f, 120.0f, 120.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			UI()->DoLabel(&Label, gName, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		mem_copy(g_Config.m_GameEmoticons, s_paSkinList[NewSelected]->m_aName, sizeof(g_Config.m_GameEmoticons));
		g_pData->m_aImages[IMAGE_EMOTICONS].m_Id = s_paSkinList[NewSelected]->m_Texture;
	}
}

void CMenus::RenderSettingsCursor(CUIRect MainView)
{
	CUIRect Button, Label;
	MainView.HSplitTop(10.0f, 0, &MainView);

	// skin selector
	static bool s_InitSkinlist = true;
	static sorted_array<const CcSkins::CcSkin *> s_paSkinList;
	static float s_ScrollValue = 0.0f;
	if(s_InitSkinlist)
	{
		s_paSkinList.clear();
		for(int i = 0; i < m_pClient->m_pcSkins->Num(); ++i)
		{
			const CcSkins::CcSkin *s = m_pClient->m_pcSkins->Get(i);
			// no special skins
			if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
				continue;
			s_paSkinList.add(s);
		}
		s_InitSkinlist = false;
	}

	int OldSelected = -1;
	UiDoListboxStart(&s_InitSkinlist, &MainView, 160.0f, Localize("Cursor"), "", s_paSkinList.size(), 3, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_paSkinList.size(); ++i)
	{
		const CcSkins::CcSkin *s = s_paSkinList[i];
		if(s == 0)
			continue;

		if(str_comp(s->m_aName, g_Config.m_GameCursor) == 0)
			OldSelected = i;

		CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);

			int gTexture = s->m_Texture;
			char gName[512];
			str_format(gName, sizeof(gName), "%s", s->m_aName);;
			Graphics()->TextureSet(gTexture);
			Graphics()->QuadsBegin();
			IGraphics::CQuadItem QuadItem(Item.m_Rect.x+Item.m_Rect.w/2 - 60.0f, Item.m_Rect.y+Item.m_Rect.h/2 - 60.0f, 120.0f, 120.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
			UI()->DoLabel(&Label, gName, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		mem_copy(g_Config.m_GameCursor, s_paSkinList[NewSelected]->m_aName, sizeof(g_Config.m_GameCursor));
		g_pData->m_aImages[IMAGE_CURSOR].m_Id = s_paSkinList[NewSelected]->m_Texture;
	}
}
