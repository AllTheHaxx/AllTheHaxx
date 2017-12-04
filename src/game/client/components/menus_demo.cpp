/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <map>

#include <base/tl/string.h>

#include <base/math.h>

#include <engine/demo.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>

#include <game/client/render.h>
#include <game/client/gameclient.h>
#include <game/localization.h>

#include <game/client/ui.h>

#include <game/generated/client_data.h>

#include "maplayers.h"
#include "menus.h"
#include <game/client/components/console.h>

int CMenus::DoButton_DemoPlayer(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect)
{
	CALLSTACK_ADD();

	RenderTools()->DrawUIRect(pRect, vec4(1,1,1, Checked ? 0.10f : 0.5f)*ButtonColorMul(pBC), CUI::CORNER_ALL, 5.0f);
	UI()->DoLabel(pRect, pText, 14.0f, 0);
	return UI()->DoButtonLogic(pBC->GetID(), pText, Checked, pRect);
}

int CMenus::DoButton_Sprite(CButtonContainer *pBC, int ImageID, int SpriteID, int Checked, const CUIRect *pRect, int Corners, const char *pTooltip)
{
	CALLSTACK_ADD();

	RenderTools()->DrawUIRect(pRect, Checked ? vec4(1.0f, 1.0f, 1.0f, 0.10f) : vec4(1.0f, 1.0f, 1.0f, 0.5f)*ButtonColorMul(pBC), Corners, 5.0f);
	Graphics()->TextureSet(g_pData->m_aImages[ImageID].m_Id);
	Graphics()->QuadsBegin();
	if(!Checked)
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	RenderTools()->SelectSprite(SpriteID);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	if(UI()->MouseInside(pRect) && pTooltip)
		m_pClient->m_pTooltip->SetTooltip(pTooltip);

	return UI()->DoButtonLogic(pBC->GetID(), "", Checked, pRect);
}

bool CMenus::DemoFilterChat(const void *pData, int Size, void *pUser)
{
	bool DoFilterChat = *(bool *)pUser;
	if(!DoFilterChat)
	{
		return false;
	}

	CUnpacker Unpacker;
	Unpacker.Reset(pData, Size);

	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	return !Unpacker.Error() && !Sys && Msg == NETMSGTYPE_SV_CHAT;
}

void CMenus::RenderDemoPlayer(CUIRect MainView)
{
	CALLSTACK_ADD();

	const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();

	const float SeekBarHeight = 15.0f;
	const float ButtonbarHeight = 20.0f;
	const float NameBarHeight = 20.0f;
	const float Margins = 5.0f;
	float TotalHeight;
	static int64 LastSpeedChange = 0;

	// render popups
	if (m_DemoPlayerState == DEMOPLAYER_SLICE_SAVE)
	{
		CUIRect Screen = *UI()->Screen();
		CUIRect Box, Part, Part2;
		Box = Screen;
		Box.VMargin(150.0f/UI()->Scale(), &Box);
#if defined(__ANDROID__)
		Box.HMargin(100.0f/UI()->Scale(), &Box);
#else
		Box.HMargin(150.0f/UI()->Scale(), &Box);
#endif

		// render the box
		RenderTools()->DrawUIRect(&Box, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 15.0f);

		Box.HSplitTop(20.f/UI()->Scale(), &Part, &Box);
		Box.HSplitTop(24.f/UI()->Scale(), &Part, &Box);
		UI()->DoLabelScaled(&Part, Localize("Select a name"), 24.f, 0);
		Box.HSplitTop(20.f/UI()->Scale(), &Part, &Box);
		Box.HSplitTop(24.f/UI()->Scale(), &Part, &Box);
		Part.VMargin(20.f/UI()->Scale(), &Part);
		UI()->DoLabelScaled(&Part, m_aDemoPlayerPopupHint, 24.f, 0);


		CUIRect Label, TextBox, Ok, Abort;

		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Part.VMargin(80.0f, &Part);

		Part.VSplitMid(&Abort, &Ok);

		Ok.VMargin(20.0f, &Ok);
		Abort.VMargin(20.0f, &Abort);

		static int s_RemoveChat = 0;

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
			m_DemoPlayerState = DEMOPLAYER_NONE;

		static CButtonContainer s_ButtonOk;
		if(DoButton_Menu(&s_ButtonOk, Localize("Ok"), 0, &Ok) || m_EnterPressed)
		{
			if (str_comp(m_lDemos[m_DemolistSelectedIndex].m_aFilename, m_aCurrentDemoFile) == 0)
				str_copy(m_aDemoPlayerPopupHint, Localize("Please use a different name"), sizeof(m_aDemoPlayerPopupHint));
			else
			{
				int len = str_length(m_aCurrentDemoFile);
				if(len < 5 || str_comp_nocase(&m_aCurrentDemoFile[len-5], ".demo"))
					str_append(m_aCurrentDemoFile, ".demo", sizeof(m_aCurrentDemoFile));

				char aPath[512];
				str_format(aPath, sizeof(aPath), "%s/%s", m_aCurrentDemoFolder, m_aCurrentDemoFile);

				IOHANDLE DemoFile = Storage()->OpenFile(aPath, IOFLAG_READ, IStorageTW::TYPE_SAVE);
				const char* pStr = Localize("File already exists, do you want to overwrite it?");
				if(DemoFile && str_comp_num(m_aDemoPlayerPopupHint, pStr, sizeof(m_aDemoPlayerPopupHint)) != 0)
				{
					io_close(DemoFile);
					str_copy(m_aDemoPlayerPopupHint, pStr, sizeof(m_aDemoPlayerPopupHint));
				}
				else
				{
					m_DemoPlayerState = DEMOPLAYER_NONE;
					Client()->DemoSlice(aPath, CMenus::DemoFilterChat, &s_RemoveChat);
				}
			}
		}

		Box.HSplitBottom(60.f, &Box, &Part);
		Box.HSplitBottom(60.f, &Box, &Part2);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part2);
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part2);
		Box.HSplitBottom(24.f, &Box, &Part);
