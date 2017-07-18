#include "../menus.h"

#include <engine/shared/jobs.h>
#include "../sounds.h"

void CMenus::RenderSettingsSound(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Right, Button;
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


	// do the left side (general sound settings)

	MainView.HSplitTop(5.0f, 0, &MainView); // a little more offset to the big fat "enable sounds" button
	MainView.VSplitMid(&MainView, &Right, 10.0f);

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
	static CButtonContainer s_CheckboxSndHighlightVanilla;
	if(DoButton_CheckBox(&s_CheckboxSndHighlightVanilla, Localize("Swap chat highlight sounds"), !g_Config.m_SndHighlightVanilla, &Button, Localize("Unchecked means vanilla behavior")))
		g_Config.m_SndHighlightVanilla ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndNonactiveMute;
	if(DoButton_CheckBox(&s_CheckboxSndNonactiveMute, Localize("Mute when not active"), g_Config.m_SndNonactiveMute, &Button))
		g_Config.m_SndNonactiveMute ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxSndThread;
	if(DoButton_CheckBox(&s_CheckboxSndThread, Localize("Threaded sound loading"), g_Config.m_ClThreadsoundloading, &Button))
		g_Config.m_ClThreadsoundloading ^= 1;

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


	// do the right side (all the "enable stuff" checkboxes

	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxSndGame;
	if(DoButton_CheckBox(&s_CheckboxSndGame, Localize("Enable game sounds"), g_Config.m_SndGame, &Button))
		g_Config.m_SndGame ^= 1;

	if(g_Config.m_SndGame) // cascaded
	{
		{
			Right.HSplitTop(3.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable hammer sound"), g_Config.m_SndHammer, &Button))
				g_Config.m_SndHammer ^= 1;
		}

		{
			Right.HSplitTop(3.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable gun sound"), g_Config.m_SndGun, &Button))
				g_Config.m_SndGun ^= 1;
		}

		{
			Right.HSplitTop(3.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable shotgun sound"), g_Config.m_SndShotgun, &Button))
				g_Config.m_SndShotgun ^= 1;
		}

		{
			Right.HSplitTop(3.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable grenade sound"), g_Config.m_SndGrenade, &Button))
				g_Config.m_SndGrenade ^= 1;
		}

		{
			Right.HSplitTop(3.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable rifle sound"), g_Config.m_SndRifle, &Button))
				g_Config.m_SndRifle ^= 1;
		}

		{
			Right.HSplitTop(3.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable spawn sound"), g_Config.m_SndSpawn, &Button))
				g_Config.m_SndSpawn ^= 1;
		}

		{
			Right.HSplitTop(3.0f, 0, &Right);
			Right.HSplitTop(20.0f, &Button, &Right);
			Button.VSplitLeft(10.0f, 0, &Button);
			static CButtonContainer s_Checkbox;
			if(DoButton_CheckBox(&s_Checkbox, Localize("Enable pain sound"), g_Config.m_SndLongPain, &Button))
				g_Config.m_SndLongPain ^= 1;
		}

		// TODO: Add more game sounds here!
	}


	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxSndServerMessage;
	if(DoButton_CheckBox(&s_CheckboxSndServerMessage, Localize("Enable server message sound"), g_Config.m_SndServerMessage, &Button))
		g_Config.m_SndServerMessage ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxSndChat;
	if(DoButton_CheckBox(&s_CheckboxSndChat, Localize("Enable regular chat sound"), g_Config.m_SndChat, &Button))
		g_Config.m_SndChat ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxSndTeamChat;
	if(DoButton_CheckBox(&s_CheckboxSndTeamChat, Localize("Enable team chat sound"), g_Config.m_SndTeamChat, &Button))
		g_Config.m_SndTeamChat ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxSndHighlight;
	if(DoButton_CheckBox(&s_CheckboxSndHighlight, Localize("Enable highlighted chat sound"), g_Config.m_SndHighlight, &Button))
		g_Config.m_SndHighlight ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxSndIRC;
	if(DoButton_CheckBox(&s_CheckboxSndIRC, Localize("Enable irc chat sound"), g_Config.m_SndIRC, &Button))
		g_Config.m_SndIRC ^= 1;
}
