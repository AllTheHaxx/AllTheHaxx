#include "../menus.h"

#include <engine/keys.h>
#include <engine/textrender.h>
#include <game/localization.h>

#include "../controls.h"
#include "../binds.h"

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
	{ "Super-DynCam", "+super_dyncam", 0 },
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
	{ "Toggle IRC", "toggle_irc", 0 },
	{ "Toggle Lua Console", "toggle_lua_console", 0 },
	{ "Toggle Hotbar", "toggle_hotbar", 0 },
	{ "Unlock Mouse", "unlock_mouse", 0 },
};

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



































// TODO: move this out of here and there extra key bindings out of there
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
	if(DoButton_CheckBox(&s_ButtonPathFinding, Localize("A* Pathfinding"), g_Config.m_ClPathFinding, &Button, Localize("Find and visualize the shortest path to the finish on Race Maps")))
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

	Left.HSplitTop(5.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_ButtonBroadcastATH;
	if(DoButton_CheckBox(&s_ButtonBroadcastATH, Localize("Show others which client you are using :3"), g_Config.m_ClNamePlatesBroadcastATH, &Button, Localize("Disabling it has only an effect as long as no lua script is active!")))
		g_Config.m_ClNamePlatesBroadcastATH ^= 1;

	{
		CUIRect ClearCacheButton;
		Left.HSplitTop(5.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitRight(220.0f, &Button, &ClearCacheButton);
		static CButtonContainer s_ButtonSkinFetcher;
		if(DoButton_CheckBox(&s_ButtonSkinFetcher, Localize("Skin Fetcher"), g_Config.m_ClSkinFetcher, &Button, Localize("Download skins from certain public skin databases automatically if a missing skin is used by somebody else on your server")))
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
		Background.h = (23.0f*10.0f + 2.0f) * UI()->Scale();
		RenderTools()->DrawUIRect(&Background, vec4(mix((vec3)ms_GuiColor*0.65f, vec3(1), 0.10f), 0.68f) /*vec4(0.2f, 0.5f, 0.2f, 0.68f)*/, CUI::CORNER_ALL, 4.0f);
		Left.HSplitTop(7.0f, 0, &Left);
		Left.VMargin(10.0f, &Left);
		Left.HSplitTop(5.0f, 0, &Left);

		UiDoGetButtons(33, 43, Left);
		Left.h = 100.0f;
	}

	RenderSettingsIRC(Right);
}