#endif

		Part2.VSplitLeft(60.0f, 0, &Label);
		static CButtonContainer s_RemoveChatCheckbox;
		if(DoButton_CheckBox(&s_RemoveChatCheckbox, Localize("Remove chat"), s_RemoveChat, &Label))
		{
			s_RemoveChat ^= 1;
		}

		Part.VSplitLeft(60.0f, 0, &Label);
		Label.VSplitLeft(120.0f, 0, &TextBox);
		TextBox.VSplitLeft(20.0f, 0, &TextBox);
		TextBox.VSplitRight(60.0f, &TextBox, 0);
		UI()->DoLabel(&Label, Localize("New name:"), 18.0f, -1);
		static float Offset = 0.0f;
		static CButtonContainer s_NameEditbox;
		if(DoEditBox(&s_NameEditbox, &TextBox, m_aCurrentDemoFile, sizeof(m_aCurrentDemoFile), 12.0f, &Offset))
			m_aDemoPlayerPopupHint[0] = '\0';
	}

	// handle mousewheel independent of active menu
	for(int i = 0; i < m_NumInputEvents; i++)
	{
		if(m_aInputEvents[i].m_Key == KEY_MOUSE_WHEEL_UP)
		{
			DemoPlayer()->SetSpeedIndex(+1);
			LastSpeedChange = time_get();
		}
		else if(m_aInputEvents[i].m_Key == KEY_MOUSE_WHEEL_DOWN)
		{
			DemoPlayer()->SetSpeedIndex(-1);
			LastSpeedChange = time_get();
		}
	}

	TotalHeight = SeekBarHeight+ButtonbarHeight+NameBarHeight+Margins*3;

	// render speed info
	if (g_Config.m_ClDemoShowSpeed && time_get() - LastSpeedChange < time_freq() * 1)
	{
		CUIRect Screen = *UI()->Screen();

		char aSpeedBuf[256];
		str_format(aSpeedBuf, sizeof(aSpeedBuf), "×%.2f", pInfo->m_Speed);
		TextRender()->Text(0, 120.0f, Screen.y+Screen.h - 120.0f - TotalHeight, 60.0f, aSpeedBuf, -1);
	}

	if(!m_MenuActive)
		return;

	MainView.HSplitBottom(TotalHeight, 0, &MainView);
	MainView.VSplitLeft(50.0f, 0, &MainView);
	MainView.VSplitLeft(450.0f, &MainView, 0);

	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_T, 10.0f);

	MainView.Margin(5.0f, &MainView);

	CUIRect SeekBar, ButtonBar, NameBar;

	int CurrentTick = pInfo->m_CurrentTick - pInfo->m_FirstTick;
	int TotalTicks = pInfo->m_LastTick - pInfo->m_FirstTick;

	MainView.HSplitTop(SeekBarHeight, &SeekBar, &ButtonBar);
	ButtonBar.HSplitTop(Margins, 0, &ButtonBar);
	ButtonBar.HSplitBottom(NameBarHeight, &ButtonBar, &NameBar);
	NameBar.HSplitTop(4.0f, 0, &NameBar);

	// do seekbar
	{
		static int s_SeekBarID = 0;
		void *id = &s_SeekBarID;
		char aBuffer[128];

		// draw seek bar
		RenderTools()->DrawUIRect(&SeekBar, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 5.0f);

		// draw filled bar
		float Amount = CurrentTick/(float)TotalTicks;
		CUIRect FilledBar = SeekBar;
		FilledBar.w = 10.0f + (FilledBar.w-10.0f)*Amount;
		RenderTools()->DrawUIRect(&FilledBar, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f);

		// draw markers
		if(pInfo->m_NumTimelineMarkers < TotalTicks / 20)
		{
			for(int i = 0; i < pInfo->m_NumTimelineMarkers; i++)
			{
				float Ratio = (pInfo->m_aTimelineMarkers[i] - pInfo->m_FirstTick) / (float)TotalTicks;
				Graphics()->TextureSet(-1);
				Graphics()->QuadsBegin();
				Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
				IGraphics::CQuadItem QuadItem(SeekBar.x + (SeekBar.w - 10.0f) * Ratio, SeekBar.y, UI()->PixelSize(), SeekBar.h);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}
		}
		else
		{
			TextRender()->TextColor(1,0,0,1);
			UI()->DoLabelScaled(&SeekBar, Localize("removed markers due to broken demo"), 9.0f, -1);
			TextRender()->TextColor(1,1,1,1);
		}

		// draw slice markers
		// begin
		if (g_Config.m_ClDemoSliceBegin != -1)
		{
			float Ratio = (g_Config.m_ClDemoSliceBegin-pInfo->m_FirstTick) / (float)TotalTicks;
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(10.0f + SeekBar.x + (SeekBar.w-10.0f)*Ratio, SeekBar.y, UI()->PixelSize(), SeekBar.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// end
		if (g_Config.m_ClDemoSliceEnd != -1)
		{
			float Ratio = (g_Config.m_ClDemoSliceEnd-pInfo->m_FirstTick) / (float)TotalTicks;
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
			IGraphics::CQuadItem QuadItem(10.0f + SeekBar.x + (SeekBar.w-10.0f)*Ratio, SeekBar.y, UI()->PixelSize(), SeekBar.h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// draw time
		str_format(aBuffer, sizeof(aBuffer), "%d:%02d / %d:%02d",
			CurrentTick/SERVER_TICK_SPEED/60, (CurrentTick/SERVER_TICK_SPEED)%60,
			TotalTicks/SERVER_TICK_SPEED/60, (TotalTicks/SERVER_TICK_SPEED)%60);
		UI()->DoLabel(&SeekBar, aBuffer, SeekBar.h*0.70f, 0);

		// do the logic
		int Inside = UI()->MouseInside(&SeekBar);

		if(UI()->ActiveItem() == id)
		{
			if(!UI()->MouseButton(0))
				UI()->SetActiveItem(0);
			else
			{
				static float PrevAmount = 0.0f;
				float Amount = (UI()->MouseX()-SeekBar.x)/SeekBar.w;

				if(KeyMods(KEYMOD_SHIFT))
				{
					Amount = PrevAmount + (Amount-PrevAmount) * 0.05f;

					if(Amount > 0.0f && Amount < 1.0f && absolute(PrevAmount-Amount) >= 0.0001f)
					{
						//PrevAmount = Amount;
						m_pClient->OnReset();
						m_pClient->m_SuppressEvents = true;
						DemoPlayer()->SetPos(Amount);
						m_pClient->m_SuppressEvents = false;
						m_pClient->m_pMapLayersBackGround->EnvelopeUpdate();
						m_pClient->m_pMapLayersForeGround->EnvelopeUpdate();
					}
				}
				else
				{
					if(Amount > 0.0f && Amount < 1.0f && absolute(PrevAmount-Amount) >= 0.001f)
					{
						PrevAmount = Amount;
						m_pClient->OnReset();
						m_pClient->m_SuppressEvents = true;
						DemoPlayer()->SetPos(Amount);
						m_pClient->m_SuppressEvents = false;
						m_pClient->m_pMapLayersBackGround->EnvelopeUpdate();
						m_pClient->m_pMapLayersForeGround->EnvelopeUpdate();
					}
				}
			}
		}
		else if(UI()->HotItem() == id)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(id);
		}

		if(Inside)
			UI()->SetHotItem(id);
	}

	if(CurrentTick == TotalTicks)
	{
		m_pClient->OnReset();
		DemoPlayer()->Pause();
		DemoPlayer()->SetPos(0);
	}

	bool IncreaseDemoSpeed = false, DecreaseDemoSpeed = false;

	// do buttons
	CUIRect Button;

	// combined play and pause button
	ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
	static CButtonContainer s_PlayPauseButton;
	if(!pInfo->m_Paused)
	{
		if(DoButton_Sprite(&s_PlayPauseButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_PAUSE, false, &Button, CUI::CORNER_ALL))
			DemoPlayer()->Pause();
	}
	else
	{
		if(DoButton_Sprite(&s_PlayPauseButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_PLAY, false, &Button, CUI::CORNER_ALL))
			DemoPlayer()->Unpause();
	}

	// stop button

	ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
	ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
	static CButtonContainer s_ResetButton;
	if(DoButton_Sprite(&s_ResetButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_STOP, false, &Button, CUI::CORNER_ALL))
	{
		m_pClient->OnReset();
		DemoPlayer()->Pause();
		DemoPlayer()->SetPos(0);
	}

	// slowdown
	ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
	ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
	static CButtonContainer s_SlowDownButton;
	if(DoButton_Sprite(&s_SlowDownButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_SLOWER, 0, &Button, CUI::CORNER_ALL))
		DecreaseDemoSpeed = true;

	// fastforward
	ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
	ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
	static CButtonContainer s_FastForwardButton;
	if(DoButton_Sprite(&s_FastForwardButton, IMAGE_DEMOBUTTONS, SPRITE_DEMOBUTTON_FASTER, 0, &Button, CUI::CORNER_ALL))
		IncreaseDemoSpeed = true;

	// speed meter
	ButtonBar.VSplitLeft(Margins*3, 0, &ButtonBar);
	char aBuffer[64];
	str_format(aBuffer, sizeof(aBuffer), "×%g", pInfo->m_Speed);
	UI()->DoLabel(&ButtonBar, aBuffer, Button.h*0.7f, -1);

	// slice begin button
	ButtonBar.VSplitLeft(Margins*10, 0, &ButtonBar);
	ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
	static CButtonContainer s_SliceBeginButton;
	if(DoButton_Sprite(&s_SliceBeginButton, IMAGE_DEMOBUTTONS2, SPRITE_DEMOBUTTON_SLICE_BEGIN, 0, &Button, CUI::CORNER_ALL))
		Client()->DemoSliceBegin();

	// slice end button
	ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
	ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
	static CButtonContainer s_SliceEndButton;
	if(DoButton_Sprite(&s_SliceEndButton, IMAGE_DEMOBUTTONS2, SPRITE_DEMOBUTTON_SLICE_END, 0, &Button, CUI::CORNER_ALL))
		Client()->DemoSliceEnd();

	// slice save button
	ButtonBar.VSplitLeft(Margins, 0, &ButtonBar);
	ButtonBar.VSplitLeft(ButtonbarHeight, &Button, &ButtonBar);
	static CButtonContainer s_SliceSaveButton;
	if(DoButton_Sprite(&s_SliceSaveButton, IMAGE_FILEICONS, SPRITE_FILE_DEMO2, 0, &Button, CUI::CORNER_ALL))
	{
		str_copy(m_aCurrentDemoFile, m_lDemos[m_DemolistSelectedIndex].m_aFilename, sizeof(m_aCurrentDemoFile));
		m_aDemoPlayerPopupHint[0] = '\0';
		m_DemoPlayerState = DEMOPLAYER_SLICE_SAVE;
	}

	// close button
	ButtonBar.VSplitRight(ButtonbarHeight*3, &ButtonBar, &Button);
	static CButtonContainer s_ExitButton;
	if(DoButton_DemoPlayer(&s_ExitButton, Localize("Close"), 0, &Button))
	{
		Client()->Disconnect();
		DemolistPopulate();
		DemolistOnUpdate(false);
	}

	// demo name
	char aDemoName[64] = {0};
	DemoPlayer()->GetDemoName(aDemoName, sizeof(aDemoName));
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), Localize("Demofile: %s"), aDemoName);
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, NameBar.x, NameBar.y, Button.h*0.5f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
	Cursor.m_LineWidth = MainView.w;
	TextRender()->TextEx(&Cursor, aBuf, -1);

	if(IncreaseDemoSpeed)
	{
		DemoPlayer()->SetSpeedIndex(+1);
		LastSpeedChange = time_get();
	}
	else if(DecreaseDemoSpeed)
	{
		DemoPlayer()->SetSpeedIndex(-1);
		LastSpeedChange = time_get();
	}
}


