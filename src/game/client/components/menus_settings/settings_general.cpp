#include "../menus.h"

#include <engine/textrender.h>

void CMenus::RenderSettingsGeneral(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Label, Button, Left, Right, Game, Client;
	MainView.HSplitTop(155.0f, &Game, &Client);

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
		if(DoButton_CheckBox(&s_DynamicCameraButton, Localize("Dynamic Camera"), g_Config.m_ClDyncam, &Button))
		{
			g_Config.m_ClDyncam ^= 1;
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

	}

	// client
	{
		// headline
		Client.HSplitTop(30.0f, &Label, &Client);
		UI()->DoLabelScaled(&Label, Localize("Client"), 20.0f, -1);
		Client.Margin(5.0f, &Client);
		Client.VSplitMid(&Left, &Right);
		Left.VSplitRight(5.0f, &Left, 0);
		Right.VMargin(5.0f, &Right);

		// auto demo settings
		{
			// auto-record demos checkbox (on the left)
			Left.HSplitTop(20.0f, &Button, &Left);
			static CButtonContainer s_CheckboxAutoDemoRecord;
			if(DoButton_CheckBox(&s_CheckboxAutoDemoRecord, Localize("Automatically record demos"), g_Config.m_ClAutoDemoRecord, &Button, 0, g_Config.m_ClAutoDemoRecord ? CUI::CORNER_T : CUI::CORNER_ALL))
				g_Config.m_ClAutoDemoRecord ^= 1;

			// auto-take screenshot checkbox (on the right)
			Right.HSplitTop(20.0f, &Button, &Right);
			static CButtonContainer s_CheckboxAutoScreenshot;
			if(DoButton_CheckBox(&s_CheckboxAutoScreenshot, Localize("Automatically take game over screenshot"), g_Config.m_ClAutoScreenshot, &Button, 0, g_Config.m_ClAutoScreenshot ? CUI::CORNER_T : CUI::CORNER_ALL))
				g_Config.m_ClAutoScreenshot ^= 1;

			// max demos (on the left)
			if(g_Config.m_ClAutoDemoRecord)
			{
				// render background
				CUIRect Background;
				Left.HSplitTop(10.0f+20.0f, &Background, 0);
				RenderTools()->DrawUIRect(&Background, vec4(0.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_B, 5.0f);

				// do 'max demos' label (on the left)
				Left.HSplitTop(10.0f, 0, &Left);
				Left.HSplitTop(20.0f, &Label, &Left);
				Label.VMargin(5.0f, &Label);
				char aBuf[64];
				if(g_Config.m_ClAutoDemoMax)
					str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max demos"), g_Config.m_ClAutoDemoMax);
				else
					str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max demos"), Localize("no limit"));
				UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);

				// do 'max demos' scrollbar (on the left)
				Label.VSplitLeft(140.0f, 0, &Button);
				Button.HMargin(2.0f, &Button);
				static CButtonContainer s_ScrollbarAutoDemoMax;
				g_Config.m_ClAutoDemoMax = (int)(DoScrollbarH(&s_ScrollbarAutoDemoMax, &Button, g_Config.m_ClAutoDemoMax/1000.0f)*1000.0f+0.1f);
			}

			// do 'max screenshots' label (on the right)
			if(g_Config.m_ClAutoScreenshot)
			{
				char aBuf[64];
				Right.HSplitTop(10.0f, 0, &Right);
				Right.HSplitTop(20.0f, &Label, &Right);
				if(g_Config.m_ClAutoScreenshotMax)
					str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("Max Screenshots"), g_Config.m_ClAutoScreenshotMax);
				else
					str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Max Screenshots"), Localize("no limit"));
				UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);

				// do 'max screenshots' scrollbar (on the right)
				Right.HSplitTop(20.0f, &Button, 0);
				Button.HMargin(2.0f, &Button);
				static CButtonContainer s_ScrollbarAutoScreenshotMax;
				g_Config.m_ClAutoScreenshotMax = static_cast<int>(DoScrollbarH(&s_ScrollbarAutoScreenshotMax, &Button, g_Config.m_ClAutoScreenshotMax/1000.0f)*1000.0f+0.1f);
			}
		}

		// do 'cpu throttle' label
		Left.HSplitTop(10.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Label, &Left);
		char aBuf[64];
		if(g_Config.m_ClCpuThrottle)
			str_format(aBuf, sizeof(aBuf), "%s: %i", Localize("CPU Throttle"), g_Config.m_ClCpuThrottle);
		else
		{
			str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("CPU Throttle"), Localize("none"));
			TextRender()->TextColor(0.8f, 0.1f, 0.1f, 1.0f);
		}
		UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
		TextRender()->TextColor(1,1,1,1);

		// do 'cpu throttle' scrollbar
		Label.VSplitLeft(150.0f, 0, &Button);
		Button.HMargin(2.0f, &Button);
		static CButtonContainer s_ScrollbarCpuThrottle;
		g_Config.m_ClCpuThrottle = round_to_int(
				DoScrollbarH(&s_ScrollbarCpuThrottle, &Button, g_Config.m_ClCpuThrottle/100.0f,
							 "Makes the client use less CPU; too high values result in stuttering (fewer fps!)\n\n"
									 "WARNING: Setting this to 'none' will make ATH use up one CPU core completely! Recommended value is 1"
				)*100.0f+0.1f);

		// reconnect
		{
			CUIRect Checkbox;
			static CButtonContainer s_CheckboxPID[2];
			Left.HSplitTop(10.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			if(g_Config.m_ClReconnectFull)
				str_format(aBuf, sizeof(aBuf), "%s: %is", Localize("Reconnect when server is full"), g_Config.m_ClReconnectFull);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Reconnect when server is full"), Localize("never"));
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

			Left.HSplitTop(25.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			if(g_Config.m_ClReconnectTimeout)
				str_format(aBuf, sizeof(aBuf), "%s: %is", Localize("Reconnect on connection timeout"), g_Config.m_ClReconnectTimeout);
			else
				str_format(aBuf, sizeof(aBuf), "%s: %s", Localize("Reconnect on connection timeout"), Localize("never"));
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

			Left.HSplitTop(25.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Label, &Left);
			Button.VSplitRight(20.0f, &Button, 0);
			str_format(aBuf, sizeof(aBuf), Localize("Disconnect on 'Connection Problems' after %i seconds"), g_Config.m_ConnTimeout);
			UI()->DoLabelScaled(&Label, aBuf, 13.0f, -1);
			Left.HSplitTop(20.0f, &Button, 0);
			Button.HMargin(2.0f, &Button);
			Button.VSplitLeft(1.5f, 0, &Button);
			static CButtonContainer s_ScrollbarCPsTimeout;
			g_Config.m_ConnTimeout = round_to_int(DoScrollbarH(&s_ScrollbarCPsTimeout, &Button, g_Config.m_ConnTimeout/600.0f)*600.0f);
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
