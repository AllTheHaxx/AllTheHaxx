/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/config.h>
#include <engine/demo.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/localization.h>
#include <game/client/components/countryflags.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/ui.h>

#include "binds.h"
#include "identity.h"
#include "menus.h"
#include "motd.h"
#include "spoofremote.h"
#include "voting.h"

#include <base/tl/string.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include "ghost.h"

void CMenus::RenderGame(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, ButtonBar;
#if defined(__ANDROID__)
	MainView.HSplitTop(100.0f, &ButtonBar, &MainView);
#else
	MainView.HSplitTop(45.0f, &ButtonBar, &MainView);
#endif
	RenderTools()->DrawUIRect(&ButtonBar, ms_ColorTabbarActive, g_Config.m_ClUiShowExtraBar ? CUI::CORNER_T : CUI::CORNER_ALL, 10.0f);

	{
		CUIRect ExtraBar;
#if defined(__ANDROID__)
		MainView.HSplitTop(100.0f, &ExtraBar, &MainView);
#else
		MainView.HSplitTop(45.0f, &ExtraBar, &MainView);
#endif
		RenderGameExtra(ExtraBar);
	}

	// button bar
	ButtonBar.HSplitTop(10.0f, 0, &ButtonBar);
#if defined(__ANDROID__)
	ButtonBar.HSplitTop(80.0f, &ButtonBar, 0);
#else
	ButtonBar.HSplitTop(25.0f, &ButtonBar, 0);
#endif
	ButtonBar.VMargin(10.0f, &ButtonBar);

	ButtonBar.VSplitRight(120.0f, &ButtonBar, &Button);
	static CButtonContainer s_DisconnectButton;
	if(DoButton_Menu(&s_DisconnectButton, Localize("Disconnect"), 0, &Button))
	{
		if(g_Config.m_ClConfirmDisconnect)
			m_Popup = POPUP_DISCONNECT;
		else
			Client()->Disconnect();
	}

	ButtonBar.VSplitRight(7.0f, &ButtonBar, &Button);
	ButtonBar.VSplitRight(120.0f, &ButtonBar, &Button);
	static CButtonContainer s_ReconnectButton;
	if(DoButton_Menu(&s_ReconnectButton, Localize("Reconnect"), 0, &Button, Localize("Rejoin the current server")))
	{
		Client()->Disconnect();
		Client()->Connect(g_Config.m_UiServerAddress);
	}

	ButtonBar.VSplitLeft(3.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(ButtonBar.h, &Button, &ButtonBar);
	DoButton_Icon(IMAGE_MENUICONS, g_Config.m_ClUiShowExtraBar ? SPRITE_MENU_EXPANDED : SPRITE_MENU_COLLAPSED, &Button);
	static CButtonContainer s_ToggleExtraButton;
	if(DoButton_Menu(&s_ToggleExtraButton, "", 0, &Button))
		g_Config.m_ClUiShowExtraBar ^= 1;

	static CButtonContainer s_SpectateButton;
	static CButtonContainer s_JoinRedButton;
	static CButtonContainer s_JoinBlueButton;
	bool DummyConnecting = m_pClient->Client()->DummyConnecting();

	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
		{
			ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
			ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
			if(!DummyConnecting && DoButton_Menu(&s_SpectateButton, Localize("Spectate"), 0, &Button))
			{
				if(g_Config.m_ClDummy == 0 || m_pClient->Client()->DummyConnected())
				{
					m_pClient->SendSwitchTeam(TEAM_SPECTATORS);
					SetActive(false);
				}
			}
		}

		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags & GAMEFLAG_TEAMS)
		{
			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_RED)
			{
				ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
				ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
				if(!DummyConnecting && DoButton_Menu(&s_JoinRedButton, Localize("Join red"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(TEAM_RED);
					SetActive(false);
				}
			}

			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_BLUE)
			{
				ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
				ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
				if(!DummyConnecting && DoButton_Menu(&s_JoinBlueButton, Localize("Join blue"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(TEAM_BLUE);
					SetActive(false);
				}
			}
		}
		else
		{
			if(m_pClient->m_Snap.m_pLocalInfo->m_Team != 0)
			{
				ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
				ButtonBar.VSplitLeft(120.0f, &Button, &ButtonBar);
				if(!DummyConnecting && DoButton_Menu(&s_SpectateButton, Localize("Join game"), 0, &Button))
				{
					m_pClient->SendSwitchTeam(0);
					SetActive(false);
				}
			}
		}
	}

	ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
	{
		bool Recording = DemoRecorder(RECORDER_MANUAL)->IsRecording();
		ButtonBar.VSplitLeft(150.0f + (Recording ? -ButtonBar.h/2.0f : ButtonBar.h/2.0f), &Button, &ButtonBar);

		static CButtonContainer s_DemoButton;
		if(DoButton_Menu(&s_DemoButton, Localize(Recording ? "Stop record" : "Record demo"), 0, &Button, "", Recording ? CUI::CORNER_L : CUI::CORNER_ALL))	// Localize("Stop record");Localize("Record demo");
		{
			if(!Recording)
				Client()->DemoRecorder_Start(Client()->GetCurrentMap(), true, RECORDER_MANUAL);
			else
				Client()->DemoRecorder_Stop(RECORDER_MANUAL);
		}

		if(Recording)
		{
			ButtonBar.VSplitLeft(ButtonBar.h, &Button, &ButtonBar);
			static float s_FadeEnding = 0;
			static CButtonContainer s_DemoMarkerButton;
			float FadeVal = max(0.0f, s_FadeEnding-Client()->LocalTime())/1.5f;
			if(DoButton_Menu(&s_DemoMarkerButton, "✰", 0, &Button, Localize("Add a demo marker"), CUI::CORNER_R, vec4(1-FadeVal*0.8f, 1-FadeVal*0.8f, 1-FadeVal*0.3f, 0.8f)))
			{
				Client()->DemoRecorder_AddDemoMarker(RECORDER_MANUAL);
				s_FadeEnding = Client()->LocalTime()+1.5f;
			}
		}
	}


	ButtonBar.VSplitLeft(5.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(170.0f, &Button, &ButtonBar);

	static CButtonContainer s_DummyButton;
	if(DummyConnecting)
		DoButton_Menu(&s_DummyButton, Localize("Connecting dummy"), 1, &Button);
	else if(DoButton_Menu(&s_DummyButton, Localize(Client()->DummyConnected() ? Localize("Disconnect dummy") : Localize("Connect dummy")), 0, &Button))
	{
		if(!Client()->DummyConnected())
			Client()->DummyConnect();
		else
			Client()->DummyDisconnect(0);
	}
}

void CMenus::RenderGameExtra(CUIRect ButtonBar)
{
	CALLSTACK_ADD();

	if(!g_Config.m_ClUiShowExtraBar)
		return;

	CUIRect Button;
	RenderTools()->DrawUIRect(&ButtonBar, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);

	// submenus
	enum
	{
		EXTRAS_NONE = 0,
		EXTRAS_SERVERCONFIG_CREATOR,
		EXTRAS_SNIFFER_SETTINGS
	};
	static int s_ExtrasPage = EXTRAS_NONE;

	// render submenus
	Button = ButtonBar;
	//Button.HSplitTop(10.0f, 0, &Button);
	Button.HSplitTop(UI()->Screen()->h*0.85f, &Button, 0);
	Button.Margin(50.0f, &Button);
	Button.y = ButtonBar.y + ButtonBar.h;

	if(s_ExtrasPage == EXTRAS_SERVERCONFIG_CREATOR)
		RenderServerConfigCreator(Button);
	else if(s_ExtrasPage == EXTRAS_SNIFFER_SETTINGS)
		RenderSnifferSettings(Button);

	// button bar
	ButtonBar.HSplitTop(10.0f, 0, &ButtonBar);
#if defined(__ANDROID__)
	ButtonBar.HSplitTop(80.0f, &ButtonBar, 0);
#else
	ButtonBar.HSplitTop(25.0f, &ButtonBar, 0);
#endif
	ButtonBar.VMargin(10.0f, &ButtonBar);

	// render buttons
	ButtonBar.VSplitLeft(3.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(130.0f, &Button, &ButtonBar);
	static CButtonContainer s_ConModeButton;
	if(DoButton_Menu(&s_ConModeButton, Localize("Console Mode"), 0, &Button, "Enter console mode (very low CPU usage, no graphics)"))
	{
		g_Config.m_ClConsoleMode ^= 1;
		dbg_msg("", "+++++++++++++++++++ CONSOLE MODE ON +++++++++++++++++++");
		dbg_msg("", "++ You can execute console commands in this console, ++");
		dbg_msg("", "++ just enter the command and press 'ENTER', for     ++");
		dbg_msg("", "++ example 'say some message' to chat with others.   ++");
		dbg_msg("", "++ Press 'q' and then 'ENTER' to exit console mode!  ++");
		dbg_msg("", "+++++++++++++++++++++++++++++++++++++++++++++++++++++++");

		SetActive(false);
	}

	ButtonBar.VSplitLeft(3.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(130.0f, &Button, &ButtonBar);
	static CButtonContainer s_OpenChatButton;
	char aBuf[64];
	if(str_comp(GameClient()->m_pBinds->GetKey("+irc"), ""))
		str_format(aBuf, sizeof(aBuf), Localize("Open the IRC overlay (Key: %s)"), GameClient()->m_pBinds->GetKey("+irc"));
	else
		str_format(aBuf, sizeof(aBuf), Localize("Open the IRC overlay"));
	if(DoButton_Menu(&s_OpenChatButton, Localize("IRC Chat"), 0, &Button, aBuf))
		ToggleIRC();

	ButtonBar.VSplitLeft(3.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(130.0f, &Button, &ButtonBar);
	static CButtonContainer s_ServerConfigButton;
	if(DoButton_Menu(&s_ServerConfigButton, Localize("Server Config"), s_ExtrasPage == EXTRAS_SERVERCONFIG_CREATOR, &Button, "Execute commands when joining this server"))
		s_ExtrasPage = s_ExtrasPage == EXTRAS_SERVERCONFIG_CREATOR ? EXTRAS_NONE : EXTRAS_SERVERCONFIG_CREATOR;

	ButtonBar.VSplitLeft(3.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(130.0f, &Button, &ButtonBar);
	static CButtonContainer s_SnifferSettingsButton;
	if(DoButton_Menu(&s_SnifferSettingsButton, Localize("Network Sniffer"), s_ExtrasPage == EXTRAS_SNIFFER_SETTINGS, &Button, "Packet sniffing settings"))
		s_ExtrasPage = s_ExtrasPage == EXTRAS_SNIFFER_SETTINGS ? EXTRAS_NONE : EXTRAS_SNIFFER_SETTINGS;

}

void CMenus::RenderServerConfigCreator(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, Button2;
	static array<CListboxItem> Items;
	static char aEditBoxBuffer[256][256] = { 0 };
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);
	TextRender()->Text(0, MainView.x+10.0f, MainView.y, 12.0f, Localize("Server-Dependent-Configuration Manager"), -1);

	{
		MainView.VSplitMid(&MainView, &Button);
		MainView.w += Button.w;
		static CButtonContainer s_Checkbox;
		Button.HSplitTop(20.0f, &Button, 0);
		Button.VSplitRight(200.0f, 0, &Button);
		Button.x -= 20.0f;
		if(DoButton_CheckBox(&s_Checkbox, Localize("Reset on Disconnect"), g_Config.m_ClResetServerCfgOnDc, &Button, Localize("Discard config changes when disconnecting")))
			g_Config.m_ClResetServerCfgOnDc ^= 1;
	}

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.Margin(20.0f, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitMid(&Button, &Button2);
	Button.VSplitRight(10.0f, &Button, 0); Button2.VSplitLeft(10.0f, 0, &Button2);
	static CButtonContainer s_AddEntryButton;
	if(DoButton_Menu(&s_AddEntryButton, Localize("Add"), 0, &Button) || (UI()->MouseInside(&MainView) && Input()->KeyPress(KEY_RETURN)))
	{
		CListboxItem n;
		Items.add(n);
	}

	static CButtonContainer s_SaveButton;
	Button2.VSplitMid(&Button, &Button2);
	Button.VSplitRight(5.0f, &Button, 0);
	Button2.VSplitLeft(5.0f, 0, &Button2);
	if(DoButton_Menu(&s_SaveButton, Localize("Save"), 0, &Button))
	{
		IStorageTW *pStorage = Kernel()->RequestInterface<IStorageTW>();
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "configs/%s.cfg", g_Config.m_UiServerAddress);
		str_replace_char(aBuf, sizeof(aBuf), ':', '_');

		IOHANDLE f = pStorage->OpenFile(aBuf, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
		if(!f)
			dbg_msg("serverconfig", "failed to open '%s' for writing", aBuf);
		else
		{
			for(int i = 0; i < Items.size(); i++)
			{
				io_write(f, aEditBoxBuffer[i], (unsigned int)str_length(aEditBoxBuffer[i]));
				io_write(f, "\n", sizeof('\n'));
			}
			io_close(f);
		}
	}

	static bool s_MustReload = false;
	static char s_LastServer[64] = {0};
	if(str_comp_nocase(s_LastServer, g_Config.m_UiServerAddress) != 0)
	{
		s_MustReload = true;
		str_copy(s_LastServer, g_Config.m_UiServerAddress, sizeof(s_LastServer));
	}
	static CButtonContainer s_LoadButton;
	if(s_MustReload || DoButton_Menu(&s_LoadButton, Localize("Load"), 0, &Button2))
	{
		s_MustReload = false;
		// load server specific config
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "configs/%s.cfg", g_Config.m_UiServerAddress);
		str_replace_char(aBuf, sizeof(aBuf), ':', '_');

		Items.clear();
		mem_zero(aEditBoxBuffer, sizeof(aEditBoxBuffer));
		IOHANDLE f = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorageTW::TYPE_ALL);
		if(f)
		{
			char aBuffer[1024], aIn[128];
			io_read(f, aBuffer, sizeof(aBuffer));
			int c = str_count_char(aBuffer, sizeof(aBuffer), '\n');
			int index = 0;
			for(int i = 0; i < c; i++)
			{
				mem_zero(aIn, sizeof(aIn));
				str_split(aIn, aBuffer, i, '\n');
				str_format(aEditBoxBuffer[index], sizeof(aEditBoxBuffer[index]), "%s", aIn);
				CListboxItem n;
				Items.add(n);
				index++;
			}
		/*
			for(int i = 0, j = 0; i < str_length(aBuffer); i++, j++)
			{
				if(aBuffer[i] != '\n')
					aIn[j] = aBuffer[i];
				else
				{
					str_format(aEditBoxBuffer[index], sizeof(aEditBoxBuffer[index]), "%s", aIn);
					//str_sanitize_strong(aEditBoxBuffer[index]);
					mem_zero(aIn, sizeof(aIn)); j = 0;
					CListboxItem n;
					Items.add(n);
					index++;
				}
			}*/
		}
	}

	static CButtonContainer s_Listbox;
	static float s_ScrollVal = 0.0f;
	MainView.HSplitTop(10.0f, 0, &MainView);
	UiDoListboxStart(&s_Listbox, &MainView, 15.0f, "", "", Items.size(), 1, -1, s_ScrollVal);

	for(int i = 0; i < Items.size(); i++)
	{
		if(i > 0xFF)
		{
			Items.remove_index(i);
			continue;
		}

		CPointerContainer Container(&Items[i]);
		CListboxItem n = UiDoListboxNextItem(&Container, false);
		if(!n.m_Visible)
			continue;

		CUIRect Box, Button;
		n.m_HitRect.VSplitRight(n.m_HitRect.h, &Box, &Button);

		static CButtonContainer s_EditBox[256];
		static float s_Offset[256] = { 0.0f };
		DoEditBox(&s_EditBox[i], &Box, aEditBoxBuffer[i], sizeof(aEditBoxBuffer[i]), 8.0f, &s_Offset[i], false, 0, Localize("Enter your f1 commands here..."), -1);

		static CButtonContainer s_ClearButton[256];
		if(DoButton_Menu(&s_ClearButton[i], "×", 0, &Button, "Remove", 0))
			Items.remove_index(i);
	}

	UiDoListboxEnd(&s_ScrollVal, 0);
}

void CMenus::RenderSnifferSettings(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button;
	MainView.VSplitLeft(MainView.w/3, 0, &MainView);
	MainView.VSplitRight(MainView.w/2, &MainView, 0);
	MainView.HSplitMid(&MainView, 0);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);
	TextRender()->Text(0, MainView.x+10.0f, MainView.y, 12.0f, Localize("Packet sniffer settings"), -1);

	{
		MainView.Margin(10.0f, &MainView);
		MainView.HSplitTop(20.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		static CButtonContainer s_Checkbox;
		if(DoButton_CheckBox(&s_Checkbox, Localize("Sniff outgoing conn packets"), g_Config.m_ClSniffSendConn, &Button))
			g_Config.m_ClSniffSendConn ^= 1;
	}
	{
		MainView.HSplitTop(7.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		static CButtonContainer s_Checkbox;
		if(DoButton_CheckBox(&s_Checkbox, Localize("Sniff outgoing connless packets"), g_Config.m_ClSniffSendConnless, &Button))
			g_Config.m_ClSniffSendConnless ^= 1;
	}
	{
		MainView.HSplitTop(7.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		static CButtonContainer s_Checkbox;
		if(DoButton_CheckBox(&s_Checkbox, Localize("Sniff incoming conn packets"), g_Config.m_ClSniffRecvConn, &Button))
			g_Config.m_ClSniffRecvConn ^= 1;
	}
	{
		MainView.HSplitTop(7.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		static CButtonContainer s_Checkbox;
		if(DoButton_CheckBox(&s_Checkbox, Localize("Sniff incoming connless packets"), g_Config.m_ClSniffRecvConnless, &Button))
			g_Config.m_ClSniffRecvConnless ^= 1;
	}
}

void CMenus::RenderPlayers(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, Button2, ButtonBar, Options, Player;
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	// player options
	MainView.Margin(10.0f, &Options);
	RenderTools()->DrawUIRect(&Options, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
	Options.Margin(10.0f, &Options);
	Options.HSplitTop(50.0f, &Button, &Options);
	UI()->DoLabelScaled(&Button, Localize("Player options"), 34.0f, -1);

	// headline
	Options.HSplitTop(34.0f, &ButtonBar, &Options);
	ButtonBar.VSplitRight(220.0f, &Player, &ButtonBar);
	UI()->DoLabelScaled(&Player, Localize("Player"), 24.0f, -1);

	ButtonBar.HMargin(1.0f, &ButtonBar);
	float Width = ButtonBar.h*2.0f;
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIICONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_GUIICON_MUTE);
	IGraphics::CQuadItem QuadItem(Button.x, Button.y, Button.w, Button.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	ButtonBar.VSplitLeft(20.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(Width, &Button, &ButtonBar);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIICONS].m_Id);
	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SPRITE_GUIICON_FRIEND);
	QuadItem = IGraphics::CQuadItem(Button.x, Button.y, Button.w, Button.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	int TotalPlayers = 0;

	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_paInfoByName[i])
			continue;

		int Index = m_pClient->m_Snap.m_paInfoByName[i]->m_ClientID;

		if(Index == m_pClient->m_Snap.m_LocalClientID)
			continue;

		TotalPlayers++;
	}

	static CButtonContainer s_VoteList;
	static float s_ScrollValue = 0;
	CUIRect List = Options;
	//List.HSplitTop(28.0f, 0, &List);
#if defined(__ANDROID__)
	UiDoListboxStart(&s_VoteList, &List, 50.0f, "", "", TotalPlayers, 1, -1, s_ScrollValue);
#else
	UiDoListboxStart(&s_VoteList, &List, 24.0f, "", "", TotalPlayers, 1, -1, s_ScrollValue);
#endif

	// options
	static int s_aPlayerIDs[MAX_CLIENTS][3] = {{0}};

	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_paInfoByName[i])
			continue;

		int Index = m_pClient->m_Snap.m_paInfoByName[i]->m_ClientID;

		if(Index == m_pClient->m_Snap.m_LocalClientID)
			continue;

		CPointerContainer Container(&m_pClient->m_aClients[Index]);
		CListboxItem Item = UiDoListboxNextItem(&Container);

		Count++;

		if(!Item.m_Visible)
			continue;

		if(Count%2 == 1)
			RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
		Item.m_Rect.VSplitRight(300.0f, &Player, &Item.m_Rect);

		// player info
		Player.VSplitLeft(28.0f, &Button, &Player);
		CTeeRenderInfo Info = m_pClient->m_aClients[Index].m_RenderInfo;
		Info.m_Size = Button.h;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Button.x+Button.h/2, Button.y+Button.h/2));

		Player.HSplitTop(1.5f, 0, &Player);
		Player.VSplitMid(&Player, &Button);
		Item.m_Rect.VSplitRight(200.0f, &Button2, &Item.m_Rect);
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, Player.x, Player.y, 14.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Player.w;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[Index].m_aName, -1);

		TextRender()->SetCursor(&Cursor, Button.x,Button.y, 14.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Button.w;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[Index].m_aClan, -1);

		//TextRender()->SetCursor(&Cursor, Button2.x,Button2.y, 14.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		//Cursor.m_LineWidth = Button.w;

		vec4 Color(1.0f, 1.0f, 1.0f, 0.5f);
		m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[Index].m_Country, &Color,
										   Button2.x, (float)(Button2.y + Button2.h / 2.0f - 0.75 * Button2.h / 2.0f), 1.5f * Button2.h, 0.75f * Button2.h);

		// ignore button
		Item.m_Rect.HMargin(2.0f, &Item.m_Rect);
		Item.m_Rect.VSplitLeft(Width, &Button, &Item.m_Rect);
		Button.VSplitLeft((Width-Button.h)/4.0f, 0, &Button);
		Button.VSplitLeft(Button.h, &Button, 0);
		if(g_Config.m_ClShowChatFriends && !m_pClient->m_aClients[Index].m_Friend)
		{
			CPointerContainer FriendContainer(&s_aPlayerIDs[Index][0]);
			DoButton_Toggle(&FriendContainer, 1, &Button, false);
		}
		else
		{
			CPointerContainer IgnoreContainer(&s_aPlayerIDs[Index][0]);
			if(DoButton_Toggle(&IgnoreContainer, m_pClient->m_aClients[Index].m_ChatIgnore, &Button, true))
				m_pClient->m_aClients[Index].m_ChatIgnore ^= 1;
		}

		// friend button
		Item.m_Rect.VSplitLeft(20.0f, &Button, &Item.m_Rect);
		Item.m_Rect.VSplitLeft(Width, &Button, &Item.m_Rect);
		Button.VSplitLeft((Width-Button.h)/4.0f, 0, &Button);
		Button.VSplitLeft(Button.h, &Button, 0);
		CPointerContainer Container2(&s_aPlayerIDs[Index][1]);
		if(DoButton_Toggle(&Container2, m_pClient->m_aClients[Index].m_Friend, &Button, true))
		{
			if(m_pClient->m_aClients[Index].m_Friend)
				m_pClient->Friends()->RemoveFriend(m_pClient->m_aClients[Index].m_aName, m_pClient->m_aClients[Index].m_aClan);
			else
				m_pClient->Friends()->AddFriend(m_pClient->m_aClients[Index].m_aName, m_pClient->m_aClients[Index].m_aClan);
		}

		// copy ident
		Item.m_Rect.VSplitLeft(Width-5.0f, &Button, &Item.m_Rect);
		Button.VSplitLeft((Width-Button.h)/4.0f, 0, &Button);
		Button.VSplitLeft(Button.h, &Button, 0);
		static CButtonContainer s_IDButton;
		if(GameClient()->m_pIdentity->GetIdent(GameClient()->m_pIdentity->GetIdentID(m_pClient->m_aClients[Index].m_aName)))
		if(DoButton_Menu(&s_IDButton, "ID", 0, &Button, "Add as new identity"))
		{
			CIdentity::CIdentEntry Entry;
			mem_zero(&Entry, sizeof(Entry));
			str_format(Entry.m_aName, sizeof(Entry.m_aName), m_pClient->m_aClients[Index].m_aName);
			str_format(Entry.m_aClan, sizeof(Entry.m_aClan), m_pClient->m_aClients[Index].m_aClan);
			str_format(Entry.m_aSkin, sizeof(Entry.m_aSkin), m_pClient->m_aClients[Index].m_aSkinName);
			Entry.m_UseCustomColor = m_pClient->m_aClients[Index].m_UseCustomColor;
			Entry.m_ColorBody = m_pClient->m_aClients[Index].m_ColorBody;
			Entry.m_ColorFeet = m_pClient->m_aClients[Index].m_ColorFeet;
			m_pClient->m_pIdentity->AddIdent(Entry);
		}
	}

	UiDoListboxEnd(&s_ScrollValue, 0);
	/*
	CUIRect bars;
	votearea.HSplitTop(10.0f, 0, &votearea);
	votearea.HSplitTop(25.0f + 10.0f*3 + 25.0f, &votearea, &bars);

	RenderTools()->DrawUIRect(&votearea, color_tabbar_active, CUI::CORNER_ALL, 10.0f);

	votearea.VMargin(20.0f, &votearea);
	votearea.HMargin(10.0f, &votearea);

	votearea.HSplitBottom(35.0f, &votearea, &bars);

	if(gameclient.voting->is_voting())
	{
		// do yes button
		votearea.VSplitLeft(50.0f, &button, &votearea);
		static int yes_button = 0;
		if(UI()->DoButton(&yes_button, "Yes", 0, &button, ui_draw_menu_button, 0))
			gameclient.voting->vote(1);

		// do no button
		votearea.VSplitLeft(5.0f, 0, &votearea);
		votearea.VSplitLeft(50.0f, &button, &votearea);
		static int no_button = 0;
		if(UI()->DoButton(&no_button, "No", 0, &button, ui_draw_menu_button, 0))
			gameclient.voting->vote(-1);

		// do time left
		votearea.VSplitRight(50.0f, &votearea, &button);
		char buf[256];
		str_format(buf, sizeof(buf), "%d", gameclient.voting->seconds_left());
		UI()->DoLabel(&button, buf, 24.0f, 0);

		// do description and command
		votearea.VSplitLeft(5.0f, 0, &votearea);
		UI()->DoLabel(&votearea, gameclient.voting->vote_description(), 14.0f, -1);
		votearea.HSplitTop(16.0f, 0, &votearea);
		UI()->DoLabel(&votearea, gameclient.voting->vote_command(), 10.0f, -1);

		// do bars
		bars.HSplitTop(10.0f, 0, &bars);
		bars.HMargin(5.0f, &bars);

		gameclient.voting->render_bars(bars, true);

	}
	else
	{
		UI()->DoLabel(&votearea, "No vote in progress", 18.0f, -1);
	}*/
}