int CMenus::DemolistFetchCallback(const char *pName, time_t Date, int IsDir, int StorageType, void *pUser)
{
	CALLSTACK_ADD();

	CMenus *pSelf = (CMenus *)pUser;
	int Length = str_length(pName);
	if((pName[0] == '.' && (pName[1] == 0 ||
		(pName[1] == '.' && pName[2] == 0 && !str_comp(pSelf->m_aCurrentDemoFolder, "demos")))) ||
		(!IsDir && (Length < 5 || str_comp(pName+Length-5, ".demo"))))
		return 0;

	CDemoItem Item;
	str_copy(Item.m_aFilename, pName, sizeof(Item.m_aFilename));
	if(IsDir)
	{
		str_format(Item.m_aName, sizeof(Item.m_aName), "%s/", pName);
		Item.m_Valid = false;
	}
	else
	{
		str_copy(Item.m_aName, pName, min(static_cast<int>(sizeof(Item.m_aName)), Length-4));
		Item.m_InfosLoaded = false;
		Item.m_Date = Date;
	}
	Item.m_IsDir = IsDir != 0;
	Item.m_StorageType = StorageType;
	pSelf->m_lDemos.add_unsorted(Item);

	return 0;
}

void CMenus::DemolistPopulate()
{
	CALLSTACK_ADD();

	m_lDemos.clear();
	if(!str_comp(m_aCurrentDemoFolder, "demos"))
		m_DemolistStorageType = IStorageTW::TYPE_ALL;
	Storage()->ListDirectoryInfo(m_DemolistStorageType, m_aCurrentDemoFolder, DemolistFetchCallback, this);
	m_lDemos.sort_range();
}

