/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/color.h>
#include <engine/config.h>
#include <engine/friends.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/textrender.h>
#include <engine/updater.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>

#include <game/localization.h>
#include <game/version.h>
#include <game/client/render.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/console.h>

#include "menus.h"


void CMenus::RenderServerbrowserServerList(CUIRect View)
{
	CALLSTACK_ADD();

	CUIRect Headers;
	CUIRect Status;

	View.HSplitTop(ms_ListheaderHeight, &Headers, &View);
	View.HSplitBottom(28.0f, &View, &Status);

	// split of the scrollbar
	RenderTools()->DrawUIRect(&Headers, vec4(1,1,1,0.25f), CUI::CORNER_T, 5.0f);
	Headers.VSplitRight(20.0f, &Headers, 0);

	struct CColumn
	{
		int m_ID;
		int m_Sort;
		CLocConstString m_Caption;
		int m_Direction;
		float m_Width;
		int m_Flags;
		CUIRect m_Rect;
		CUIRect m_Spacer;
	};

	enum
	{
		FIXED=1,
		SPACER=2,

		COL_FLAG_LOCK=0,
		COL_FLAG_FAV,
		COL_NAME,
		COL_GAMETYPE,
		COL_MAP,
		COL_PLAYERS,
		COL_PING,
		COL_VERSION,
	};

//<<<! HEAD
	static CColumn s_aCols[] = {
		{-1				, -1								, " "		, -1,   2.0f, 0, 0, 0},
		{COL_FLAG_LOCK	, -1								, " "		, -1,  14.0f, 0, 0, 0},
		{COL_FLAG_FAV	, -1								, " "		, -1,  14.0f, 0, 0, 0},
		{COL_NAME		, IServerBrowser::SORT_NAME			, "Name"	,  0,  50.0f, 0, 0, 0},	// Localize - these strings are localized within CLocConstString
		{COL_GAMETYPE	, IServerBrowser::SORT_GAMETYPE		, "Type"	,  1,  50.0f, 0, 0, 0},
		{COL_MAP		, IServerBrowser::SORT_MAP			, "Map"		,  1, 100.0f + (Headers.w - 480) / 8, 0, 0, 0},
		{COL_PLAYERS	, IServerBrowser::SORT_NUMPLAYERS	, "Players"	,  1,  60.0f, 0, 0, 0},
		{-1				, -1								, " "		,  1,  10.0f, 0, 0, 0},
		{COL_PING		, IServerBrowser::SORT_PING			, "Ping"	,  1,  40.0f, FIXED, 0, 0},
/*=======
	CColumn s_aCols[] = {
		{-1,			-1,						" ",		-1, 2.0f, 0, {0}, {0}},
		{COL_FLAG_LOCK,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_FLAG_FAV,	-1,						" ",		-1, 14.0f, 0, {0}, {0}},
		{COL_NAME,		IServerBrowser::SORT_NAME,		"Name",		0, 50.0f, 0, {0}, {0}},	// Localize - these strings are localized within CLocConstString
		{COL_GAMETYPE,	IServerBrowser::SORT_GAMETYPE,	"Type",		1, 50.0f, 0, {0}, {0}},
		{COL_MAP,		IServerBrowser::SORT_MAP,			"Map", 		1, 100.0f + (Headers.w - 480) / 8, 0, {0}, {0}},
		{COL_PLAYERS,	IServerBrowser::SORT_NUMPLAYERS,	"Players",	1, 60.0f, 0, {0}, {0}},
		{-1,			-1,						" ",		1, 10.0f, 0, {0}, {0}},
		{COL_PING,		IServerBrowser::SORT_PING,		"Ping",		1, 40.0f, FIXED, {0}, {0}},
>>>>>>> ddnet/master*/
#if defined(__ANDROID__)
		{-1,			-1,						" ",		1, 50.0f, 0, 0, 0}, // Scrollbar
#endif
	};
	// This is just for scripts/update_localization.py to work correctly (all other strings are already Localize()'d somewhere else). Don't remove!
	// Localize("Type");

	int NumCols = sizeof(s_aCols)/sizeof(CColumn);

	// do layout
	for(int i = 0; i < NumCols; i++)
	{
		if(s_aCols[i].m_Direction == -1)
		{
			Headers.VSplitLeft(s_aCols[i].m_Width, &s_aCols[i].m_Rect, &Headers);

			if(i+1 < NumCols)
			{
				//Cols[i].flags |= SPACER;
				Headers.VSplitLeft(2, &s_aCols[i].m_Spacer, &Headers);
			}
		}
	}

	for(int i = NumCols-1; i >= 0; i--)
	{
		if(s_aCols[i].m_Direction == 1)
		{
			Headers.VSplitRight(s_aCols[i].m_Width, &Headers, &s_aCols[i].m_Rect);
			Headers.VSplitRight(2, &Headers, &s_aCols[i].m_Spacer);
		}
	}

	for(int i = 0; i < NumCols; i++)
	{
		if(s_aCols[i].m_Direction == 0)
			s_aCols[i].m_Rect = Headers;
	}

	// do headers
	for(int i = 0; i < NumCols; i++)
	{
		CPointerContainer Container(s_aCols[i].m_Caption);
		if(DoButton_GridHeader(&Container, s_aCols[i].m_Caption, g_Config.m_BrSort == s_aCols[i].m_Sort, &s_aCols[i].m_Rect, 0))
		{
			if(s_aCols[i].m_Sort != -1)
			{
				if(g_Config.m_BrSort == s_aCols[i].m_Sort)
					g_Config.m_BrSortOrder ^= 1;
				else
					g_Config.m_BrSortOrder = 0;
				g_Config.m_BrSort = s_aCols[i].m_Sort;
			}
		}
	}

	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.15f), 0, 0);

	CUIRect Scroll;
#if defined(__ANDROID__)
	View.VSplitRight(50, &View, &Scroll);
#else
	View.VSplitRight(15, &View, &Scroll);