void CMenus::RenderServerInfo(CUIRect MainView)
{
	CALLSTACK_ADD();

	if(!m_pClient->m_Snap.m_pLocalInfo)
		return;

	// fetch server info
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);

	// render background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	CUIRect View, ServerInfo, GameInfo, Motd;

	char aBuf[1024];

	// set view to use for all sub-modules
	MainView.Margin(10.0f, &View);

	// serverinfo
	View.HSplitTop(View.h/2/UI()->Scale()-5.0f, &ServerInfo, &Motd);
	ServerInfo.VSplitLeft(View.w/2/UI()->Scale()-5.0f, &ServerInfo, &GameInfo);
	RenderTools()->DrawUIRect(&ServerInfo, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);

	ServerInfo.Margin(5.0f, &ServerInfo);

	float x = 5.0f;
	float y = 0.0f;

	TextRender()->Text(0, ServerInfo.x+x, ServerInfo.y+y, 32, Localize("Server info"), 250);
	y += 32.0f+5.0f;

	mem_zero(aBuf, sizeof(aBuf));
	str_format(
		aBuf,
		sizeof(aBuf),
		"%s\n\n"
		"%s: %s\n"
		"%s: %d\n"
		"%s: %s\n"
		"%s: %s\n",
		CurrentServerInfo.m_aName,
		Localize("Address"), CurrentServerInfo.m_aAddress,
		Localize("Ping"), m_pClient->m_Snap.m_pLocalInfo->m_Latency,
		Localize("Version"), CurrentServerInfo.m_aVersion,
		Localize("Password"), CurrentServerInfo.m_Flags &1 ? Localize("Yes") : Localize("No")
	);

	TextRender()->Text(0, ServerInfo.x+x, ServerInfo.y+y, 20, aBuf, 250);

	{
		CUIRect Button;
		int IsFavorite = ServerBrowser()->IsFavorite(CurrentServerInfo.m_NetAddr);
		ServerInfo.HSplitBottom(20.0f, &ServerInfo, &Button);
		static CButtonContainer s_AddFavButton;
		if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), IsFavorite, &Button))
		{
			if(IsFavorite)
				ServerBrowser()->RemoveFavorite(CurrentServerInfo.m_NetAddr);
			else
				ServerBrowser()->AddFavorite(CurrentServerInfo.m_NetAddr);
		}
	}

	// gameinfo
	GameInfo.VSplitLeft(10.0f, 0x0, &GameInfo);
	RenderTools()->DrawUIRect(&GameInfo, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);

	GameInfo.Margin(5.0f, &GameInfo);

	x = 5.0f;
	y = 0.0f;

	TextRender()->Text(0, GameInfo.x+x, GameInfo.y+y, 32, Localize("Game info"), 250);
	y += 32.0f+5.0f;

	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		mem_zero(aBuf, sizeof(aBuf));
		str_format(
			aBuf,
			sizeof(aBuf),
			"\n\n"
			"%s: %s\n"
			"%s: %s\n"
			"%s: %d\n"
			"%s: %d\n"
			"\n"
			"%s: %d/%d\n",
			Localize("Game type"), CurrentServerInfo.m_aGameType,
			Localize("Map"), CurrentServerInfo.m_aMap,
			Localize("Score limit"), m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit,
			Localize("Time limit"), m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit,
			Localize("Players"), m_pClient->m_Snap.m_NumPlayers, CurrentServerInfo.m_MaxClients
		);
		TextRender()->Text(0, GameInfo.x+x, GameInfo.y+y, 20, aBuf, 250);
	}

	// motd
	Motd.HSplitTop(10.0f, 0, &Motd);
	RenderTools()->DrawUIRect(&Motd, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 10.0f);
	Motd.Margin(5.0f, &Motd);
	y = 0.0f;
	x = 5.0f;
	TextRender()->Text(0, Motd.x+x, Motd.y+y, 32, Localize("MOTD"), -1);
	y += 32.0f+5.0f;
	TextRender()->Text(0, Motd.x+x, Motd.y+y, 16, m_pClient->m_pMotd->m_aServerMotd, (int)Motd.w);
}

