/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include "SDL.h" // SDL_VIDEO_DRIVER_X11

#include <base/tl/string.h>
#include <base/tl/array.h>

#include <base/math.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/updater.h>
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
#include <game/version.h>

#include "binds.h"
#include "camera.h"
#include "countryflags.h"
#include "fontmgr.h"
#include "menus.h"
#include "identity.h"
#include "skins.h"

CMenusKeyBinder CMenus::m_Binder;

CMenusKeyBinder::CMenusKeyBinder()
{
	m_TakeKey = false;
	m_GotKey = false;
}

bool CMenusKeyBinder::OnInput(IInput::CEvent Event)
{
	if(m_TakeKey)
	{
		if(Event.m_Flags&IInput::FLAG_PRESS)
		{
			m_Key = Event;
			m_GotKey = true;
			m_TakeKey = false;
		}
		return true;
	}

	return false;
}

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Label, Button, Left, Right, Game, Client, AutoReconnect;
	MainView.HSplitTop(180.0f, &Game, &Client);
	Client.HSplitTop(160.0f, &Client, &AutoReconnect);

	// game
	{
		// headline
		Game.HSplitTop(30.0f, &Label, &Game);
		UI()->DoLabelScaled(&Label, Localize("Game"), 20.0f, -1);
		Game.Margin(5.0f, &Game);
		Game.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// dynamic camera
		Left.HSplitTop(20.0f, &Button, &Left);
		static CButtonContainer s_DynamicCameraButton;
		if(DoButton_CheckBox(&s_DynamicCameraButton, Localize("Dynamic Camera"), g_Config.m_ClMouseDeadzone != 0, &Button))
		{
			if(g_Config.m_ClMouseDeadzone)
			{
				g_Config.m_ClMouseFollowfactor = 0;
				g_Config.m_ClMouseMaxDistance = 400;
				g_Config.m_ClMouseDeadzone = 0;
			}
			else
			{
				g_Config.m_ClMouseFollowfactor = 60;
				g_Config.m_ClMouseMaxDistance = 1000;
				g_Config.m_ClMouseDeadzone = 300;
			}
		}

		// weapon pickup
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		static CButtonContainer s_CheckboxAutoswitchWeapons;
		if(DoButton_CheckBox(&s_CheckboxAutoswitchWeapons, Localize("Switch weapon on pickup"), g_Config.m_ClAutoswitchWeapons, &Button))
			g_Config.m_ClAutoswitchWeapons ^= 1;

		// weapon out of ammo autoswitch
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		static CButtonContainer s_CheckboxAutoswitchWeaponsOutOfAmmo;
		if(DoButton_CheckBox(&s_CheckboxAutoswitchWeaponsOutOfAmmo, Localize("Switch weapon when out of ammo"), g_Config.m_ClAutoswitchWeaponsOutOfAmmo, &Button))
			g_Config.m_ClAutoswitchWeaponsOutOfAmmo ^= 1;

		// weapon reset on death
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		static CButtonContainer s_CheckboxResetWantedWeaponOnDeath;
		if(DoButton_CheckBox(&s_CheckboxResetWantedWeaponOnDeath, Localize("Reset wanted weapon on death"), g_Config.m_ClResetWantedWeaponOnDeath, &Button))
			g_Config.m_ClResetWantedWeaponOnDeath ^= 1;

		// chat messages
		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_CheckboxShowChatFriends;
		if(DoButton_CheckBox(&s_CheckboxShowChatFriends, Localize("Show only chat messages from friends"), g_Config.m_ClShowChatFriends, &Button))
			g_Config.m_ClShowChatFriends ^= 1;

		// name plates
		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_CheckboxNameplates;
		if(DoButton_CheckBox(&s_CheckboxNameplates, Localize("Show name plates"), g_Config.m_ClNameplates, &Button))
			g_Config.m_ClNameplates ^= 1;

		if(g_Config.m_ClNameplates)
		{
			Right.HSplitTop(2.5f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			UI()->DoLabelScaled(&Label, Localize("Name plates size"), 13.0f, -1);
			Button.HMargin(2.0f, &Button);
			static CButtonContainer s_Scrollbar;
			g_Config.m_ClNameplatesSize = (int)(DoScrollbarH(&s_Scrollbar, &Button, g_Config.m_ClNameplatesSize/100.0f, 0, g_Config.m_ClNameplatesSize)*100.0f+0.1f);

			Right.HSplitTop(20.0f, &Button, &Right);
			static CButtonContainer s_CheckboxNameplatesTeamcolors;
			if(DoButton_CheckBox(&s_CheckboxNameplatesTeamcolors, Localize("Use team colors for name plates"), g_Config.m_ClNameplatesTeamcolors, &Button))
				g_Config.m_ClNameplatesTeamcolors ^= 1;

			Right.HSplitTop(5.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			static CButtonContainer s_CheckboxNameplatesClan;
			if(DoButton_CheckBox(&s_CheckboxNameplatesClan, Localize("Show clan above name plates"), g_Config.m_ClNameplatesClan, &Button))
				g_Config.m_ClNameplatesClan ^= 1;
		}

		if(g_Config.m_ClNameplatesClan)
		{
			Right.HSplitTop(2.5f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			UI()->DoLabelScaled(&Label, Localize("Clan plates size"), 13.0f, -1);
			Button.HMargin(2.0f, &Button);
			CButtonContainer s_Scrollbar;
			g_Config.m_ClNameplatesClanSize = (int)(DoScrollbarH(&s_Scrollbar, &Button, g_Config.m_ClNameplatesClanSize/100.0f, 0, g_Config.m_ClNameplatesClanSize)*100.0f+0.1f);
		}

		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_CheckboxNamePlatesATH;
		if(DoButton_CheckBox(&s_CheckboxNamePlatesATH, Localize("Show other ATH user"), g_Config.m_ClNamePlatesATH, &Button))
			g_Config.m_ClNamePlatesATH ^= 1;
	}

	// client
	{
		// headline
		Client.HSplitTop(65.0f, &Label, &Client);	//changed due to Show other ATH user
		UI()->DoLabelScaled(&Label, Localize("Client"), 20.0f, -1);
		Client.Margin(5.0f, &Client);
		Client.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// auto demo settings
		{
			Left.HSplitTop(20.0f, &Button, &Left);
			static CButtonContainer s_CheckboxAutoDemoRecord;
			if(DoButton_CheckBox(&s_CheckboxAutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button))
				g_Config.m_ClAutoDemoRecord ^= 1;

			Right.HSplitTop(20.0f, &Button, &Right);
			static CButtonContainer s_CheckboxAutoScreenshot;
			if(DoButton_CheckBox(&s_CheckboxAutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button))
				g_Config.m_ClAutoScreenshot ^= 1;

			Left.HSplitTop(10.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			char aBuf[64];
			if(g_Config.m_ClAutoDemoMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max demos"), g_Config.m_ClAutoDemoMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max demos"), Localize("no limit"));
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Left.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			static CButtonContainer s_ScrollbarAutoDemoMax;
			g_Config.m_ClAutoDemoMax = static_cast<int>(DoScrollbarH(&s_ScrollbarAutoDemoMax, &Button, g_Config.m_ClAutoDemoMax/1000.0f)*1000.0f+0.1f);

			Right.HSplitTop(10.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Button.VSplitRight(20.0f, &Button, 0);
			if(g_Config.m_ClAutoScreenshotMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_ClAutoScreenshotMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), Localize("no limit"));
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Right.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			static CButtonContainer s_ScrollbarAutoScreenshotMax;
			g_Config.m_ClAutoScreenshotMax = static_cast<int>(DoScrollbarH(&s_ScrollbarAutoScreenshotMax, &Button, g_Config.m_ClAutoScreenshotMax/1000.0f)*1000.0f+0.1f);
		}

		Left.HSplitTop(20.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Label, &Left);
		Button.VSplitRight(20.0f, &Button, 0);
		char aBuf[64];
		if(g_Config.m_ClCpuThrottle)
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("CPU Throttle"), g_Config.m_ClCpuThrottle);
		else
			str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("CPU Throttle"), Localize("none"));
		UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
		Left.HSplitTop(20.0f, &Button, 0);
		Button.HMargin(2.0f, &Button);
		static CButtonContainer s_ScrollbarCpuThrottle;
		g_Config.m_ClCpuThrottle = round_to_int(DoScrollbarH(&s_ScrollbarCpuThrottle, &Button, g_Config.m_ClCpuThrottle/100.0f)*100.0f+0.1f);

		{
			CUIRect Checkbox;
			static CButtonContainer s_CheckboxPID[2];
			Left.HSplitTop(20.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Reconnect when server is full"), g_Config.m_ClReconnectFull);
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Left.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			Button.VSplitLeft(Button.h, &Checkbox, &Button);
			if(DoButton_CheckBox(&s_CheckboxPID[0], "", g_Config.m_ClReconnectFull, &Checkbox))
				g_Config.m_ClReconnectFull = g_Config.m_ClReconnectFull ? 0 : 5;
			Button.VSplitLeft(Button.h/2, 0, &Button);
			static CButtonContainer s_ScrollbarReconnectFull;
			if(g_Config.m_ClReconnectFull)
				g_Config.m_ClReconnectFull = max(5, round_to_int(DoScrollbarH(&s_ScrollbarReconnectFull, &Button, g_Config.m_ClReconnectFull/180.0f)*180.0f));

			Left.HSplitTop(20.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Reconnect on connection timeout"), g_Config.m_ClReconnectTimeout);
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Left.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			Button.VSplitLeft(Button.h, &Checkbox, &Button);
			if(DoButton_CheckBox(&s_CheckboxPID[1], "", g_Config.m_ClReconnectTimeout, &Checkbox))
				g_Config.m_ClReconnectTimeout = g_Config.m_ClReconnectTimeout ? 0 : 10;
			Button.VSplitLeft(Button.h/2, 0, &Button);
			static CButtonContainer s_ScrollbarReconnectTimeout;
			if(g_Config.m_ClReconnectTimeout)
				g_Config.m_ClReconnectTimeout = max(5, round_to_int(DoScrollbarH(&s_ScrollbarReconnectTimeout, &Button, g_Config.m_ClReconnectTimeout/180.0f)*180.0f));
		}

#if defined(CONF_FAMILY_WINDOWS)
		Left.HSplitTop(20.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		CButtonContainer s_HideConsoleButton;
		if(DoButton_CheckBox(&s_HideConsoleButton, Localize("Hide console window"), g_Config.m_ClHideConsole, &Button))
			g_Config.m_ClHideConsole ^= 1;
#endif

		// auto statboard screenshot
		{
			Right.HSplitTop(20.0f, 0, &Right); //
			Right.HSplitTop(20.0f, 0, &Right); // Make some distance so it looks more natural
			Right.HSplitTop(20.0f, &Button, &Right);
			static CButtonContainer s_CheckboxAutoStatboardScreenshot;
			if(DoButton_CheckBox(&s_CheckboxAutoStatboardScreenshot,
						Localize("Automatically take statboard screenshot"),
						g_Config.m_ClAutoStatboardScreenshot, &Button))
			{
				g_Config.m_ClAutoStatboardScreenshot ^= 1;
			}

			Right.HSplitTop(10.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Label, &Right);
			Button.VSplitRight(20.0f, &Button, 0);
			if(g_Config.m_ClAutoStatboardScreenshotMax)
				str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_ClAutoStatboardScreenshotMax);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), Localize("no limit"));
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Right.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			static CButtonContainer s_CheckboxAutoStatboardScreenshotMax;
			g_Config.m_ClAutoStatboardScreenshotMax =
				static_cast<int>(DoScrollbarH(&s_CheckboxAutoStatboardScreenshotMax,
							&Button,
							g_Config.m_ClAutoStatboardScreenshotMax/1000.0f)*1000.0f+0.1f);
		}
	}
}

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, Label, Dummy;
	MainView.HSplitTop(10.0f, 0, &MainView);

	char *Name = g_Config.m_PlayerName;
	char *Clan = g_Config.m_PlayerClan;
	int *Country = &g_Config.m_PlayerCountry;

	if(m_Dummy)
	{
		Name = g_Config.m_ClDummyName;
		Clan = g_Config.m_ClDummyClan;
		Country = &g_Config.m_ClDummyCountry;
	}

	// player name
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(200.0f, &Button, &Dummy);
	Button.VSplitLeft(150.0f, &Button, 0);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetName = 0.0f;
	static CButtonContainer s_EditboxName;
	if(DoEditBox(&s_EditboxName, &Button, Name, sizeof(g_Config.m_PlayerName), 14.0f, &s_OffsetName))
	{
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}

	Dummy.w /= 2.3f;
	static CButtonContainer s_CheckboxShowKillMessages;
	if(DoButton_CheckBox(&s_CheckboxShowKillMessages, Localize("Dummy settings"), m_Dummy, &Dummy))
	{
		m_Dummy ^= 1;
	}

	// player clan
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetClan = 0.0f;
	static CButtonContainer s_EditboxClan;
	if(DoEditBox(&s_EditboxClan, &Button, Clan, sizeof(g_Config.m_PlayerClan), 14.0f, &s_OffsetClan))
	{
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}

	// country flag selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	static CButtonContainer s_Listbox;
	UiDoListboxStart(&s_Listbox, &MainView, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

	for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
	{
		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
		if(pEntry->m_CountryCode == *Country)
			OldSelected = i;
		CPointerContainer Container(&pEntry->m_CountryCode);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);
			float OldWidth = Item.m_Rect.w;
			Item.m_Rect.w = Item.m_Rect.h*2;
			Item.m_Rect.x += (OldWidth-Item.m_Rect.w)/ 2.0f;
			vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
			m_pClient->m_pCountryFlags->Render(pEntry->m_CountryCode, &Color, Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
			if(pEntry->m_Texture != -1)
				UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		*Country = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}
}

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
		OwnSkinInfo.m_Texture = pOwnSkin->m_ColorTexture;
		OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(*ColorBody);
		OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(*ColorFeet);
	}
	else
	{
		OwnSkinInfo.m_Texture = pOwnSkin->m_OrgTexture;
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
			str_format(Entry.m_aName, sizeof(Entry.m_aName), str_comp(aName, "") != 0 ? aName : g_Config.m_PlayerName);
			str_format(Entry.m_aClan, sizeof(Entry.m_aClan), g_Config.m_PlayerClan);
			str_format(Entry.m_aSkin, sizeof(Entry.m_aSkin), g_Config.m_ClPlayerSkin);
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
		static int s_aColorSlider[2][3] = {{0}};

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
	UiDoListboxStart(&s_Listbox, &SkinList, 50.0f, Localize("Skins"), "", m_apSkinList.size(), 4, OldSelected, s_ScrollValue);
	for(int i = 0; i < m_apSkinList.size(); ++i)
	{
		const CSkins::CSkin *s = m_apSkinList[i];
		if(s == 0)
			continue;

		if(str_comp(s->m_aName, Skin) == 0)
			OldSelected = i;

		CPointerContainer Container(&m_apSkinList[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		char aBuf[128];
		if(Item.m_Visible)
		{
			CTeeRenderInfo Info;
			if(*UseCustomColor)
			{
				Info.m_Texture = s->m_ColorTexture;
				Info.m_ColorBody = m_pClient->m_pSkins->GetColorV4(*ColorBody);
				Info.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(*ColorFeet);
			}
			else
			{
				Info.m_Texture = s->m_OrgTexture;
				Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			}

			Info.m_Size = UI()->Scale()*50.0f;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+30, Item.m_Rect.y+Item.m_Rect.h/2));


			Item.m_Rect.VSplitLeft(60.0f, 0, &Item.m_Rect);
			Item.m_Rect.HSplitTop(10.0f, 0, &Item.m_Rect);
			str_format(aBuf, sizeof(aBuf), "%s", s->m_aName);
			RenderTools()->UI()->DoLabelScaled(&Item.m_Rect, aBuf, 12.0f, -1, Item.m_Rect.w, g_Config.m_ClSkinFilterString);
			if(g_Config.m_Debug)
			{
				vec3 BloodColor = *UseCustomColor ? m_pClient->m_pSkins->GetColorV3(*ColorBody) : s->m_BloodColor;
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
		mem_copy(Skin, m_apSkinList[NewSelected]->m_aName, sizeof(g_Config.m_ClPlayerSkin));
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
		QuickSearch.VSplitLeft(QuickSearch.w-15.0f, &QuickSearch, &QuickSearchClearButton);
		static float Offset = 0.0f;
		static CButtonContainer s_SkinFilterString;
		if(DoEditBox(&s_SkinFilterString, &QuickSearch, g_Config.m_ClSkinFilterString, sizeof(g_Config.m_ClSkinFilterString), 14.0f, &Offset, false, CUI::CORNER_L, Localize("Search")))
			m_InitSkinlist = true;

		// clear button
		{
			CPointerContainer s_ClearButton(&g_Config.m_ClSkinFilterString);
			if(DoButton_Menu(&s_ClearButton, "×", 0, &QuickSearchClearButton, "clear", CUI::CORNER_R, vec4(1,1,1,0.33f)))
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


typedef void (*pfnAssignFuncCallback)(CConfiguration *pConfig, int Value);

typedef struct
{
	CLocConstString m_Name;
	const char *m_pCommand;
	int m_KeyId;
} CKeyInfo;

static CKeyInfo gs_aKeys[] =
{
	{ "Move left", "+left", 0},		// Localize - these strings are localized within CLocConstString
	{ "Move right", "+right", 0 },
	{ "Jump", "+jump", 0 },
	{ "Fire", "+fire", 0 },
	{ "Hook", "+hook", 0 },
	{ "Hook Collisions", "+showhookcoll", 0 },
	{ "Toggle DynCam", "toggle cl_dyncam 0 1", 0 },
	{ "Hammer", "+weapon1", 0 },
	{ "Pistol", "+weapon2", 0 },
	{ "Shotgun", "+weapon3", 0 },
	{ "Grenade", "+weapon4", 0 },
	{ "Rifle", "+weapon5", 0 },
	{ "Next weapon", "+nextweapon", 0 },
	{ "Prev. weapon", "+prevweapon", 0 },
	{ "Vote yes", "vote yes", 0 },
	{ "Vote no", "vote no", 0 },
	{ "Chat", "+show_chat; chat all", 0 },
	{ "Team chat", "+show_chat; chat team", 0 },
	{ "Converse", "+show_chat; chat all /c ", 0 },
	{ "Show chat", "+show_chat", 0 },
	{ "Emoticon", "+emote", 0 },
	{ "Spectator mode", "+spectate", 0 },
	{ "Spectate next", "spectate_next", 0 },
	{ "Spectate previous", "spectate_previous", 0 },
	{ "Console", "toggle_local_console", 0 },
	{ "Remote console", "toggle_remote_console", 0 },
	{ "Screenshot", "screenshot", 0 },
	{ "Scoreboard", "+scoreboard", 0 },
	{ "Statboard", "+statboard", 0 },
	{ "Respawn", "kill", 0 },
	{ "Toggle Dummy", "toggle cl_dummy 0 1", 0 },
	{ "Dummy Copy", "toggle cl_dummy_copy_moves 0 1", 0 },
	{ "Hammerfly Dummy", "toggle cl_dummy_hammer 0 1", 0 },
	// ATH stuff
	{ "Hidden Chat", "+show_chat; chat hidden", 0 },
	{ "Crypted Chat", "+show_chat; chat crypt", 0 },
	{ "Hookfly Dummy", "toggle cl_dummy_hook_fly 0 1", 0 },
	{ "Toggle X-Ray", "toggle cl_overlay_entities 0 90", 0 },
	{ "Zoom in", "zoom+", 0 },
	{ "Zoom out", "zoom-", 0 },
	{ "Toggle IRC", "+irc", 0 },
	{ "Toggle Lua Console", "toggle_lua_console", 0 },
	{ "Toggle Hotbar", "+hotbar", 0 },
	{ "Unlock Mouse", "+unlock_mouse", 0 },
};

/*	This is for scripts/update_localization.py to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");Localize("Hammer");
	Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Rifle");Localize("Next weapon");Localize("Prev. weapon");
	Localize("Vote yes");Localize("Vote no");Localize("Chat");Localize("Team chat");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");Localize("Remote console");Localize("Screenshot");Localize("Scoreboard");Localize("Respawn");
	Localize("Hammerfly Dummy");Localize("Hidden Chat");Localize("Crypted Chat");Localize("Hookfly Dummy");Localize("Toggle X-Ray");Localize("Zoom in");
	Localize("Zoom out");Localize("Toggle IRC");Localize("Toggle Lua Console");Localize("Toggle Hotbar");Localize("Unlock Mouse");
*/

const int g_KeyCount = sizeof(gs_aKeys) / sizeof(CKeyInfo);

void CMenus::UiDoGetButtons(int Start, int Stop, CUIRect View)
{
	CALLSTACK_ADD();

	for(int i = Start; i < Stop; i++)
	{
		CKeyInfo &Key = gs_aKeys[i];
		CUIRect Button, Label;
		View.HSplitTop(20.0f, &Button, &View);
		Button.VSplitLeft(135.0f, &Label, &Button);

		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", (const char *)Key.m_Name);

		UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
		int OldId = Key.m_KeyId;
		CPointerContainer Container((void *)&gs_aKeys[i].m_Name);
		int NewId = DoKeyReader(&Container, &Button, OldId);
		if(NewId != OldId)
		{
			if(OldId != 0 || NewId == 0)
				m_pClient->m_pBinds->Bind(OldId, "");
			if(NewId != 0)
				m_pClient->m_pBinds->Bind(NewId, gs_aKeys[i].m_pCommand);
		}
		View.HSplitTop(2.0f, 0, &View);
	}
}

void CMenus::RenderSettingsControls(CUIRect MainView)
{
	CALLSTACK_ADD();

	// this is kinda slow, but whatever
	for(int i = 0; i < g_KeyCount; i++)
		gs_aKeys[i].m_KeyId = 0;

	for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
	{
		const char *pBind = m_pClient->m_pBinds->Get(KeyId);
		if(!pBind[0])
			continue;

		for(int i = 0; i < g_KeyCount; i++)
			if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
			{
				gs_aKeys[i].m_KeyId = KeyId;
				break;
			}
	}

	CUIRect MovementSettings, WeaponSettings, VotingSettings, ChatSettings, MiscSettings, ResetButton;
	MainView.VSplitMid(&MovementSettings, &VotingSettings);

	// movement settings
	{
		MovementSettings.VMargin(5.0f, &MovementSettings);
		MovementSettings.HSplitTop(MainView.h/3+75.0f, &MovementSettings, &WeaponSettings);
		static float s_AlphaAddition = 0.0f;
		if(UI()->MouseInside(&MovementSettings))
			smooth_set(&s_AlphaAddition, 0.10f, 60.0f*30.0f, Client()->RenderFrameTime());
		else
			smooth_set(&s_AlphaAddition, 0.0f, 60.0f*30.0f, Client()->RenderFrameTime());
		RenderTools()->DrawUIRect(&MovementSettings, vec4(1,1,1,0.25f+s_AlphaAddition), CUI::CORNER_ALL, 10.0f);
		MovementSettings.VMargin(10.0f, &MovementSettings);

		//TextRender()->Text(0, MovementSettings.x, MovementSettings.y, 14.0f*UI()->Scale(), Localize("Movement"), -1);

		MovementSettings.HSplitTop(14.0f, 0, &MovementSettings);

		{
			CUIRect Button, Label, InputBox;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(135.0f, &Label, &Button);
			UI()->DoLabel(&Label, Localize("Mouse sens."), 14.0f*UI()->Scale(), -1);
			Button.HMargin(2.0f, &Button);
			Button.VSplitLeft(50.0f, &InputBox, &Button);
			Button.VSplitLeft(9.0f, 0, &Button);
			static char aInput[6] = {0}; static float s_Offset = 0.0f;
			str_format(aInput, sizeof(aInput), "%i", g_Config.m_UiMousesens);
			static CButtonContainer s_Input, s_Scrollbar;
			DoEditBox(&s_Input, &InputBox, aInput, sizeof(aInput), 9.0f, &s_Offset, false);
			g_Config.m_UiMousesens = atoi(aInput);
			g_Config.m_UiMousesens = round_to_int(DoScrollbarH(&s_Scrollbar, &Button, (g_Config.m_UiMousesens-5)/995.0f)*995.0f)+5;
			MovementSettings.HSplitTop(5.0f, 0, &MovementSettings);
			if(g_Config.m_UiMousesens < 5) g_Config.m_UiMousesens = 5;
		}

		{
			CUIRect Button, Label, InputBox;
			MovementSettings.HSplitTop(20.0f, &Button, &MovementSettings);
			Button.VSplitLeft(135.0f, &Label, &Button);
			UI()->DoLabel(&Label, Localize("Ingame sens."), 14.0f*UI()->Scale(), -1);
			Button.HMargin(2.0f, &Button);
			Button.VSplitLeft(50.0f, &InputBox, &Button);
			Button.VSplitLeft(9.0f, 0, &Button);
			static char aInput[6] = {0}; static float s_Offset = 0.0f;
			str_format(aInput, sizeof(aInput), "%i", g_Config.m_InpMousesens);
			static CButtonContainer s_Input, s_Scrollbar;
			DoEditBox(&s_Input, &InputBox, aInput, sizeof(aInput), 9.0f, &s_Offset, false);
			g_Config.m_InpMousesens = atoi(aInput);
			g_Config.m_InpMousesens = round_to_int(DoScrollbarH(&s_Scrollbar, &Button, (g_Config.m_InpMousesens-5)/1995.0f)*1995.0f)+5;
			MovementSettings.HSplitTop(20.0f, 0, &MovementSettings);
			if(g_Config.m_InpMousesens < 5) g_Config.m_InpMousesens = 5;
		}

		UiDoGetButtons(0, 7, MovementSettings);

	}

	// weapon settings
	{
		WeaponSettings.HSplitTop(10.0f, 0, &WeaponSettings);
		WeaponSettings.HSplitTop(MainView.h/3+35.0f, &WeaponSettings, &ResetButton);
		static float s_AlphaAddition = 0.0f;
		if(UI()->MouseInside(&WeaponSettings))
			smooth_set(&s_AlphaAddition, 0.10f, 60.0f*30.0f, Client()->RenderFrameTime());
		else
			smooth_set(&s_AlphaAddition, 0.0f, 60.0f*30.0f, Client()->RenderFrameTime());
		RenderTools()->DrawUIRect(&WeaponSettings, vec4(1,1,1,0.25f+s_AlphaAddition), CUI::CORNER_ALL, 6.0f);
		WeaponSettings.VMargin(10.0f, &WeaponSettings);

		TextRender()->Text(0, WeaponSettings.x, WeaponSettings.y, 14.0f*UI()->Scale(), Localize("Weapon"), -1);

		WeaponSettings.HSplitTop(14.0f+5.0f+10.0f, 0, &WeaponSettings);
		UiDoGetButtons(7, 14, WeaponSettings);
	}

	// defaults
	{
		ResetButton.HSplitTop(10.0f, 0, &ResetButton);
		static float s_AlphaAddition = 0.0f;
		if(UI()->MouseInside(&ResetButton))
			smooth_set(&s_AlphaAddition, 0.10f, 60.0f*30.0f, Client()->RenderFrameTime());
		else
			smooth_set(&s_AlphaAddition, 0.0f, 60.0f*30.0f, Client()->RenderFrameTime());
		RenderTools()->DrawUIRect(&ResetButton, vec4(1,1,1,0.25f+s_AlphaAddition), CUI::CORNER_ALL, 10.0f);
		ResetButton.HMargin(10.0f, &ResetButton);
		ResetButton.VMargin(30.0f, &ResetButton);
		//ResetButton.HSplitTop(20.0f, &ResetButton, 0);
		static CButtonContainer s_DefaultButton;
		static int64 s_Clicked = 0;
		if(!s_Clicked)
		{
			if(DoButton_Menu(&s_DefaultButton, Localize("Reset to defaults"), 0, &ResetButton))
				s_Clicked = time_get();
		}
		else
		{
			if(DoButton_Menu(&s_DefaultButton, Localize("Are you sure?"), 0, &ResetButton, 0, CUI::CORNER_ALL, vec4(0.7f, 0.2f, 0.2f, 0.5f)))
			{
				m_pClient->m_pBinds->SetDefaults();
				s_Clicked = 0;
			}
		}
		if((s_Clicked && time_get() > s_Clicked + time_freq()*4) || !UI()->MouseInside(&ResetButton))
			s_Clicked = 0;
	}

	// voting settings
	{
		VotingSettings.VMargin(5.0f, &VotingSettings);
		VotingSettings.HSplitTop(MainView.h/3-106.0f, &VotingSettings, &ChatSettings);
		static float s_AlphaAddition = 0.0f;
		if(UI()->MouseInside(&VotingSettings))
			smooth_set(&s_AlphaAddition, 0.10f, 60.0f*30.0f, Client()->RenderFrameTime());
		else
			smooth_set(&s_AlphaAddition, 0.0f, 60.0f*30.0f, Client()->RenderFrameTime());
		RenderTools()->DrawUIRect(&VotingSettings, vec4(1,1,1,0.25f+s_AlphaAddition), CUI::CORNER_ALL, 6.0f);
		VotingSettings.VMargin(10.0f, &VotingSettings);

		//TextRender()->Text(0, VotingSettings.x, VotingSettings.y, 14.0f*UI()->Scale(), Localize("Voting"), -1);

		VotingSettings.HSplitTop(10.0f, 0, &VotingSettings);
		UiDoGetButtons(14, 16, VotingSettings);
	}

	// chat settings
	{
		ChatSettings.HSplitTop(10.0f, 0, &ChatSettings);
		ChatSettings.HSplitTop(MainView.h/3-56.0f, &ChatSettings, &MiscSettings);
		static float s_AlphaAddition = 0.0f;
		if(UI()->MouseInside(&ChatSettings))
			smooth_set(&s_AlphaAddition, 0.10f, 60.0f*30.0f, Client()->RenderFrameTime());
		else
			smooth_set(&s_AlphaAddition, 0.0f, 60.0f*30.0f, Client()->RenderFrameTime());
		RenderTools()->DrawUIRect(&ChatSettings, vec4(1,1,1,0.25f+s_AlphaAddition), CUI::CORNER_ALL, 6.0f);
		ChatSettings.VMargin(10.0f, &ChatSettings);

		//TextRender()->Text(0, ChatSettings.x, ChatSettings.y, 14.0f*UI()->Scale(), Localize("Chat"), -1);

		ChatSettings.HSplitTop(12.0f, 0, &ChatSettings);
		UiDoGetButtons(16, 20, ChatSettings);
	}

	// misc settings
	{
		MiscSettings.HSplitTop(10.0f, 0, &MiscSettings);
		static float s_AlphaAddition = 0.0f;
		if(UI()->MouseInside(&MiscSettings))
			smooth_set(&s_AlphaAddition, 0.10f, 60.0f*30.0f, Client()->RenderFrameTime());
		else
			smooth_set(&s_AlphaAddition, 0.0f, 60.0f*30.0f, Client()->RenderFrameTime());
		RenderTools()->DrawUIRect(&MiscSettings, vec4(1,1,1,0.25f+s_AlphaAddition), CUI::CORNER_ALL, 6.0f);
		MiscSettings.VMargin(10.0f, &MiscSettings);

		//TextRender()->Text(0, MiscSettings.x, MiscSettings.y, 14.0f*UI()->Scale(), Localize("Miscellaneous"), -1);

		MiscSettings.HSplitTop(13.0f, 0, &MiscSettings);
		UiDoGetButtons(20, 33, MiscSettings);
	}

}

void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button;
	char aBuf[128];
	bool CheckSettings = false;

	static const int MAX_RESOLUTIONS = 256;
	static CVideoMode s_aModes[MAX_RESOLUTIONS];
	static int s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
	static int s_GfxScreenWidth = g_Config.m_GfxScreenWidth;
	static int s_GfxScreenHeight = g_Config.m_GfxScreenHeight;
	static int s_GfxColorDepth = g_Config.m_GfxColorDepth;
	static int s_GfxVsync = g_Config.m_GfxVsync;
	static int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static int s_GfxTextureQuality = g_Config.m_GfxTextureQuality;
	static int s_GfxTextureCompression = g_Config.m_GfxTextureCompression;
	static int s_GfxHighdpi = g_Config.m_GfxHighdpi;

	CUIRect ModeList;
	MainView.VSplitLeft(300.0f, &MainView, &ModeList);

	// draw allmodes switch
	ModeList.HSplitTop(20, &Button, &ModeList);
	static CButtonContainer s_CheckboxDisplayAllModes;
	if(DoButton_CheckBox(&s_CheckboxDisplayAllModes, Localize("Show only supported"), g_Config.m_GfxDisplayAllModes^1, &Button))
	{
		g_Config.m_GfxDisplayAllModes ^= 1;
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
	}

	// display mode list
	static float s_ScrollValue = 0;
	int OldSelected = -1;
	int G = gcd(s_GfxScreenWidth, s_GfxScreenHeight);
	str_format(aBuf, sizeof(aBuf), "%s: %dx%d %d bit (%d:%d)", Localize("Current"), s_GfxScreenWidth, s_GfxScreenHeight, s_GfxColorDepth, s_GfxScreenWidth/G, s_GfxScreenHeight/G);
	static CButtonContainer s_Listbox;
	UiDoListboxStart(&s_Listbox , &ModeList, 24.0f, Localize("Display Modes"), aBuf, s_NumNodes, 1, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_NumNodes; ++i)
	{
		const int Depth = s_aModes[i].m_Red+s_aModes[i].m_Green+s_aModes[i].m_Blue > 16 ? 24 : 16;
		if(g_Config.m_GfxColorDepth == Depth &&
			g_Config.m_GfxScreenWidth == s_aModes[i].m_Width &&
			g_Config.m_GfxScreenHeight == s_aModes[i].m_Height)
		{
			OldSelected = i;
		}

		CPointerContainer Container(&s_aModes[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		if(Item.m_Visible)
		{
			int G = gcd(s_aModes[i].m_Width, s_aModes[i].m_Height);
			str_format(aBuf, sizeof(aBuf), " %dx%d %d bit (%d:%d)", s_aModes[i].m_Width, s_aModes[i].m_Height, Depth, s_aModes[i].m_Width/G, s_aModes[i].m_Height/G);
			UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, -1);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		const int Depth = s_aModes[NewSelected].m_Red+s_aModes[NewSelected].m_Green+s_aModes[NewSelected].m_Blue > 16 ? 24 : 16;
		g_Config.m_GfxColorDepth = Depth;
		g_Config.m_GfxScreenWidth = s_aModes[NewSelected].m_Width;
		g_Config.m_GfxScreenHeight = s_aModes[NewSelected].m_Height;
#if defined(SDL_VIDEO_DRIVER_X11)
		Graphics()->Resize(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
#else
		CheckSettings = true;
#endif
	}

	// switches
	static CButtonContainer s_Checkbox1, s_Checkbox2, s_Checkbox3, s_Checkbox4, s_Checkbox5, s_Checkbox6, s_Checkbox7;
	MainView.VSplitRight(30.0f, &MainView, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox1, Localize("Borderless window"), g_Config.m_GfxBorderless, &Button))
	{
		Client()->ToggleWindowBordered();
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox2, Localize("Fullscreen"), g_Config.m_GfxFullscreen, &Button))
	{
		Client()->ToggleFullscreen();
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox3, Localize("V-Sync"), g_Config.m_GfxVsync, &Button, Localize("Disable this if your game reacts too slow")))
	{
		Client()->ToggleWindowVSync();
	}

	if(Graphics()->GetNumScreens() > 1)
	{
		int NumScreens = Graphics()->GetNumScreens();
		MainView.HSplitTop(3.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		int Screen_MouseButton = DoButton_CheckBox_Number(&s_Checkbox4, Localize("Screen"), g_Config.m_GfxScreen, &Button);
		if(Screen_MouseButton == 1) //inc
		{
			Client()->SwitchWindowScreen((g_Config.m_GfxScreen+1)%NumScreens);
			s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		}
		else if(Screen_MouseButton == 2) //dec
		{
			Client()->SwitchWindowScreen((g_Config.m_GfxScreen-1+NumScreens)%NumScreens);
			s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		}
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	int GfxFsaaSamples_MouseButton = DoButton_CheckBox_Number(&s_Checkbox5, Localize("FSAA samples"), g_Config.m_GfxFsaaSamples, &Button, Localize("Smooths graphics at the expense of FPS"));
	if(GfxFsaaSamples_MouseButton == 1) //inc
	{
		g_Config.m_GfxFsaaSamples = (g_Config.m_GfxFsaaSamples+1)%17;
		CheckSettings = true;
	}
	else if(GfxFsaaSamples_MouseButton == 2) //dec
	{
		g_Config.m_GfxFsaaSamples = (g_Config.m_GfxFsaaSamples-1 +17)%17;
		CheckSettings = true;
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox6, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox7, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button, Localize("Disable this if you get blurry textures")))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxHighDetail;
	if(DoButton_CheckBox(&s_CheckboxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button, Localize("Show map decoration elements")))
		g_Config.m_GfxHighDetail ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxLowGraphics;
	if(DoButton_CheckBox(&s_CheckboxLowGraphics, Localize("Low Graphics Mode"), g_Config.m_GfxLowGraphics, &Button, Localize("Disable fancy effects to gain more fps")))
		g_Config.m_GfxLowGraphics ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxBackgroundRender;
	if(DoButton_CheckBox(&s_CheckboxBackgroundRender, Localize("Render when inactive"), g_Config.m_GfxBackgroundRender, &Button, Localize("Render graphics when window is in background")))
		g_Config.m_GfxBackgroundRender ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxNoclip;
	if(DoButton_CheckBox(&s_CheckboxNoclip, Localize("Disable clipping"), g_Config.m_GfxNoclip, &Button, Localize("May kill any performance teeworlds could have. Be careful with it.\n~ Info for nerds: GL_SCISSOR_TEST will be disabled and thus EVERYTHING will be rendered → hard laggs.")))
		g_Config.m_GfxNoclip ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxQuadAsTriangle;
	if(DoButton_CheckBox(&s_CheckboxQuadAsTriangle, Localize("Render quads as triangles"), g_Config.m_GfxQuadAsTriangle, &Button, Localize("Fixes quad coloring on some GPUs")))
		g_Config.m_GfxQuadAsTriangle ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxFinish;
	if(DoButton_CheckBox(&s_CheckboxFinish, Localize("Wait for GL commands to finish"), g_Config.m_GfxFinish, &Button, Localize("Can cause FPS laggs if enabled\n~ Info for nerds: glFinish() blocks until the effects of all GL executions are complete.")))
		g_Config.m_GfxFinish ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxHighdpi;
	if(DoButton_CheckBox(&s_CheckboxHighdpi, Localize("High-DPI screen support"), g_Config.m_GfxHighdpi, &Button, Localize("Be careful: experimental")))
	{
		g_Config.m_GfxHighdpi ^= 1;
		CheckSettings = true;
	}


	// check if the new settings require a restart
	if(CheckSettings)
	{
		if(s_GfxScreenWidth == g_Config.m_GfxScreenWidth &&
			s_GfxScreenHeight == g_Config.m_GfxScreenHeight &&
			s_GfxColorDepth == g_Config.m_GfxColorDepth &&
			s_GfxVsync == g_Config.m_GfxVsync &&
			s_GfxFsaaSamples == g_Config.m_GfxFsaaSamples &&
			s_GfxTextureQuality == g_Config.m_GfxTextureQuality &&
			s_GfxTextureCompression == g_Config.m_GfxTextureCompression &&
			s_GfxHighdpi == g_Config.m_GfxHighdpi)
			m_NeedRestartGraphics = false;
		else
			m_NeedRestartGraphics = true;
	}

	CUIRect Text;
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	//text.VSplitLeft(15.0f, 0, &text);
	{
		CUIRect temp;
		temp = Text;
		temp.y -= 2.5f*1.5f;
		temp.h = 21.0f*5+2.75f;
		RenderTools()->DrawUIRect(&temp, vec4(0,0,0,0.2f), CUI::CORNER_ALL, 5.0f);
	}
	Text.VMargin(15.0f, &Text);
	UI()->DoLabelScaled(&Text, Localize("UI Color"), 14.0f, -1);

	const char *paLabels[] = {
		Localize("Hue"),
		Localize("Sat."),
		Localize("Lht."),
		Localize("Alpha")};
	int *pColorSlider[4] = {&g_Config.m_UiColorHue, &g_Config.m_UiColorSat, &g_Config.m_UiColorLht, &g_Config.m_UiColorAlpha};
	for(int s = 0; s < 4; s++)
	{
		CUIRect Text;
		MainView.HSplitTop(19.0f, &Button, &MainView);
		Button.VMargin(15.0f, &Button);
		Button.VSplitLeft(100.0f, &Text, &Button);
		//Button.VSplitRight(5.0f, &Button, 0);
		Button.HSplitTop(4.0f, 0, &Button);

		float k = (*pColorSlider[s]) / 255.0f;
		CPointerContainer Container(pColorSlider[s]);
		k = DoScrollbarH(&Container, &Button, k, 0, k*255.0f);
		*pColorSlider[s] = (int)(k*255.0f);
		UI()->DoLabelScaled(&Text, paLabels[s], 15.0f, -1);
	}

	MainView.HSplitTop(20.0f, 0, &MainView);

	MainView.HSplitTop(20.0f+10.0f+15.0f, &Text, 0);
	Text.HMargin(-2.75f, &Text);
	Text.h += 2.75f;
	RenderTools()->DrawUIRect(&Text, vec4(0,0,0,0.2f), CUI::CORNER_ALL, 5.0f);

	MainView.VMargin(15.0f, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	UI()->DoLabelScaled(&Text, Localize("UI Scale"), 14.0f, -1);

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(15.0f, &Text, &MainView);
	static CButtonContainer s_Scrollbar;
	static int s_NewVal = g_Config.m_UiScale; // proxy it to not instantly change the ui size
	if(g_Config.m_UiScale != s_NewVal && UI()->ActiveItem() != (void*)&s_Scrollbar) // if it has been changed in f1
		s_NewVal = g_Config.m_UiScale;
	s_NewVal = round_to_int(50.0f+100.0f*DoScrollbarH(&s_Scrollbar, &Text, ((float)s_NewVal-50.0f)/100.0f, Localize("READ BEFORE CHANGING:\nIf you happen to mess it up so that this slider\nis not on your screen anymore, type in f1:\nui_scale 100"), s_NewVal));
	if(UI()->ActiveItem() != (void*)&s_Scrollbar)
		g_Config.m_UiScale = s_NewVal;
}

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button;
	MainView.VSplitMid(&MainView, 0);
	static int s_SndEnable = g_Config.m_SndEnable;
	static int s_SndRate = g_Config.m_SndRate;

	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndEnable;
	if(DoButton_CheckBox(&s_CheckboxSndEnable, Localize("Use sounds"), g_Config.m_SndEnable, &Button))
	{
		g_Config.m_SndEnable ^= 1;
		if(g_Config.m_SndEnable)
		{
			if(g_Config.m_SndMusic && Client()->State() == IClient::STATE_OFFLINE)
				m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
		}
		else
			m_pClient->m_pSounds->Stop(SOUND_MENU);
		m_NeedRestartSound = g_Config.m_SndEnable && (!s_SndEnable || s_SndRate != g_Config.m_SndRate);
	}

	if(!g_Config.m_SndEnable)
		return;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndMusic;
	if(DoButton_CheckBox(&s_CheckboxSndMusic, Localize("Play background music"), g_Config.m_SndMusic, &Button))
	{
		g_Config.m_SndMusic ^= 1;
		if(Client()->State() == IClient::STATE_OFFLINE)
		{
			if(g_Config.m_SndMusic)
				m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
			else
				m_pClient->m_pSounds->Stop(SOUND_MENU);
		}
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndNonactiveMute;
	if(DoButton_CheckBox(&s_CheckboxSndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
		g_Config.m_SndNonactiveMute ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndGame;
	if(DoButton_CheckBox(&s_CheckboxSndGame, Localize("Enable game sounds"), g_Config.m_SndGame, &Button))
		g_Config.m_SndGame ^= 1;

	if(g_Config.m_SndGame)
	{
		{
			MainView.HSplitTop(3.0f, 0, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable hammer sound"), g_Config.m_SndHammer, &Button))
				g_Config.m_SndHammer ^= 1;
		}

		{
			MainView.HSplitTop(3.0f, 0, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable gun sound"), g_Config.m_SndGun, &Button))
				g_Config.m_SndGun ^= 1;
		}

		{
			MainView.HSplitTop(3.0f, 0, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable shotgun sound"), g_Config.m_SndShotgun, &Button))
				g_Config.m_SndShotgun ^= 1;
		}

		{
			MainView.HSplitTop(3.0f, 0, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable grenade sound"), g_Config.m_SndGrenade, &Button))
				g_Config.m_SndGrenade ^= 1;
		}

		{
			MainView.HSplitTop(3.0f, 0, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable rifle sound"), g_Config.m_SndRifle, &Button))
				g_Config.m_SndRifle ^= 1;
		}

		{
			MainView.HSplitTop(3.0f, 0, &MainView);
			MainView.HSplitTop(20.0f, &Button, &MainView);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable spawn sound"), g_Config.m_SndSpawn, &Button))
				g_Config.m_SndSpawn ^= 1;
		}

		// TODO: Add more game sounds here!
	}


	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndServerMessage;
	if(DoButton_CheckBox(&s_CheckboxSndServerMessage, Localize("Enable server message sound"), g_Config.m_SndServerMessage, &Button))
		g_Config.m_SndServerMessage ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndChat;
	if(DoButton_CheckBox(&s_CheckboxSndChat, Localize("Enable regular chat sound"), g_Config.m_SndChat, &Button))
		g_Config.m_SndChat ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndTeamChat;
	if(DoButton_CheckBox(&s_CheckboxSndTeamChat, Localize("Enable team chat sound"), g_Config.m_SndTeamChat, &Button))
		g_Config.m_SndTeamChat ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndHighlight;
	if(DoButton_CheckBox(&s_CheckboxSndHighlight, Localize("Enable highlighted chat sound"), g_Config.m_SndHighlight, &Button))
		g_Config.m_SndHighlight ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndIRC;
	if(DoButton_CheckBox(&s_CheckboxSndIRC, Localize("Enable irc chat sound"), g_Config.m_SndIRC, &Button))
		g_Config.m_SndIRC ^= 1;

/*	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&g_Config.m_ClThreadsoundloading, Localize("Threaded sound loading"), g_Config.m_ClThreadsoundloading, &Button))
		g_Config.m_ClThreadsoundloading ^= 1;
*/
	// sample rate box
	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_SndRate);
		UI()->DoLabelScaled(&Button, Localize("Sample rate"), 14.0f, -1);
		Button.VSplitLeft(200.0f, 0, &Button);
		static float Offset = 0.0f;
		static CButtonContainer s_EditboxSndRate;
		DoEditBox(&s_EditboxSndRate, &Button, aBuf, sizeof(aBuf), 14.0f, &Offset);
		g_Config.m_SndRate = max(1, str_toint(aBuf));
		m_NeedRestartSound = !s_SndEnable || s_SndRate != g_Config.m_SndRate;
	}

	// volume slider
	{
		CUIRect Button, Label;
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(200.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sound volume"), 14.0f, -1);
		static CButtonContainer s_Scrollbar;
		g_Config.m_SndVolume = round_to_int(DoScrollbarH(&s_Scrollbar, &Button, g_Config.m_SndVolume/100.0f, 0, g_Config.m_SndVolume)*100.0f);
	}

	// volume slider map sounds
	{
		CUIRect Button, Label;
		MainView.HSplitTop(5.0f, &Button, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		Button.VSplitLeft(200.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Map sound volume"), 14.0f, -1);
		static CButtonContainer s_Scrollbar;
		g_Config.m_SndMapSoundVolume = round_to_int(DoScrollbarH(&s_Scrollbar, &Button, g_Config.m_SndMapSoundVolume/100.0f, 0, g_Config.m_SndMapSoundVolume)*100.0f);
	}
}

class CLanguage
{
public:
	CLanguage() {}
	CLanguage(const char *n, const char *f, int Code) : m_Name(n), m_FileName(f), m_CountryCode(Code) {}

	string m_Name;
	string m_FileName;
	int m_CountryCode;

	bool operator<(const CLanguage &Other) { return m_Name < Other.m_Name; }
};

void LoadLanguageIndexfile(IStorageTW *pStorage, IConsole *pConsole, sorted_array<CLanguage> *pLanguages)
{
	IOHANDLE File = pStorage->OpenFile("languages/index.txt", IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(!File)
	{
		pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "couldn't open index file");
		return;
	}

	char aOrigin[128];
	char aReplacement[128];
	CLineReader LineReader;
	LineReader.Init(File);
	char *pLine;
	while((pLine = LineReader.Get()))
	{
		if(!str_length(pLine) || pLine[0] == '#') // skip empty lines and comments
			continue;

		str_copy(aOrigin, pLine, sizeof(aOrigin));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			(void)LineReader.Get();
			continue;
		}
		str_copy(aReplacement, pLine+3, sizeof(aReplacement));

		pLine = LineReader.Get();
		if(!pLine)
		{
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", "unexpected end of index file");
			break;
		}

		if(pLine[0] != '=' || pLine[1] != '=' || pLine[2] != ' ')
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "malform replacement for index '%s'", aOrigin);
			pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "localization", aBuf);
			continue;
		}

		char aFileName[128];
		str_format(aFileName, sizeof(aFileName), "languages/%s.txt", aOrigin);
		pLanguages->add(CLanguage(aReplacement, aFileName, str_toint(pLine+3)));
	}
	io_close(File);
}

void CMenus::RenderLanguageSelection(CUIRect MainView)
{
	CALLSTACK_ADD();

	static CButtonContainer s_LanguageList;
	static int s_SelectedLanguage = 0;
	static sorted_array<CLanguage> s_Languages;
	static float s_ScrollValue = 0;

	if(s_Languages.size() == 0)
	{
		s_Languages.add(CLanguage("English", "", 826));
		LoadLanguageIndexfile(Storage(), Console(), &s_Languages);
		for(int i = 0; i < s_Languages.size(); i++)
			if(str_comp(s_Languages[i].m_FileName, g_Config.m_ClLanguagefile) == 0)
			{
				s_SelectedLanguage = i;
				break;
			}
	}

	int OldSelected = s_SelectedLanguage;

#if defined(__ANDROID__)
	UiDoListboxStart(&s_LanguageList , &MainView, 50.0f, Localize("Language"), "", s_Languages.size(), 1, s_SelectedLanguage, s_ScrollValue);
#else
	UiDoListboxStart(&s_LanguageList , &MainView, 24.0f, Localize("Language"), "", s_Languages.size(), 2, s_SelectedLanguage, s_ScrollValue);
#endif

	for(sorted_array<CLanguage>::range r = s_Languages.all(); !r.empty(); r.pop_front())
	{
		CPointerContainer Container(&r.front());
		CListboxItem Item = UiDoListboxNextItem(&Container);

		if(Item.m_Visible)
		{
			CUIRect Rect;
			Item.m_Rect.VSplitLeft(Item.m_Rect.h*2.0f, &Rect, &Item.m_Rect);
			Rect.VMargin(6.0f, &Rect);
			Rect.HMargin(3.0f, &Rect);
			vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
			m_pClient->m_pCountryFlags->Render(r.front().m_CountryCode, &Color, Rect.x, Rect.y, Rect.w, Rect.h);
			Item.m_Rect.HSplitTop(2.0f, 0, &Item.m_Rect);
			UI()->DoLabelScaled(&Item.m_Rect, r.front().m_Name, 16.0f, -1);
		}
	}

	s_SelectedLanguage = UiDoListboxEnd(&s_ScrollValue, 0);

	if(OldSelected != s_SelectedLanguage)
	{
		str_copy(g_Config.m_ClLanguagefile, s_Languages[s_SelectedLanguage].m_FileName, sizeof(g_Config.m_ClLanguagefile));
		g_Localization.Load(s_Languages[s_SelectedLanguage].m_FileName, Storage(), Console());

		// Load Font
		static CFont *pDefaultFont = 0;
		char aFilename[512];
		const char *pFontFile = "fonts/DejaVuSansCJKName.ttf";
		if (str_find(g_Config.m_ClLanguagefile, "chinese") != NULL || str_find(g_Config.m_ClLanguagefile, "japanese") != NULL ||
			str_find(g_Config.m_ClLanguagefile, "korean") != NULL)
			pFontFile = "fonts/DejavuWenQuanYiMicroHei.ttf";
		IOHANDLE File = Storage()->OpenFile(pFontFile, IOFLAG_READ, IStorageTW::TYPE_ALL, aFilename, sizeof(aFilename));
		if(File)
		{
			io_close(File);
			pDefaultFont = TextRender()->LoadFont(aFilename);
			TextRender()->SetDefaultFont(pDefaultFont);
		}
		if(!pDefaultFont)
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "gameclient", "failed to load font. filename='%s'", pFontFile);
	}
}

void CMenus::RenderSettings(CUIRect MainView)
{
	CALLSTACK_ADD();

	//static int s_SettingsPage = 0;

	// render background
	CUIRect Temp, TabBar, RestartWarning;
	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B|CUI::CORNER_TL, 10.0f);
	MainView.Margin(10.0f, &MainView);
	MainView.HSplitBottom(15.0f, &MainView, &RestartWarning);
	TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_R, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);

	CUIRect Button;

	const char *aTabs[] = {
		Localize("Language"),
		Localize("General"),
		Localize("Identities"),
		Localize("Controls"),
		Localize("Graphics"),
		Localize("Sound"),
		("Haxx"),
		m_pfnAppearanceSubpage == NULL ? Localize("Appearance") : Localize("< back"),
		Localize("Misc."),
#if defined(FEATURE_LUA)
		Localize("Lua"),
#endif
		//Localize("All")
	};

	const int NumTabs = (int)(sizeof(aTabs)/sizeof(*aTabs));
	static float FadeVals[NumTabs] = {0.0f};

	for(int i = 0; i < NumTabs; i++)
	{
		TabBar.HSplitTop(i == PAGE_SETTINGS_HAXX || i == PAGE_SETTINGS_LUA ? 24 : 10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(UI()->MouseInside(&Button))
			smooth_set(&FadeVals[i], 5.0f, 10.0f, Client()->RenderFrameTime());
		else
			smooth_set(&FadeVals[i], 0.0f, 10.0f, Client()->RenderFrameTime());
		Button.w += FadeVals[i];
		CPointerContainer Container(&aTabs[i]);
		if(DoButton_MenuTab(&Container, aTabs[i], g_Config.m_UiSettingsPage == i, &Button, CUI::CORNER_R,
				i == PAGE_SETTINGS_APPEARANCE && m_pfnAppearanceSubpage ? vec4(0.8f, 0.6f, 0.25f, ms_ColorTabbarActive.a) : ms_ColorTabbarActive
				))
		{
			m_pfnAppearanceSubpage = 0;
			g_Config.m_UiSettingsPage = i;
		}
	}

	MainView.Margin(10.0f, &MainView);

	if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_LANGUAGE)
		RenderLanguageSelection(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_GENERAL)
		RenderSettingsGeneral(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_IDENTITIES)
		RenderSettingsIdent(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_CONTROLS)
		RenderSettingsControls(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_GRAPHICS)
		RenderSettingsGraphics(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_SOUND)
		RenderSettingsSound(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_HAXX)
		RenderSettingsHaxx(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_APPEARANCE)
		RenderSettingsAppearance(MainView);
	else if(g_Config.m_UiSettingsPage == PAGE_SETTINGS_MISC)
		RenderSettingsDDNet(MainView);
	else if	(g_Config.m_UiSettingsPage == PAGE_SETTINGS_LUA)
		RenderSettingsLua(MainView);
	//else if	(g_Config.m_UiSettingsPage == PAGE_SETTINGS_ALL)
	//	RenderSettingsAll(MainView);

	if(m_NeedRestartUpdate)
	{
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		UI()->DoLabelScaled(&RestartWarning, Localize("DDNet Client needs to be restarted to complete update!"), 14.0f, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if(m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartDDNet)
	{
		static CButtonContainer s_ButtonRestart;
		if(DoButton_Menu(&s_ButtonRestart, Localize("You must restart the game for all settings to take effect."), 0, &RestartWarning, 0, CUI::CORNER_ALL, vec4(0.75f, 0.18f, 0.18f, 0.83f)))
			Client()->Restart();
	}
}

void CMenus::RenderSettingsHUDGeneral(CUIRect MainView)
{
	CUIRect Left, Right, HUD, Button, Label;

	MainView.HSplitTop(150.0f, &HUD, &MainView);

	HUD.HSplitTop(30.0f, &Label, &HUD);
	UI()->DoLabelScaled(&Label, Localize("HUD"), 20.0f, -1);
	HUD.Margin(5.0f, &HUD);
	HUD.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	// show hud
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxShowhud;
	if(DoButton_CheckBox(&s_CheckboxShowhud, Localize("Show ingame HUD"), g_Config.m_ClShowhud, &Button))
		g_Config.m_ClShowhud ^= 1;


	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxDDRaceScoreBoard;
	if (DoButton_CheckBox(&s_CheckboxDDRaceScoreBoard, Localize("Use DDRace Scoreboard"), g_Config.m_ClDDRaceScoreBoard, &Button))
	{
		g_Config.m_ClDDRaceScoreBoard ^= 1;
	}

	{
		CUIRect Second;
		Left.HSplitTop(3.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitMid(&Button, &Second);
		Button.w -= 2.5f; Second.w -= 2.5f;
		Second.x += 2.5f;
		static CButtonContainer s_CheckboxShowIDsScoreboard;
		if (DoButton_CheckBox(&s_CheckboxShowIDsScoreboard, Localize("Show IDs in Scoreboard"), g_Config.m_ClShowIDsScoreboard, &Button))
		{
			g_Config.m_ClShowIDsScoreboard ^= 1;
		}

		static CButtonContainer s_CheckboxShowIDsChat;
		if (DoButton_CheckBox(&s_CheckboxShowIDsChat, Localize("Show IDs in Chat"), g_Config.m_ClShowIDsChat, &Second))
		{
			g_Config.m_ClShowIDsChat ^= 1;
		}
	}

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxShowhudScore;
	if (DoButton_CheckBox(&s_CheckboxShowhudScore, Localize("Show score"), g_Config.m_ClShowhudScore, &Button))
	{
		g_Config.m_ClShowhudScore ^= 1;
	}

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxShowhudHealthAmmo;
	if (DoButton_CheckBox(&s_CheckboxShowhudHealthAmmo, Localize("Show health + ammo"), g_Config.m_ClShowhudHealthAmmo, &Button))
	{
		g_Config.m_ClShowhudHealthAmmo ^= 1;
	}

	if(g_Config.m_ClShowhudHealthAmmo)
	{
		Right.HSplitTop(3.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		Button.VSplitLeft(10.0f, 0, &Button);
		static CButtonContainer s_CheckboxShowhudHealthAmmoBars;
		char aLabel[128];
		if(g_Config.m_ClShowhudMode == 0)
			str_format(aLabel, sizeof(aLabel), "Mode: vanilla");
		if(g_Config.m_ClShowhudMode == 1)
			str_format(aLabel, sizeof(aLabel), "Mode: bars");
		if(g_Config.m_ClShowhudMode == 2)
			str_format(aLabel, sizeof(aLabel), "Mode: numbers");
		int ButtonUsed = DoButton_CheckBox_Number(&s_CheckboxShowhudHealthAmmoBars, aLabel, g_Config.m_ClShowhudMode, &Button);
		if(ButtonUsed == 1)
		{
			g_Config.m_ClShowhudMode = (g_Config.m_ClShowhudMode + 1) % 3;
		}
		else if(ButtonUsed == 2)
		{
			g_Config.m_ClShowhudMode = (g_Config.m_ClShowhudMode + 3 - 1) % 3;
		}
	}

	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxShowChat;
	if (DoButton_CheckBox(&s_CheckboxShowChat, Localize("Show chat"), g_Config.m_ClShowChat, &Button))
	{
		g_Config.m_ClShowChat ^= 1;
	}

	if(g_Config.m_ClShowChat)
	{
		Left.HSplitTop(3.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(10.0f, 0, &Button);
		static CButtonContainer s_CheckboxChatTeamColors;
		if(DoButton_CheckBox(&s_CheckboxChatTeamColors, Localize("Show names in chat in team colors"), g_Config.m_ClChatTeamColors, &Button))
		{
			g_Config.m_ClChatTeamColors ^= 1;
		}
	}

	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxShowKillMessages;
	if (DoButton_CheckBox(&s_CheckboxShowKillMessages, Localize("Show kill messages"), g_Config.m_ClShowKillMessages, &Button))
	{
		g_Config.m_ClShowKillMessages ^= 1;
	}

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxShowVotesAfterVoting;
	if (DoButton_CheckBox(&s_CheckboxShowVotesAfterVoting, Localize("Show votes window after voting"), g_Config.m_ClShowVotesAfterVoting, &Button))
	{
		g_Config.m_ClShowVotesAfterVoting ^= 1;
	}

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxNotifications;
	if (DoButton_CheckBox(&s_CheckboxNotifications, Localize("Show notifications"), g_Config.m_ClNotifications, &Button))
	{
		g_Config.m_ClNotifications ^= 1;
	}

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxChatbox;
	if (DoButton_CheckBox(&s_CheckboxChatbox, Localize("Show chatbox"), g_Config.m_ClShowhudChatbox > 0, &Button))
	{
		g_Config.m_ClShowhudChatbox = g_Config.m_ClShowhudChatbox ? 0 : 34;
	}

	if(g_Config.m_ClShowhudChatbox)
	{
		Right.HSplitTop(3.0f, 0, &Right);
		Right.HSplitTop(15.0f, &Button, &Right);
		Button.VSplitLeft(10.0f, 0, &Button);
		static CButtonContainer s_Scrollbar;
		g_Config.m_ClShowhudChatbox = round_to_int(DoScrollbarH(&s_Scrollbar, &Button, ((float)g_Config.m_ClShowhudChatbox-1.0f)/99.0f, Localize("Chatbox Alpha"), g_Config.m_ClShowhudChatbox)*99+1);
	}


}

void CMenus::RenderSettingsHUDColors(CUIRect MainView)
{
	CUIRect Left, Right, Button, Label, Messages, Weapon, Laser;
	MainView.HSplitTop(170.0f, &Messages, &MainView);
	Messages.HSplitTop(30.0f, &Label, &Messages);
	Label.VSplitMid(&Label, &Button);
	UI()->DoLabelScaled(&Label, Localize("Messages"), 20.0f, -1);
	Messages.Margin(5.0f, &Messages);
	Messages.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);
	{
		char aBuf[64];
		Left.HSplitTop(20.0f, &Label, &Left);
		Label.VSplitRight(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("System message"), 16.0f, -1);
		{
			static CButtonContainer s_DefaultButton;
			vec3 HSL = RgbToHsl(vec3(1.0f, 1.0f, 0.5f)); // default values
			if(((int)HSL.h != g_Config.m_ClMessageSystemHue) || ((int)HSL.s != g_Config.m_ClMessageSystemSat) || ((int)HSL.l != g_Config.m_ClMessageSystemLht))
			if (DoButton_Menu(&s_DefaultButton, Localize("Reset"), 0, &Button))
			{
				g_Config.m_ClMessageSystemHue = (int)HSL.h;
				g_Config.m_ClMessageSystemSat = (int)HSL.s;
				g_Config.m_ClMessageSystemLht = (int)HSL.l;
			}
		}
		static CButtonContainer s_Scrollbar1, s_Scrollbar2, s_Scrollbar3;
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Hue"), 14.0f, -1);
		g_Config.m_ClMessageSystemHue = (int)(DoScrollbarH(&s_Scrollbar1, &Button, g_Config.m_ClMessageSystemHue / 255.0f, 0, g_Config.m_ClMessageSystemHue)*255.0f);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sat."), 14.0f, -1);
		g_Config.m_ClMessageSystemSat = (int)(DoScrollbarH(&s_Scrollbar2, &Button, g_Config.m_ClMessageSystemSat / 255.0f, 0, g_Config.m_ClMessageSystemSat)*255.0f);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Lht."), 14.0f, -1);
		g_Config.m_ClMessageSystemLht = (int)(DoScrollbarH(&s_Scrollbar3, &Button, g_Config.m_ClMessageSystemLht / 255.0f, 0, g_Config.m_ClMessageSystemLht)*255.0f);

		Left.HSplitTop(10.0f, &Label, &Left);

		vec3 rgb = HslToRgb(vec3(g_Config.m_ClMessageSystemHue / 255.0f, g_Config.m_ClMessageSystemSat / 255.0f, g_Config.m_ClMessageSystemLht / 255.0f));
		TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);


		char aName[16];
		str_copy(aName, g_Config.m_PlayerName, sizeof(aName));
		str_format(aBuf, sizeof(aBuf), "*** '%s' entered and joined the spectators", aName);
		while (TextRender()->TextWidth(0, 12.0f, aBuf, -1) > Label.w)
		{
			aName[str_length(aName) - 1] = 0;
			str_format(aBuf, sizeof(aBuf), "*** '%s' entered and joined the spectators", aName);
		}
		UI()->DoLabelScaled(&Label, aBuf, 12.0f, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		Left.HSplitTop(20.0f, 0, &Left);
	}
	{
		char aBuf[64];
		Right.HSplitTop(20.0f, &Label, &Right);
		Label.VSplitRight(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Highlighted message"), 16.0f, -1);
		{
			static CButtonContainer s_DefaultButton;
			vec3 HSL = RgbToHsl(vec3(1.0f, 0.5f, 0.5f)); // default values
			if(((int)HSL.h != g_Config.m_ClMessageHighlightHue) || ((int)HSL.s != g_Config.m_ClMessageHighlightSat) || ((int)HSL.l != g_Config.m_ClMessageHighlightLht))
			if (DoButton_Menu(&s_DefaultButton, Localize("Reset"), 0, &Button))
			{
				g_Config.m_ClMessageHighlightHue = (int)HSL.h;
				g_Config.m_ClMessageHighlightSat = (int)HSL.s;
				g_Config.m_ClMessageHighlightLht = (int)HSL.l;
			}
		}
		static CButtonContainer s_Scrollbar1, s_Scrollbar2, s_Scrollbar3;
		Right.HSplitTop(20.0f, &Button, &Right);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Hue"), 14.0f, -1);
		g_Config.m_ClMessageHighlightHue = (int)(DoScrollbarH(&s_Scrollbar1, &Button, g_Config.m_ClMessageHighlightHue / 255.0f, 0, g_Config.m_ClMessageHighlightHue)*255.0f);

		Right.HSplitTop(20.0f, &Button, &Right);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sat."), 14.0f, -1);
		g_Config.m_ClMessageHighlightSat = (int)(DoScrollbarH(&s_Scrollbar2, &Button, g_Config.m_ClMessageHighlightSat / 255.0f, 0, g_Config.m_ClMessageHighlightSat)*255.0f);

		Right.HSplitTop(20.0f, &Button, &Right);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Lht."), 14.0f, -1);
		g_Config.m_ClMessageHighlightLht = (int)(DoScrollbarH(&s_Scrollbar3, &Button, g_Config.m_ClMessageHighlightLht / 255.0f, 0, g_Config.m_ClMessageHighlightLht)*255.0f);

		Right.HSplitTop(10.0f, &Label, &Right);

		TextRender()->TextColor(0.75f, 0.5f, 0.75f, 1.0f);
		float tw = TextRender()->TextWidth(0, 12.0f, Localize("Spectator"), -1);
		Label.VSplitLeft(tw, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Spectator"), 12.0f, -1);

		vec3 rgb = HslToRgb(vec3(g_Config.m_ClMessageHighlightHue / 255.0f, g_Config.m_ClMessageHighlightSat / 255.0f, g_Config.m_ClMessageHighlightLht / 255.0f));
		TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
		char name[16];
		str_copy(name, g_Config.m_PlayerName, sizeof(name));
		str_format(aBuf, sizeof(aBuf), ": %s: %s", name, Localize ("Look out!"));
		while (TextRender()->TextWidth(0, 12.0f, aBuf, -1) > Button.w)
		{
			name[str_length(name) - 1] = 0;
			str_format(aBuf, sizeof(aBuf), ": %s: %s", name, Localize("Look out!"));
		}
		UI()->DoLabelScaled(&Button, aBuf, 12.0f, -1);

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		Right.HSplitTop(20.0f, 0, &Right);
	}
	{
		char aBuf[64];
		Left.HSplitTop(20.0f, &Label, &Left);
		Label.VSplitRight(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Team message"), 16.0f, -1);
		{
			vec3 HSL = RgbToHsl(vec3(0.65f, 1.0f, 0.65f)); // default values
			static CButtonContainer s_DefaultButton;
			if(((int)HSL.h != g_Config.m_ClMessageTeamHue) || ((int)HSL.s != g_Config.m_ClMessageTeamSat) || ((int)HSL.l != g_Config.m_ClMessageTeamLht))
			if (DoButton_Menu(&s_DefaultButton, Localize("Reset"), 0, &Button))
			{
				g_Config.m_ClMessageTeamHue = (int)HSL.h;
				g_Config.m_ClMessageTeamSat = (int)HSL.s;
				g_Config.m_ClMessageTeamLht = (int)HSL.l;
			}
		}
		static CButtonContainer s_Scrollbar1, s_Scrollbar2, s_Scrollbar3;
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Hue"), 14.0f, -1);
		g_Config.m_ClMessageTeamHue = (int)(DoScrollbarH(&s_Scrollbar1, &Button, g_Config.m_ClMessageTeamHue / 255.0f, 0, g_Config.m_ClMessageTeamHue)*255.0f);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sat."), 14.0f, -1);
		g_Config.m_ClMessageTeamSat = (int)(DoScrollbarH(&s_Scrollbar2, &Button, g_Config.m_ClMessageTeamSat / 255.0f, 0, g_Config.m_ClMessageTeamSat)*255.0f);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Lht."), 14.0f, -1);
		g_Config.m_ClMessageTeamLht = (int)(DoScrollbarH(&s_Scrollbar3, &Button, g_Config.m_ClMessageTeamLht / 255.0f, 0, g_Config.m_ClMessageTeamLht)*255.0f);

		Left.HSplitTop(10.0f, &Label, &Left);

		TextRender()->TextColor(0.45f, 0.9f, 0.45f, 1.0f);
		float tw = TextRender()->TextWidth(0, 12.0f, Localize("Player"), -1);
		Label.VSplitLeft(tw, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Player"), 12.0f, -1);

		vec3 rgb = HslToRgb(vec3(g_Config.m_ClMessageTeamHue / 255.0f, g_Config.m_ClMessageTeamSat / 255.0f, g_Config.m_ClMessageTeamLht / 255.0f));
		TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
		str_format(aBuf, sizeof(aBuf), ": %s!",  Localize("We will win"));
		UI()->DoLabelScaled(&Button, aBuf, 12.0f, -1);

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		Left.HSplitTop(20.0f, 0, &Left);
	}
	{
		char aBuf[64];
		Left.HSplitTop(20.0f, &Label, &Left);
		Label.VSplitRight(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Normal message"), 16.0f, -1);
		{
			static CButtonContainer s_DefaultButton;
			vec3 HSL = RgbToHsl(vec3(1.0f, 1.0f, 1.0f)); // default values
			if(((int)HSL.h != g_Config.m_ClMessageHue) || ((int)HSL.s != g_Config.m_ClMessageSat) || ((int)HSL.l != g_Config.m_ClMessageLht))
			if (DoButton_Menu(&s_DefaultButton, Localize("Reset"), 0, &Button))
			{
				g_Config.m_ClMessageHue = (int)HSL.h;
				g_Config.m_ClMessageSat = (int)HSL.s;
				g_Config.m_ClMessageLht = (int)HSL.l;
			}
		}
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Hue"), 14.0f, -1);
		static CButtonContainer s_ScrollbarMsgHue;
		g_Config.m_ClMessageHue = (int)(DoScrollbarH(&s_ScrollbarMsgHue, &Button, g_Config.m_ClMessageHue / 255.0f, 0, g_Config.m_ClMessageHue)*255.0f);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sat."), 14.0f, -1);
		static CButtonContainer s_ScrollbarMsgSat;
		g_Config.m_ClMessageSat = (int)(DoScrollbarH(&s_ScrollbarMsgSat, &Button, g_Config.m_ClMessageSat / 255.0f, 0, g_Config.m_ClMessageSat)*255.0f);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Lht."), 14.0f, -1);
		static CButtonContainer s_ScrollbarMsgLht;
		g_Config.m_ClMessageLht = (int)(DoScrollbarH(&s_ScrollbarMsgLht, &Button, g_Config.m_ClMessageLht / 255.0f, 0, g_Config.m_ClMessageLht)*255.0f);

		Left.HSplitTop(10.0f, &Label, &Left);

		TextRender()->TextColor(0.8f, 0.8f, 0.8f, 1.0f);
		float tw = TextRender()->TextWidth(0, 12.0f, Localize("Player"), -1);
		Label.VSplitLeft(tw, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Player"), 12.0f, -1);

		vec3 rgb = HslToRgb(vec3(g_Config.m_ClMessageHue / 255.0f, g_Config.m_ClMessageSat / 255.0f, g_Config.m_ClMessageLht / 255.0f));
		TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
		str_format(aBuf, sizeof(aBuf), ": %s :D", Localize("Hello and welcome"));
		UI()->DoLabelScaled(&Button, aBuf, 12.0f, -1);

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	{
		Right.HSplitTop(220.0f, &Laser, &Right);
		RenderTools()->DrawUIRect(&Laser, vec4(1.0f, 1.0f, 1.0f, 0.1f), CUI::CORNER_ALL, 5.0f);
		Laser.Margin(10.0f, &Laser);
		Laser.HSplitTop(30.0f, &Label, &Laser);
		Label.VSplitLeft(TextRender()->TextWidth(0, 20.0f, Localize("Laser"), -1) + 5.0f, &Label, &Weapon);
		UI()->DoLabelScaled(&Label, Localize("Laser"), 20.0f, -1);

		Laser.HSplitTop(20.0f, &Label, &Laser);
		Label.VSplitLeft(5.0f, 0, &Label);
		Label.VSplitRight(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Inner color"), 16.0f, -1);
		{
			static CButtonContainer s_DefaultButton;
			vec3 HSL = RgbToHsl(vec3(0.5f, 0.5f, 1.0f)); // default values
			if(((int)HSL.h != g_Config.m_ClLaserInnerHue) || ((int)HSL.s != g_Config.m_ClLaserInnerSat) || ((int)HSL.l != g_Config.m_ClLaserInnerLht))
			if (DoButton_Menu(&s_DefaultButton, Localize("Reset"), 0, &Button))
			{
				g_Config.m_ClLaserInnerHue = (int)HSL.h;
				g_Config.m_ClLaserInnerSat = (int)HSL.s;
				g_Config.m_ClLaserInnerLht = (int)HSL.l;
			}
		}

		Laser.HSplitTop(20.0f, &Button, &Laser);
		Button.VSplitLeft(20.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Hue"), 12.0f, -1);
		static CButtonContainer s_ScrollbarLaserInnerHue;
		g_Config.m_ClLaserInnerHue = (int)(DoScrollbarH(&s_ScrollbarLaserInnerHue, &Button, g_Config.m_ClLaserInnerHue / 255.0f, 0, g_Config.m_ClLaserInnerHue)*255.0f);
		Laser.HSplitTop(20.0f, &Button, &Laser);
		Button.VSplitLeft(20.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sat."), 12.0f, -1);
		static CButtonContainer s_ScrollbarLaserInnerSat;
		g_Config.m_ClLaserInnerSat = (int)(DoScrollbarH(&s_ScrollbarLaserInnerSat, &Button, g_Config.m_ClLaserInnerSat / 255.0f, 0, g_Config.m_ClLaserInnerSat)*255.0f);
		Laser.HSplitTop(20.0f, &Button, &Laser);
		Button.VSplitLeft(20.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Lht."), 12.0f, -1);
		static CButtonContainer s_ScrollbarLaserInnerLht;
		g_Config.m_ClLaserInnerLht = (int)(DoScrollbarH(&s_ScrollbarLaserInnerLht, &Button, g_Config.m_ClLaserInnerLht / 255.0f, 0, g_Config.m_ClLaserInnerLht)*255.0f);

		Laser.HSplitTop(10.0f, 0, &Laser);

		Laser.HSplitTop(20.0f, &Label, &Laser);
		Label.VSplitLeft(5.0f, 0, &Label);
		Label.VSplitRight(50.0f, &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Outline color"), 16.0f, -1);
		{
			static CButtonContainer s_DefaultButton;
			vec3 HSL = RgbToHsl(vec3(0.075f, 0.075f, 0.25f)); // default values
			if(((int)HSL.h != g_Config.m_ClLaserOutlineHue) || ((int)HSL.s != g_Config.m_ClLaserOutlineSat) || ((int)HSL.l != g_Config.m_ClLaserOutlineLht))
			if (DoButton_Menu(&s_DefaultButton, Localize("Reset"), 0, &Button))
			{
				g_Config.m_ClLaserOutlineHue = (int)HSL.h;
				g_Config.m_ClLaserOutlineSat = (int)HSL.s;
				g_Config.m_ClLaserOutlineLht = (int)HSL.l;
			}
		}

		Laser.HSplitTop(20.0f, &Button, &Laser);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Hue"), 12.0f, -1);
		static CButtonContainer s_ScrollbarLaserOutlineHue;
		g_Config.m_ClLaserOutlineHue = (int)(DoScrollbarH(&s_ScrollbarLaserOutlineHue, &Button, g_Config.m_ClLaserOutlineHue / 255.0f, 0, g_Config.m_ClLaserOutlineHue)*255.0f);
		Laser.HSplitTop(20.0f, &Button, &Laser);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Sat."), 12.0f, -1);
		static CButtonContainer s_ScrollbarLaserOutlineSat;
		g_Config.m_ClLaserOutlineSat = (int)(DoScrollbarH(&s_ScrollbarLaserOutlineSat, &Button, g_Config.m_ClLaserOutlineSat / 255.0f, 0, g_Config.m_ClLaserOutlineSat)*255.0f);
		Laser.HSplitTop(20.0f, &Button, &Laser);
		Button.VSplitLeft(15.0f, 0, &Button);
		Button.VSplitLeft(100.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Lht."), 12.0f, -1);
		static CButtonContainer s_ScrollbarLaserOutlineLht;
		g_Config.m_ClLaserOutlineLht = (int)(DoScrollbarH(&s_ScrollbarLaserOutlineLht, &Button, g_Config.m_ClLaserOutlineLht / 255.0f, 0, g_Config.m_ClLaserOutlineLht)*255.0f);


		//Laser.HSplitTop(8.0f, &Weapon, &Laser);
		Weapon.VSplitLeft(30.0f, 0, &Weapon);

		vec3 RGB;
		vec2 From = vec2(Weapon.x, Weapon.y + Weapon.h / 2.0f);
		vec2 Pos = vec2(Weapon.x + Weapon.w - 10.0f, Weapon.y + Weapon.h / 2.0f);

		vec2 Out, Border;

		Graphics()->BlendNormal();
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();

		// do outline
		RGB = HslToRgb(vec3(g_Config.m_ClLaserOutlineHue / 255.0f, g_Config.m_ClLaserOutlineSat / 255.0f, g_Config.m_ClLaserOutlineLht / 255.0f));
		vec4 OuterColor(RGB.r, RGB.g, RGB.b, 1.0f);
		Graphics()->SetColor(RGB.r, RGB.g, RGB.b, 1.0f); // outline
		Out = vec2(0.0f, -1.0f) * (3.15f);

		IGraphics::CFreeformItem Freeform(
				From.x - Out.x, From.y - Out.y,
				From.x + Out.x, From.y + Out.y,
				Pos.x - Out.x, Pos.y - Out.y,
				Pos.x + Out.x, Pos.y + Out.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);

		// do inner
		RGB = HslToRgb(vec3(g_Config.m_ClLaserInnerHue / 255.0f, g_Config.m_ClLaserInnerSat / 255.0f, g_Config.m_ClLaserInnerLht / 255.0f));
		vec4 InnerColor(RGB.r, RGB.g, RGB.b, 1.0f);
		Out = vec2(0.0f, -1.0f) * (2.25f);
		Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f); // center

		Freeform = IGraphics::CFreeformItem(
				From.x - Out.x, From.y - Out.y,
				From.x + Out.x, From.y + Out.y,
				Pos.x - Out.x, Pos.y - Out.y,
				Pos.x + Out.x, Pos.y + Out.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);

		Graphics()->QuadsEnd();

		// render head
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
			Graphics()->QuadsBegin();

			int Sprites[] = { SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03 };
			RenderTools()->SelectSprite(Sprites[time_get() % 3]);
			Graphics()->QuadsSetRotation(time_get());
			Graphics()->SetColor(OuterColor.r, OuterColor.g, OuterColor.b, 1.0f);
			IGraphics::CQuadItem QuadItem(Pos.x, Pos.y, 24, 24);
			Graphics()->QuadsDraw(&QuadItem, 1);
			Graphics()->SetColor(InnerColor.r, InnerColor.g, InnerColor.b, 1.0f);
			QuadItem = IGraphics::CQuadItem(Pos.x, Pos.y, 20, 20);
			Graphics()->QuadsDraw(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
		// draw laser weapon
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
		Graphics()->QuadsBegin();

		RenderTools()->SelectSprite(SPRITE_WEAPON_RIFLE_BODY);
		RenderTools()->DrawSprite(Weapon.x, Weapon.y + Weapon.h / 2.0f, 60.0f);

		Graphics()->QuadsEnd();
	}
}

void CMenus::RenderSettingsHUD(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Left, Right;
	static int s_Page = 0;

	MainView.HSplitTop(20.0f, &Left, &MainView);
	Left.VSplitMid(&Left, &Right);
	static CButtonContainer s_TabSettings;
	if(DoButton_MenuTab(&s_TabSettings, Localize("Settings"), s_Page == 0, &Left, CUI::CORNER_L))
		s_Page = 0;

	static CButtonContainer s_TabColors;
	if(DoButton_MenuTab(&s_TabColors, Localize("Color Customization"), s_Page == 1, &Right, CUI::CORNER_R))
		s_Page = 1;

	if(s_Page == 0)
		RenderSettingsHUDGeneral(MainView);
	else if(s_Page == 1)
		RenderSettingsHUDColors(MainView);
	else
		s_Page = 0;

}

void CMenus::RenderSettingsDDNet(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, Left, Right, LeftLeft, Demo, Gameplay, Miscellaneous, Label, Background;

	bool CheckSettings = false;
	static int s_InpMouseOld = g_Config.m_InpMouseOld;

	MainView.HSplitTop(100.0f, &Demo , &MainView);

	Demo.HSplitTop(30.0f, &Label, &Demo);
	UI()->DoLabelScaled(&Label, Localize("Demo"), 20.0f, -1);
	Demo.Margin(5.0f, &Demo);
	Demo.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonAutoRaceRecord;
	if(DoButton_CheckBox(&s_ButtonAutoRaceRecord, Localize("Save the best demo of each race"), g_Config.m_ClAutoRaceRecord, &Button))
	{
		g_Config.m_ClAutoRaceRecord ^= 1;
	}

	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_ButtonRaceGhost;
	if(DoButton_CheckBox(&s_ButtonRaceGhost, Localize("Ghost"), g_Config.m_ClRaceGhost, &Button))
	{
		g_Config.m_ClRaceGhost ^= 1;
	}

	if(g_Config.m_ClRaceGhost)
	{
		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_ButtonRaceShowGhost;
		if(DoButton_CheckBox(&s_ButtonRaceShowGhost, Localize("Show ghost"), g_Config.m_ClRaceShowGhost, &Button))
		{
			g_Config.m_ClRaceShowGhost ^= 1;
		}

		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_ButtonRaceSaveGhost;
		if(DoButton_CheckBox(&s_ButtonRaceSaveGhost, Localize("Save ghost"), g_Config.m_ClRaceSaveGhost, &Button))
		{
			g_Config.m_ClRaceSaveGhost ^= 1;
		}
	}

	MainView.HSplitTop(290.0f, &Gameplay , &MainView);

	Gameplay.HSplitTop(30.0f, &Label, &Gameplay);
	UI()->DoLabelScaled(&Label, Localize("Gameplay"), 20.0f, -1);
	Gameplay.Margin(5.0f, &Gameplay);
	Gameplay.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	{
		CUIRect Button, Label;
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(120.0f, &Label, &Button);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Overlay entities"), 14.0f, -1);
		static CButtonContainer s_ButtonOverlayEntities;
		g_Config.m_ClOverlayEntities = (int)(DoScrollbarH(&s_ButtonOverlayEntities, &Button, g_Config.m_ClOverlayEntities/100.0f, 0, g_Config.m_ClOverlayEntities)*100.0f);
	}

	{
		CUIRect Button, Label;
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitMid(&LeftLeft, &Button);

		Button.VSplitLeft(50.0f, &Label, &Button);
		Label.VSplitRight(5.0f, &Label, 0);
		Button.HMargin(2.0f, &Button);
		UI()->DoLabelScaled(&Label, Localize("Alpha"), 14.0f, -1);
		static CButtonContainer s_ButtonShowOthersAlpha;
		g_Config.m_ClShowOthersAlpha = (int)(DoScrollbarH(&s_ButtonShowOthersAlpha, &Button, g_Config.m_ClShowOthersAlpha /100.0f, 0, g_Config.m_ClShowOthersAlpha)*100.0f);

		static CButtonContainer s_ButtonShowOthers;
		if(DoButton_CheckBox(&s_ButtonShowOthers, Localize("Show others"), g_Config.m_ClShowOthers, &LeftLeft))
		{
			g_Config.m_ClShowOthers ^= 1;
		}
	}

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonShowQuads;
	if(DoButton_CheckBox(&s_ButtonShowQuads, Localize("Show quads"), g_Config.m_ClShowQuads, &Button))
	{
		g_Config.m_ClShowQuads ^= 1;
	}

	Right.HSplitTop(20.0f, &Label, &Right);
	Label.VSplitLeft(130.0f, &Label, &Button);
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Default zoom"), g_Config.m_ClDefaultZoom);
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, -1);
	//Right.HSplitTop(20.0f, &Button, 0);
	Button.HMargin(2.0f, &Button);
	static CButtonContainer s_ButtonDefaultZoom;
	g_Config.m_ClDefaultZoom = static_cast<int>(DoScrollbarH(&s_ButtonDefaultZoom, &Button, g_Config.m_ClDefaultZoom/20.0f, 0, g_Config.m_ClDefaultZoom)*20.0f+0.1f);

	Right.HSplitTop(20.0f, &Label, &Right);
	Label.VSplitLeft(130.0f, &Label, &Button);
	str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("AntiPing limit"), g_Config.m_ClAntiPingLimit);
	UI()->DoLabelScaled(&Label, aBuf, 14.0f, -1);
	//Right.HSplitTop(20.0f, &Button, 0);
	Button.HMargin(2.0f, &Button);
	static CButtonContainer s_ButtonAntiPingLimit;
	g_Config.m_ClAntiPingLimit = static_cast<int>(DoScrollbarH(&s_ButtonAntiPingLimit, &Button, g_Config.m_ClAntiPingLimit/200.0f, 0, g_Config.m_ClAntiPingLimit)*200.0f+0.1f);

	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_ButtonAntiPing;
	if(DoButton_CheckBox(&s_ButtonAntiPing, Localize("AntiPing"), g_Config.m_ClAntiPing, &Button))
	{
		g_Config.m_ClAntiPing ^= 1;
	}

	if(g_Config.m_ClAntiPing)
	{
		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_ButtonAntiPingPlayers;
		if(DoButton_CheckBox(&s_ButtonAntiPingPlayers, Localize("AntiPing: predict other players"), g_Config.m_ClAntiPingPlayers, &Button))
		{
			g_Config.m_ClAntiPingPlayers ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_ButtonAntiPingWeapons;
		if(DoButton_CheckBox(&s_ButtonAntiPingWeapons, Localize("AntiPing: predict weapons"), g_Config.m_ClAntiPingWeapons, &Button))
		{
			g_Config.m_ClAntiPingWeapons ^= 1;
		}

		Right.HSplitTop(20.0f, &Button, &Right);
		static CButtonContainer s_ButtonAntiPingGrenade;
		if(DoButton_CheckBox(&s_ButtonAntiPingGrenade, Localize("AntiPing: predict grenade paths"), g_Config.m_ClAntiPingGrenade, &Button))
		{
			g_Config.m_ClAntiPingGrenade ^= 1;
		}
	}
	else
	{
		Right.HSplitTop(60.0f, 0, &Right);
	}

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonShowOtherHookColl;
	if(DoButton_CheckBox(&s_ButtonShowOtherHookColl, Localize("Show other players' hook collision lines"), g_Config.m_ClShowOtherHookColl, &Button))
	{
		g_Config.m_ClShowOtherHookColl ^= 1;
	}

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonShowDirection;
	if(DoButton_CheckBox(&s_ButtonShowDirection, Localize("Show other players' key presses"), g_Config.m_ClShowDirection, &Button))
	{
		g_Config.m_ClShowDirection ^= 1;
	}

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonMouseOld;
	if(DoButton_CheckBox(&s_ButtonMouseOld, Localize("Raw Mouse Input"), !g_Config.m_InpMouseOld, &Button, Localize("Use raw mouse input mode (the \"new one\")\nWARNING: MIGHT BE BUGGY/SLOW ON SOME SYSTEMS! In that case turn it off.")))
	{
		g_Config.m_InpMouseOld ^= 1;
		CheckSettings = true;
	}

	if(CheckSettings)
	{
		if(s_InpMouseOld == g_Config.m_InpMouseOld)
			m_NeedRestartDDNet = false;
		else
			m_NeedRestartDDNet = true;
	}

	Left.HSplitTop(5.0f, &Button, &Left);
	Right.HSplitTop(5.0f, &Button, &Right);
	CUIRect aRects[2] = { Left, Right };
	aRects[0].VSplitRight(10.0f, &aRects[0], 0);
	aRects[1].VSplitLeft(10.0f, 0, &aRects[1]);

	int *pColorSlider[2][3] = {{&g_Config.m_ClBackgroundHue, &g_Config.m_ClBackgroundSat, &g_Config.m_ClBackgroundLht}, {&g_Config.m_ClBackgroundEntitiesHue, &g_Config.m_ClBackgroundEntitiesSat, &g_Config.m_ClBackgroundEntitiesLht}};

	const char *paParts[] = {
		Localize("Background (regular)"),
		Localize("Background (entities)")};
	const char *paLabels[] = {
		Localize("Hue"),
		Localize("Sat."),
		Localize("Lht.")};

	for(int i = 0; i < 2; i++)
	{
		aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
		UI()->DoLabelScaled(&Label, paParts[i], 14.0f, -1);
		aRects[i].VSplitLeft(20.0f, 0, &aRects[i]);
		aRects[i].HSplitTop(2.5f, 0, &aRects[i]);

		for(int s = 0; s < 3; s++)
		{
			aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
			Label.VSplitLeft(100.0f, &Label, &Button);
			Button.HMargin(2.0f, &Button);

			float k = (*pColorSlider[i][s]) / 255.0f;
			CPointerContainer s_Scrollbar(&pColorSlider[i][s]);
			k = DoScrollbarH(&s_Scrollbar, &Button, k, 0, k*255.0f);
			*pColorSlider[i][s] = (int)(k*255.0f);
			UI()->DoLabelScaled(&Label, paLabels[s], 15.0f, -1);
		}
	}

	{
		static float s_Map = 0.0f;
		aRects[1].HSplitTop(25.0f, &Background, &aRects[1]);
		Background.HSplitTop(20.0f, &Background, 0);
		Background.VSplitLeft(100.0f, &Label, &Left);
		UI()->DoLabelScaled(&Label, Localize("Map"), 14.0f, -1);
		static CButtonContainer s_ButtonBackgroundEntities;
		DoEditBox(&s_ButtonBackgroundEntities, &Left, g_Config.m_ClBackgroundEntities, sizeof(g_Config.m_ClBackgroundEntities), 14.0f, &s_Map);

		aRects[1].HSplitTop(20.0f, &Button, 0);
		static CButtonContainer s_ButtonBackgroundShowTilesLayers;
		if(DoButton_CheckBox(&s_ButtonBackgroundShowTilesLayers, Localize("Show tiles layers from BG map"), g_Config.m_ClBackgroundShowTilesLayers, &Button))
		{
			g_Config.m_ClBackgroundShowTilesLayers ^= 1;
		}
	}

	MainView.HSplitTop(30.0f, &Label, &Miscellaneous);
	UI()->DoLabelScaled(&Label, Localize("Miscellaneous"), 20.0f, -1);
	Miscellaneous.VMargin(5.0f, &Miscellaneous);
	Miscellaneous.VSplitMid(&Left, &Right);
	Left.VSplitRight(5.0f, &Left, 0);
	Right.VMargin(5.0f, &Right);

	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonHttpMapDownload;
	if(DoButton_CheckBox(&s_ButtonHttpMapDownload, Localize("Try fast HTTP map download first"), g_Config.m_ClHttpMapDownload, &Button))
	{
		g_Config.m_ClHttpMapDownload ^= 1;
	}

	// Updater
#if defined(CONF_FAMILY_WINDOWS) || (defined(CONF_PLATFORM_LINUX) && !defined(__ANDROID__))
	{
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Label, &Left);
		bool NeedUpdate = Client()->LatestVersion()[2] && str_comp(Client()->LatestVersion(), GAME_ATH_VERSION) != 0;
		int State = Updater()->GetCurrentState();

		// update button
		if(NeedUpdate && State <= IUpdater::CLEAN)
		{
			str_format(aBuf, sizeof(aBuf), Localize("New Client Version '%s' is available!"), Client()->LatestVersion());
			Label.VSplitLeft(TextRender()->TextWidth(0, 14.0f, aBuf, -1) + 10.0f, &Label, &Button);
			Button.VSplitLeft(TextRender()->TextWidth(0, Button.h*ms_FontmodHeight, Localize("Update now"), -1), &Button, 0);
			static CButtonContainer s_ButtonUpdate;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Update now"), 0, &Button))
				Updater()->InitiateUpdate();
		}
		else if(State >= IUpdater::GETTING_MANIFEST && State < IUpdater::NEED_RESTART)
			str_format(aBuf, sizeof(aBuf), Localize("Updating..."));
		else if(State == IUpdater::NEED_RESTART){
			str_format(aBuf, sizeof(aBuf), Localize("AllTheHaxx updated!"));
			m_NeedRestartUpdate = true;
		}
		else
		{
			str_copy(aBuf, Localize("No updates available"), sizeof(aBuf));
			Label.VSplitLeft(TextRender()->TextWidth(0, 14.0f, Localize("No updates available"), -1) + 10.0f, &Label, &Button);
			Button.VSplitLeft(max(100.0f, TextRender()->TextWidth(0, 14.0f, Localize("Check now"), -1)), &Button, 0);
			static CButtonContainer s_ButtonUpdate;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Check now"), 0, &Button))
			{
				Client()->CheckVersionUpdate();
			}
		}
		UI()->DoLabelScaled(&Label, aBuf, 14.0f, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
#endif

	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_Checkbox;
	if(DoButton_CheckBox(&s_Checkbox, Localize("Enable Timeout Protection"), g_Config.m_ClTimeoutProtection, &Button))
		g_Config.m_ClTimeoutProtection ^= 1;

	if(g_Config.m_ClTimeoutProtection)
	{
		static CButtonContainer s_ButtonTimeout;
		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		Button.VSplitLeft(Button.w*(2.0f/3.0f), &Button, &Label);
		if(DoButton_Menu(&s_ButtonTimeout, Localize("New random timeout code"), 0, &Button, Localize("WARNING: In case you recently timed out somewhere:\nGenerating a new code will invalidate your timeout protection on all servers!")))
		{
			Client()->GenerateTimeoutSeed();
		}

		UI()->DoLabelScaled(&Label, g_Config.m_ClTimeoutSeed, 12, 1);
	}
}

int CMenus::SkinCacheListdirCallback(const char *name, int is_dir, int dir_type, void *user)
{
	if(is_dir)
	{
		if(name[0] != '.')
			dbg_msg("skincache", "warning: skincache seems to be polluted: Found a folder '%s', ignoring.", name);
		return 0;
	}
	IStorageTW *pStorage = (IStorageTW *)user;
	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "downloadedskins/%s", name);
	if(pStorage->RemoveFile(aBuf, IStorageTW::TYPE_SAVE))
		dbg_msg("skincache", "deleted file '%s'", aBuf);
	else
		dbg_msg("skincache", "failed to delete file %s", aBuf);
	return 0;
}

void CMenus::RenderSettingsHaxx(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Left, Right, Button;
	MainView.VSplitMid(&Left, &Right);
	Left.Margin(5.0f, &Left);
	//Right.Margin(5.0f, &Right);

	Left.HSplitTop(30.0f, &Button, &Left);
	UI()->DoLabelScaled(&Button, Localize("Haxx"), 20.0f, 0, -1);

	Left.HSplitTop(7.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonUsernameFetching;
	if(DoButton_CheckBox(&s_ButtonUsernameFetching, Localize("Gather Statistics"), g_Config.m_ClUsernameFetching, &Button))
		g_Config.m_ClUsernameFetching ^= 1;

//	Left.HSplitTop(5.0f, 0, &Left);
//	Left.HSplitTop(20.0f, &Button, &Left);
//	if(DoButton_CheckBox(&g_Config.m_ClChatDennisProtection, ("Dennis Protection"), g_Config.m_ClChatDennisProtection, &Button))
//		g_Config.m_ClChatDennisProtection ^= 1;

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonColorfulClient;
	if(DoButton_CheckBox(&s_ButtonColorfulClient, ("Colorful Client"), g_Config.m_ClColorfulClient, &Button, Localize("Makes everything look way more awesome!")))
		g_Config.m_ClColorfulClient ^= 1;

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonPathFinding;
	if(DoButton_CheckBox(&s_ButtonPathFinding, Localize("A* Path Finding"), g_Config.m_ClPathFinding, &Button, Localize("Find and visualize the shortest path to the finish on Race Maps")))
		g_Config.m_ClPathFinding ^= 1;

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonSmartZoom;
	if(DoButton_CheckBox(&s_ButtonSmartZoom, Localize("Smart zoom"), g_Config.m_ClSmartZoom, &Button, Localize("Smart zoom on race gametypes")))
		g_Config.m_ClSmartZoom ^= 1;

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonConsoleModeEmotes;
	if(DoButton_CheckBox(&s_ButtonConsoleModeEmotes, Localize("Console Mode Indicator"), g_Config.m_ClConsoleModeEmotes, &Button, Localize("Send Zzz emotes when in console mode")))
		g_Config.m_ClConsoleModeEmotes ^= 1;

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonChatbubble;
	if(DoButton_CheckBox(&s_ButtonChatbubble, Localize("Chatbubble"), g_Config.m_ClChatbubble, &Button, Localize("Send the chatbubble when you are typing")))
		g_Config.m_ClChatbubble ^= 1;

	{
		CUIRect ClearCacheButton;
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitRight(220.0f, &Button, &ClearCacheButton);
		static CButtonContainer s_ButtonSkinFetcher;
		if(DoButton_CheckBox(&s_ButtonSkinFetcher, Localize("Skin Fetcher"), g_Config.m_ClSkinFetcher, &Button, Localize("[EXPERIMENTAL!] Download skins from certain public skin databases automatically\nif a missing skin is used by somebody else on your server")))
			g_Config.m_ClSkinFetcher ^= 1;

		static CButtonContainer s_ClearCacheButton;
		ClearCacheButton.VSplitLeft(5.0f, 0, &ClearCacheButton);
		if(DoButton_Menu(&s_ClearCacheButton, Localize("Clear downloaded skins cache"), 0, &ClearCacheButton))
		{
			Storage()->ListDirectory(IStorageTW::TYPE_SAVE, "downloadedskins", SkinCacheListdirCallback, this->Storage());
			dbg_msg("skincache", "cleared");
		}
	}

	// extra binds!
	{
		// this is kinda slow, but whatever
		for(int i = 0; i < g_KeyCount; i++)
			gs_aKeys[i].m_KeyId = 0;

		for(int KeyId = 0; KeyId < KEY_LAST; KeyId++)
		{
			const char *pBind = m_pClient->m_pBinds->Get(KeyId);
			if(!pBind[0])
				continue;

			for(int i = 0; i < g_KeyCount; i++)
				if(str_comp(pBind, gs_aKeys[i].m_pCommand) == 0)
				{
					gs_aKeys[i].m_KeyId = KeyId;
					break;
				}
		}

		CUIRect Background;
		Left.HSplitTop(7.5f, 0, &Background);
		Background.h = 23.0f*10+2.0f;
		RenderTools()->DrawUIRect(&Background, vec4(0.2f, 0.5f, 0.2f, 0.68f), CUI::CORNER_ALL, 4.0f);
		Left.HSplitTop(7.0f, 0, &Left);
		Left.VMargin(10.0f, &Left);
		Left.HSplitTop(5.0f, 0, &Left);

		UiDoGetButtons(33, 43, Left);
		Left.h = 100.0f;
	}

	RenderSettingsIRC(Right);
}

void CMenus::RenderSettingsAppearance(CUIRect MainView)
{
	CALLSTACK_ADD();

	if(m_pfnAppearanceSubpage)
	{
		(*this.*m_pfnAppearanceSubpage)(MainView);
		return;
	}

	CUIRect Left, Right, Button;
	//MainView.VSplitMid(&Left, &Right);
	MainView.VMargin(MainView.w/3, &Left);

	static int s_Buttons[16] = {0};
	unsigned int index = 0;

#define DO_NEXT_BUTTON(BASERECT, TITLE, CALLBACK) \
	Left.HSplitTop(10.0f, 0, &BASERECT); \
	Left.HSplitTop(50.0f, &Button, &BASERECT); \
	TextRender()->Text(0, Button.x+Button.w-TextRender()->TextWidth(0, Button.h, ">", 1)-10.0f, Button.y-Button.h/3.6f, Button.h, ">", 9999); \
	{CPointerContainer Container(&s_Buttons[index++]); \
	if(DoButton_MenuTab(&Container, TITLE, 0, &Button, CUI::CORNER_ALL)) \
		m_pfnAppearanceSubpage = &CMenus::CALLBACK;}

	DO_NEXT_BUTTON(Left, "HUD", RenderSettingsAppearanceHUD);
	DO_NEXT_BUTTON(Left, Localize("Textures"), RenderSettingsAppearanceTexture);
	DO_NEXT_BUTTON(Left, Localize("Fonts"), RenderSettingsAppearanceFont);

//	RenderTools()->DrawUIRect(&Button, vec4(0,1,1,1), 0, 0);
}

void CMenus::RenderSettingsAppearanceHUD(CUIRect MainView) // here will be more tabs and stuff I think
{
	CALLSTACK_ADD();

	//RenderTools()->DrawUIRect(&MainView, vec4(1,0,1,1), 0, 0); // debuggi ^^
	RenderSettingsHUD(MainView);
}

void CMenus::RenderSettingsAppearanceTexture(CUIRect MainView)
{
	CALLSTACK_ADD();

	//RenderTools()->DrawUIRect(&MainView, vec4(0,1,1,1), 0, 0);
	RenderSettingsTexture(MainView);
}

void CMenus::RenderSettingsAppearanceFont(CUIRect MainView)
{
	CALLSTACK_ADD();

	//RenderTools()->DrawUIRect(&MainView, vec4(1,1,0,1), 0, 0);
	const int NUM_FONTS = m_pClient->m_pFontMgr->GetNumFonts();

	{
		char aBuf[64];
		CUIRect OptionsBar, Button;
		MainView.VSplitRight(MainView.w/3, &MainView, &OptionsBar);
		OptionsBar.x += 5.0f;

		OptionsBar.HSplitTop(20.0f, &Button, &OptionsBar);
		str_format(aBuf, sizeof(aBuf), Localize("%i fonts installed, %i loaded."), NUM_FONTS, m_pClient->m_pFontMgr->GetNumLoadedFonts());
		Button.x += 10.0f;
		UI()->DoLabelScaled(&Button, aBuf, Button.h-5.0f, -1, Button.w-3.0f);

		OptionsBar.HSplitTop(10.0f, &Button, &OptionsBar);
		OptionsBar.HSplitTop(20.0f, &Button, &OptionsBar);
		static CButtonContainer s_ReloadButton;
		if(DoButton_Menu(&s_ReloadButton, Localize("Reload"), 0, &Button))
			m_pClient->m_pFontMgr->ReloadFontlist();

		OptionsBar.HSplitTop(10.0f, &Button, &OptionsBar);
		OptionsBar.HSplitTop(20.0f, &Button, &OptionsBar);
		static CButtonContainer s_Checkbox;
		if(DoButton_CheckBox(&s_Checkbox, Localize("Preload fonts on starup"), g_Config.m_FtPreloadFonts, &Button))
			g_Config.m_FtPreloadFonts ^= 1;
	}

	const int OldSelected = m_pClient->m_pFontMgr->GetSelectedFontIndex();
	static int pIDItem[128] = {0};
	static CButtonContainer s_Listbox;
	static float s_ScrollValue = 0.0f;
	UiDoListboxStart(&s_Listbox, &MainView, 30.0f, Localize("Font Selector"), 0, NUM_FONTS, 1, OldSelected, s_ScrollValue, CUI::CORNER_ALL);
	for(int i = 0; i < NUM_FONTS; i++)
	{
		const char *pFilePath = m_pClient->m_pFontMgr->GetFontPath(i);
		if(!pFilePath || pFilePath[0] == '\0') continue;

		CPointerContainer Container(&pIDItem[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container, 0);

		if(Item.m_Visible)
		{
			if((i%2) && i != OldSelected)
				RenderTools()->DrawUIRect(&Item.m_Rect, vec4(0,0,0,0.35f), CUI::CORNER_ALL, 4.0f);
			//UI()->DoLabelScaled(&Item.m_Rect, pFilePath, Item.m_Rect.h-10.0f, -1, -1, 0, m_pClient->m_pFontMgr->GetFont(i));
			CTextCursor Cursor;
			TextRender()->SetCursor(&Cursor, Item.m_Rect.x, Item.m_Rect.y, 17.0f, TEXTFLAG_RENDER);
			Cursor.m_pFont = m_pClient->m_pFontMgr->GetFont(i);
			if(m_pClient->m_pFontMgr->GetFont(i))
				TextRender()->TextColor(1,1,0.85f, 1);
			else
				TextRender()->TextColor(1,1,1,1);
			TextRender()->TextEx(&Cursor, pFilePath, -1);
		}
	}

	int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(NewSelected != OldSelected)
	{
		m_pClient->m_pFontMgr->ActivateFont(NewSelected);
		str_copy(g_Config.m_FtFont, m_pClient->m_pFontMgr->GetFontPath(NewSelected), sizeof(g_Config.m_FtFont));
	}

}

void CMenus::RenderSettingsIRC(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button;
	MainView.Margin(5.0f, &MainView);

	MainView.HSplitTop(30.0f, &Button, &MainView);
	UI()->DoLabelScaled(&Button, Localize("Chat"), 20.0f, 0, -1);

	MainView.HSplitTop(7.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_ButtonIRCAutoconnect;
	if(DoButton_CheckBox(&s_ButtonIRCAutoconnect, Localize("Connect automatically"), g_Config.m_ClIRCAutoconnect, &Button, Localize("Connect to the Chat automatically on startup ... yeah, be social!")))
		g_Config.m_ClIRCAutoconnect ^= 1;

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_ButtonIRCPrintChat;
	if(DoButton_CheckBox(&s_ButtonIRCPrintChat, Localize("Print to console"), g_Config.m_ClIRCPrintChat, &Button))
		g_Config.m_ClIRCPrintChat ^= 1;

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_ButtonSndIRC;
	if(DoButton_CheckBox(&s_ButtonSndIRC, Localize("Play sound notification"), g_Config.m_SndIRC, &Button))
		g_Config.m_SndIRC ^= 1;

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_ButtonIRCAllowJoin;
	if(DoButton_CheckBox(&s_ButtonIRCAllowJoin, Localize("Allow others to join you"), g_Config.m_ClIRCAllowJoin, &Button))
		g_Config.m_ClIRCAllowJoin ^= 1;

	const char *s_apLabels[] = {
			Localize("Nickname"),
		//	Localize("Username"),
			Localize("Password"),
			Localize("Q Auth Name"),
			Localize("Q Auth Pass"),
			Localize("Modes"),
			Localize("Leave Message"),
	};

	CUIRect Background;
	MainView.HSplitTop(7.5f, 0, &Background);
	Background.h = 25.0f*(sizeof(s_apLabels)/sizeof(s_apLabels[0]))+7.5f;
	RenderTools()->DrawUIRect(&Background, vec4(0.2f, 0.5f, 0.2f, 0.68f), CUI::CORNER_ALL, 4.0f);

	CUIRect Label;
	int LabelIndex = 0;
#define DO_NEXT_LABEL Button.VSplitLeft(Button.w*0.4f, &Label, &Button); UI()->DoLabel(&Label, s_apLabels[LabelIndex++], 12.0f, -1, Label.w-3);

	MainView.VMargin(5.0f, &MainView);
	MainView.HSplitTop(15.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	DO_NEXT_LABEL
	static float s_OffsetNick = 0.0f;
	static CButtonContainer s_EditboxIRCNick;
	DoEditBox(&s_EditboxIRCNick, &Button, g_Config.m_ClIRCNick, sizeof(g_Config.m_ClIRCNick), 12.0f, &s_OffsetNick, false);
/*
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	DO_NEXT_LABEL
	static float s_OffsetUser = 0.0f;
	static CButtonContainer s_EditboxIRCUser;
	DoEditBox(&s_EditboxIRCUser, &Button, g_Config.m_ClIRCUser, sizeof(g_Config.m_ClIRCUser), 12.0f, &s_OffsetUser, false);
*/
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	DO_NEXT_LABEL
	static float s_OffsetPass = 0.0f;
	static CButtonContainer s_EditboxIRCPass;
	DoEditBox(&s_EditboxIRCPass, &Button, g_Config.m_ClIRCPass, sizeof(g_Config.m_ClIRCPass), 12.0f, &s_OffsetPass, true);

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	DO_NEXT_LABEL
	static float s_OffsetQAuthName = 0.0f;
	static CButtonContainer s_EditboxIRCQAuthName;
	DoEditBox(&s_EditboxIRCQAuthName, &Button, g_Config.m_ClIRCQAuthName, sizeof(g_Config.m_ClIRCQAuthName), 12.0f, &s_OffsetQAuthName, false);

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	DO_NEXT_LABEL
	static float s_OffsetQAuthPass = 0.0f;
	static CButtonContainer s_EditboxIRCQAuthPass;
	DoEditBox(&s_EditboxIRCQAuthPass, &Button, g_Config.m_ClIRCQAuthPass, sizeof(g_Config.m_ClIRCQAuthPass), 12.0f, &s_OffsetQAuthPass, true);

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	DO_NEXT_LABEL
	static float s_OffsetModes = 0.0f;
	static CButtonContainer s_EditboxIRCModes;
	DoEditBox(&s_EditboxIRCModes, &Button, g_Config.m_ClIRCModes, sizeof(g_Config.m_ClIRCModes), 12.0f, &s_OffsetModes, false);

	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	DO_NEXT_LABEL
	static float s_OffsetLeaveMsg = 0.0f;
	static CButtonContainer s_EditboxIRCLeaveMsg;
	DoEditBox(&s_EditboxIRCLeaveMsg, &Button, g_Config.m_ClIRCLeaveMsg, sizeof(g_Config.m_ClIRCLeaveMsg), 12.0f, &s_OffsetLeaveMsg, false);

#undef DO_NEXT_LABEL
}

#if defined(FEATURE_LUA)
void CMenus::RenderLoadingLua()
{
	return;
	CALLSTACK_ADD();

	Graphics()->Swap();

	CUIRect Bar, Rect = *UI()->Screen();

	Rect.Margin(Rect.w*0.23f, &Rect);

	Rect.HSplitTop(15.0f, &Bar, &Rect);
	RenderTools()->DrawUIRect(&Bar, vec4(0.2f, 0.7f, 0.2f, 0.8f), CUI::CORNER_T, 5.0f);
	RenderTools()->DrawUIRect(&Rect, vec4(0,0,0, 0.8f), CUI::CORNER_B, 5.0f);
	UI()->DoLabelScaled(&Bar, Localize("Please wait..."), 12.0f, 0, -1, 0);
	Rect.y += Rect.h/3; Rect.h *= 2/3;
	UI()->DoLabelScaled(&Rect, Localize("Loading Lua Script"), 18.0f, 0, -1, 0);

	Graphics()->Swap();
}

void CMenus::RenderSettingsLuaExceptions(CUIRect MainView, CLuaFile *L)
{
	static CButtonContainer s_BCListbox;
	static float s_ScrollVal = 1.0f;

	char aBuf[256];
	char aTitle[256];
	char aBottomText[32];
	str_format(aTitle, sizeof(aTitle), Localize("Exceptions thrown by script '%s'"), L->GetFilename());
	if(L->State() == CLuaFile::STATE_LOADED)
	{
		str_format(aBuf, sizeof(aBuf), " [%s]", Localize("Running"));
		str_append(aTitle, aBuf, sizeof(aTitle));
	}
	else if(L->State() == CLuaFile::STATE_ERROR)
	{
		str_format(aBuf, sizeof(aBuf), " [%s]", Localize("Aborted"));
		str_append(aTitle, aBuf, sizeof(aTitle));
	}
	str_format(aBottomText, sizeof(aBottomText), "%i/100", L->m_Exceptions.size());
	UiDoListboxStart(&s_BCListbox, &MainView, 20.0f, aTitle, aBottomText, L->m_Exceptions.size(), 1, -1, s_ScrollVal);

	for(int i = 0; i < L->m_Exceptions.size(); i++)
	{
		CPointerContainer Container(&(L->m_Exceptions[i]));
		CListboxItem Item = UiDoListboxNextItem(&Container);
		if(!Item.m_Visible)
			continue;

		if(UI()->MouseInside(&Item.m_Rect))
			RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 8.0f);

		CUIRect Button;
		str_format(aBuf, sizeof(aBuf), "#%i", i+1);
		//const float tw = TextRender()->TextWidth(0, 16.0f, aBuf, -1);
		Item.m_Rect.VSplitLeft(35.0f, &Button, &Item.m_Rect);
		UI()->DoLabelScaled(&Button, aBuf, 16.0f, -1);

		Item.m_Rect.VSplitLeft(10.0f, 0, &Item.m_Rect);
		str_format(aBuf, sizeof(aBuf), "@@ %s", L->m_Exceptions[i].c_str() +4/* + (str_find(L->m_Exceptions[i].c_str(), L->GetFilename()) != NULL ? str_length(L->GetFilename()) + 1 : 0)*/);
		UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, -1);
	}

	UiDoListboxEnd(&s_ScrollVal, 0);
}


void CMenus::RenderSettingsLua(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect ListView, Button, BottomBar;
	static int s_ActiveLuaSettings = -1;
	static int s_ActiveLuaExceptions = -1;
	static int s_SelectedScript = -1;

	if(g_Config.m_ClLua)
	{
		// render settings page if open
		if(s_ActiveLuaSettings >= 0)
		{
			try
			{
				CUIRect CloseButton;
				MainView.HSplitTop(20.0f, &CloseButton, &MainView);
				static CButtonContainer s_CloseButton;
				if(DoButton_Menu(&s_CloseButton, Localize("Close"), 0, &CloseButton, 0, CUI::CORNER_B) || !g_Config.m_ClLua)
				{
					Client()->Lua()->GetLuaFiles()[s_ActiveLuaSettings]->GetFunc("OnScriptSaveSettings")();
					s_ActiveLuaSettings = -1;
				}
				else
				{
					MainView.HSplitTop(10.0f, 0, &MainView);
					Client()->Lua()->GetLuaFiles()[s_ActiveLuaSettings]->GetFunc("OnScriptRenderSettings")(MainView);
					return;
				}
			}
			catch(std::exception& e)
			{
				Client()->Lua()->HandleException(e, Client()->Lua()->GetLuaFiles()[s_ActiveLuaSettings]);
				s_ActiveLuaSettings = -1;
			}
		}

		// render exceptions page if open
		if(s_ActiveLuaExceptions >= 0)
		{
			CUIRect CloseButton;
			MainView.HSplitTop(20.0f, &CloseButton, &MainView);
			static CButtonContainer s_CloseButton;
			if(DoButton_Menu(&s_CloseButton, Localize("Close"), 0, &CloseButton, 0, CUI::CORNER_B) || !g_Config.m_ClLua)
			{
				s_ActiveLuaExceptions = -1;
			}
			else
			{
				MainView.HSplitTop(10.0f, 0, &MainView);
				RenderSettingsLuaExceptions(MainView, Client()->Lua()->GetLuaFiles()[s_ActiveLuaExceptions]);
				return;
			}
		}
	}
	else
	{
		s_ActiveLuaSettings = -1;
		s_ActiveLuaExceptions = -1;
		s_SelectedScript = -1;
	}

	// render the main selection view
	MainView.VSplitLeft(MainView.w/2.0f, &ListView, &MainView);
	ListView.HSplitBottom(50.0f+5.0f, &ListView, &BottomBar);
	ListView.HSplitTop(20.0f, &Button, &ListView);


	CUIRect RefreshButton;
	Button.VSplitRight(max(100.0f, TextRender()->TextWidth(0, Button.h*ms_FontmodHeight, Localize("Refresh"), -1)), &Button, &RefreshButton);
	static CButtonContainer s_RefreshButton, s_LuaButton;
	if(g_Config.m_ClLua)
		if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &RefreshButton, "Reload the list of files", s_SelectedScript > -1 ? 0 : CUI::CORNER_TR))
		{
			s_SelectedScript = -1;
			Client()->Lua()->LoadFolder();
		}

	if(DoButton_CheckBox(&s_LuaButton, Localize("Use Lua"), g_Config.m_ClLua, &Button, 0, g_Config.m_ClLua ? CUI::CORNER_TL : CUI::CORNER_ALL))
	{
		s_SelectedScript = -1;
		if(!(g_Config.m_ClLua ^= 1))
		{
			Client()->Lua()->SaveAutoloads();
			Client()->Lua()->GetLuaFiles().delete_all();
			Client()->Lua()->GetLuaFiles().clear();
		}
		else
			Client()->Lua()->LoadFolder();
	}
	if(!g_Config.m_ClLua)
		return;


	static int ShowActiveOnly = 0;
	{
		int NumLuaFiles = Client()->Lua()->GetLuaFiles().size();

		// display mode list
		static float s_ScrollValue = 0;
		static CButtonContainer pIDItem[256];
		static CButtonContainer pIDButtonToggleScript[256];
		static CButtonContainer pIDButtonPermissions[256];
		static CButtonContainer pIDButtonAutoload[256];

		static CButtonContainer s_Listbox;
		CUIRect ListBox = ListView;
		char aHeadline[128], aBottomLine[128];
		static int NumListedFiles = 0, NumActiveScripts = 0;
		str_format(aHeadline, sizeof(aHeadline), Localize("%s (%i files listed, %i scripts active)"), Localize("Lua files"), NumListedFiles, NumActiveScripts);
		str_format(aBottomLine, sizeof(aBottomLine), Localize("(%i files found – %i filtered away)"), Client()->Lua()->GetLuaFiles().size(), Client()->Lua()->GetLuaFiles().size() - NumListedFiles);
		UiDoListboxStart(&s_Listbox, &ListBox, 50.0f, aHeadline, aBottomLine, NumListedFiles, 1, -1/*s_SelectedScript*/, s_ScrollValue, 0, 0);
		NumListedFiles = 0; NumActiveScripts = 0;
		for(int i = 0; i < NumLuaFiles; i++)
		{
			CLuaFile *L = Client()->Lua()->GetLuaFiles()[i];
			if(!L)
				continue;

			if(L->State() == CLuaFile::STATE_LOADED)
				NumActiveScripts++;

			// filter
			if(g_Config.m_ClLuaFilterString[0] != '\0' && (!str_find_nocase(L->GetFilename(), g_Config.m_ClLuaFilterString) && !str_find_nocase(L->GetScriptTitle(), g_Config.m_ClLuaFilterString)))
				continue;
			if(ShowActiveOnly == 1 && L->State() != CLuaFile::STATE_LOADED)
				continue;
			else if(ShowActiveOnly == 2 && L->State() == CLuaFile::STATE_LOADED)
				continue;

			CListboxItem Item = UiDoListboxNextItem(&pIDItem[i], 0);
			NumListedFiles++;

			if(Item.m_Visible)
			{
				CUIRect Label, Buttons, Button;

				Item.m_Rect.HMargin(2.5f, &Item.m_Rect);
				Item.m_Rect.HSplitTop(5.0f, 0, &Label);

				if(Item.m_Rect.y+Item.m_Rect.h > Item.m_HitRect.y)
				{
					if(UI()->MouseInside(&Item.m_Rect) && Input()->KeyPress(KEY_MOUSE_1))
						s_SelectedScript = i;

					// activate button
					Item.m_Rect.VSplitRight(Item.m_Rect.h*0.83f, &Item.m_Rect, &Button);
					if (DoButton_Menu(&pIDButtonToggleScript[i], L->State() == CLuaFile::STATE_LOADED ? "×" : "→", 0, &Button, L->State() == CLuaFile::STATE_LOADED ? Localize("Deactivate") : Localize("Activate"), CUI::CORNER_R))
					{
						if(L->State() == CLuaFile::STATE_LOADED)
							L->Unload();
						else
						{
							RenderLoadingLua();
							L->Init();
						}
					}


					// permission indicator
					Item.m_Rect.VSplitRight(Item.m_Rect.h/2.0f, &Item.m_Rect, &Buttons);
					Buttons.HSplitMid(&Buttons, &Button); // top: permission indicator, bottom: autoload checkbox

					int PermissionFlags = L->GetPermissionFlags();
					char aTooltip[1024] = {0};
					if(PermissionFlags == 0)
						str_format(aTooltip, sizeof(aTooltip), Localize("This script has no additional permissions and is thus considered safe."));
					else
					{
						str_format(aTooltip, sizeof(aTooltip), Localize("This script has the following additional permission:"));
#define PERM_STR(TYPE, STR) if(PermissionFlags&CLuaFile::PERMISSION_##TYPE) { str_append(aTooltip, "\n\n- ", sizeof(aTooltip)); str_append(aTooltip, STR, sizeof(aTooltip)); }
						PERM_STR(IO, Localize("IO (Write and read files)"))
						PERM_STR(DEBUG, Localize("DEBUG (WARNING: if you are not currently debugging this script, DO NOT TO USE IT!! It may cause security and performance problems!)"))
						PERM_STR(FFI, Localize("FFI (Execution of native C code from within Lua - please be sure that this code is not malicious, as the ATH API cannot control it"))
						PERM_STR(OS, Localize("OS (Access to various operation system functionalities such as time and date"))
						PERM_STR(PACKAGE, Localize("PACKAGE (Modules)"))
#undef PERM_APPEND
					}
					if(DoButton_Menu(&pIDButtonPermissions[i], "!", PermissionFlags, &Buttons, aTooltip, 0, vec4(PermissionFlags > 0 ? .7f : .2f, PermissionFlags > 0 ? .2f : .7f, .2f, .8f)))
						dbg_msg("lua/permissions", "'%s' | %i (%i)", L->GetFilename(), PermissionFlags, L->GetPermissionFlags());


					// autoload button
					if(DoButton_CheckBox(&pIDButtonAutoload[i], "", L->GetScriptIsAutoload(), &Button, Localize("Autoload"), 0))
					{
						L->SetScriptIsAutoload(!L->GetScriptIsAutoload());
					}


					// nice background
					vec4 Color = L->State() == CLuaFile::STATE_ERROR ? vec4(0.7f,0,0,0.3f) :
								 L->State() == CLuaFile::STATE_LOADED ? vec4(0,0.7f,0,0.3f) : vec4(0,0,0,0.3f);
					if(i == s_SelectedScript)
						RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1,1,1,0.5f), 0, 0);
					else if(i%2)
						Color.a += 0.2f;

					RenderTools()->DrawUIRect(&Item.m_Rect, Color, 0, 0);

					// script filename
					UI()->DoLabelScaled(&Label, L->GetFilename()+4, 14.0f, -1, -1/*Buttons.w-5.0f*/, g_Config.m_ClLuaFilterString);

				}

			}
		}

		UiDoListboxEnd(&s_ScrollValue, 0);
#undef PREPARE_BUTTON

		if(NumLuaFiles == 0)
		{
			CUIRect Label;
			ListView.HSplitBottom(ListView.h/2+15.0f, 0, &Label);
			UI()->DoLabelScaled(&Label, Localize("No files listed, click \"Refresh\" to reload the list"), 15.0f, 0, -1);
		}
	}


	// render the box at the right
	if(s_SelectedScript > -1 && s_SelectedScript < Client()->Lua()->GetLuaFiles().size())
	{
		RenderTools()->DrawUIRect(&MainView, vec4(0,0,0,0.25f), CUI::CORNER_R, 5.0f);

		CLuaFile *L = Client()->Lua()->GetLuaFiles()[s_SelectedScript];
		CUIRect Label;
		MainView.HSplitTop(10.0f, 0, &MainView);
		MainView.HSplitTop(25.0f, &Label, &MainView);
		if (L->GetScriptTitle()[0] != '\0')
			UI()->DoLabelScaled(&Label, L->GetScriptTitle(), 18.0f, 0, Label.w, g_Config.m_ClLuaFilterString);
		else
			UI()->DoLabelScaled(&Label, L->GetFilename()+4, 18.0f, 0, Label.w, g_Config.m_ClLuaFilterString);

		MainView.HSplitTop(10.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Label, &MainView);
		UI()->DoLabelScaled(&Label, L->GetScriptInfo(), 14.0f, 0, Label.w);


		// button bar at the bottom right
		CUIRect Button, Bar;
		MainView.HSplitBottom(10.0f, &MainView, 0);
		MainView.HSplitBottom(35.0f, &MainView, &Bar);

		Bar.VMargin(7.5f, &Bar);
//		Bar.VSplitRight(Bar.w/4.0f-4*5.0f, &Bar, &Button);

#define PREPARE_BUTTON(TEXT) Bar.VSplitRight(5.0f, &Bar, 0); Bar.VSplitRight(max(100.0f, TextRender()->TextWidth(0, Bar.h*ms_FontmodHeight, TEXT, -1)), &Bar, &Button);
		if(L->State() == CLuaFile::STATE_LOADED)
		{
			PREPARE_BUTTON(Localize("Deactivate"));
			static CButtonContainer s_DeactivateButton;
			if (DoButton_Menu(&s_DeactivateButton, Localize("Deactivate"), 0, &Button))
			{
				L->Unload();
			}

			static float s_ButtonReloadColorFade = 0.0f;
			static int s_PrevScriptIndex = s_SelectedScript;
			if(s_SelectedScript != s_PrevScriptIndex) s_ButtonReloadColorFade = 0.0f;
			smooth_set(&s_ButtonReloadColorFade, 0.0f, 15.0f, Client()->RenderFrameTime());
			PREPARE_BUTTON(Localize("Reload"));
			static CButtonContainer s_ReloadButton;
			if(DoButton_Menu(&s_ReloadButton, Localize("Reload"), 0, &Button, 0, CUI::CORNER_ALL, vec4(1.0f-s_ButtonReloadColorFade, 1.0f-s_ButtonReloadColorFade, 1.0f, 0.5f)))
			{
				s_ButtonReloadColorFade = 1.0f;
				RenderLoadingLua();
				L->Init();
			}


			if (L->GetScriptHasSettings())
			{
				PREPARE_BUTTON(Localize("Settings"));
				static CButtonContainer s_ButtonSettings;
				if (DoButton_Menu(&s_ButtonSettings, Localize("Settings"), 0, &Button))
				{
					s_ActiveLuaSettings = s_SelectedScript;
				}
			}

		}
		else
		{
			PREPARE_BUTTON(Localize("Activate"));
			static CButtonContainer s_ButtonActivate;
			if (DoButton_Menu(&s_ButtonActivate, Localize("Activate"), 0, &Button))
			{
				RenderLoadingLua();
				L->Init();
			}
		}

#undef PREPARE_BUTTON

		MainView.HSplitBottom(5.0f, &MainView, 0);
		MainView.HSplitBottom(20.0f, &MainView, &Bar);
		Bar.VMargin(7.5f+5.0f, &Bar);

		if(L->m_Exceptions.size() > 0)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), Localize("Exceptions (%i)"), L->m_Exceptions.size());
			Bar.VSplitRight(max(100.0f, TextRender()->TextWidth(0, Bar.h*ms_FontmodHeight, aBuf, -1)), &Bar, &Button);
			static CButtonContainer s_ButtonExceptions;
			if(DoButton_Menu(&s_ButtonExceptions, aBuf, 0, &Button, "", CUI::CORNER_ALL, mix(vec4(0,1,0,0.5f), vec4(1,0,0,0.5f), (float)L->m_Exceptions.size()/100.0f)))
			{
				s_ActiveLuaExceptions = s_SelectedScript;
			}
		}

		if(L->State() == CLuaFile::STATE_ERROR)
		{
			float FadeVal = sinf(Client()->SteadyTimer()*1.4f)/2.0f+0.5f;
			TextRender()->TextColor(1.0f, 0.25f+FadeVal*0.75f, 0.25f+FadeVal*0.75f, 1.0f);
			UI()->DoLabelScaled(&Bar, L->m_pErrorStr && L->m_pErrorStr[0] ? L->m_pErrorStr : Localize("An error occured"), 12.0f, -1, Bar.w);
			TextRender()->TextColor(1,1,1,1);
		}


		if(L->State() == CLuaFile::STATE_LOADED)
		{
			// let the script render stuff, if it wants to
			CUIRect View;
			MainView.VMargin(7.5f, &View);
			View.HSplitBottom(10.0f, &View, 0);
			View.HSplitTop(14.0f*(TextRender()->TextLineCount(0, 14.0f, L->GetScriptInfo(), MainView.w)-1)+5.0f, 0, &View);
			LuaRef func = Client()->Lua()->GetLuaFiles()[s_SelectedScript]->GetFunc("OnScriptRenderInfo");
			if(func.cast<bool>())
			{
				try
				{
					func(View);
				} catch(std::exception &e) { Client()->Lua()->HandleException(e, Client()->Lua()->GetLuaFiles()[s_SelectedScript]); }
			}
		}

	}


	// render the bottom bar at the left
	RenderTools()->DrawUIRect(&BottomBar, vec4(1,1,1,0.25f), s_SelectedScript > -1 ? CUI::CORNER_BL : CUI::CORNER_B, 5.0f);
	BottomBar.VMargin(10.0f, &BottomBar);
	BottomBar.HSplitBottom(5.0f, &BottomBar, 0);
	// render quick search
	{
		CUIRect QuickSearch, QuickSearchClearButton;
		BottomBar.HSplitTop(25.0f, &QuickSearch, &BottomBar);
		QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
		UI()->DoLabelScaled(&QuickSearch, "⚲", 14.0f, -1);
		float wSearch = TextRender()->TextWidth(0, 14.0f, "⚲", -1);
		QuickSearch.VSplitLeft(wSearch, 0, &QuickSearch);
		QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
		QuickSearch.VSplitLeft(QuickSearch.w-15.0f, &QuickSearch, &QuickSearchClearButton);
		static float Offset = 0.0f;
		static CButtonContainer s_LuaFilterStringEditbox;
		DoEditBox(&s_LuaFilterStringEditbox, &QuickSearch, g_Config.m_ClLuaFilterString, sizeof(g_Config.m_ClLuaFilterString), 14.0f, &Offset, false, CUI::CORNER_L, Localize("Search"));

		// clear button
		{
			static CButtonContainer s_ClearButton;
			if(DoButton_Menu(&s_ClearButton, "×", 0, &QuickSearchClearButton, "clear", CUI::CORNER_R, vec4(1,1,1,0.33f)))
			{
				g_Config.m_ClLuaFilterString[0] = 0;
				UI()->SetActiveItem(s_LuaFilterStringEditbox.GetID());
			}
		}
	}

	// render script-activation-filter button
	{
		const char *s_apLabels[] = {
				Localize("Showing all files"),
				Localize("Showing active scripts only"),
				Localize("Showing inactive scripts only")
		};

		static float Width = -1;
		if(Width < 0)
			for(int i = 0; i < 2; i++)
				Width = max(Width, TextRender()->TextWidth(0, BottomBar.h-10.0f, s_apLabels[i], -1));

		CUIRect Checkbox;
		BottomBar.HSplitTop(5.0f, 0, &BottomBar);
		BottomBar.VSplitLeft(TextRender()->TextWidth(0, 14.0f, Localize("Quickfilter:"), -1) + 5.0f, &BottomBar, &Checkbox);
		UI()->DoLabelScaled(&BottomBar, Localize("Quickfilter:"), 14.0f, -1);

		static CButtonContainer s_Checkbox;
		int MouseButton = DoButton_CheckBox_Number(&s_Checkbox, s_apLabels[ShowActiveOnly], ShowActiveOnly, &Checkbox);
		if(MouseButton == 1)
		{
			if(++ShowActiveOnly > 2)
				ShowActiveOnly = 0;
		}
		else if(MouseButton == 2)
		{
			if(--ShowActiveOnly < 0)
				ShowActiveOnly = 2;
		}
	}
}
#endif


// sort arrays
template<class T>
static void sort_simple_array(array<T> *pArray)
{
	const int NUM = pArray->size();
	if(NUM < 2)
		return;

	for(int curr = 0; curr < NUM-1; curr++)
	{
		int minIndex = curr;
		for(int i = curr + 1; i < NUM; i++)
		{
			int c = 4;
			for(; str_uppercase((*pArray)[i].pName[c]) == str_uppercase((*pArray)[minIndex].pName[c]); c++);
			if(str_uppercase((*pArray)[i].pName[c]) < str_uppercase((*pArray)[minIndex].pName[c]))
				minIndex = i;
		}

		if(minIndex != curr)
		{
			T temp = (*pArray)[curr];
			(*pArray)[curr] = (*pArray)[minIndex];
			(*pArray)[minIndex] = temp;
		}
	}
}

struct ConfigVar
{
	const char *pName;
	int Type;
	const char *pTooltip;
};

struct ConfigInt : public ConfigVar
{
	int *pValue;
	int Default;
	int Min;
	int Max;
};

struct ConfigString : public ConfigVar
{
	char *pStr;
	const char *pDef;
	int MaxLength;
};

void CMenus::RenderSettingsAll(CUIRect MainView)
{
	CALLSTACK_ADD();

	static array<ConfigInt> s_IntVars;
	static array<ConfigString> s_StringVars;

	if(s_IntVars.size() == 0)
	{

#define MACRO_CONFIG_INT(NAME,SCRIPTNAME,DEF,MIN,MAX,SAVE,DESC) \
		if((SAVE)&CFGFLAG_CLIENT) \
		{ \
			ConfigInt e; \
			e.pName = #SCRIPTNAME; \
			e.Type = 1; \
			e.pValue = &g_Config.m_##NAME; \
			e.pTooltip = DESC; \
			e.Default = DEF; \
			e.Min = MIN; \
			e.Max = MAX; \
			s_IntVars.add(e); \
		}


#define MACRO_CONFIG_STR(NAME,SCRIPTNAME,LEN,DEF,SAVE,DESC) \
		if((SAVE)&CFGFLAG_CLIENT) \
		{ \
			ConfigString e; \
			e.pName = #SCRIPTNAME; \
			e.Type = 2; \
			e.pTooltip = DESC; \
			e.pStr = g_Config.m_##NAME; \
			e.pDef = DEF; \
			e.MaxLength = LEN; \
			s_StringVars.add(e); \
		}

#include <engine/shared/config_variables.h>
#undef MACRO_CONFIG_INT
#undef MACRO_CONFIG_STR

		sort_simple_array<ConfigInt>(&s_IntVars);
		sort_simple_array<ConfigString>(&s_StringVars);

	} // end of one-time-initialization thingy


	static CButtonContainer s_Listbox;
	static float s_ScrollVal = 0.0f;
	static CButtonContainer s_IDs[1024];
	static CButtonContainer s_ScrollbarIDs[1024];
	static CButtonContainer s_EditboxIDs[1024];
	static float s_EditboxOffsets[1024] = {0.0f};
	CUIRect Test; MainView.Margin(20.0f, &Test);
	UiDoListboxStart(&s_Listbox, &MainView, 40.0f, Localize("-- Collection of all config variables --"), Localize("(only for advanced players - be careful!)"), s_IntVars.size()+s_StringVars.size(), 1, -1, s_ScrollVal);
	int i;
	for(i = 0; i < s_IntVars.size(); i++) // INT VARS VIA SLIDER
	{
		if(i >= 1024)
			break;

		CListboxItem Item = UiDoListboxNextItem(&s_IDs[i], 0);

		if(!Item.m_Visible)
			continue;

		CUIRect Button, Text;

		ConfigInt *var = &s_IntVars[i];
		int *pVal = var->pValue;
		if(!pVal)
		{
			dbg_msg("WTF", "%i %p %s", i, var, var->pName);
			continue;
		}
		Item.m_Rect.VSplitLeft(Item.m_Rect.w/3, &Text, &Button);
		UI()->DoLabelScaled(&Text, var->pName, 13.0f, -1, Text.w-5.0f);
		Button.Margin(12.0f, &Button);
		//**(var->pValue) = */round_to_int((var->Max-var->Min)*DoScrollbarH(&s_IDs[i], &Button, (*(var->pValue))/(var->Max-var->Min), var->pTooltip, *(var->pValue)));
		*pVal = max(var->Min, round_to_int((var->Max)*DoScrollbarH(&s_ScrollbarIDs[i], &Button, (float)*pVal/(float)var->Max, var->pTooltip, *pVal)));
	}
	for(int j = 0; j < s_StringVars.size(); j++) // STRING VARS VIA EDITBOX
	{
		if(i+j >= 1024)
			break;

		CListboxItem Item = UiDoListboxNextItem(&s_IDs[i+j], 0);

		if(!Item.m_Visible)
			continue;

		CUIRect Button, Text;
		ConfigString *var = &s_StringVars[j];
		if(!(var->pStr))
		{
			dbg_msg("so ne", "kacke %s", var->pName);
			continue;
		}
		Item.m_Rect.VSplitLeft(Item.m_Rect.w/3, &Text, &Button);
		UI()->DoLabelScaled(&Text, var->pName, 13.0f, -1, Text.w-5.0f);
		Button.Margin(7.0f, &Button);
		Button.VSplitRight(Button.w/3, &Button, &Text); Text.x+=2.5f;
		DoEditBox(&s_EditboxIDs[j], &Button, var->pStr, var->MaxLength, 13.0f, &s_EditboxOffsets[j], false, CUI::CORNER_ALL, 0, -1, var->pTooltip);
		Text.VSplitRight(100.0f, &Text, &Button);
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), Localize("Length: %i/%i"), str_length(var->pStr), var->MaxLength-1);
		UI()->DoLabelScaled(&Text, aBuf, 12.0f, -1, Text.w-5.0f);
		static CButtonContainer s_ResetButtons[1024];
		if(DoButton_Menu(&s_ResetButtons[j], Localize("Default"), 0, &Button, var->pDef))
			str_copy(var->pStr, var->pDef, var->MaxLength);
	}

	UiDoListboxEnd(&s_ScrollVal, 0);
}