#endif

	int NumServers = ServerBrowser()->NumSortedServers();

	// display important messages in the middle of the screen so no
	// users misses it
	{
		CUIRect MsgBox = View;
		MsgBox.y += View.h/3;

		if(m_ActivePage == PAGE_BROWSER && g_Config.m_UiBrowserPage == PAGE_BROWSER_INTERNET && ServerBrowser()->IsRefreshingMasters())
			UI()->DoLabelScaled(&MsgBox, Localize("Refreshing master servers"), 16.0f, 0);
		else if(!ServerBrowser()->NumServers())
			UI()->DoLabelScaled(&MsgBox, Localize("No servers found"), 16.0f, 0);
		else if(ServerBrowser()->NumServers() && !NumServers)
			UI()->DoLabelScaled(&MsgBox, Localize("No servers match your filter criteria"), 16.0f, 0);
	}

	int Num = (int)(View.h/s_aCols[0].m_Rect.h) + 1;
	static CButtonContainer s_ScrollBar;
	static float s_ScrollValue = 0, s_WantedScrollValue = 0.0f;

	Scroll.HMargin(5.0f, &Scroll);
	float LastScrollValue = s_ScrollValue;
	s_ScrollValue = DoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);
	if(s_ScrollValue != LastScrollValue)
		s_WantedScrollValue = s_ScrollValue;

	int ScrollNum = NumServers-Num+1;
	if(ScrollNum > 0)
	{
		if(m_ScrollOffset >= 0)
		{
			s_WantedScrollValue = (float)(m_ScrollOffset)/ScrollNum;
			m_ScrollOffset = -1;
		}
		if(UI()->MouseInside(&View))
		{
			if(KeyEvent(KEY_MOUSE_WHEEL_UP))
				s_WantedScrollValue -= Input()->KeyIsPressed(KEY_LSHIFT) ? 1.5f / ScrollNum : Input()->KeyIsPressed(KEY_LCTRL) ? 6.0f / ScrollNum : 3.0f / ScrollNum;
			else if(KeyEvent(KEY_MOUSE_WHEEL_DOWN))
				s_WantedScrollValue += Input()->KeyIsPressed(KEY_LSHIFT) ? 1.5f / ScrollNum : Input()->KeyIsPressed(KEY_LCTRL) ? 6.0f / ScrollNum : 3.0f / ScrollNum;
		}
	}
	else
		ScrollNum = 0;

	if(KeyEvent(KEY_TAB))
	{
		if(KeyMods(KEYMOD_SHIFT))
			g_Config.m_UiToolboxPage = (g_Config.m_UiToolboxPage + 3 - 1) % 3;
		else
			g_Config.m_UiToolboxPage = (g_Config.m_UiToolboxPage + 3 + 1) % 3;
	}
	if(m_SelectedIndex > -1)
	{
		for(int i = 0; i < m_NumInputEvents; i++)
		{
			int NewIndex = -1;
			if(m_aInputEvents[i].m_Flags&IInput::FLAG_PRESS)
			{
				if(m_aInputEvents[i].m_Key == KEY_DOWN) NewIndex = m_SelectedIndex + 1;
				if(m_aInputEvents[i].m_Key == KEY_UP) NewIndex = m_SelectedIndex - 1;
				if(m_aInputEvents[i].m_Key == KEY_PAGEUP) NewIndex = max(m_SelectedIndex - 25, 0);
				if(m_aInputEvents[i].m_Key == KEY_PAGEDOWN) NewIndex = min(m_SelectedIndex + 25, NumServers - 1);
				if(m_aInputEvents[i].m_Key == KEY_HOME) NewIndex = 0;
				if(m_aInputEvents[i].m_Key == KEY_END) NewIndex = NumServers - 1;
			}
			if(NewIndex > -1 && NewIndex < NumServers)
			{
				//scroll
				float IndexY = View.y - s_WantedScrollValue*ScrollNum*s_aCols[0].m_Rect.h + NewIndex*s_aCols[0].m_Rect.h;
				int Scroll = View.y > IndexY ? -1 : View.y+View.h < IndexY+s_aCols[0].m_Rect.h ? 1 : 0;
				if(Scroll)
				{
					if(Scroll < 0)
					{
						int NumScrolls = (View.y-IndexY+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
						s_WantedScrollValue -= (1.0f/ScrollNum)*NumScrolls;
					}
					else
					{
						int NumScrolls = (IndexY+s_aCols[0].m_Rect.h-(View.y+View.h)+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
						s_WantedScrollValue += (1.0f/ScrollNum)*NumScrolls;
					}
				}

				m_SelectedIndex = NewIndex;


				const CServerInfo *pItem;
				if(g_Config.m_UiBrowserPage == PAGE_BROWSER_RECENT)
					pItem = ServerBrowser()->Get(m_SelectedIndex);
				else
					pItem = ServerBrowser()->SortedGet(m_SelectedIndex);
				str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
			}
		}
	}

	if(s_WantedScrollValue < 0) s_WantedScrollValue = 0;
	if(s_WantedScrollValue > 1) s_WantedScrollValue = 1;
	smooth_set(&s_ScrollValue, s_WantedScrollValue, 27.0f, Client()->RenderFrameTime());

	// set clipping
	UI()->ClipEnable(&View);

	CUIRect OriginalView = View;
	View.y -= s_ScrollValue*ScrollNum*s_aCols[0].m_Rect.h;

	int NewSelected = -1;
	int DoubleClicked = 0;
	int NumPlayers = 0;

	m_SelectedIndex = -1;

	// reset friend counter
	for(int i = 0; i < m_lFriends.size(); m_lFriends[i++].m_NumFound = 0);

	for (int i = 0; i < NumServers; i++)
	{
		int ItemIndex = i;
		const CServerInfo *pItem = ServerBrowser()->SortedGet(ItemIndex);
		if(!pItem || pItem->m_NumClients > pItem->m_MaxClients) // make sure we have only sane entries
			continue;
		NumPlayers += g_Config.m_BrFilterSpectators ? pItem->m_NumPlayers : pItem->m_NumClients;
		CUIRect Row;
		CUIRect SelectHitBox;

		int Selected = str_comp(pItem->m_aAddress, g_Config.m_UiServerAddress) == 0; //selected_index==ItemIndex;

		View.HSplitTop(ms_ListheaderHeight, &Row, &View);
		SelectHitBox = Row;

		if(Selected)
			m_SelectedIndex = i;

		// update friend counter
		if(pItem->m_FriendState != IFriends::FRIEND_NO)
		{
			for(int j = 0; j < pItem->m_NumReceivedClients; ++j)
			{
				if(pItem->m_aClients[j].m_FriendState != IFriends::FRIEND_NO)
				{
					unsigned NameHash = str_quickhash(pItem->m_aClients[j].m_aName);
					unsigned ClanHash = str_quickhash(pItem->m_aClients[j].m_aClan);
					for(int f = 0; f < m_lFriends.size(); ++f)
					{
						if(((g_Config.m_ClFriendsIgnoreClan && m_lFriends[f].m_pFriendInfo->m_aName[0]) || (ClanHash == m_lFriends[f].m_pFriendInfo->m_ClanHash && !str_comp(m_lFriends[f].m_pFriendInfo->m_aClan, pItem->m_aClients[j].m_aClan))) &&
							(!m_lFriends[f].m_pFriendInfo->m_aName[0] || (NameHash == m_lFriends[f].m_pFriendInfo->m_NameHash && !str_comp(m_lFriends[f].m_pFriendInfo->m_aName, pItem->m_aClients[j].m_aName))))
						{
							m_lFriends[f].m_NumFound++;
							if(m_lFriends[f].m_pFriendInfo->m_aName[0])
								break;
						}
					}
				}
			}
		}

		// make sure that only those in view can be selected
		if(Row.y+Row.h > OriginalView.y && Row.y < OriginalView.y+OriginalView.h)
		{
			if(Selected)
			{
				CUIRect r = Row;
				r.Margin(1.5f, &r);
				RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
			}

			// indicator for how old the server info is
			if(g_Config.m_Debug)
			{
				CUIRect r = Row;
				r.Margin(1.5f, &r);

				const float MAX_AGE = 60 * 60 * 24*2;
				int Seconds = ServerBrowser()->GetInfoAge(ItemIndex);
				if(Seconds >= 0)
				{
					float red = clamp((float)Seconds / MAX_AGE, 0.0f, 1.0f);
					float green = 1.0f - clamp((float)Seconds / MAX_AGE, 0.0f, 1.0f);
					RenderTools()->DrawUIRect(&r, vec4(red,green,1,0.3f), CUI::CORNER_ALL, 4.0f);
				}
			}


			// clip the selection
			if(SelectHitBox.y < OriginalView.y) // top
			{
				SelectHitBox.h -= OriginalView.y-SelectHitBox.y;
				SelectHitBox.y = OriginalView.y;
			}
			else if(SelectHitBox.y+SelectHitBox.h > OriginalView.y+OriginalView.h) // bottom
				SelectHitBox.h = OriginalView.y+OriginalView.h-SelectHitBox.y;

			if(UI()->DoButtonLogic(pItem, "", Selected, &SelectHitBox) && !m_MouseUnlocked)
			{
				NewSelected = ItemIndex;
				if(NewSelected == m_DoubleClickIndex)
					DoubleClicked = 1;
				m_DoubleClickIndex = NewSelected;
			}

			if(g_Config.m_Debug && UI()->MouseInside(&SelectHitBox))
				m_pClient->m_pTooltip->SetTooltip(ServerBrowser()->GetDebugString(ItemIndex));
		}
		else
		{
			// reset active item, if not visible
			if(UI()->ActiveItem() == pItem)
				UI()->SetActiveItem(0);

			// don't render invisible items
			continue;
		}

		for(int c = 0; c < NumCols; c++)
		{
			CUIRect Button;
			char aTemp[64];
			Button.x = s_aCols[c].m_Rect.x;
			Button.y = Row.y;
			Button.h = Row.h;
			Button.w = s_aCols[c].m_Rect.w;

			int ID = s_aCols[c].m_ID;

			if(ID == COL_FLAG_LOCK)
			{
				if(pItem->m_Flags & SERVER_FLAG_PASSWORD)
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_LOCK, &Button);
			}
			else if(ID == COL_FLAG_FAV)
			{
				if(pItem->m_Favorite)
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART, &Button);
			}
			else if(ID == COL_NAME)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_SERVERNAME))
				{
					// highlight the parts that matches
					const char *pStr = str_find_nocase(pItem->m_aName, g_Config.m_BrFilterString);
					if(pStr)
					{
						TextRender()->TextEx(&Cursor, pItem->m_aName, (int)(pStr-pItem->m_aName));
						TextRender()->TextColor(0.4f,0.4f,1.0f,1);
						TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(1,1,1,1);
						TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
					}
					else
						TextRender()->TextEx(&Cursor, pItem->m_aName, -1);
				}
				else
					TextRender()->TextEx(&Cursor, pItem->m_aName, -1);
			}
			else if(ID == COL_MAP)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_MAPNAME))
				{
					// highlight the parts that matches
					const char *pStr = str_find_nocase(pItem->m_aMap, g_Config.m_BrFilterString);
					if(pStr)
					{
						TextRender()->TextEx(&Cursor, pItem->m_aMap, (int)(pStr-pItem->m_aMap));
						TextRender()->TextColor(0.4f,0.4f,1.0f,1);
						TextRender()->TextEx(&Cursor, pStr, str_length(g_Config.m_BrFilterString));
						TextRender()->TextColor(1,1,1,1);
						TextRender()->TextEx(&Cursor, pStr+str_length(g_Config.m_BrFilterString), -1);
					}
					else
						TextRender()->TextEx(&Cursor, pItem->m_aMap, -1);
				}
				else
					TextRender()->TextEx(&Cursor, pItem->m_aMap, -1);
			}
			else if(ID == COL_PLAYERS)
			{
				CUIRect Icon;
				Button.VMargin(4.0f, &Button);
				if(pItem->m_FriendState != IFriends::FRIEND_NO)
				{
					Button.VSplitLeft(Button.h, &Icon, &Button);
					Icon.Margin(2.0f, &Icon);
					DoButton_Icon(IMAGE_BROWSEICONS, SPRITE_BROWSE_HEART, &Icon);
				}

				if(g_Config.m_BrFilterSpectators)
					str_format(aTemp, sizeof(aTemp), "%i/%i", pItem->m_NumPlayers, pItem->m_MaxPlayers);
				else
					str_format(aTemp, sizeof(aTemp), "%i/%i", pItem->m_NumClients, pItem->m_MaxClients);
				if(g_Config.m_BrFilterString[0] && (pItem->m_QuickSearchHit&IServerBrowser::QUICK_PLAYER))
					TextRender()->TextColor(0.4f,0.4f,1.0f,1);
				UI()->DoLabelScaled(&Button, aTemp, 12.0f, 1);
				TextRender()->TextColor(1,1,1,1);
			}
			else if(ID == COL_PING)
			{
				str_format(aTemp, sizeof(aTemp), "%i", pItem->m_Latency);
				if (g_Config.m_UiColorizePing)
				{
					vec3 rgb = HslToRgb(vec3((300.0f - clamp(pItem->m_Latency, 0, 300)) / 1000.0f, 1.0f, 0.5f));
					TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
				}

				UI()->DoLabelScaled(&Button, aTemp, 12.0f, 1);
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
			else if(ID == COL_VERSION)
			{
				const char *pVersion = pItem->m_aVersion;
				UI()->DoLabelScaled(&Button, pVersion, 12.0f, 1);
			}
			else if(ID == COL_GAMETYPE)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f*UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				if (g_Config.m_UiColorizeGametype)
				{
					vec3 hsl = vec3(1.0f, 1.0f, 1.0f);

					if (IsVanilla(pItem))
						hsl = vec3(0.33f, 1.0f, 0.75f);
					else if (IsCatch(pItem))
						hsl = vec3(0.17f, 1.0f, 0.75f);
					else if (IsInsta(pItem))
						hsl = vec3(0.00f, 1.0f, 0.75f);
					else if (IsFNG(pItem))
						hsl = vec3(0.83f, 1.0f, 0.75f);
					else if (IsDDNet(pItem))
						hsl = vec3(0.58f, 1.0f, 0.75f);
					else if (IsDDRace(pItem))
						hsl = vec3(0.75f, 1.0f, 0.75f);
					else if (IsRace(pItem))
						hsl = vec3(0.46f, 1.0f, 0.75f);

					vec3 rgb = HslToRgb(hsl);
					TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
					TextRender()->TextEx(&Cursor, pItem->m_aGameType, -1);
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				}
				else
					TextRender()->TextEx(&Cursor, pItem->m_aGameType, -1);
			}
		}
	}

	UI()->ClipDisable();

	if(NewSelected != -1)
	{
		// select the new server
		const CServerInfo *pItem = ServerBrowser()->SortedGet(NewSelected);
		str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
#if defined(__ANDROID__)
		if(DoubleClicked)
#else
		if(Input()->MouseDoubleClick() && !m_MouseUnlocked && DoubleClicked)
#endif
			Client()->Connect(g_Config.m_UiServerAddress);
	}

	RenderTools()->DrawUIRect(&Status, vec4(1,1,1,0.25f), CUI::CORNER_B, 5.0f);
	Status.Margin(5.0f, &Status);

	CUIRect QuickSearch, QuickExclude, Button, Status2, Status3;
	Status.VSplitRight(250.0f, &Status2, &Status3);

	Status2.VSplitMid(&QuickSearch, &QuickExclude);
	QuickExclude.VSplitLeft(5.0f, 0, &QuickExclude);
	// render quick search
	{
		const char *pLabel = "⚲";
		UI()->DoLabelScaled(&QuickSearch, pLabel, 12.0f, -1);
		float w = TextRender()->TextWidth(0, 12.0f, pLabel, -1);
		QuickSearch.VSplitLeft(w, 0, &QuickSearch);
		QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
		QuickSearch.VSplitLeft(QuickSearch.w-15.0f, &QuickSearch, &Button);
		static float Offset = 0.0f;
		CPointerContainer s_FilterStringEditbox(&g_Config.m_BrFilterString);
		if(DoEditBox(&s_FilterStringEditbox, &QuickSearch, g_Config.m_BrFilterString, sizeof(g_Config.m_BrFilterString), 12.0f, &Offset, false, CUI::CORNER_L, Localize("Search")))
			Client()->ServerBrowserUpdate();
	}

	// clear button
	{
		static CButtonContainer s_ClearButton;
		if(DoButton_Menu(&s_ClearButton, "×", 0, &Button, Localize("clear"), CUI::CORNER_R, vec4(1,1,1,0.35f)))
		{
			g_Config.m_BrFilterString[0] = 0;
			UI()->SetActiveItem(&g_Config.m_BrFilterString);
			Client()->ServerBrowserUpdate();
		}
	}

	// render quick exclude
	{
		UI()->DoLabelScaled(&QuickExclude, "✗", 12.0f, -1);
		float w = TextRender()->TextWidth(0, 12.0f, "✗", -1);
		QuickExclude.VSplitLeft(w, 0, &QuickExclude);
		QuickExclude.VSplitLeft(5.0f, 0, &QuickExclude);
		QuickExclude.VSplitLeft(QuickExclude.w-15.0f, &QuickExclude, &Button);
		static float Offset = 0.0f;
		CPointerContainer s_ExcludeStringEditbox(&g_Config.m_BrExcludeString);
		if(DoEditBox(&s_ExcludeStringEditbox, &QuickExclude, g_Config.m_BrExcludeString, sizeof(g_Config.m_BrExcludeString), 12.0f, &Offset, false, CUI::CORNER_L, Localize("Exclude")))
			Client()->ServerBrowserUpdate();
	}

	// clear button
	{
		static CButtonContainer s_ClearButton;
		if(DoButton_Menu(&s_ClearButton, "×", 0, &Button, Localize("clear"), CUI::CORNER_R, vec4(1,1,1,0.35f)))
		{
			g_Config.m_BrExcludeString[0] = 0;
			UI()->SetActiveItem(&g_Config.m_BrExcludeString);
			Client()->ServerBrowserUpdate();
		}
	}

	// render status
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), Localize("%d of %d servers, %d players"), ServerBrowser()->NumSortedServers(), ServerBrowser()->NumServers(), NumPlayers);
	Status3.VSplitRight(TextRender()->TextWidth(0, 14.0f, aBuf, -1), 0, &Status3);
	UI()->DoLabelScaled(&Status3, aBuf, 14.0f, -1);

	// auto-cache when refreshed
	{
		static bool s_HasCached = false;
		if(g_Config.m_BrAutoCache)
		{
			if(ServerBrowser()->GetCurrentType() == IServerBrowser::TYPE_INTERNET && !s_HasCached &&
					!ServerBrowser()->IsRefreshing() && m_ActivePage == PAGE_BROWSER && g_Config.m_UiBrowserPage == PAGE_BROWSER_INTERNET)
			{
				ServerBrowser()->SaveCache();
				s_HasCached = true;
			}
			else if(s_HasCached && ServerBrowser()->IsRefreshing())
			{
				s_HasCached = false;
			}
		}
	}

	// auto-refresh
	if(g_Config.m_BrAutoRefresh && !ServerBrowser()->IsRefreshing())
	{
		if(time_get() > m_RefreshTimer + time_freq() * g_Config.m_BrAutoRefresh)
			ServerBrowser()->RefreshQuick();
		else if((m_RefreshTimer - time_get()) / time_freq() + (int64)g_Config.m_BrAutoRefresh < 0 && !ServerBrowser()->IsRefreshing())
			m_RefreshTimer = time_get();
	}
	if(g_Config.m_BrAutoRefresh && ServerBrowser()->IsRefreshing())
		m_RefreshTimer = time_get();
}