void CMenus::RenderServerControlServer(CUIRect MainView)
{
	CALLSTACK_ADD();

	static CButtonContainer s_VoteList;
	static float s_ScrollValue = 0;
	CUIRect List = MainView;
	int Total = m_pClient->m_pVoting->m_NumVoteOptions;
	int NumVoteOptions = 0;
	int aIndices[MAX_VOTE_OPTIONS];
	static int s_CurVoteOption = 0;
	int TotalShown = 0;

	for(CVoteOptionClient *pOption = m_pClient->m_pVoting->m_pFirst; pOption; pOption = pOption->m_pNext)
	{
		if(m_aFilterString[0] != '\0' && !str_find_nocase(pOption->m_aDescription, m_aFilterString))
			continue;
		TotalShown++;
	}

#if defined(__ANDROID__)
	UiDoListboxStart(&s_VoteList, &List, 50.0f, "", "", TotalShown, 1, s_CurVoteOption, s_ScrollValue);
#else
	UiDoListboxStart(&s_VoteList, &List, 24.0f, "", "", TotalShown, 1, s_CurVoteOption, s_ScrollValue);
#endif

	int i = -1;
	for(CVoteOptionClient *pOption = m_pClient->m_pVoting->m_pFirst; pOption; pOption = pOption->m_pNext)
	{
		i++;
		if(m_aFilterString[0] != '\0' && !str_find_nocase(pOption->m_aDescription, m_aFilterString))
			continue;

		CPointerContainer Container(pOption);
		CListboxItem Item = UiDoListboxNextItem(&Container);

		if(Item.m_Visible)
			UI()->DoLabelScaled(&Item.m_Rect, pOption->m_aDescription, 16.0f, -1);

		if(NumVoteOptions < Total)
			aIndices[NumVoteOptions] = i;
		NumVoteOptions++;
	}

	s_CurVoteOption = UiDoListboxEnd(&s_ScrollValue, 0);
	if(s_CurVoteOption < Total)
		m_CallvoteSelectedOption = aIndices[s_CurVoteOption];
}