void CMenus::DemolistOnUpdate(bool Reset)
{
	CALLSTACK_ADD();

	if (Reset)
		g_Config.m_UiDemoSelected[0] = '\0';
	else
	{
		bool Found = false;
		int SelectedIndex = -1;
		// search for selected index
		for(sorted_array<CDemoItem>::range r = m_lDemos.all(); !r.empty(); r.pop_front())
		{
			SelectedIndex++;

			if (str_comp(g_Config.m_UiDemoSelected, r.front().m_aName) == 0)
			{
				Found = true;
				break;
			}
		}

		if (Found)
			m_DemolistSelectedIndex = SelectedIndex;
	}

	m_DemolistSelectedIndex = Reset ? m_lDemos.size() > 0 ? 0 : -1 :
										m_DemolistSelectedIndex >= m_lDemos.size() ? m_lDemos.size()-1 : m_DemolistSelectedIndex;
	m_DemolistSelectedIsDir = m_DemolistSelectedIndex < 0 ? false : m_lDemos[m_DemolistSelectedIndex].m_IsDir;
}

void CMenus::RenderDemoList(CUIRect MainView)
{
	CALLSTACK_ADD();

	static int s_Inited = 0;
	if(!s_Inited)
	{
		DemolistPopulate();
		DemolistOnUpdate(true);
		s_Inited = 1;
	}

	char aFooterLabel[128] = {0};
	if(m_DemolistSelectedIndex >= 0)
	{
		CDemoItem *Item = &m_lDemos[m_DemolistSelectedIndex];
		if(str_comp(Item->m_aFilename, "..") == 0)
			str_copy(aFooterLabel, Localize("Parent Folder"), sizeof(aFooterLabel));
		else if(m_DemolistSelectedIsDir)
			str_copy(aFooterLabel, Localize("Folder"), sizeof(aFooterLabel));
		else
		{
			if(!Item->m_InfosLoaded)
			{
				char aBuffer[512];
				str_format(aBuffer, sizeof(aBuffer), "%s/%s", m_aCurrentDemoFolder, Item->m_aFilename);
				Item->m_Valid = DemoPlayer()->GetDemoInfo(Storage(), aBuffer, Item->m_StorageType, &Item->m_Info);
				Item->m_InfosLoaded = true;
			}
			if(!Item->m_Valid)
				str_copy(aFooterLabel, Localize("Invalid Demo"), sizeof(aFooterLabel));
			else
				str_copy(aFooterLabel, Localize("Demo details"), sizeof(aFooterLabel));
		}
	}

	// render background
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);
	MainView.Margin(10.0f, &MainView);

	CUIRect ButtonBar, RefreshRect, PlayRect, DeleteRect, RenameRect, ListBox;
	MainView.HSplitBottom(ms_ButtonHeight+5.0f, &MainView, &ButtonBar);
	ButtonBar.HSplitTop(5.0f, 0, &ButtonBar);
	ButtonBar.VSplitRight(130.0f, &ButtonBar, &PlayRect);
	ButtonBar.VSplitLeft(130.0f, &RefreshRect, &ButtonBar);
	ButtonBar.VSplitLeft(10.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(120.0f, &DeleteRect, &ButtonBar);
	ButtonBar.VSplitLeft(10.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(120.0f, &RenameRect, &ButtonBar);
	MainView.HSplitBottom(140.0f, &ListBox, &MainView);

	// render demo info
	MainView.VMargin(5.0f, &MainView);
	MainView.HSplitBottom(5.0f, &MainView, 0);
	RenderTools()->DrawUIRect(&MainView, vec4(0,0,0,0.15f), CUI::CORNER_B, 4.0f);
	if(!m_DemolistSelectedIsDir && m_DemolistSelectedIndex >= 0 && m_lDemos[m_DemolistSelectedIndex].m_Valid)
	{
		CUIRect Left, Right, Labels;
		MainView.Margin(20.0f, &MainView);
		MainView.VSplitMid(&Labels, &MainView);

		// left side
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(150.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Created:"), 14.0f, -1);

		char aTimestamp[256];
		str_timestamp_ex(m_lDemos[m_DemolistSelectedIndex].m_Date, aTimestamp, sizeof(aTimestamp), "%Y-%m-%d %H:%M:%S");

		UI()->DoLabelScaled(&Right, aTimestamp, 14.0f, -1);
		Labels.HSplitTop(5.0f, 0, &Labels);
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(150.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Type:"), 14.0f, -1);
		UI()->DoLabelScaled(&Right, m_lDemos[m_DemolistSelectedIndex].m_Info.m_aType, 14.0f, -1);
		Labels.HSplitTop(5.0f, 0, &Labels);
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(150.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Length:"), 14.0f, -1);
		int Length = ((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[0]<<24)&0xFF000000) | ((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[1]<<16)&0xFF0000) |
					((m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[2]<<8)&0xFF00) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aLength[3]&0xFF);
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "%d:%02d", Length/60, Length%60);
		UI()->DoLabelScaled(&Right, aBuf, 14.0f, -1);
		Labels.HSplitTop(5.0f, 0, &Labels);
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(150.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Version:"), 14.0f, -1);
		str_format(aBuf, sizeof(aBuf), "%d", m_lDemos[m_DemolistSelectedIndex].m_Info.m_Version);
		UI()->DoLabelScaled(&Right, aBuf, 14.0f, -1);

		// right side
		Labels = MainView;
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(150.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Map:"), 14.0f, -1);
		UI()->DoLabelScaled(&Right, m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapName, 14.0f, -1);
		Labels.HSplitTop(5.0f, 0, &Labels);
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(20.0f, 0, &Left);
		Left.VSplitLeft(130.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Size:"), 14.0f, -1);
		unsigned Size = (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[0]<<24) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[1]<<16) |
					(m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[2]<<8) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapSize[3]);
		if(Size > 1024*1024)
			str_format(aBuf, sizeof(aBuf), Localize("%.2f MiB"), float(Size)/(1024*1024));
		else
			str_format(aBuf, sizeof(aBuf), Localize("%.2f KiB"), float(Size)/1024);
		UI()->DoLabelScaled(&Right, aBuf, 14.0f, -1);
		Labels.HSplitTop(5.0f, 0, &Labels);
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(20.0f, 0, &Left);
		Left.VSplitLeft(130.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Crc:"), 14.0f, -1);
		unsigned Crc = (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[0]<<24) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[1]<<16) |
					(m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[2]<<8) | (m_lDemos[m_DemolistSelectedIndex].m_Info.m_aMapCrc[3]);
		str_format(aBuf, sizeof(aBuf), "%08x", Crc);
		UI()->DoLabelScaled(&Right, aBuf, 14.0f, -1);
		Labels.HSplitTop(5.0f, 0, &Labels);
		Labels.HSplitTop(20.0f, &Left, &Labels);
		Left.VSplitLeft(150.0f, &Left, &Right);
		UI()->DoLabelScaled(&Left, Localize("Netversion:"), 14.0f, -1);
		UI()->DoLabelScaled(&Right, m_lDemos[m_DemolistSelectedIndex].m_Info.m_aNetversion, 14.0f, -1);
	}


	// demo list

	CUIRect Headers;

	ListBox.HSplitTop(ms_ListheaderHeight, &Headers, &ListBox);

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
		COL_ICON=0,
		COL_DEMONAME,
		COL_DATE,

		SORT_DEMONAME=0,
		SORT_DATE,
	};

	static CColumn s_aCols[] = {
		{COL_ICON,     -1,            " ",    -1,  14.0f, 0, 0, 0},
		{COL_DEMONAME, SORT_DEMONAME, "Demo",  0,   0.0f, 0, 0, 0},
		{COL_DATE,     SORT_DATE,     "Date",  1, 300.0f, 0, 0, 0},
	};

	RenderTools()->DrawUIRect(&Headers, vec4(0.0f,0,0,0.15f), 0, 0);

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
		if(DoButton_GridHeader(&Container, s_aCols[i].m_Caption, g_Config.m_BrDemoSort == s_aCols[i].m_Sort, &s_aCols[i].m_Rect))
		{
			if(s_aCols[i].m_Sort != -1)
			{
				if(g_Config.m_BrDemoSort == s_aCols[i].m_Sort)
					g_Config.m_BrDemoSortOrder ^= 1;
				else
					g_Config.m_BrDemoSortOrder = 0;
				g_Config.m_BrDemoSort = s_aCols[i].m_Sort;
			}

			DemolistPopulate();
			DemolistOnUpdate(false);
		}
	}

	// scrollbar
	CUIRect Scroll;
#if defined(__ANDROID__)
	ListBox.VSplitRight(50, &ListBox, &Scroll);
#else
	ListBox.VSplitRight(15, &ListBox, &Scroll);
#endif

	int Num = (int)(ListBox.h/s_aCols[0].m_Rect.h) + 1;
	static CButtonContainer s_ScrollBar;
	static float s_ScrollValue = 0;

	Scroll.HMargin(5.0f, &Scroll);
	s_ScrollValue = DoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

	int ScrollNum = m_lDemos.size()-Num+1;
	if(ScrollNum > 0)
	{
		if(m_ScrollOffset)
		{
			s_ScrollValue = (float)(m_ScrollOffset)/ScrollNum;
			m_ScrollOffset = 0;
		}
		if(KeyEvent(KEY_MOUSE_WHEEL_UP) && UI()->MouseInside(&ListBox))
			s_ScrollValue -= 3.0f/ScrollNum;
		else if(KeyEvent(KEY_MOUSE_WHEEL_DOWN) && UI()->MouseInside(&ListBox))
			s_ScrollValue += 3.0f/ScrollNum;
	}
	else
		ScrollNum = 0;

	if(m_DemolistSelectedIndex > -1)
	{
		for(int i = 0; i < m_NumInputEvents; i++)
		{
			int NewIndex = -1;
			if(m_aInputEvents[i].m_Flags&IInput::FLAG_PRESS)
			{
				if(m_aInputEvents[i].m_Key == KEY_DOWN) NewIndex = m_DemolistSelectedIndex + 1;
				else if(m_aInputEvents[i].m_Key == KEY_UP) NewIndex = m_DemolistSelectedIndex - 1;
				else if(m_aInputEvents[i].m_Key == KEY_PAGEUP) NewIndex = max(m_DemolistSelectedIndex - 20, 0);
				else if(m_aInputEvents[i].m_Key == KEY_PAGEDOWN) NewIndex = min(m_DemolistSelectedIndex + 20, m_lDemos.size() - 1);
				else if(m_aInputEvents[i].m_Key == KEY_HOME) NewIndex = 0;
				else if(m_aInputEvents[i].m_Key == KEY_END) NewIndex = m_lDemos.size() - 1;
			}
			if(NewIndex > -1 && NewIndex < m_lDemos.size())
			{
				//scroll
				float IndexY = ListBox.y - s_ScrollValue*ScrollNum*s_aCols[0].m_Rect.h + NewIndex*s_aCols[0].m_Rect.h;
				int Scroll = ListBox.y > IndexY ? -1 : ListBox.y+ListBox.h < IndexY+s_aCols[0].m_Rect.h ? 1 : 0;
				if(Scroll)
				{
					if(Scroll < 0)
					{
						int NumScrolls = (ListBox.y-IndexY+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
						s_ScrollValue -= (1.0f/ScrollNum)*NumScrolls;
					}
					else
					{
						int NumScrolls = (IndexY+s_aCols[0].m_Rect.h-(ListBox.y+ListBox.h)+s_aCols[0].m_Rect.h-1.0f)/s_aCols[0].m_Rect.h;
						s_ScrollValue += (1.0f/ScrollNum)*NumScrolls;
					}
				}

				m_DemolistSelectedIndex = NewIndex;

				str_copy(g_Config.m_UiDemoSelected, m_lDemos[NewIndex].m_aName, sizeof(g_Config.m_UiDemoSelected));
				DemolistOnUpdate(false);
			}
		}
	}

	if(s_ScrollValue < 0) s_ScrollValue = 0;
	if(s_ScrollValue > 1) s_ScrollValue = 1;

	// set clipping
	UI()->ClipEnable(&ListBox);

	CUIRect OriginalView = ListBox;
	ListBox.y -= s_ScrollValue*ScrollNum*s_aCols[0].m_Rect.h;

	int NewSelected = -1;
#if defined(__ANDROID__)
	int DoubleClicked = 0;
#endif
	int ItemIndex = -1;

	for(sorted_array<CDemoItem>::range r = m_lDemos.all(); !r.empty(); r.pop_front())
	{
		ItemIndex++;

		CUIRect Row;
		CUIRect SelectHitBox;

		ListBox.HSplitTop(ms_ListheaderHeight, &Row, &ListBox);
		SelectHitBox = Row;

		int Selected = ItemIndex == m_DemolistSelectedIndex;

		// make sure that only those in view can be selected
		if(Row.y+Row.h > OriginalView.y && Row.y < OriginalView.y+OriginalView.h)
		{
			if(Selected)
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

			if(UI()->DoButtonLogic(r.front().m_aName /* TODO: */, "", Selected, &SelectHitBox))
			{
				NewSelected = ItemIndex;
				str_copy(g_Config.m_UiDemoSelected, r.front().m_aName, sizeof(g_Config.m_UiDemoSelected));
				DemolistOnUpdate(false);
#if defined(__ANDROID__)
				if(NewSelected == m_DoubleClickIndex)
					DoubleClicked = 1;
#endif

				m_DoubleClickIndex = NewSelected;
			}
		}
		else
		{
			// don't render invisible items
			continue;
		}

		for(int c = 0; c < NumCols; c++)
		{
			CUIRect Button;
			Button.x = s_aCols[c].m_Rect.x;
			Button.y = Row.y;
			Button.h = Row.h;
			Button.w = s_aCols[c].m_Rect.w;

			int ID = s_aCols[c].m_ID;

			if (ID == COL_ICON)
			{
				DoButton_Icon(IMAGE_FILEICONS, r.front().m_IsDir?SPRITE_FILE_FOLDER:SPRITE_FILE_DEMO1, &Button);
			}
			else if(ID == COL_DEMONAME)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				TextRender()->TextEx(&Cursor, r.front().m_aName, -1);

			}
			else if (ID == COL_DATE && !r.front().m_IsDir)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Button.x, Button.y, 12.0f * UI()->Scale(), TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = Button.w;

				char aBuf[256];
				str_timestamp_ex(r.front().m_Date, aBuf, sizeof(aBuf), "%Y-%m-%d %H:%M:%S");
				TextRender()->TextEx(&Cursor, aBuf, -1);
			}
		}
	}

	UI()->ClipDisable();


	bool Activated = false;

#if defined(__ANDROID__)
	if (m_EnterPressed || (DoubleClicked && UI()->HotItem() == m_lDemos[m_DemolistSelectedIndex].m_aName))
#else
	if (m_EnterPressed || (Input()->MouseDoubleClick() && UI()->HotItem() == m_lDemos[m_DemolistSelectedIndex].m_aName))
#endif
	{
		Input()->MouseDoubleClickReset();
		UI()->SetActiveItem(0);
		Activated = true;
	}

	static CButtonContainer s_RefreshButton;
	if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &RefreshRect) || Input()->KeyPress(KEY_F5) || (Input()->KeyPress(KEY_R) && (Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL))))
	{
		DemolistPopulate();
		DemolistOnUpdate(false);
	}

	static CButtonContainer s_PlayButton;
	if(DoButton_Menu(&s_PlayButton, m_DemolistSelectedIsDir?Localize("Open"):Localize("Play"), 0, &PlayRect) || Activated)
	{
		if(m_DemolistSelectedIndex >= 0)
		{
			if(m_DemolistSelectedIsDir)	// folder
			{
				if(str_comp(m_lDemos[m_DemolistSelectedIndex].m_aFilename, "..") == 0)	// parent folder
					fs_parent_dir(m_aCurrentDemoFolder);
				else	// sub folder
				{
					char aTemp[256];
					str_copy(aTemp, m_aCurrentDemoFolder, sizeof(aTemp));
					str_format(m_aCurrentDemoFolder, sizeof(m_aCurrentDemoFolder), "%s/%s", aTemp, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
					m_DemolistStorageType = m_lDemos[m_DemolistSelectedIndex].m_StorageType;
				}
				DemolistPopulate();
				DemolistOnUpdate(true);
			}
			else // file
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
				const char *pError = Client()->DemoPlayer_Play(aBuf, m_lDemos[m_DemolistSelectedIndex].m_StorageType);
				if(pError)
					PopupMessage(Localize("Error"), str_comp(pError, "error loading demo") ? pError : Localize("Error loading demo"), Localize("Ok"));
				else
				{
					UI()->SetActiveItem(0);
					return;
				}
			}
		}
	}

	if(!m_DemolistSelectedIsDir)
	{
		static CButtonContainer s_DeleteButton;
		if(DoButton_Menu(&s_DeleteButton, Localize("Delete"), 0, &DeleteRect) || m_DeletePressed)
		{
			if(m_DemolistSelectedIndex >= 0)
			{
				UI()->SetActiveItem(0);
				m_Popup = POPUP_DELETE_DEMO;
				return;
			}
		}

		static CButtonContainer s_RenameButton;
		if(DoButton_Menu(&s_RenameButton, Localize("Rename"), 0, &RenameRect))
		{
			if(m_DemolistSelectedIndex >= 0)
			{
				UI()->SetActiveItem(0);
				m_Popup = POPUP_RENAME_DEMO;
				str_copy(m_aCurrentDemoFile, m_lDemos[m_DemolistSelectedIndex].m_aFilename, sizeof(m_aCurrentDemoFile));
				return;
			}
		}
	}
}