void CMenus::RenderServerbrowserFilters(CUIRect View)
{
	CALLSTACK_ADD();

	CUIRect ServerFilter = View, FilterHeader;
	const float FontSize = 12.0f;
	ServerFilter.HSplitBottom(0.0f, &ServerFilter, 0);

	// server filter
	ServerFilter.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFilter);
	RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerFilter, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	UI()->DoLabelScaled(&FilterHeader, Localize("Server filter"), FontSize+2.0f, 0);
	CUIRect Button, Button2;

	ServerFilter.VSplitLeft(5.0f, 0, &ServerFilter);
	ServerFilter.Margin(3.0f, &ServerFilter);
	ServerFilter.VMargin(5.0f, &ServerFilter);

	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterEmptyCheckbox;
	if(DoButton_CheckBox(&s_BrFilterEmptyCheckbox, Localize("Has people playing"), g_Config.m_BrFilterEmpty, &Button))
	{
		if(g_Config.m_BrFilterEmpty ^= 1)
			g_Config.m_BrFilterNonEmpty = 0;
	}

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterNonEmptyCheckbox;
	if(DoButton_CheckBox(&s_BrFilterNonEmptyCheckbox, Localize("Server is empty"), g_Config.m_BrFilterNonEmpty, &Button))
	{
		if(g_Config.m_BrFilterNonEmpty ^= 1)
			g_Config.m_BrFilterEmpty = 0;
	}

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterSpectatorsCheckbox;
	if(DoButton_CheckBox(&s_BrFilterSpectatorsCheckbox, Localize("Count players only"), g_Config.m_BrFilterSpectators, &Button))
		g_Config.m_BrFilterSpectators ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterFullCheckbox;
	if (DoButton_CheckBox(&s_BrFilterFullCheckbox, Localize("Server not full"), g_Config.m_BrFilterFull, &Button))
		g_Config.m_BrFilterFull ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterFriendsCheckbox;
	if (DoButton_CheckBox(&s_BrFilterFriendsCheckbox, Localize("Show friends only"), g_Config.m_BrFilterFriends, &Button))
		g_Config.m_BrFilterFriends ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterPwCheckbox;
	if (DoButton_CheckBox(&s_BrFilterPwCheckbox, Localize("No password"), g_Config.m_BrFilterPw, &Button))
		g_Config.m_BrFilterPw ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterCompatversionCheckbox;
	if (DoButton_CheckBox(&s_BrFilterCompatversionCheckbox, Localize("Compatible version"), g_Config.m_BrFilterCompatversion, &Button))
		g_Config.m_BrFilterCompatversion ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterPureCheckbox;
	if (DoButton_CheckBox(&s_BrFilterPureCheckbox, Localize("Standard gametype"), g_Config.m_BrFilterPure, &Button))
		g_Config.m_BrFilterPure ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterCheckbox;
	if (DoButton_CheckBox(&s_BrFilterCheckbox, Localize("Standard map"), g_Config.m_BrFilterPureMap, &Button))
		g_Config.m_BrFilterPureMap ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterGametypeStrictCheckbox;
	if (DoButton_CheckBox(&s_BrFilterGametypeStrictCheckbox, Localize("Strict gametype filter"), g_Config.m_BrFilterGametypeStrict, &Button))
		g_Config.m_BrFilterGametypeStrict ^= 1;

	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	static CButtonContainer s_BrFilterVersionStrictCheckbox;
	if (DoButton_CheckBox(&s_BrFilterVersionStrictCheckbox, Localize("Strict version filter"), g_Config.m_BrFilterVersionStrict, &Button))
		g_Config.m_BrFilterVersionStrict ^= 1;

	/*ServerFilter.HSplitTop(5.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
	if (DoButton_CheckBox((char *)&g_Config.m_BrFilterDDRaceNetwork, Localize("Hide DDRaceNetwork"), g_Config.m_BrFilterDDRaceNetwork, &Button, Localize("Activate this if you use the DDNet serverbrowser")))
		g_Config.m_BrFilterDDRaceNetwork ^= 1;*/

	ServerFilter.HSplitTop(7.0f, 0, &ServerFilter);

	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	UI()->DoLabelScaled(&Button, Localize("Game types:"), FontSize, -1);
	Button.VSplitRight(60.0f, 0, &Button);
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	static float GametypeOffset = 0.0f;
	static CButtonContainer s_FilterGametypeEditbox;
	if(DoEditBox(&s_FilterGametypeEditbox, &Button, g_Config.m_BrFilterGametype, sizeof(g_Config.m_BrFilterGametype), FontSize, &GametypeOffset))
		Client()->ServerBrowserUpdate();

	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	UI()->DoLabelScaled(&Button, Localize("Version:"), FontSize, -1);
	Button.VSplitRight(60.0f, 0, &Button);
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	static float VersionOffset = 0.0f;
	static CButtonContainer s_FilterVersionEditbox;
	if(DoEditBox(&s_FilterVersionEditbox, &Button, g_Config.m_BrFilterVersion, sizeof(g_Config.m_BrFilterVersion), FontSize, &VersionOffset))
		Client()->ServerBrowserUpdate();

	{
		ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
		CUIRect EditBox;
		Button.VSplitRight(60.0f, &Button, &EditBox);

		UI()->DoLabelScaled(&Button, Localize("Maximum ping:"), FontSize, -1);

		char aBuf[5];
		str_format(aBuf, sizeof(aBuf), "%d", g_Config.m_BrFilterPing);
		static float Offset = 0.0f;
		static CButtonContainer s_FilterPingEditbox;
		DoEditBox(&s_FilterPingEditbox, &EditBox, aBuf, sizeof(aBuf), FontSize, &Offset);
		g_Config.m_BrFilterPing = clamp(str_toint(aBuf), 0, 999);
	}

	// server address
	ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
	ServerFilter.HSplitTop(19.0f, &Button, &ServerFilter);
	UI()->DoLabelScaled(&Button, Localize("Server address:"), FontSize, -1);
	Button.VSplitRight(60.0f, 0, &Button);
	static float OffsetAddr = 0.0f;
	static CButtonContainer s_FilterServerAddressEditbox;
	if(DoEditBox(&s_FilterServerAddressEditbox, &Button, g_Config.m_BrFilterServerAddress, sizeof(g_Config.m_BrFilterServerAddress), FontSize, &OffsetAddr))
		Client()->ServerBrowserUpdate();

	// player country
	{
		CUIRect Rect;
		ServerFilter.HSplitTop(3.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(26.0f, &Button, &ServerFilter);
		Button.VSplitRight(60.0f, &Button, &Rect);
		Button.HMargin(3.0f, &Button);
		static CButtonContainer s_FilterCountryEditbox;
		if(DoButton_CheckBox(&s_FilterCountryEditbox, Localize("Player country:"), g_Config.m_BrFilterCountry, &Button))
			g_Config.m_BrFilterCountry ^= 1;

		float OldWidth = Rect.w;
		Rect.w = Rect.h*2;
		Rect.x += (OldWidth-Rect.w)/2.0f;
		vec4 Color(1.0f, 1.0f, 1.0f, g_Config.m_BrFilterCountry?1.0f: 0.5f);
		m_pClient->m_pCountryFlags->Render(g_Config.m_BrFilterCountryIndex, Color, Rect.x, Rect.y, Rect.w, Rect.h);

		if(g_Config.m_BrFilterCountry && UI()->DoButtonLogic(&g_Config.m_BrFilterCountryIndex, "", 0, &Rect))
			m_Popup = POPUP_COUNTRY;
	}

	// additional settings
	if(g_Config.m_UiBrowserPage != PAGE_BROWSER_DDNET)
	{
		ServerFilter.HSplitTop(20.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(15.0f, &Button, &ServerFilter);
		static CButtonContainer Scrollbar;
		g_Config.m_BrAutoRefresh = round_to_int(DoScrollbarH(&Scrollbar, &Button, (float)g_Config.m_BrAutoRefresh/60.0f, Localize("Time in seconds to refresh the infos"), g_Config.m_BrAutoRefresh)*60.0f);

		ServerFilter.HSplitTop(10.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
		static CButtonContainer s_AutocacheCheckbox;
		if(DoButton_CheckBox(&s_AutocacheCheckbox, Localize("Cache Manager"), g_Config.m_BrAutoCache, &Button))
			g_Config.m_BrAutoCache ^= 1;
	}

	CUIRect ResetButton;

	//ServerFilter.HSplitBottom(5.0f, &ServerFilter, 0);
	ServerFilter.HSplitBottom(ms_ButtonHeight-2.0f, &ServerFilter, &ResetButton);

	// ddnet country filters
	if(g_Config.m_UiBrowserPage == PAGE_BROWSER_DDNET)
	{
		// add more space
		ServerFilter.HSplitTop(10.0f, 0, &ServerFilter);
		ServerFilter.HSplitTop(20.0f, &Button, &ServerFilter);
		ServerFilter.HSplitTop(95.0f, &ServerFilter, 0);

		RenderTools()->DrawUIRect(&ServerFilter, ms_ColorTabbarActive, CUI::CORNER_B, 10.0f);

		Button.VSplitMid(&Button, &Button2);

		static int s_ActivePage = 0;

		static CButtonContainer s_CountriesButton;
		if(DoButton_MenuTab(&s_CountriesButton, Localize("Countries"), s_ActivePage == 0, &Button, CUI::CORNER_TL))
		{
			s_ActivePage = 0;
		}

		static CButtonContainer s_TypesButton;
		if(DoButton_MenuTab(&s_TypesButton, Localize("Types"), s_ActivePage == 1, &Button2, CUI::CORNER_TR))
		{
			s_ActivePage = 1;
		}

		if(s_ActivePage == 1)
		{
			int MaxTypes = ServerBrowser()->NumDDNetTypes();
			int NumTypes = ServerBrowser()->NumDDNetTypes();
			int PerLine = 3;

			ServerFilter.HSplitTop(4.0f, 0, &ServerFilter);
			ServerFilter.HSplitBottom(4.0f, &ServerFilter, 0);

			const float TypesWidth = 40.0f;
			const float TypesHeight = (const float)(ServerFilter.h / ceil(MaxTypes / (float)PerLine));

			CUIRect TypesRect, Left, Right;

			static int s_aTypeButtons[64];

			while(NumTypes > 0)
			{
				ServerFilter.HSplitTop(TypesHeight, &TypesRect, &ServerFilter);
				TypesRect.VSplitMid(&Left, &Right);

				for(int i = 0; i < PerLine && NumTypes > 0; i++, NumTypes--)
				{
					int TypeIndex = MaxTypes - NumTypes;
					const char *pName = ServerBrowser()->GetDDNetType(TypeIndex);
					bool Active = !ServerBrowser()->DDNetFiltered(g_Config.m_BrFilterExcludeTypes, pName);

					vec2 Pos = vec2(TypesRect.x+TypesRect.w*((i+0.5f)/(float) PerLine), TypesRect.y);

					// correct pos
					Pos.x -= TypesWidth / 2.0f;

					// create button logic
					CUIRect Rect;

					Rect.x = Pos.x;
					Rect.y = Pos.y;
					Rect.w = TypesWidth;
					Rect.h = TypesHeight;

					if (UI()->DoButtonLogic(&s_aTypeButtons[TypeIndex], "", 0, &Rect))
					{
						// toggle flag filter
						if (Active)
							ServerBrowser()->DDNetFilterAdd(g_Config.m_BrFilterExcludeTypes, pName);
						else
							ServerBrowser()->DDNetFilterRem(g_Config.m_BrFilterExcludeTypes, pName);

						ServerBrowser()->Refresh(IServerBrowser::TYPE_DDNET);
					}

					vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);

					if (!Active)
						Color.a = 0.2f;
					TextRender()->TextColor(Color.r, Color.g, Color.b, Color.a);
					UI()->DoLabelScaled(&Rect, pName, FontSize, 0);
					TextRender()->TextColor(1.0, 1.0, 1.0, 1.0f);
				}
			}
		}
		else
		{
			ServerFilter.HSplitTop(17.0f, &ServerFilter, &ServerFilter);

			vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);

			const float FlagWidth = 40.0f;
			const float FlagHeight = 20.0f;

			int MaxFlags = ServerBrowser()->NumDDNetCountries();
			int NumFlags = ServerBrowser()->NumDDNetCountries();
			int PerLine = MaxFlags > 9 ? 4 : 3;

			CUIRect FlagsRect;

			static int s_aFlagButtons[64];

			while(NumFlags > 0)
			{
				ServerFilter.HSplitTop(30.0f, &FlagsRect, &ServerFilter);

				for(int i = 0; i < PerLine && NumFlags > 0; i++, NumFlags--)
				{
					int CountryIndex = MaxFlags - NumFlags;
					const char *pName = ServerBrowser()->GetDDNetCountryName(CountryIndex);
					bool Active = !ServerBrowser()->DDNetFiltered(g_Config.m_BrFilterExcludeCountries, pName);
					int FlagID = ServerBrowser()->GetDDNetCountryFlag(CountryIndex);

					vec2 Pos = vec2(FlagsRect.x+FlagsRect.w*((i+0.5f)/(float) PerLine), FlagsRect.y);

					// correct pos
					Pos.x -= FlagWidth / 2.0f;
					Pos.y -= FlagHeight / 2.0f;

					// create button logic
					CUIRect Rect;

					Rect.x = Pos.x;
					Rect.y = Pos.y;
					Rect.w = FlagWidth;
					Rect.h = FlagHeight;

					if (UI()->DoButtonLogic(&s_aFlagButtons[CountryIndex], "", 0, &Rect))
					{
						// toggle flag filter
						if (Active)
							ServerBrowser()->DDNetFilterAdd(g_Config.m_BrFilterExcludeCountries, pName);
						else
							ServerBrowser()->DDNetFilterRem(g_Config.m_BrFilterExcludeCountries, pName);

						ServerBrowser()->Refresh(IServerBrowser::TYPE_DDNET);
					}

					vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);

					if (!Active)
						Color.a = 0.2f;

					m_pClient->m_pCountryFlags->Render(FlagID, Color, Pos.x, Pos.y, FlagWidth, FlagHeight);
				}
			}
		}
	}

	static CButtonContainer s_ClearButton;
	if(DoButton_Menu(&s_ClearButton, Localize("Reset filter"), 0, &ResetButton))
	{
		g_Config.m_BrFilterString[0] = 0;
		g_Config.m_BrExcludeString[0] = 0;
		g_Config.m_BrFilterFull = 0;
		g_Config.m_BrFilterEmpty = 0;
		g_Config.m_BrFilterSpectators = 0;
		g_Config.m_BrFilterFriends = 0;
		g_Config.m_BrFilterCountry = 0;
		g_Config.m_BrFilterCountryIndex = -1;
		g_Config.m_BrFilterPw = 0;
		g_Config.m_BrFilterPing = 999;
		g_Config.m_BrFilterGametype[0] = 0;
		g_Config.m_BrFilterGametypeStrict = 0;
		g_Config.m_BrFilterVersion[0] = 0;
		g_Config.m_BrFilterVersionStrict = 0;
		//g_Config.m_BrFilterDDRaceNetwork = 0;
		g_Config.m_BrFilterServerAddress[0] = 0;
		g_Config.m_BrFilterPure = 0;
		g_Config.m_BrFilterPureMap = 0;
		g_Config.m_BrFilterCompatversion = 0;
		g_Config.m_BrFilterExcludeCountries[0] = 0;
		g_Config.m_BrFilterExcludeTypes[0] = 0;
		Client()->ServerBrowserUpdate();
	}
}