void CMenus::RenderServerControlKick(CUIRect MainView, bool FilterSpectators)
{
	CALLSTACK_ADD();

	int NumOptions = 0;
	int Selected = -1;
	static int aPlayerIDs[MAX_CLIENTS];
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_Snap.m_paInfoByName[i])
			continue;

		int Index = m_pClient->m_Snap.m_paInfoByName[i]->m_ClientID;
		if(Index == m_pClient->m_Snap.m_LocalClientID || (FilterSpectators && m_pClient->m_Snap.m_paInfoByName[i]->m_Team == TEAM_SPECTATORS))
			continue;

		if(!str_find_nocase(m_pClient->m_aClients[Index].m_aName, m_aFilterString))
			continue;

		if(m_CallvoteSelectedPlayer == Index)
			Selected = NumOptions;
		aPlayerIDs[NumOptions++] = Index;
	}

	static CButtonContainer s_VoteList;
	static float s_ScrollValue = 0;
	CUIRect List = MainView;
#if defined(__ANDROID__)
	UiDoListboxStart(&s_VoteList, &List, 50.0f, "", "", NumOptions, 1, Selected, s_ScrollValue);
#else
	UiDoListboxStart(&s_VoteList, &List, 24.0f, "", "", NumOptions, 1, Selected, s_ScrollValue);
#endif

	for(int i = 0; i < NumOptions; i++)
	{
		CPointerContainer Container(&aPlayerIDs[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container);

		if(Item.m_Visible)
		{
			CTeeRenderInfo Info = m_pClient->m_aClients[aPlayerIDs[i]].m_RenderInfo;
			Info.m_Size = Item.m_Rect.h;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0), vec2(Item.m_Rect.x+Item.m_Rect.h/2, Item.m_Rect.y+Item.m_Rect.h/2));
			Item.m_Rect.x +=Info.m_Size;
			UI()->DoLabelScaled(&Item.m_Rect, m_pClient->m_aClients[aPlayerIDs[i]].m_aName, 16.0f, -1);
		}
	}

	Selected = UiDoListboxEnd(&s_ScrollValue, 0);
	m_CallvoteSelectedPlayer = Selected != -1 ? aPlayerIDs[Selected] : -1;
}

void CMenus::RenderServerControl(CUIRect MainView)
{
	CALLSTACK_ADD();

	static int s_ControlPage = 0;

	// render background
	CUIRect Bottom, Extended, TabBar, Button;
#if defined(__ANDROID__)
	MainView.HSplitTop(50.0f, &Bottom, &MainView);
#else
	MainView.HSplitTop(20.0f, &Bottom, &MainView);
#endif
	RenderTools()->DrawUIRect(&Bottom, ms_ColorTabbarActive, CUI::CORNER_T, 10.0f);
#if defined(__ANDROID__)
	MainView.HSplitTop(50.0f, &TabBar, &MainView);
#else
	MainView.HSplitTop(20.0f, &TabBar, &MainView);
#endif
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);
#if defined(__ANDROID__)
	MainView.HSplitBottom(10.0f, &MainView, &Extended);
#else
	MainView.HSplitBottom(90.0f, &MainView, &Extended);
#endif

	// tab bar
	{
		TabBar.VSplitLeft(TabBar.w/2, &Button, &TabBar);
		static CButtonContainer s_Button0;
		if(DoButton_MenuTab(&s_Button0, Localize("Change settings"), s_ControlPage == 0, &Button, 0))
			s_ControlPage = 0;

		TabBar.VSplitRight(0, &Button, &TabBar);
		static CButtonContainer s_Button1;
		if(DoButton_MenuTab(&s_Button1, Localize("Player related"), s_ControlPage == 1, &Button, 0))
			s_ControlPage = 1;
	}

	// render page
	MainView.HSplitBottom(ms_ButtonHeight + 5*2, &MainView, &Bottom);
	Bottom.HMargin(5.0f, &Bottom);

	if(s_ControlPage == 0)
		RenderServerControlServer(MainView);
	else if(s_ControlPage == 1)
		RenderServerControlKick(MainView, m_FilterSpectators);

	// vote menu
	{
		CUIRect Button, Button2, QuickSearch;

		// render quick search
		{
			Bottom.VSplitLeft(240.0f, &QuickSearch, &Bottom);
			QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
			const char *pSearchLabel = "⚲";
			UI()->DoLabelScaled(&QuickSearch, pSearchLabel, 14.0f, -1);
			float wSearch = TextRender()->TextWidth(0, 14.0f, pSearchLabel, -1);
			QuickSearch.VSplitLeft(wSearch, 0, &QuickSearch);
			QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
			QuickSearch.VSplitLeft(QuickSearch.w-15.0f, &QuickSearch, &Button2);
			static float Offset = 0.0f;
			static CButtonContainer s_FilterStringEditbox;
			if(DoEditBox(&s_FilterStringEditbox, &QuickSearch, m_aFilterString, sizeof(m_aFilterString), 14.0f, &Offset, false, CUI::CORNER_L, Localize("Search")))
			{
				// TODO: Implement here
			}

			// clear button
			{
				static CButtonContainer s_ClearButton;
				if(DoButton_Menu(&s_ClearButton, "×", 0, &Button2, 0, CUI::CORNER_R, vec4(1,1,1,0.35f)))
				{
					m_aFilterString[0] = 0;
					UI()->SetActiveItem(&m_aFilterString);
					Client()->ServerBrowserUpdate();
				}
			}
		}

		Bottom.VSplitRight(120.0f, &Bottom, &Button);

		static CButtonContainer s_CallVoteButton;
		if(DoButton_Menu(&s_CallVoteButton, Localize("Call vote"), 0, &Button))
		{
			if(s_ControlPage == 0)
				m_pClient->m_pVoting->CallvoteOption(m_CallvoteSelectedOption, m_aCallvoteReason);
			else if(!m_FilterSpectators)
			{
				if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
					m_pClient->m_Snap.m_paPlayerInfos[m_CallvoteSelectedPlayer])
				{
					m_pClient->m_pVoting->CallvoteKick(m_CallvoteSelectedPlayer, m_aCallvoteReason);
					//SetActive(false);
				}
			}
			else if(m_FilterSpectators)
			{
				if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
					m_pClient->m_Snap.m_paPlayerInfos[m_CallvoteSelectedPlayer])
				{
					m_pClient->m_pVoting->CallvoteSpectate(m_CallvoteSelectedPlayer, m_aCallvoteReason);
					//SetActive(false);
				}
			}
			//m_aCallvoteReason[0] = 0;
			SetActive(false);
		}

		// render reason
		{
			CUIRect Reason, ClearButton;
			Bottom.VSplitRight(40.0f, &Bottom, 0);
			Bottom.VSplitRight(160.0f*2.0f, &Bottom, &Reason);
			Reason.HSplitTop(5.0f, 0, &Reason);
			const char *pLabel = Localize("Reason:");
			UI()->DoLabelScaled(&Reason, pLabel, 14.0f, -1);
			float w = TextRender()->TextWidth(0, 14.0f, pLabel, -1);
			Reason.VSplitLeft(w+10.0f, 0, &Reason);
			Reason.VSplitLeft(Reason.w-15.0f, &Reason, &ClearButton);
			static float s_Offset = 0.0f;
			static CButtonContainer s_CallvoteReasonEditbox;
			DoEditBox(&s_CallvoteReasonEditbox, &Reason, m_aCallvoteReason, sizeof(m_aCallvoteReason), 14.0f, &s_Offset, false, CUI::CORNER_L);
			// clear button
			static CButtonContainer s_ClearButton;
			if(DoButton_Menu(&s_ClearButton, "×", 0, &ClearButton, 0, CUI::CORNER_R, vec4(1,1,1,0.35f)))
			{
				m_aCallvoteReason[0] = 0;
				UI()->SetActiveItem(&m_aCallvoteReason);
			}
		}

		if(s_ControlPage == 1)
		{
			Bottom.VSplitRight(50.0f, &Bottom, 0);
			Bottom.VSplitRight(95.0f, &Bottom, &Button);
			Button.y += 2.0f;
			static CButtonContainer s_SpecKickButton;
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf), "%s (%s)", m_FilterSpectators ? Localize("Move player to the spectators") : Localize("Kick player from the server"), Localize("click to toggle"));
			if(DoButton_Menu(&s_SpecKickButton, m_FilterSpectators ? Localize("Spectate") : Localize("Kick"), 0, &Button, aBuf))
			{
				m_FilterSpectators ^= 1;
			}

			const char *pLabel = Localize("Action:");
			Bottom.VSplitRight(60.0f, &Bottom, &Button);
			Button.y += 3.0f;
			UI()->DoLabelScaled(&Button, pLabel, 16.0f, -1);
		}


		// extended features (only available when authed in rcon)
		if(Client()->RconAuthed())
		{
			// background
			Extended.Margin(10.0f, &Extended);
			Extended.HSplitTop(20.0f, &Bottom, &Extended);
			Extended.HSplitTop(5.0f, 0, &Extended);

			// force vote
			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(120.0f, &Button, &Bottom);
			static CButtonContainer s_ForceVoteButton;
			if(DoButton_Menu(&s_ForceVoteButton, Localize("Force vote"), 0, &Button))
			{
				if(s_ControlPage == 0)
					m_pClient->m_pVoting->CallvoteOption(m_CallvoteSelectedOption, m_aCallvoteReason, true);
				else if(s_ControlPage == 1)
				{
					if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
						m_pClient->m_Snap.m_paPlayerInfos[m_CallvoteSelectedPlayer])
					{
						m_pClient->m_pVoting->CallvoteKick(m_CallvoteSelectedPlayer, m_aCallvoteReason, true);
						SetActive(false);
					}
				}
				else if(s_ControlPage == 2)
				{
					if(m_CallvoteSelectedPlayer >= 0 && m_CallvoteSelectedPlayer < MAX_CLIENTS &&
						m_pClient->m_Snap.m_paPlayerInfos[m_CallvoteSelectedPlayer])
					{
						m_pClient->m_pVoting->CallvoteSpectate(m_CallvoteSelectedPlayer, m_aCallvoteReason, true);
						SetActive(false);
					}
				}
				m_aCallvoteReason[0] = 0;
			}

			if(s_ControlPage == 0)
			{
				// remove vote
				Bottom.VSplitRight(10.0f, &Bottom, 0);
				Bottom.VSplitRight(120.0f, 0, &Button);
				static CButtonContainer s_RemoveVoteButton;
				if(DoButton_Menu(&s_RemoveVoteButton, Localize("Remove"), 0, &Button))
					m_pClient->m_pVoting->RemovevoteOption(m_CallvoteSelectedOption);


				// add vote
				Extended.HSplitTop(20.0f, &Bottom, &Extended);
				Bottom.VSplitLeft(5.0f, 0, &Bottom);
				Bottom.VSplitLeft(250.0f, &Button, &Bottom);
				UI()->DoLabelScaled(&Button, Localize("Vote description:"), 14.0f, -1);

				Bottom.VSplitLeft(20.0f, 0, &Button);
				UI()->DoLabelScaled(&Button, Localize("Vote command:"), 14.0f, -1);

				static char s_aVoteDescription[64] = {0};
				static char s_aVoteCommand[512] = {0};
				Extended.HSplitTop(20.0f, &Bottom, &Extended);
				Bottom.VSplitRight(10.0f, &Bottom, 0);
				Bottom.VSplitRight(120.0f, &Bottom, &Button);
				static CButtonContainer s_AddVoteButton;
				if(DoButton_Menu(&s_AddVoteButton, Localize("Add"), 0, &Button))
					if(s_aVoteDescription[0] != 0 && s_aVoteCommand[0] != 0)
						m_pClient->m_pVoting->AddvoteOption(s_aVoteDescription, s_aVoteCommand);

				Bottom.VSplitLeft(5.0f, 0, &Bottom);
				Bottom.VSplitLeft(250.0f, &Button, &Bottom);
				static float s_OffsetDesc = 0.0f;
				static CButtonContainer s_VoteDescriptionEditbox;
				DoEditBox(&s_VoteDescriptionEditbox, &Button, s_aVoteDescription, sizeof(s_aVoteDescription), 14.0f, &s_OffsetDesc, false, CUI::CORNER_ALL);

				Bottom.VMargin(20.0f, &Button);
				static float s_OffsetCmd = 0.0f;
				static CButtonContainer s_VoteCommandEditbox;
				DoEditBox(&s_VoteCommandEditbox, &Button, s_aVoteCommand, sizeof(s_aVoteCommand), 14.0f, &s_OffsetCmd, false, CUI::CORNER_ALL);
			}
		}
	}
}

