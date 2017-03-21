#include "../menus.h"

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/generated/client_data.h>


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