void CMenus::RenderServerbrowserServerDetail(CUIRect View)
{
	CALLSTACK_ADD();

	CUIRect ServerDetails = View;
	CUIRect ServerScoreBoard, ServerHeader;

	const CServerInfo *pSelectedServer = ServerBrowser()->SortedGet(m_SelectedIndex);

	// split off a piece to use for scoreboard
	ServerDetails.HSplitTop(90.0f, &ServerDetails, &ServerScoreBoard);
	ServerDetails.HSplitBottom(2.5f, &ServerDetails, 0x0);

	// server details
	CTextCursor Cursor;
	const float FontSize = 12.0f;
	ServerDetails.HSplitTop(ms_ListheaderHeight, &ServerHeader, &ServerDetails);
	RenderTools()->DrawUIRect(&ServerHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerDetails, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	UI()->DoLabelScaled(&ServerHeader, Localize("Server details"), FontSize+2.0f, 0);

	if (pSelectedServer)
	{
		ServerDetails.VSplitLeft(5.0f, 0, &ServerDetails);
		ServerDetails.Margin(3.0f, &ServerDetails);

		CUIRect Row;
		static CLocConstString s_aLabels[] = {
			"Version",	// Localize - these strings are localized within CLocConstString
			"Game type",
			"Ping"};

		CUIRect LeftColumn;
		CUIRect RightColumn;

		//
		{
			CUIRect Button;
			ServerDetails.HSplitBottom(20.0f, &ServerDetails, &Button);
			Button.VSplitLeft(5.0f, 0, &Button);
			static CButtonContainer s_AddFavButton;
			if(DoButton_CheckBox(&s_AddFavButton, Localize("Favorite"), pSelectedServer->m_Favorite, &Button))
			{
				if(pSelectedServer->m_Favorite)
					ServerBrowser()->RemoveFavorite(pSelectedServer->m_NetAddr);
				else
					ServerBrowser()->AddFavorite(pSelectedServer->m_NetAddr);
			}
		}

		ServerDetails.VSplitLeft(5.0f, 0x0, &ServerDetails);
		ServerDetails.VSplitLeft(80.0f, &LeftColumn, &RightColumn);

		for (unsigned int i = 0; i < sizeof(s_aLabels) / sizeof(s_aLabels[0]); i++)
		{
			LeftColumn.HSplitTop(15.0f, &Row, &LeftColumn);
			UI()->DoLabelScaled(&Row, s_aLabels[i], FontSize, -1);
		}

		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		TextRender()->SetCursor(&Cursor, Row.x, Row.y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Row.w;
		TextRender()->TextEx(&Cursor, pSelectedServer->m_aVersion, -1);

		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		TextRender()->SetCursor(&Cursor, Row.x, Row.y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Row.w;
		TextRender()->TextEx(&Cursor, pSelectedServer->m_aGameType, -1);

		char aTemp[16];
		str_format(aTemp, sizeof(aTemp), "%d", pSelectedServer->m_Latency);
		RightColumn.HSplitTop(15.0f, &Row, &RightColumn);
		TextRender()->SetCursor(&Cursor, Row.x, Row.y, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = Row.w;
		TextRender()->TextEx(&Cursor, aTemp, -1);

	}

	// server scoreboard
	//ServerScoreBoard.HSplitBottom(20.0f, &ServerScoreBoard, 0x0);

	if(pSelectedServer)
	{
		static CButtonContainer s_VoteList;
		static float s_ScrollValue = 0;
		UiDoListboxStart(&s_VoteList, &ServerScoreBoard, 26.0f, Localize("Scoreboard"), "", pSelectedServer->m_NumReceivedClients, 1, -1, s_ScrollValue);

		for (int i = 0; i < pSelectedServer->m_NumReceivedClients; i++)
		{
			CPointerContainer Container(&i);
			CListboxItem Item = UiDoListboxNextItem(&Container);

			if(!Item.m_Visible)
				continue;

			CUIRect Name, Clan, Score, Flag;
			Item.m_Rect.HSplitTop(25.0f, &Name, &Item.m_Rect);
			if(UI()->DoButtonLogic(&pSelectedServer->m_aClients[i], "", 0, &Name))
			{
				if(pSelectedServer->m_aClients[i].m_FriendState == IFriends::FRIEND_PLAYER)
					m_pClient->Friends()->RemoveFriend(pSelectedServer->m_aClients[i].m_aName, pSelectedServer->m_aClients[i].m_aClan);
				else
					m_pClient->Friends()->AddFriend(pSelectedServer->m_aClients[i].m_aName, pSelectedServer->m_aClients[i].m_aClan);
				FriendlistOnUpdate();
				Client()->ServerBrowserUpdate();
			}

			vec4 Colour = pSelectedServer->m_aClients[i].m_FriendState == IFriends::FRIEND_NO ? vec4(1.0f, 1.0f, 1.0f, (i%2+1)*0.05f) :
																								vec4(0.5f, 1.0f, 0.5f, 0.15f+(i%2+1)*0.05f);
			RenderTools()->DrawUIRect(&Name, Colour, CUI::CORNER_ALL, 4.0f);
			Name.VSplitLeft(5.0f, 0, &Name);
			Name.VSplitLeft(34.0f, &Score, &Name);
			Name.VSplitRight(34.0f, &Name, &Flag);
			Flag.HMargin(4.0f, &Flag);
			Name.HSplitTop(11.0f, &Name, &Clan);

			// score
			char aTemp[16];

			if(!pSelectedServer->m_aClients[i].m_Player)
				str_copy(aTemp, "SPEC", sizeof(aTemp));
			else if(IsRace(pSelectedServer))
			{
				if(pSelectedServer->m_aClients[i].m_Score == -9999 || pSelectedServer->m_aClients[i].m_Score == 0)
					aTemp[0] = 0;
				else
				{
					int Time = abs(pSelectedServer->m_aClients[i].m_Score);
					str_format(aTemp, sizeof(aTemp), "%02d:%02d", Time/60, Time%60);
				}
			}
			else
				str_format(aTemp, sizeof(aTemp), "%d", pSelectedServer->m_aClients[i].m_Score);

			TextRender()->SetCursor(&Cursor, Score.x, Score.y+(Score.h-FontSize)/4.0f, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Score.w;
			TextRender()->TextEx(&Cursor, aTemp, -1);

			// name
			TextRender()->SetCursor(&Cursor, Name.x, Name.y, FontSize-2, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Name.w;
			const char *pName = pSelectedServer->m_aClients[i].m_aName;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(pName, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pName, (int)(s-pName));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pName, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pName, -1);

			// clan
			TextRender()->SetCursor(&Cursor, Clan.x, Clan.y, FontSize-2, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = Clan.w;
			const char *pClan = pSelectedServer->m_aClients[i].m_aClan;
			if(g_Config.m_BrFilterString[0])
			{
				// highlight the parts that matches
				const char *s = str_find_nocase(pClan, g_Config.m_BrFilterString);
				if(s)
				{
					TextRender()->TextEx(&Cursor, pClan, (int)(s-pClan));
					TextRender()->TextColor(0.4f, 0.4f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s, str_length(g_Config.m_BrFilterString));
					TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
					TextRender()->TextEx(&Cursor, s+str_length(g_Config.m_BrFilterString), -1);
				}
				else
					TextRender()->TextEx(&Cursor, pClan, -1);
			}
			else
				TextRender()->TextEx(&Cursor, pClan, -1);

			// flag
			vec4 Color(1.0f, 1.0f, 1.0f, 0.5f);
			m_pClient->m_pCountryFlags->Render(pSelectedServer->m_aClients[i].m_Country, Color, Flag.x, Flag.y, Flag.w, Flag.h);
		}

		UiDoListboxEnd(&s_ScrollValue, 0);
	}
}

void CMenus::FriendlistOnUpdate()
{
	CALLSTACK_ADD();

	m_lFriends.clear();
	for(int i = 0; i < m_pClient->Friends()->NumFriends(); ++i)
	{
		CFriendItem Item;
		Item.m_pFriendInfo = m_pClient->Friends()->GetFriend(i);
		Item.m_NumFound = 0;
		m_lFriends.add_unsorted(Item);
	}
	m_lFriends.sort_range();
}

void CMenus::RenderServerbrowserFriends(CUIRect View)
{
	CALLSTACK_ADD();

	static int s_Inited = 0;
	if(!s_Inited)
	{
		FriendlistOnUpdate();
		s_Inited = 1;
	}

	CUIRect ServerFriends = View, FilterHeader;
	const float FontSize = 10.0f;

	// header
	ServerFriends.HSplitTop(ms_ListheaderHeight, &FilterHeader, &ServerFriends);
	RenderTools()->DrawUIRect(&FilterHeader, vec4(1,1,1,0.25f), CUI::CORNER_T, 4.0f);
	RenderTools()->DrawUIRect(&ServerFriends, vec4(0,0,0,0.15f), 0, 4.0f);
	UI()->DoLabelScaled(&FilterHeader, Localize("Friends"), FontSize+4.0f, 0);
	CUIRect Button, List;

	ServerFriends.Margin(3.0f, &ServerFriends);
	ServerFriends.VMargin(3.0f, &ServerFriends);
	ServerFriends.HSplitBottom(100.0f, &List, &ServerFriends);

	// friends list(remove friend)
	static float s_ScrollValue = 0;
	if(m_FriendlistSelectedIndex >= m_lFriends.size())
		m_FriendlistSelectedIndex = m_lFriends.size()-1;
	static CButtonContainer lFriends;
#if defined(__ANDROID__)
	UiDoListboxStart(&lFriends, &List, 50.0f, "", "", m_lFriends.size(), 1, m_FriendlistSelectedIndex, s_ScrollValue);
#else
	UiDoListboxStart(&lFriends, &List, 30.0f, "", "", m_lFriends.size(), 1, m_FriendlistSelectedIndex, s_ScrollValue);
#endif

	m_lFriends.sort_range();
	for(int i = 0; i < m_lFriends.size(); ++i)
	{
		CPointerContainer Container(&m_lFriends[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container, false, false);

		if(Item.m_Visible)
		{
			Item.m_Rect.Margin(1.5f, &Item.m_Rect);
			CUIRect OnState;
			Item.m_Rect.VSplitRight(30.0f, &Item.m_Rect, &OnState);
			RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1.0f, 1.0f, 1.0f, 0.1f), CUI::CORNER_L, 4.0f);

			Item.m_Rect.VMargin(2.5f, &Item.m_Rect);
			Item.m_Rect.HSplitTop(12.0f, &Item.m_Rect, &Button);
			UI()->DoLabelScaled(&Item.m_Rect, m_lFriends[i].m_pFriendInfo->m_aName, FontSize, -1);
			UI()->DoLabelScaled(&Button, m_lFriends[i].m_pFriendInfo->m_aClan, FontSize, -1);

			RenderTools()->DrawUIRect(&OnState, m_lFriends[i].m_NumFound ? vec4(0.0f, 1.0f, 0.0f, 0.25f) : vec4(1.0f, 0.0f, 0.0f, 0.25f), CUI::CORNER_R, 4.0f);
			OnState.HMargin((OnState.h-FontSize)/3, &OnState);
			OnState.VMargin(5.0f, &OnState);
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%i", m_lFriends[i].m_NumFound);
			UI()->DoLabelScaled(&OnState, aBuf, FontSize+2, 1);
		}
	}

	bool Activated = false;
	m_FriendlistSelectedIndex = UiDoListboxEnd(&s_ScrollValue, &Activated);

	// activate found server with friend
	if(Activated && !m_EnterPressed && m_lFriends[m_FriendlistSelectedIndex].m_NumFound)
	{
		bool Found = false;
		int NumServers = ServerBrowser()->NumSortedServers();
		for (int i = 0; i < NumServers && !Found; i++)
		{
			int ItemIndex = m_SelectedIndex != -1 ? (m_SelectedIndex+i+1)%NumServers : i;
			const CServerInfo *pItem = ServerBrowser()->SortedGet(ItemIndex);
			if(pItem->m_FriendState != IFriends::FRIEND_NO)
			{
				for(int j = 0; j < pItem->m_NumReceivedClients && !Found; ++j)
				{
					if(pItem->m_aClients[j].m_FriendState != IFriends::FRIEND_NO &&
						((g_Config.m_ClFriendsIgnoreClan && m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aName[0]) || str_quickhash(pItem->m_aClients[j].m_aClan) == m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_ClanHash) &&
						(!m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aName[0] ||
						str_quickhash(pItem->m_aClients[j].m_aName) == m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_NameHash))
					{
						str_copy(g_Config.m_UiServerAddress, pItem->m_aAddress, sizeof(g_Config.m_UiServerAddress));
						m_ScrollOffset = ItemIndex;
						m_SelectedIndex = ItemIndex;
						Found = true;
					}
				}
			}
		}
	}

	ServerFriends.HSplitTop(2.5f, 0, &ServerFriends);
	ServerFriends.HSplitTop(20.0f, &Button, &ServerFriends);
	if(m_FriendlistSelectedIndex != -1)
	{
		static CButtonContainer s_RemoveButton;
		if(DoButton_Menu(&s_RemoveButton, Localize("Remove"), 0, &Button))
			m_Popup = POPUP_REMOVE_FRIEND;
	}

	// add friend
	if(m_pClient->Friends()->NumFriends() < IFriends::MAX_FRIENDS)
	{
		ServerFriends.HSplitTop(10.0f, 0, &ServerFriends);
		ServerFriends.HSplitTop(19.0f, &Button, &ServerFriends);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
		UI()->DoLabelScaled(&Button, aBuf, FontSize, -1);
		Button.VSplitLeft(80.0f, 0, &Button);
		static char s_aName[MAX_NAME_LENGTH] = {0};
		static float s_OffsetName = 0.0f;
		static CButtonContainer s_NameEditbox;
		DoEditBox(&s_NameEditbox, &Button, s_aName, sizeof(s_aName), FontSize, &s_OffsetName);

		ServerFriends.HSplitTop(3.0f, 0, &ServerFriends);
		ServerFriends.HSplitTop(19.0f, &Button, &ServerFriends);
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
		UI()->DoLabelScaled(&Button, aBuf, FontSize, -1);
		Button.VSplitLeft(80.0f, 0, &Button);
		static char s_aClan[MAX_CLAN_LENGTH] = {0};
		static float s_OffsetClan = 0.0f;
		static CButtonContainer s_ClanEditbox;
		DoEditBox(&s_ClanEditbox, &Button, s_aClan, sizeof(s_aClan), FontSize, &s_OffsetClan);

		ServerFriends.HSplitTop(3.0f, 0, &ServerFriends);
		ServerFriends.HSplitTop(20.0f, &Button, &ServerFriends);
		static CButtonContainer s_AddButton;
		if(DoButton_Menu(&s_AddButton, Localize("Add Friend"), 0, &Button))
		{
			m_pClient->Friends()->AddFriend(s_aName, s_aClan);
			FriendlistOnUpdate();
			Client()->ServerBrowserUpdate();
		}
	}
}

void CMenus::RenderServerbrowser(CUIRect MainView)
{
	CALLSTACK_ADD();

	/*
		+-----------------+	+-------+
		|				  |	|		|
		|				  |	| tool	|
		|   server list	  |	| box 	|
		|				  |	|	  	|
		|				  |	|		|
		+-----------------+	|	 	|
			status box	tab	+-------+
	*/

	CUIRect ServerList, ToolBox, StatusBox, TabBar;

	// background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	MainView.Margin(10.0f, &MainView);

	// create server list, status box, tab bar and tool box area
	MainView.VSplitRight(205.0f, &ServerList, &ToolBox);
	ServerList.HSplitBottom(70.0f, &ServerList, &StatusBox);
	StatusBox.VSplitRight(100.0f, &StatusBox, &TabBar);
	ServerList.VSplitRight(5.0f, &ServerList, 0);

	// server list
	{
		RenderServerbrowserServerList(ServerList);
	}

	int ToolboxPage = g_Config.m_UiToolboxPage;

	// tab bar
	{
		CUIRect TabButton0, TabButton1, TabButton2;
		TabBar.HSplitTop(5.0f, 0, &TabBar);
		TabBar.HSplitTop(20.0f, &TabButton0, &TabBar);
		TabBar.HSplitTop(2.5f, 0, &TabBar);
		TabBar.HSplitTop(20.0f, &TabButton1, &TabBar);
		TabBar.HSplitTop(2.5f, 0, &TabBar);
		TabBar.HSplitTop(20.0f, &TabButton2, 0);
		vec4 Active = ms_ColorTabbarActive;
		vec4 InActive = ms_ColorTabbarInactive;
		ms_ColorTabbarActive = vec4(0.0f, 0.0f, 0.0f, 0.3f);
		ms_ColorTabbarInactive = vec4(0.0f, 0.0f, 0.0f, 0.15f);

		static CButtonContainer s_FiltersTab;
		if (DoButton_MenuTab(&s_FiltersTab, Localize("Filter"), ToolboxPage==0, &TabButton0, CUI::CORNER_L))
			ToolboxPage = 0;

		static CButtonContainer s_InfoTab;
		if (DoButton_MenuTab(&s_InfoTab, Localize("Info"), ToolboxPage==1, &TabButton1, CUI::CORNER_L))
			ToolboxPage = 1;

		static CButtonContainer s_FriendsTab;
		if (DoButton_MenuTab(&s_FriendsTab, Localize("Friends"), ToolboxPage==2, &TabButton2, CUI::CORNER_L))
			ToolboxPage = 2;

		ms_ColorTabbarActive = Active;
		ms_ColorTabbarInactive = InActive;
		g_Config.m_UiToolboxPage = ToolboxPage;
	}

	// tool box
	{
		RenderTools()->DrawUIRect(&ToolBox, vec4(0.0f, 0.0f, 0.0f, 0.15f), CUI::CORNER_T, 4.0f);


		if(ToolboxPage == 0)
			RenderServerbrowserFilters(ToolBox);
		else if(ToolboxPage == 1)
			RenderServerbrowserServerDetail(ToolBox);
		else if(ToolboxPage == 2)
			RenderServerbrowserFriends(ToolBox);
	}

	// status box
	{
		CUIRect Button, ButtonArea;
		StatusBox.HSplitTop(5.0f, 0, &StatusBox);

		// version note
#if defined(CONF_FAMILY_WINDOWS) || (defined(CONF_PLATFORM_LINUX) && !defined(__ANDROID__))
		CUIRect Part;
		StatusBox.HSplitBottom(15.0f, &StatusBox, &Button);
		char aBuf[128];
		int State = Updater()->State();
		bool NeedUpdate = str_comp(Client()->LatestVersion(), "0") != 0;
#if defined(CONF_SPOOFING)
		NeedUpdate = false;
#endif
		if(State == IUpdater::STATE_CLEAN && NeedUpdate)
		{
			str_format(aBuf, sizeof(aBuf), "New Version '%s' is out!", Client()->LatestVersion());
			float fade = sinf(Client()->LocalTime()*3.1415f)*0.2f;
			TextRender()->TextColor(1.0f, 0.4f+fade, 0.4f+fade, 1.0f+fade/2.0f-0.1f);
		}
		else if(State == IUpdater::STATE_CLEAN)
		{
			if(g_Config.m_Debug)
				str_format(aBuf, sizeof(aBuf), "Client version string: tw:%s-%s-ddnet:%s", GAME_VERSION, GAME_ATH_VERSION, GAME_RELEASE_VERSION);
			else
				aBuf[0] = '\0';
		}
		else if(State == IUpdater::STATE_SYNC_REFRESH)
			str_format(aBuf, sizeof(aBuf), "Refreshing version info...");
		else if(State == IUpdater::STATE_GETTING_MANIFEST || State == IUpdater::STATE_SYNC_POSTGETTING)
			str_format(aBuf, sizeof(aBuf), "Checking out files...");
		else if(State == IUpdater::STATE_DOWNLOADING)
			str_format(aBuf, sizeof(aBuf), "Downloading '%s':", Updater()->GetCurrentFile());
		else if(State == IUpdater::STATE_MOVE_FILES)
			str_format(aBuf, sizeof(aBuf), "Installing '%s'", Updater()->GetCurrentFile());
		else if(State == IUpdater::STATE_FAIL)
		{
			str_format(aBuf, sizeof(aBuf), "Updater: %s failed.", Updater()->GetWhatFailed());
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		}
		else if(State == IUpdater::STATE_NEED_RESTART)
		{
			str_format(aBuf, sizeof(aBuf), "AllTheHaxx updated!");
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		}
		UI()->DoLabelScaled(&Button, aBuf, 14.0f, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

		Button.VSplitLeft(TextRender()->TextWidth(0, 14.0f, aBuf, -1) + 10.0f, &Button, &Part);

		if(State == IUpdater::STATE_CLEAN && NeedUpdate)
		{
			CUIRect Update;
			Part.VSplitLeft(100.0f, &Update, NULL);

			static CButtonContainer s_ButtonUpdate;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Update now"), 0, &Update))
			{
				Updater()->PerformUpdate();
			}
		}
		else if(State == IUpdater::STATE_NEED_RESTART)
		{
			CUIRect Restart;
			Part.VSplitLeft(50.0f, &Restart, &Part);

			static CButtonContainer s_ButtonUpdate;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Restart"), 0, &Restart))
			{
				Client()->Restart();
			}
		}
		else if(State >= IUpdater::STATE_GETTING_MANIFEST && State <= IUpdater::STATE_DOWNLOADING)
		{
			CUIRect ProgressBar, Percent;
			Part.VSplitLeft(100.0f, &ProgressBar, &Percent);
			ProgressBar.y += 2.0f;
			ProgressBar.HMargin(1.0f, &ProgressBar);
			RenderTools()->DrawUIRect(&ProgressBar, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
			ProgressBar.w = clamp((float)Updater()->GetCurrentPercent(), 10.0f, 100.0f);
			RenderTools()->DrawUIRect(&ProgressBar, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 5.0f);
		}
/*#else // XXX no official android and osx support.
		StatusBox.HSplitBottom(15.0f, &StatusBox, &Button);
		char aBuf[64];
		if(str_comp(Client()->LatestVersion(), "0") != 0)
		{
<<<! HEAD
			str_format(aBuf, sizeof(aBuf), Localize("AllTheHaxx %s is out! Download it at allthehaxx.tk!"), Client()->LatestVersion());
=======
			str_format(aBuf, sizeof(aBuf), Localize("DDNet %s is out! Download it at DDNet.tw!"), Client()->LatestVersion());
>>>>>>> ddnet/master
			TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		}
		else
			str_format(aBuf, sizeof(aBuf), Localize("Current version: %s"), GAME_VERSION);
		UI()->DoLabelScaled(&Button, aBuf, 14.0f, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);*/
#endif
		// button area
		CUIRect AbortButton;
		//StatusBox.VSplitRight(80.0f, &StatusBox, 0);
		StatusBox.VSplitRight(170.0f, &StatusBox, &ButtonArea);
		//ButtonArea.VSplitRight(150.0f, 0, &ButtonArea);
		ButtonArea.HSplitTop(20.0f, &Button, &ButtonArea);
		Button.VMargin(20.0f, &Button);

		if(ServerBrowser()->IsRefreshing())
		{
			Button.VSplitRight(Button.h, &Button, &AbortButton);
			static CButtonContainer s_AbortButton;
			if(DoButton_Menu(&s_AbortButton, "×", 0, &AbortButton, 0, CUI::CORNER_R))
				ServerBrowser()->AbortRefresh();
		}

		if(ServerBrowser()->IsRefreshing())
			str_format(aBuf, sizeof(aBuf), "%s (%d%%)", Localize("Refresh"), ServerBrowser()->LoadingProgression());
		else
			str_copy(aBuf, Localize("Refresh"), sizeof(aBuf));

		static CButtonContainer s_RefreshButton;
		if(DoButton_Menu(&s_RefreshButton, aBuf, 0, &Button, Localize("Refresh the serverlist completely"), ServerBrowser()->IsRefreshing() ? CUI::CORNER_L : CUI::CORNER_ALL) || (KeyMods(KEYMOD_CTRL) && KeyEvent(KEY_R)))
		{
			if(g_Config.m_UiBrowserPage == PAGE_BROWSER_INTERNET)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_LAN)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_FAVORITES)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_RECENT)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_RECENT);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_DDNET)
			{
				// start a new serverlist request
				Client()->RequestDDNetSrvList();
				ServerBrowser()->Refresh(IServerBrowser::TYPE_DDNET);
			}
			m_DoubleClickIndex = -1;
		}

		ButtonArea.HSplitTop(5.0f, 0, &ButtonArea);
		ButtonArea.HSplitTop(20.0f, &Button, &ButtonArea);
		Button.VMargin(20.0f, &Button);

		{
			static CButtonContainer s_UpdateButton;
			if (g_Config.m_BrAutoRefresh)
				str_format(aBuf, sizeof(aBuf), "%s (%is)", Localize("Update"), (int)max((int64)0, (m_RefreshTimer - time_get()) / time_freq() + (int64)g_Config.m_BrAutoRefresh));
			else
				str_copy(aBuf, Localize("Update"), sizeof(aBuf));
			if(DoButton_Menu(&s_UpdateButton, aBuf, 0, &Button, Localize("Just update the info of the entries")))
			{
				ServerBrowser()->RefreshQuick();
			}
		}

		ButtonArea.HSplitTop(5.0f, 0, &ButtonArea);
		ButtonArea.HSplitTop(20.0f, &Button, &ButtonArea);
		Button.VMargin(20.0f, &Button);

		if(g_Config.m_UiBrowserPage == PAGE_BROWSER_INTERNET)
		{
			CUIRect Right;
			Button.VSplitMid(&Button, &Right);
			static CButtonContainer s_SaveButton;
			if(DoButton_Menu(&s_SaveButton, Localize("Save"), 0, &Button, Localize("Save the serverlist"), CUI::CORNER_L))
				ServerBrowser()->SaveCache();

			static CButtonContainer s_LoadButton;
			if(DoButton_Menu(&s_LoadButton, Localize("Load"), 0, &Right, Localize("Load the saved serverlist"), CUI::CORNER_R))
			{
				/*if(!*/ServerBrowser()->LoadCache()/*)
							Console()->Print(0, "browser", "failed to load cache file", false)*/;
			}
		}
		else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_FAVORITES)
		{
		/*	NETADDR netaddr;
			static CButtonContainer s_Button;
			if(net_addr_from_str(&netaddr, g_Config.m_UiServerAddress) == 0)
				if(DoButton_Menu(&s_Button, Localize("Remove"), 0, &Button, Localize("Remove the selected entry from your favorites"), CUI::CORNER_ALL, vec4(1, 0.5f, 0.5f, 0.5f)))
					ServerBrowser()->RemoveFavorite(netaddr);*/
		}
		else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_RECENT)
		{
		/*	NETADDR netaddr;
			static CButtonContainer s_Button;
			if(net_addr_from_str(&netaddr, g_Config.m_UiServerAddress) == 0)
				if(DoButton_Menu(&s_Button, Localize("Remove"), 0, &Button, Localize("Remove the selected entry from the list of recent servers"), CUI::CORNER_ALL, vec4(1, 0.5f, 0.5f, 0.5f)))
					ServerBrowser()->RemoveRecent(netaddr);*/
			const int64 Now = time_get();
			static int64 Pressed = 0;
			if(Pressed && Now > Pressed+3*time_freq())
				Pressed = 0;

			char aLabel[64];
			if(Pressed)
				str_format(aLabel, sizeof(aLabel), "%s (%i)", Localize("Really?"), (int)((int64)round_to_int(Pressed+3*time_freq()-Now)/time_freq()+1));
			else
				str_copy(aLabel, Localize("Clear Recent"), sizeof(aLabel));
			static CButtonContainer s_Button;
			if(DoButton_Menu(&s_Button, aLabel, 0, &Button, Localize("WARNING: removes all servers from your recents!"), CUI::CORNER_ALL, Pressed ? vec4(1, 0.5f, 0.5f, 0.5f) : vec4(1,1,1,0.5f)))
			{
				if(Pressed)
				{
					ServerBrowser()->ClearRecent();
					Pressed = 0;
				}
				else
					Pressed = Now;
			}
		}

		if(m_EnterPressed)
		{
			m_EnterPressed = false;
			Client()->Connect(g_Config.m_UiServerAddress);
		}

	/*	if(g_Config.m_BrAutoRefresh)
		{
			ButtonArea.HSplitTop(5.0f, 0, &ButtonArea);
			ButtonArea.HSplitTop(20.0f, &Button, &ButtonArea);
			Button.VMargin(20.0f, &Button);

			str_format(aBuf, sizeof(aBuf), Localize("Next refresh: %ds"), ((m_RefreshTimer - time_get()) / time_freq() + g_Config.m_BrAutoRefresh));
			UI()->DoLabelScaled(&Button, aBuf, 12.0f, -1);
			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}*/

		// box at the bottom left - currently only the address bar + connect button
		StatusBox.HSplitTop(20.0f, &StatusBox, 0);

		// connect button
		StatusBox.VSplitRight(10.0f + TextRender()->TextWidth(0, 14.0f, Localize("Connect")), &StatusBox, &Button);
		static CButtonContainer s_ConnectButton;
		if(DoButton_Menu(&s_ConnectButton, Localize("Connect"), 0, &Button, 0, CUI::CORNER_R))
			Client()->Connect(g_Config.m_UiServerAddress); // is that how we do it?

		// address box
		static float s_Offset = 0.0f;
		static CButtonContainer s_ServerAdressEditbox;
		DoEditBox(&s_ServerAdressEditbox, &StatusBox, g_Config.m_UiServerAddress, sizeof(g_Config.m_UiServerAddress), 14.0f, &s_Offset, false, CUI::CORNER_L);
	}
}