void CMenus::RenderSpoofingGeneral(CUIRect MainView)
{
	CALLSTACK_ADD();

#if !defined(CONF_FAMILY_UNIX)
	return;
#endif

	CUIRect Button, Box;

	char aServerAddr[NETADDR_MAXSTRSIZE];
	char aClientAddr[NETADDR_MAXSTRSIZE];
	str_copy(aServerAddr, g_Config.m_UiServerAddress, sizeof(aServerAddr));
	net_addr_split(aServerAddr, sizeof(aServerAddr));

	if(m_SpoofSelectedPlayer >= 0)
	{
		str_copy(aClientAddr, m_pClient->m_aClients[m_SpoofSelectedPlayer].m_Addr, sizeof(aClientAddr));
		net_addr_split(aClientAddr, sizeof(aClientAddr));
	}

	MainView.Margin(10.0f, &MainView);
	MainView.HSplitBottom(25.0f, &MainView, &Box);

	// ----------- misc stuff

	MainView.VSplitLeft(200.0f, &Box, &MainView);

	// window
	Box.HSplitTop(22.4f, &Button, &Box);
	RenderTools()->DrawUIRect(&Button, vec4(0.6f, 0.17f, 0.17f, 0.6f), CUI::CORNER_T, 5.0f);
	UI()->DoLabel(&Button, Localize("M15C 5TUFF"), 17.0f, 0);
	RenderTools()->DrawUIRect(&Box, vec4(0.7f, 0.1f, 0.1f, 0.6f), 0, 0);

	Box.VMargin(10.0f, &Box);
	Box.HSplitTop(25.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
#if defined(CONF_FAMILY_UNIX)

	if(!m_pClient->m_pSpoofRemote->IsConnected())
	{
		static CButtonContainer s_ButtonConnect;
		if(DoButton_Menu(&s_ButtonConnect, Localize("Connect to server"), 0, &Button, Localize("Connect to the spoofing-server")))
		{
			if(!m_pClient->m_pSpoofRemote->IsConnected())
				m_pClient->m_pSpoofRemote->Connect(g_Config.m_ClSpoofSrvIP, g_Config.m_ClSpoofSrvPort);
		}
		return;
	}
#endif

	Box.HSplitTop(40.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	static CButtonContainer s_ButtonTest;
	if(DoButton_Menu(&s_ButtonTest, Localize("Zervor status"), 0, &Button))
	{
		m_pClient->m_pSpoofRemote->SendCommand("status");
	}

	Box.HSplitTop(70.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	static CButtonContainer s_ButtonDC;
	if(DoButton_Menu(&s_ButtonDC, Localize("Disconnect"), 0, &Button))
	{
		if(m_pClient->m_pSpoofRemote->IsConnected())
			m_pClient->m_pSpoofRemote->SendCommand("exit");
	}

	Box.HSplitTop(40.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	static CButtonContainer s_ButtonForceDC;
	if(DoButton_Menu(&s_ButtonForceDC, Localize("Force disconnect"), 0, &Button))
	{
		m_pClient->m_pSpoofRemote->Disconnect();
	}

	Box.HSplitTop(70.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	static CButtonContainer s_ButtonRestart;
	if(DoButton_Menu(&s_ButtonRestart, Localize("Restart"), 0, &Button))
	{
		if(m_pClient->m_pSpoofRemote->IsConnected())
			m_pClient->m_pSpoofRemote->SendCommand("restart");
	}

	// ----------- zervor tools

	MainView.VSplitLeft(70.0f, &Box, &MainView);
	MainView.VSplitLeft(200.0f, &Box, &MainView);

	// window
	Box.HSplitTop(22.4f, &Button, &Box);
	RenderTools()->DrawUIRect(&Button, vec4(0, 0.6f, 0.17f, 0.6f), CUI::CORNER_T, 5.0f);
	UI()->DoLabel(&Button, Localize("Z3RV0R T00LZ"), 17.0f, 0);
	RenderTools()->DrawUIRect(&Box, vec4(0, 0.7f, 0.1f, 0.6f), 0, 0);

	Box.VMargin(10.0f, &Box);
	Box.HSplitTop(25.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	static CButtonContainer s_ButtonFetch;
	if(DoButton_Menu(&s_ButtonFetch, Localize("Fetch IPs"), 0, &Button, Localize("Fetch player IPs from master")))
	{
		m_pClient->m_pSpoofRemote->SendCommand("fetchips");
	}

	Box.HSplitTop(40.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	static CButtonContainer s_ButtonGet;
	if(DoButton_Menu(&s_ButtonGet, Localize("Grab IPs"), 0, &Button, Localize("View player IPs (run Fetch IPs first)")))
	{
		char aCmd[256];
		str_format(aCmd, sizeof(aCmd), "ipspam %s", aServerAddr);
		m_pClient->m_pSpoofRemote->SendCommand(aCmd);
	}

	Box.HSplitTop(40.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	if(m_pClient->m_pSpoofRemote->IsState(CSpoofRemote::SPOOF_STATE_DUMMIES))
	{
		static CButtonContainer s_ButtonGetDum;
		if(DoButton_Menu(&s_ButtonGetDum, Localize("Grab dummy IPs"), 0, &Button))
		{
			char aCmd[256];
			str_format(aCmd, sizeof(aCmd), "ipspamdummies %s", aServerAddr);
			m_pClient->m_pSpoofRemote->SendCommand(aCmd);
		}
	}

	Box.HSplitTop(40.0f, 0, &Box);
	Box.HSplitTop(25.0f, &Button, 0);
	static CButtonContainer s_ButtonKickAll;
	if(DoButton_Menu(&s_ButtonKickAll, Localize("Vote-kick all"), 0, &Button))
	{
		m_pClient->m_pSpoofRemote->VotekickAll();
	}


	// ----------- dummy tools

	MainView.VSplitLeft(70.0f, &Box, &MainView);
	MainView.VSplitLeft(200.0f, &Box, &MainView);

	// window
	Box.HSplitTop(22.4f, &Button, &Box);
	RenderTools()->DrawUIRect(&Button, vec4(0.17f, 0, 0.6f, 0.6f), CUI::CORNER_T, 5.0f);
	UI()->DoLabel(&Button, Localize("DUMMY SH1T"), 17.0f, 0);
	RenderTools()->DrawUIRect(&Box, vec4(0.2f, 0, 0.7f, 0.6f), 0, 0);

	Box.VMargin(10.0f, &Box);
	Box.HSplitTop(20.0f, 0, &Box);
	Box.HSplitTop(15.0f, &Button, 0);
	static CButtonContainer s_ScrollbarDummy;
	static float s_ScrollValue = 0.0f;
	s_ScrollValue = round_to_int(63.0f * (DoScrollbarH(&s_ScrollbarDummy, &Button, s_ScrollValue / 63.0f)));

	Box.HSplitTop(20.0f, 0, &Box);
	Box.HSplitTop(15.0f, &Button, 0);
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "Dummies: %d", s_ScrollValue+1);
		UI()->DoLabelScaled(&Button, aBuf, 10.5f, -1, Button.w);
	}

	Box.HSplitTop(25.0f, 0, &Box);
	Box.HSplitTop(25.5f, &Button, 0);
	static CButtonContainer s_ButtonDummiesConnect;
	{
		if(DoButton_Menu(&s_ButtonDummiesConnect, Localize("Connect dummies"), 0, &Button))
		{
			char aCmd[256];
				str_format(aCmd, sizeof(aCmd), "dum %s %i", aServerAddr, s_ScrollValue+1);
			m_pClient->m_pSpoofRemote->SendCommand(aCmd);
		}
	}

	Box.HSplitTop(32.0f, 0, &Box);
	Box.HSplitTop(25.5f, &Button, 0);
	static CButtonContainer s_ButtonDummiesDisconnect;
	{
		if(m_pClient->m_pSpoofRemote->IsState(CSpoofRemote::SPOOF_STATE_DUMMIES))
		if(DoButton_Menu(&s_ButtonDummiesDisconnect, Localize("Disconnect dummies"), 0, &Button))
		{
			m_pClient->m_pSpoofRemote->SendCommand("dcdum");
		}
	}

	Box.HSplitTop(32.0f, 0, &Box);
	Box.HSplitTop(25.5f, &Button, 0);
	static CButtonContainer s_ButtonDummySpam;
	{
		char aBuf[64];
		if(!m_pClient->m_pSpoofRemote->IsState(CSpoofRemote::SPOOF_STATE_DUMMYSPAM))
			str_format(aBuf, sizeof(aBuf), Localize("Start flooding"));
		else
			str_format(aBuf, sizeof(aBuf), Localize("Stop flooding"));
		if(DoButton_Menu(&s_ButtonDummySpam, aBuf, 0, &Button, Localize("Fire the laz0r!!!")))
		{
			char aCmd[256];
			str_format(aCmd, sizeof(aCmd), "ds %s %i", aServerAddr, s_ScrollValue+1);
			m_pClient->m_pSpoofRemote->SendCommand(aCmd);
		}
	}

	// this stuff is only active when there are no dummies connected
	//if(!m_pClient->m_pSpoofRemote->IsState(CSpoofRemote::SPOOF_STATE_DUMMIES))
	{
		Box.HSplitTop(43.0f, 0, &Box);
		Box.HSplitTop(20.0f, &Button, 0);
		static CButtonContainer s_ButtonVoteYes;
		if(DoButton_Menu(&s_ButtonVoteYes, Localize("Votebot 'Yes'"), 0, &Button))
		{
			char aCmd[256];
			str_format(aCmd, sizeof(aCmd), "vb %s %d 1", aServerAddr, s_ScrollValue);
			m_pClient->m_pSpoofRemote->SendCommand(aCmd);
		}

		Box.HSplitTop(25.0f, 0, &Box);
		Box.HSplitTop(20.0f, &Button, 0);
		static CButtonContainer s_ButtonVoteNo;
		if(DoButton_Menu(&s_ButtonVoteNo, Localize("Votebot 'No'"), 0, &Button))
		{
			char aCmd[256];
			str_format(aCmd, sizeof(aCmd), "vb %s %d -1", aServerAddr, s_ScrollValue);
			m_pClient->m_pSpoofRemote->SendCommand(aCmd);
		}
	}

}

void CMenus::RenderSpoofingPlayers(CUIRect MainView)
{
	CALLSTACK_ADD();

	int NumOptions = 0;
	int Selected = -1;
	static int aPlayerIDs[MAX_CLIENTS];
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_Snap.m_paInfoByName[i])
			continue;

		int Index = m_pClient->m_Snap.m_paInfoByName[i]->m_ClientID;
		if(Index == m_pClient->m_Snap.m_LocalClientID)
			continue;

		// skip the players we can't spoof anyways
		if(!m_pClient->m_aClients[Index].m_Spoofable)
			continue;

		if(!str_find_nocase(m_pClient->m_aClients[Index].m_aName, m_aFilterString))
			continue;

		if(m_SpoofSelectedPlayer == Index)
			Selected = NumOptions;
		aPlayerIDs[NumOptions++] = Index;
	}

	static CButtonContainer s_PlayerList;
	static float s_ScrollValue = 0;
	CUIRect List = MainView;
#if defined(__ANDROID__)
	UiDoListboxStart(&s_PlayerList, &List, 50.0f, "", "", NumOptions, 1, Selected, s_ScrollValue);
#else
	UiDoListboxStart(&s_PlayerList, &List, 24.0f, "", "", NumOptions, 1, Selected, s_ScrollValue);
#endif

	for(int i = 0; i < NumOptions; i++)
	{
		CPointerContainer Container(&aPlayerIDs[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container);

		if(Item.m_Visible)
		{
			CTeeRenderInfo Info = m_pClient->m_aClients[aPlayerIDs[i]].m_RenderInfo;
			Info.m_Size = Item.m_Rect.h;
			Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
			RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, EMOTE_NORMAL, vec2(1,0), vec2(Item.m_Rect.x+Item.m_Rect.h/2, Item.m_Rect.y+Item.m_Rect.h/2));
			Item.m_Rect.x +=Info.m_Size;
			char aBuf[256];
			NETADDR temp_addr;
			net_addr_from_str(&temp_addr, m_pClient->m_aClients[aPlayerIDs[i]].m_Addr);
		///	if(temp_addr.port == 1337)
		///		str_format(aBuf, sizeof(aBuf), "\\\\-D-\\\\ %s", );
		///	else
		///		str_format(aBuf, sizeof(aBuf), "%s", m_pClient->m_aClients[aPlayerIDs[i]].m_aName);
			str_format(aBuf, sizeof(aBuf), "%s%s [[%s]]", temp_addr.port == 1337 ? "\\\\-D-\\\\  " : "", m_pClient->m_aClients[aPlayerIDs[i]].m_aName, m_pClient->m_aClients[aPlayerIDs[i]].m_Addr);
			UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, -1);
		}
	}

	Selected = UiDoListboxEnd(&s_ScrollValue, 0);
	m_SpoofSelectedPlayer = Selected != -1 ? aPlayerIDs[Selected] : -1;
}

void CMenus::RenderSpoofing(CUIRect MainView)
{
	CALLSTACK_ADD();

	static int s_ControlPage = 0;

	// render background
	CUIRect Bottom, Extended, TabBar, Button;
#if defined(__ANDROID__)
	MainView.HSplitTop(50.0f, &Bottom, &MainView);
#else
	MainView.HSplitTop(20.0f, &Bottom, &MainView);
#endif
	RenderTools()->DrawUIRect(&Bottom, ms_ColorTabbarActive, CUI::CORNER_T, 10.0f);
#if defined(__ANDROID__)
	MainView.HSplitTop(50.0f, &TabBar, &MainView);
#else
	MainView.HSplitTop(20.0f, &TabBar, &MainView);
#endif
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);
	MainView.Margin(10.0f, &MainView);
#if defined(__ANDROID__)
	MainView.HSplitBottom(10.0f, &MainView, &Extended);
#else
	MainView.HSplitBottom(90.0f, &MainView, &Extended);
#endif

	// tab bar
	{
		// general stuff, fetching IPs, kicking everyone, votebot etc.
		TabBar.VSplitLeft(TabBar.w/2, &Button, &TabBar);
		static CButtonContainer s_Button0;
		if(DoButton_MenuTab(&s_Button0, Localize("General queries"), s_ControlPage == 0, &Button, 0))
			s_ControlPage = 0;

			if(m_pClient->m_pSpoofRemote->IsConnected())
			{
				// control specific players
				TabBar.VSplitRight(0, &Button, &TabBar);
				static CButtonContainer s_Button1;
				if(DoButton_MenuTab(&s_Button1, Localize("Tee controlling related"), s_ControlPage == 1, &Button, 0))
					s_ControlPage = 1;
			}
			else
				RenderTools()->DrawUIRect(&TabBar, ms_ColorTabbarActive, 0, 0);
	}

	// render page
	MainView.HSplitBottom(ms_ButtonHeight + 5*2, &MainView, &Bottom);
	Bottom.HMargin(5.0f, &Bottom);

	if(s_ControlPage == 0)
		RenderSpoofingGeneral(MainView);
	else if(s_ControlPage == 1)
		RenderSpoofingPlayers(MainView);

	// spoofing menu
	{
		CUIRect Button;

		CServerInfo CurrentServerInfo;
		Client()->GetServerInfo(&CurrentServerInfo);

		// addresses and stuff
		char aServerAddr[64];
		char aClientAddr[64];

		str_copy(aServerAddr, CurrentServerInfo.m_aAddress, sizeof(aServerAddr));
		str_copy(aClientAddr, m_pClient->m_aClients[m_SpoofSelectedPlayer].m_Addr, sizeof(aClientAddr));
		net_addr_split(aServerAddr, sizeof(aServerAddr));
		net_addr_split(aClientAddr, sizeof(aClientAddr));

		// always render the last message and a fancy box
		{
			CUIRect Left, Right;
			const float HighlightTime = 2.0f; // highlight the box for 2 seconds
			Extended.HSplitBottom(18.5f, &Extended, &Left);
			float x = max(0-(pi/4), m_pClient->m_pSpoofRemote->LastMessageTime()+HighlightTime*time_freq()-time_get())/HighlightTime;
			float y = (float)sin(pi * (pi / 4.0) * x + (pi / 4.0));
			vec4 Color(0.3f + clamp(y*0.7f, 0.0f, 0.7f), 0.3f, 0.3f, 0.45f);
			Left.VSplitMid(&Left, &Right);
			// inbox
			RenderTools()->DrawUIRect(&Left, Color, CUI::CORNER_ALL, 2.3f);
			Left.VSplitLeft(3.0f, 0, &Left);
			UI()->DoLabelScaled(&Left, m_pClient->m_pSpoofRemote->LastMessage(), 12.0f, -1, Extended.w*0.95f);
			// outbox
			Right.VSplitLeft(5.0f, 0, &Right);
			RenderTools()->DrawUIRect(&Right, vec4(0.3f, 0.33f, 0.3f, 0.45f), CUI::CORNER_ALL, 2.3f);
			Right.VSplitLeft(3.0f, 0, &Right);
			UI()->DoLabelScaled(&Right, m_pClient->m_pSpoofRemote->LastCommand(), 12.0f, -1, Extended.w*0.95f);
		}

		// all deh laz0rs
		if(s_ControlPage == 1)
		{
			// background
			Extended.VMargin(10.0f, &Extended);
			Extended.HSplitTop(20.0f, &Bottom, &Extended);
			//Extended.HSplitTop(5.0f, 0, &Extended);

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(75.0f, &Button, &Bottom);

			static CButtonContainer s_AllCheckbox;
			static int s_DoForAll = 0;
			static int PrevSelectedPlayer = -1;
			if(s_DoForAll && m_SpoofSelectedPlayer > -1)
				s_DoForAll = 0;
			if(DoButton_CheckBox(&s_AllCheckbox, Localize("All"), s_DoForAll, &Button, Localize("Perform the actions for all players")))
			{
				if(s_DoForAll ^= 1)
				{
					PrevSelectedPlayer = m_SpoofSelectedPlayer;
					m_SpoofSelectedPlayer = -1;
				}
				else
				{
					m_SpoofSelectedPlayer = PrevSelectedPlayer;
					PrevSelectedPlayer = -1;
				}
			}

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(75.0f, &Button, &Bottom);
			static CButtonContainer s_KillButton;
			if(DoButton_Menu(&s_KillButton, Localize("Kill"), 0, &Button))
			{
				char aCmd[256];
				if(s_DoForAll)
					str_format(aCmd, sizeof(aCmd), "killall %s", aServerAddr);
				else
					str_format(aCmd, sizeof(aCmd), "kill %s %s", aClientAddr, aServerAddr);
				m_pClient->m_pSpoofRemote->SendCommand(aCmd);
			}

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(100.0f, &Button, &Bottom);
			static CButtonContainer s_DCButton;
			if(DoButton_Menu(&s_DCButton, Localize("Disconnect"), 0, &Button, Localize("Disconnect packet from client to server")))
			{
				char aCmd[256];
				if(s_DoForAll)
					str_format(aCmd, sizeof(aCmd), "dcall %s", aServerAddr);
				else
					str_format(aCmd, sizeof(aCmd), "disconnect %s %s", aClientAddr, aServerAddr);
				m_pClient->m_pSpoofRemote->SendCommand(aCmd);
			}

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(100.0f, &Button, &Bottom);
			static CButtonContainer s_TimeoutButton;
			if(DoButton_Menu(&s_TimeoutButton, Localize("Timeout"), 0, &Button, Localize("Disconnect packet from server to client")))
			{
				// glitchy! found by accident :D
				if(s_DoForAll)
				{
					for(int i = 0; i < MAX_CLIENTS; i++)
					{
						// please don't do drugs while coding... thank you.
						if(!m_pClient->m_aClients[i].m_Spoofable)
							continue;

						char aBuf[NETADDR_MAXSTRSIZE] = {0};
						str_copy(aBuf, m_pClient->m_aClients[i].m_Addr, sizeof(aBuf));
						net_addr_split(aBuf, sizeof(aBuf));
						char aCmd[256];
						str_format(aCmd, sizeof(aCmd), "disconnect %s %s", aServerAddr, aBuf);
						m_pClient->m_pSpoofRemote->SendCommand(aCmd);
					}
				}
				else
				{
					char aCmd[256];
					str_format(aCmd, sizeof(aCmd), "disconnect %s %s", aServerAddr, aClientAddr);
					m_pClient->m_pSpoofRemote->SendCommand(aCmd);
				}
			}

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(100.0f, &Button, &Bottom);
			static CButtonContainer s_StressingButton;
			if(DoButton_Menu(&s_StressingButton, Localize("Stressing"), 0, &Button, Localize("Flood with the players IP to ban him")))
			{
				char aCmd[256];
				if(s_DoForAll)
					str_format(aCmd, sizeof(aCmd), "stressingall %s", aServerAddr);
				else
					str_format(aCmd, sizeof(aCmd), "stressing %s %s", aClientAddr, aServerAddr);
				m_pClient->m_pSpoofRemote->SendCommand(aCmd);
			}

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			//RenderTools()->DrawUIRect(&Bottom, vec4(0.5f, 0.5f, 0.5f, 0.3f), CUI::CORNER_ALL, 5.0f);
			Bottom.VSplitLeft(100.0f, &Button, &Bottom);
			static CButtonContainer s_VoteYesButton;
			if(DoButton_Menu(&s_VoteYesButton, Localize("Vote 'Yes'"), 0, &Button))
			{
				char aCmd[256];
				str_format(aCmd, sizeof(aCmd), "va %s 1", aServerAddr);
				m_pClient->m_pSpoofRemote->SendCommand(aCmd);
			}

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(100.0f, &Button, &Bottom);
			static CButtonContainer s_VoteNoButton;
			if(DoButton_Menu(&s_VoteNoButton, Localize("Vote 'No'"), 0, &Button))
			{
				char aCmd[256];
				str_format(aCmd, sizeof(aCmd), "va %s -1", aServerAddr); // ITS FRIGGIN' -1 NOT 0
				m_pClient->m_pSpoofRemote->SendCommand(aCmd);
			}

			// add vote
			Extended.HSplitTop(20.0f, &Bottom, &Extended);
			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(250.0f, &Button, &Bottom);
			UI()->DoLabelScaled(&Button, Localize("Chat message:"), 14.0f, -1);

			static char s_aChatMessage[128] = {0};
			Extended.HSplitTop(20.0f, &Bottom, &Extended);

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(300.0f, &Button, &Bottom);
			static float s_OffsetChatMsg = 0.0f;
			Button.w -= Button.h;
			static CButtonContainer s_ChatMessageEditbox;
			DoEditBox(&s_ChatMessageEditbox, &Button, s_aChatMessage, sizeof(s_aChatMessage), 14.0f, &s_OffsetChatMsg, false, CUI::CORNER_L);
			// hacky clear button
			{
				CUIRect ClrBt;
				ClrBt.x = Button.x + Button.w;
				ClrBt.y = Button.y;
				ClrBt.w = Button.h;
				ClrBt.h = Button.h;
				static CButtonContainer s_ClearButton;
				if(DoButton_Menu(&s_ClearButton, "×", 0, &ClrBt, 0, CUI::CORNER_R))
				{
					s_OffsetChatMsg = 0.0f;
					mem_zero(s_aChatMessage, sizeof(s_aChatMessage));
				}
			}
			Button.w += Button.h;

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(75.0f, &Button, &Bottom);
			static CButtonContainer s_SendChatButton;
			if(DoButton_Menu(&s_SendChatButton, Localize("Send"), 0, &Button, Localize("Send a chat message")))
			{
				char aCmd[256];
				if(s_DoForAll)
					str_format(aCmd, sizeof(aCmd), "chatall %s %s", aServerAddr, s_aChatMessage);
				else
					str_format(aCmd, sizeof(aCmd), "chat %s %s %s", aClientAddr, aServerAddr, s_aChatMessage);
				m_pClient->m_pSpoofRemote->SendCommand(aCmd);
			}

			Bottom.VSplitLeft(5.0f, 0, &Bottom);
			Bottom.VSplitLeft(110.0f, &Button, &Bottom);
			if(m_pClient->m_pSpoofRemote->IsState(CSpoofRemote::SPOOF_STATE_DUMMIES))
			{
				static CButtonContainer s_SendChatDummiesButton;
				if(DoButton_Menu(&s_SendChatDummiesButton, Localize("Send (Dummies)"), 0, &Button, Localize("Send a chat message from the dummies")))
				{
					char aCmd[256];
					str_format(aCmd, sizeof(aCmd), "chatdum %s", s_aChatMessage);
					m_pClient->m_pSpoofRemote->SendCommand(aCmd);
				}
			}

			Bottom.VSplitLeft(7.5f, 0, &Bottom);
			Bottom.VSplitLeft(150.0f, &Button, &Bottom);
			{
				NETADDR Addr;
				char aBuf[NETADDR_MAXSTRSIZE] = {0};
				static char s_aCustomAddr[NETADDR_MAXSTRSIZE] = {0};
				static int s_CustomAddrState = 0x0; // 0x1 = valid, 0x2 = use

				str_copy(aBuf, s_aCustomAddr, sizeof(aBuf));
				net_addr_split(aBuf, sizeof(aBuf));

				if(str_comp(aClientAddr, s_aCustomAddr) != 0 && (s_CustomAddrState&2))
					s_CustomAddrState &= ~2;

				if(net_addr_from_str(&Addr, s_aCustomAddr) == 0)
					s_CustomAddrState |= 1; // valid
				else
					s_CustomAddrState &= ~1;

				// nice colored background for the IP box
				RenderTools()->DrawUIRect(&Button, vec4(0, s_CustomAddrState&1 ? 1.0f : 0.0f, s_CustomAddrState&2 ? 1.0f : 0.0f, 0.8f), CUI::CORNER_ALL, 5.0f);

				static float s_OffsetCustomAddr = 0.0f;
				static CButtonContainer s_CustomAddrEditbox;
				DoEditBox(&s_CustomAddrEditbox, &Button, s_aCustomAddr, sizeof(s_aCustomAddr), 14.0f, &s_OffsetCustomAddr,
						false, CUI::CORNER_ALL, "Enter custom IP here");

				Bottom.VSplitLeft(5.0f, 0, &Bottom);
				Bottom.VSplitLeft(75.0f, &Button, &Bottom);
				static CButtonContainer s_UseCustomAddrButton;
				if((s_CustomAddrState&1) && !(s_CustomAddrState&2)) // display button only if valid and not in use
				{
					if(DoButton_Menu(&s_UseCustomAddrButton, Localize("Use"), 0, &Button, Localize("Use a custom IP as source address")))
					{
						s_CustomAddrState |= 2; // use
						m_SpoofSelectedPlayer = -1;
						s_DoForAll = 0;
						str_copy(aClientAddr, s_aCustomAddr, sizeof(aClientAddr));
						net_addr_split(aClientAddr, sizeof(aClientAddr));
					}
				}
			}
		}
	}
}

void CMenus::RenderInGameDDRace(CUIRect MainView) //XXX
{
	CALLSTACK_ADD();

	CUIRect Box = MainView;
	CUIRect Button;

	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	Box.HSplitTop(5.0f, &MainView, &MainView);
	Box.HSplitTop(24.0f, &Box, &MainView);
	Box.VMargin(20.0f, &Box);

	Box.VSplitLeft(100.0f, &Button, &Box);
	static CButtonContainer s_BrwoserButton;
	if(DoButton_MenuTab(&s_BrwoserButton, Localize("Browser"), m_DDRacePage==PAGE_BROWSER, &Button, CUI::CORNER_TL))
	{
		m_DDRacePage = PAGE_BROWSER;
	}

	//Box.VSplitLeft(4.0f, 0, &Box);
	Box.VSplitLeft(80.0f, &Button, &Box);
	static CButtonContainer s_GhostButton;
	if(DoButton_MenuTab(&s_GhostButton, Localize("Ghost"), m_DDRacePage==PAGE_GHOST, &Button, 0))
	{
		m_DDRacePage = PAGE_GHOST;
	}

	if(m_DDRacePage != -1)
	{
		if(m_DDRacePage == PAGE_GHOST)
			RenderGhost(MainView);
		else
			RenderInGameBrowser(MainView);
	}

	return;
}

void CMenus::RenderInGameBrowser(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Box = MainView;
	CUIRect Button;

	int Page = g_Config.m_UiPage;
	int NewPage = -1;

	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	Box.HSplitTop(5.0f, &MainView, &MainView);
	Box.HSplitTop(24.0f, &Box, &MainView);
	Box.VMargin(20.0f, &Box);

	Box.VSplitLeft(90.0f+90.0f+130.0f+100.0f+30.0f-100.0f, &Button, &Box);
	Box.VSplitLeft(100.0f, &Button, &Box);
	static CButtonContainer s_InternetButton;
	if(DoButton_MenuTab(&s_InternetButton, Localize("Internet"), Page==PAGE_INTERNET, &Button, 0))
	{
		if (Page != PAGE_INTERNET)
		{
			if(ServerBrowser()->CacheExists())
				ServerBrowser()->LoadCache();
			else
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
		}
		NewPage = PAGE_INTERNET;
	}

	Box.VSplitLeft(80.0f, &Button, &Box);
	static CButtonContainer s_LanButton;
	if(DoButton_MenuTab(&s_LanButton, Localize("LAN"), Page==PAGE_LAN, &Button, 0))
	{
		if (Page != PAGE_LAN)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
		NewPage = PAGE_LAN;
	}

	Box.VSplitLeft(110.0f, &Button, &Box);
	static CButtonContainer s_FavoritesButton;
	if(DoButton_MenuTab(&s_FavoritesButton, Localize("Favorites"), Page==PAGE_FAVORITES, &Button, 0))
	{
		if (Page != PAGE_FAVORITES)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
		NewPage = PAGE_FAVORITES;
	}

	Box.VSplitLeft(90.0f, &Button, &Box);
	static CButtonContainer s_RecentButton;
	if(DoButton_MenuTab(&s_RecentButton, Localize("Recent"), Page==PAGE_RECENT, &Button, 0))
	{
		if(Page != PAGE_RECENT)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_RECENT);
		NewPage = PAGE_RECENT;
	}

	if(g_Config.m_BrShowDDNet)
	{
		Box.VSplitLeft(110.0f, &Button, &Box);
		static CButtonContainer s_DDNetButton;
		if(DoButton_MenuTab(&s_DDNetButton, Localize("DDNet"), Page==PAGE_DDNET, &Button, 0))
		{
			if (Page != PAGE_DDNET)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_DDNET);
			NewPage = PAGE_DDNET;
		}
	}

	if(NewPage != -1)
	{
		if(Client()->State() != IClient::STATE_OFFLINE)
			g_Config.m_UiPage = NewPage;
	}

	RenderServerbrowser(MainView);
	return;
}

// ghost stuff
int CMenus::GhostlistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser)
{
	CALLSTACK_ADD();

	CMenus *pSelf = (CMenus *)pUser;
	int Length = str_length(pName);
	if((pName[0] == '.' && (pName[1] == 0 ||
		(pName[1] == '.' && pName[2] == 0))) ||
		(!IsDir && (Length < 4 || str_comp(pName+Length-4, ".gho"))))
		return 0;

	CGhost::CGhostHeader Header;
	if(!pSelf->m_pClient->m_pGhost->GetInfo(pName, &Header))
		return 0;

	CGhostItem Item;
	str_copy(Item.m_aFilename, pName, sizeof(Item.m_aFilename));
	str_copy(Item.m_aPlayer, Header.m_aOwner, sizeof(Item.m_aPlayer));
	Item.m_Time = Header.m_Time;
	Item.m_Active = false;
	Item.m_ID = pSelf->m_lGhosts.add(Item);

	return 0;
}

void CMenus::GhostlistPopulate()
{
	CALLSTACK_ADD();

	m_OwnGhost = 0;
	m_lGhosts.clear();
	Storage()->ListDirectory(IStorageTW::TYPE_ALL, "ghosts", GhostlistFetchCallback, this);

	for(int i = 0; i < m_lGhosts.size(); i++)
	{
		if(str_comp(m_lGhosts[i].m_aPlayer, g_Config.m_PlayerName) == 0 && (!m_OwnGhost || m_lGhosts[i] < *m_OwnGhost))
			m_OwnGhost = &m_lGhosts[i];
	}

	if(m_OwnGhost)
	{
		m_OwnGhost->m_ID = -1;
		m_OwnGhost->m_Active = true;
		m_pClient->m_pGhost->Load(m_OwnGhost->m_aFilename, -1);
	}
}