void CMenus::RenderBrowser(CUIRect MainView, bool Ingame)
{
	CALLSTACK_ADD();

	CUIRect Box = MainView;
	CUIRect Button;

	int Page = g_Config.m_UiBrowserPage;
	int NewPage = -1;

	if(Ingame)
		RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	Box.HSplitTop(5.0f, &MainView, &MainView);
	Box.HSplitTop(24.0f, &Box, &MainView);
	Box.VMargin(20.0f, &Box);

	if(Ingame)
		Box.VSplitLeft(90.0f+90.0f+130.0f+100.0f+30.0f-100.0f, &Button, &Box);

#define PREPARE_BUTTON(LABEL) \
		const char *pLabelText = LABEL; \
		Box.VSplitLeft(max(90.0f, TextRender()->TextWidth(0, Box.h, pLabelText, str_length(LABEL))), &Button, &Box);

	// internet
	{
		PREPARE_BUTTON(Localize("Internet"))
		static CButtonContainer s_InternetButton;
		if(DoButton_MenuTab(&s_InternetButton, pLabelText, Page == PAGE_BROWSER_INTERNET, &Button, 0))
		{
			if(Page != PAGE_BROWSER_INTERNET)
			{
				if(g_Config.m_BrAutoCache && ServerBrowser()->CacheExists())
					ServerBrowser()->LoadCache();
				else
					ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			}
			NewPage = PAGE_BROWSER_INTERNET;
		}
	}

	// lan
	{
		PREPARE_BUTTON(Localize("LAN"))
		static CButtonContainer s_LanButton;
		if(DoButton_MenuTab(&s_LanButton, pLabelText, Page == PAGE_BROWSER_LAN, &Button, 0) && !ServerBrowser()->IsLocked())
		{
			if(Page != PAGE_BROWSER_LAN)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			NewPage = PAGE_BROWSER_LAN;
		}
	}

	// favorites
	{
		PREPARE_BUTTON(Localize("Favorites"))
		static CButtonContainer s_FavoritesButton;
		if(DoButton_MenuTab(&s_FavoritesButton, pLabelText, Page == PAGE_BROWSER_FAVORITES, &Button, 0) && !ServerBrowser()->IsLocked())
		{
			if(Page != PAGE_BROWSER_FAVORITES)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
			NewPage = PAGE_BROWSER_FAVORITES;
		}
	}

	// recent
	{
		PREPARE_BUTTON(Localize("Recent"))
		static CButtonContainer s_RecentButton;
		if(DoButton_MenuTab(&s_RecentButton, pLabelText, Page == PAGE_BROWSER_RECENT, &Button, 0) && !ServerBrowser()->IsLocked())
		{
			if(Page != PAGE_BROWSER_RECENT)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_RECENT);
			NewPage = PAGE_BROWSER_RECENT;
		}
	}

	// ddnet, if wanted
	if(g_Config.m_BrShowDDNet)
	{
		PREPARE_BUTTON(Localize("DDNet"))
		static CButtonContainer s_DDNetButton;
		if(DoButton_MenuTab(&s_DDNetButton, pLabelText, Page==PAGE_BROWSER_DDNET, &Button, 0) && !ServerBrowser()->IsLocked())
		{
			if (Page != PAGE_BROWSER_DDNET)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_DDNET);
			NewPage = PAGE_BROWSER_DDNET;
		}
	}