void CMenus::RenderGhost(CUIRect MainView)
{
	CALLSTACK_ADD();

	// render background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B|CUI::CORNER_TL, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitBottom(5.0f, &MainView, 0);
	MainView.VSplitLeft(5.0f, 0, &MainView);
	MainView.VSplitRight(5.0f, &MainView, 0);

	CUIRect Headers, Status;
	CUIRect View = MainView;

	View.HSplitTop(17.0f, &Headers, &View);
	View.HSplitBottom(28.0f, &View, &Status);

	// split of the scrollbar
	RenderTools()->DrawUIRect(&Headers, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f);
	Headers.VSplitRight(20.0f, &Headers, 0);

	struct CColumn
	{
		int m_Id;
		CLocConstString m_Caption;
		float m_Width;
		CUIRect m_Rect;
		CUIRect m_Spacer;
	};

	enum
	{
		COL_ACTIVE=0,
		COL_NAME,
		COL_TIME,
	};

	static CColumn s_aCols[] = {
		{-1,			" ",		2.0f,		0, 0},
		{COL_ACTIVE,	" ",		30.0f,		0, 0},
		{COL_NAME,		"Name",		300.0f,		0, 0},
		{COL_TIME,		"Time",		200.0f,		0, 0},
	};

	int NumCols = sizeof(s_aCols)/sizeof(CColumn);

	// do layout
	for(int i = 0; i < NumCols; i++)
	{
		Headers.VSplitLeft(s_aCols[i].m_Width, &s_aCols[i].m_Rect, &Headers);

		if(i+1 < NumCols)
			Headers.VSplitLeft(2, &s_aCols[i].m_Spacer, &Headers);
	}

	// do headers
	for(int i = 0; i < NumCols; i++)
	{
		CPointerContainer Container(s_aCols[i].m_Caption);
		DoButton_GridHeader(&Container, s_aCols[i].m_Caption, 0, &s_aCols[i].m_Rect);
	}

	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.15f), 0, 0);

	CUIRect Scroll;
	View.VSplitRight(15, &View, &Scroll);

	int NumGhosts = m_lGhosts.size();

	int Num = (int)(View.h/s_aCols[0].m_Rect.h) + 1;
	static CButtonContainer s_ScrollBar;
	static float s_ScrollValue = 0;

	Scroll.HMargin(5.0f, &Scroll);
	s_ScrollValue = DoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

	int ScrollNum = NumGhosts-Num+1;
	if(ScrollNum > 0)
	{
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP))
			s_ScrollValue -= 1.0f/ScrollNum;
		if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN))
			s_ScrollValue += 1.0f/ScrollNum;
	}
	else
		ScrollNum = 0;

	static int s_SelectedIndex = 0;
	for(int i = 0; i < m_NumInputEvents; i++)
	{
		int NewIndex = -1;
		if(m_aInputEvents[i].m_Flags&IInput::FLAG_PRESS)
		{
			if(m_aInputEvents[i].m_Key == KEY_DOWN) NewIndex = s_SelectedIndex + 1;
			if(m_aInputEvents[i].m_Key == KEY_UP) NewIndex = s_SelectedIndex - 1;
		}
		if(NewIndex > -1 && NewIndex < NumGhosts)
		{
			//scroll
			float IndexY = View.y - s_ScrollValue*ScrollNum*s_aCols[0].m_Rect.h + NewIndex*s_aCols[0].m_Rect.h;
			int Scroll = View.y > IndexY ? -1 : View.y+View.h < IndexY+s_aCols[0].m_Rect.h ? 1 : 0;
			if(Scroll)
			{
				if(Scroll < 0)
				{
					int NumScrolls = (View.y-IndexY+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
					s_ScrollValue -= (1.0f/ScrollNum)*NumScrolls;
				}
				else
				{
					int NumScrolls = (IndexY+s_aCols[0].m_Rect.h-(View.y+View.h)+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
					s_ScrollValue += (1.0f/ScrollNum)*NumScrolls;
				}
			}

			s_SelectedIndex = NewIndex;
		}
	}

	if(s_ScrollValue < 0) s_ScrollValue = 0;
	if(s_ScrollValue > 1) s_ScrollValue = 1;

	// set clipping
	UI()->ClipEnable(&View);

	CUIRect OriginalView = View;
	View.y -= s_ScrollValue*ScrollNum*s_aCols[0].m_Rect.h;

	int NewSelected = -1;

	for (int i = 0; i < NumGhosts; i++)
	{
		const CGhostItem *pItem = &m_lGhosts[i];
		CUIRect Row;
		CUIRect SelectHitBox;

		View.HSplitTop(17.0f, &Row, &View);
		SelectHitBox = Row;

		// make sure that only those in view can be selected
		if(Row.y+Row.h > OriginalView.y && Row.y < OriginalView.y+OriginalView.h)
		{
			if(i == s_SelectedIndex)
			{
				CUIRect r = Row;
				r.Margin(1.5f, &r);
				RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
			}

			// clip the selection
			if(SelectHitBox.y < OriginalView.y) // top
			{
				SelectHitBox.h -= OriginalView.y-SelectHitBox.y;
				SelectHitBox.y = OriginalView.y;
			}
			else if(SelectHitBox.y+SelectHitBox.h > OriginalView.y+OriginalView.h) // bottom
				SelectHitBox.h = OriginalView.y+OriginalView.h-SelectHitBox.y;

			if(UI()->DoButtonLogic(pItem, "", 0, &SelectHitBox))
			{
				NewSelected = i;
			}
		}

		for(int c = 0; c < NumCols; c++)
		{
			CUIRect Button;
			Button.x = s_aCols[c].m_Rect.x;
			Button.y = Row.y;
			Button.h = Row.h;
			Button.w = s_aCols[c].m_Rect.w;

			int Id = s_aCols[c].m_Id;

			if(Id == COL_ACTIVE)
			{
				if(pItem->m_Active)
				{
					Graphics()->TextureSet(g_pData->m_aImages[IMAGE_EMOTICONS].m_Id);
					Graphics()->QuadsBegin();
					RenderTools()->SelectSprite(SPRITE_OOP + 7);
					IGraphics::CQuadItem QuadItem(Button.x+Button.w/2, Button.y+Button.h/2, 20.0f, 20.0f);
					Graphics()->QuadsDraw(&QuadItem, 1);

					Graphics()->QuadsEnd();
				}
			}
			else if(Id == COL_NAME)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				char aBuf[128];
				bool Own = m_OwnGhost && pItem == m_OwnGhost;
				str_format(aBuf, sizeof(aBuf), "%s%s", pItem->m_aPlayer, Own?" (own)":"");
				TextRender()->TextEx(&Cursor, aBuf, -1);
			}
			else if(Id == COL_TIME)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				char aBuf[64];
				str_format(aBuf, sizeof(aBuf), "%02d:%06.3f", (int)pItem->m_Time/60, pItem->m_Time-((int)pItem->m_Time/60*60));
				TextRender()->TextEx(&Cursor, aBuf, -1);
			}
		}
	}

	if(NewSelected != -1)
		s_SelectedIndex = NewSelected;

	CGhostItem *pGhost = &m_lGhosts[s_SelectedIndex];

	UI()->ClipDisable();

	RenderTools()->DrawUIRect(&Status, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	Status.Margin(5.0f, &Status);

	CUIRect Button;
	Status.VSplitRight(120.0f, &Status, &Button);

	static CButtonContainer s_GhostButton;
	const char *pText = pGhost->m_Active ? "Deactivate" : "Activate";

	if(DoButton_Menu(&s_GhostButton, Localize(pText), 0, &Button) || (NewSelected != -1 && Input()->MouseDoubleClick()))
	{
		if(pGhost->m_Active)
			m_pClient->m_pGhost->Unload(pGhost->m_ID);
		else
			m_pClient->m_pGhost->Load(pGhost->m_aFilename, pGhost->m_ID);
		pGhost->m_Active ^= 1;
	}
}