#undef PREPARE_BUTTON

	if(!Ingame)
		RenderTools()->DrawUIRect(&Box, ms_ColorTabbarInactive, 0, 0);

	if(NewPage != -1)
	{
		//if(Client()->State() != IClient::STATE_OFFLINE)
			g_Config.m_UiBrowserPage = NewPage;
	}

	RenderServerbrowser(MainView);
	return;
}

void CMenus::ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 2 && ((CMenus *)pUserData)->Client()->State() == IClient::STATE_OFFLINE)
	{
		((CMenus *)pUserData)->FriendlistOnUpdate();
		((CMenus *)pUserData)->Client()->ServerBrowserUpdate();
	}
}

void CMenus::ConchainDDraceNetworkFilterUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	pfnCallback(pResult, pCallbackUserData);
	((CMenus *)pUserData)->Client()->ServerBrowserUpdate();
}

void CMenus::ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() && (g_Config.m_UiBrowserPage == PAGE_BROWSER_FAVORITES || g_Config.m_UiBrowserPage == PAGE_BROWSER_DDNET) && ((CMenus *)pUserData)->Client()->State() == IClient::STATE_OFFLINE)
		((CMenus *)pUserData)->ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
	if(pResult->NumArguments() && g_Config.m_UiBrowserPage == PAGE_BROWSER_RECENT && ((CMenus *)pUserData)->Client()->State() == IClient::STATE_OFFLINE)
		((CMenus *)pUserData)->ServerBrowser()->Refresh(IServerBrowser::TYPE_RECENT);
}
