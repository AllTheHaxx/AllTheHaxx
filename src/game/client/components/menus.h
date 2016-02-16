/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_MENUS_H
#define GAME_CLIENT_COMPONENTS_MENUS_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <engine/demo.h>
#include <engine/friends.h>

#include <game/voting.h>
#include <game/client/component.h>
#include <game/client/ui.h>


// compnent to fetch keypresses, override all other input
class CMenusKeyBinder : public CComponent
{
public:
	bool m_TakeKey;
	bool m_GotKey;
	IInput::CEvent m_Key;
	CMenusKeyBinder();
	virtual bool OnInput(IInput::CEvent Event);
};

class CMenusTooltip : public CComponent
{
	char m_aTooltip[64];

public:
	CMenusTooltip() { m_aTooltip[0] = 0; }
	void SetTooltip(const char *pTooltip) { str_copy(m_aTooltip, pTooltip, sizeof(m_aTooltip)); }

	virtual void OnRender();
};

class CMenus : public CComponent
{
	friend class CGameConsole; // need this for IRC GUI

	static vec4 ms_GuiColor;
	static vec4 ms_ColorTabbarInactiveOutgame;
	static vec4 ms_ColorTabbarActiveOutgame;
	static vec4 ms_ColorTabbarInactiveIngame;
	static vec4 ms_ColorTabbarActiveIngame;
	static vec4 ms_ColorTabbarInactive;
	static vec4 ms_ColorTabbarActive;

	float *ButtonFade(const void *pID, float Seconds, int Checked=0);

	vec4 ButtonColorMul(const void *pID);


	int DoButton_DemoPlayer(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	int DoButton_Sprite(const void *pID, int ImageID, int SpriteID, int Checked, const CUIRect *pRect, int Corners, const char *pTooltip = 0);
	int DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active, const char *pTooltip = 0);
	int DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip = 0, int Corner = CUI::CORNER_ALL, vec4 Color = vec4(1,1,1,0.5f));
	int DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners, vec4 ColorActive = ms_ColorTabbarActive, vec4 ColorInactive = ms_ColorTabbarInactive, const char *pTooltip = 0);

	int DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect, const char *pTooltip = 0, bool Checked = false);
	int DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip = 0);
	int DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip = 0);

	/*static void ui_draw_menu_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_keyselect_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_menu_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_settings_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/

	int DoButton_Icon(int ImageId, int SpriteId, const CUIRect *pRect);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners = CUI::CORNER_T);

	//static void ui_draw_browse_icon(int what, const CUIRect *r);
	//static void ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);

	/*static void ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox_number(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/
	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden=false, int Corners=CUI::CORNER_ALL, const char *pEmptyText = "", int Align = 0, const char *pTooltip = 0);
	//static int ui_do_edit_box(void *id, const CUIRect *rect, char *str, unsigned str_size, float font_size, bool hidden=false);

	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current, const char *pTooltip = 0);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current, const char *pTooltip = 0);
	void DoButton_KeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip = 0);
	int DoKeyReader(void *pID, const CUIRect *pRect, int Key, const char *pTooltip = 0);

	//static int ui_do_key_reader(void *id, const CUIRect *rect, int key);
	void UiDoGetButtons(int Start, int Stop, CUIRect View);

	struct CListboxItem
	{
		int m_Visible;
		int m_Selected;
		CUIRect m_Rect;
		CUIRect m_HitRect;
	};

	void UiDoListboxStart(const void *pID, const CUIRect *pRect, float RowHeight, const char *pTitle, const char *pBottomText, int NumItems,
						int ItemsPerRow, int SelectedIndex, float ScrollValue, int Corners = CUI::CORNER_T);
	CListboxItem UiDoListboxNextItem(const void *pID, bool Selected = false);
	CListboxItem UiDoListboxNextRow();
	int UiDoListboxEnd(float *pScrollValue, bool *pItemActivated);

	//static void demolist_listdir_callback(const char *name, int is_dir, void *user);
	//static void demolist_list_callback(const CUIRect *rect, int index, void *user);

	int m_GamePage;
	int m_Popup;
	int m_ActivePage;
	bool m_MenuActive;
	bool m_UseMouseButtons;
	vec2 m_MousePos;

	int64 m_LastInput;

	// loading
	int m_LoadCurrent;
	int m_LoadTotal;

	//
	char m_aMessageTopic[512];
	char m_aMessageBody[512];
	char m_aMessageButton[512];

	void PopupMessage(const char *pTopic, const char *pBody, const char *pButton);

	// TODO: this is a bit ugly but.. well.. yeah
	enum { MAX_INPUTEVENTS = 32 };
	static IInput::CEvent m_aInputEvents[MAX_INPUTEVENTS];
	static int m_NumInputEvents;

	// some settings
	static float ms_ButtonHeight;
	static float ms_ListheaderHeight;
	static float ms_ListitemAdditionalHeight;
	static float ms_FontmodHeight;

	// for settings
	bool m_NeedRestartSkins;
	bool m_NeedRestartGraphics;
	bool m_NeedRestartSound;
	bool m_NeedRestartUpdate;
	bool m_NeedSendinfo;
	bool m_NeedSendDummyinfo;
	int m_SettingPlayerPage;

	//
	bool m_EscapePressed;
	bool m_EnterPressed;
	bool m_DeletePressed;

	// for map download popup
	int64 m_DownloadLastCheckTime;
	int m_DownloadLastCheckSize;
	float m_DownloadSpeed;

	// for call vote
	int m_CallvoteSelectedOption;
	int m_CallvoteSelectedPlayer;
	char m_aCallvoteReason[VOTE_REASON_LENGTH<<4];
	char m_aFilterString[25];

	// for teh haxx
	int m_SpoofSelectedPlayer;

	// demo
	struct CDemoItem
	{
		char m_aFilename[128];
		char m_aName[128];
		bool m_IsDir;
		int m_StorageType;
		time_t m_Date;

		bool m_InfosLoaded;
		bool m_Valid;
		CDemoHeader m_Info;

		bool operator<(const CDemoItem &Other)
		{
			if (g_Config.m_BrDemoSort)
			{
				if (g_Config.m_BrDemoSortOrder)
				{
					return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														m_Date < Other.m_Date;
				}
				else
				{
					return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														m_Date > Other.m_Date;
				}
			}
			else
			{
				if (g_Config.m_BrDemoSortOrder)
				{
					return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														str_comp_filenames(m_aFilename, Other.m_aFilename) < 0;
				}
				else
				{
					return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														str_comp_filenames(m_aFilename, Other.m_aFilename) > 0;
				}
			}
		}
	};

	//sorted_array<CDemoItem> m_lDemos;
	char m_aCurrentDemoFolder[256];
	char m_aCurrentDemoFile[64];
	int m_DemolistSelectedIndex;
	bool m_DemolistSelectedIsDir;
	int m_DemolistStorageType;

	void DemolistOnUpdate(bool Reset);
	//void DemolistPopulate();
	static int DemolistFetchCallback(const char *pName, time_t Date, int IsDir, int StorageType, void *pUser);

	// friends
	struct CFriendItem
	{
		const CFriendInfo *m_pFriendInfo;
		int m_NumFound;

		bool operator<(const CFriendItem &Other)
		{
			if(m_NumFound && !Other.m_NumFound)
				return true;
			else if(!m_NumFound && Other.m_NumFound)
				return false;
			else
			{
				int Result = str_comp(m_pFriendInfo->m_aName, Other.m_pFriendInfo->m_aName);
				if(Result)
					return Result < 0;
				else
					return str_comp(m_pFriendInfo->m_aClan, Other.m_pFriendInfo->m_aClan) < 0;
			}
		}
	};

	sorted_array<CFriendItem> m_lFriends;
	int m_FriendlistSelectedIndex;

	void FriendlistOnUpdate();

	// found in menus.cpp
	int Render();
	//void render_background();
	//void render_loading(float percent);
	int RenderMenubar(CUIRect r);
	void RenderNews(CUIRect MainView);

	// found in menus_demo.cpp
	void RenderDemoPlayer(CUIRect MainView);
	void RenderDemoList(CUIRect MainView);

	// found in menus_ingame.cpp
	void RenderGame(CUIRect MainView);
	void RenderPlayers(CUIRect MainView);
	void RenderServerInfo(CUIRect MainView);
	void RenderServerControl(CUIRect MainView);
	void RenderServerControlKick(CUIRect MainView, bool FilterSpectators);
	void RenderServerControlServer(CUIRect MainView);

	void RenderSpoofing(CUIRect MainView);
	void RenderSpoofingGeneral(CUIRect MainView);
	void RenderSpoofingPlayers(CUIRect MainView);

	// found in menus_browser.cpp
	int m_SelectedIndex;
	int m_DoubleClickIndex;
	int m_ScrollOffset;
	void RenderServerbrowserServerList(CUIRect View);
	void RenderServerbrowserServerDetail(CUIRect View);
	void RenderServerbrowserFilters(CUIRect View);
	void RenderServerbrowserFriends(CUIRect View);
	void RenderServerbrowser(CUIRect MainView);
	static void ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	// found in menus_settings.cpp
	void RenderLanguageSelection(CUIRect MainView);
	void RenderSettingsGeneral(CUIRect MainView);
	void RenderSettingsPlayer(CUIRect MainView);
	void RenderSettingsDummyPlayer(CUIRect MainView);
	void RenderSettingsTee(CUIRect MainView);
	void RenderSettingsControls(CUIRect MainView);
	void RenderSettingsGraphics(CUIRect MainView);
	void RenderSettingsSound(CUIRect MainView);
	void RenderSettingsHaxx(CUIRect MainView);
	void RenderSettingsIRC(CUIRect MainView);
	void RenderSettings(CUIRect MainView);

	// found in menus_identity.cpp
	void RenderSettingsIdent(CUIRect MainView);

	// found in menus_hotbar.cpp
	bool m_HotbarActive;
	bool m_HotbarWasActive;
	void RenderHotbar(CUIRect MainView);
	void RenderIdents(CUIRect MainView);
	void RenderTrans(CUIRect MainView);
	static void ConKeyShortcut(IConsole::IResult *pResult, void *pUserData);

	// found in menus_irc.cpp
	bool m_IRCActive;
	void RenderIrc(CUIRect MainView);
	bool ToggleIrc();
	static void ConKeyShortcutIRC(IConsole::IResult *pResult, void *pUserData);

	void SetActive(bool Active);
	bool LockInput(IInput::CEvent Event);
public:
	void RenderBackground();

	void UseMouseButtons(bool Use) { m_UseMouseButtons = Use; }

	static CMenusKeyBinder m_Binder;

	CMenus();

	void RenderLoading();
	void RenderUpdating(const char *pCaption, int current=0, int total=0);

	bool IsActive() const { return m_MenuActive || m_HotbarActive || m_IRCActive; }

	virtual void OnInit();
	virtual void OnConsoleInit();

	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnReset();
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent Event);
	virtual bool OnMouseMove(float x, float y);

	enum
	{
		PAGE_NEWS=1,
		PAGE_GAME,
		PAGE_PLAYERS,
		PAGE_SERVER_INFO,
		PAGE_CALLVOTE,
		PAGE_SPOOFING,
		PAGE_INTERNET,
		PAGE_LAN,
		PAGE_FAVORITES,
		PAGE_DDNET,
		PAGE_DEMOS,
		PAGE_SETTINGS,
		PAGE_SYSTEM,
		PAGE_DDRace,
		PAGE_BROWSER,
		PAGE_GHOST,

		PAGE_SETTINGS_LANGUAGE=1,
		PAGE_SETTINGS_GENERAL,
		PAGE_SETTINGS_PLAYER,
		PAGE_SETTINGS_TEE,
		PAGE_SETTINGS_CONTROLS,
		PAGE_SETTINGS_GRAPHICS,
		PAGE_SETTINGS_SOUND,
		PAGE_SETTINGS_HAXX,
		PAGE_SETTINGS_HUD,
		PAGE_SETTINGS_CHAT,
		PAGE_SETTINGS_IDENTITIES,
		PAGE_SETTINGS_DDNET,
		PAGE_SETTINGS_INFO // TODO: implement info page, should contain tabs "about" and "help"
	};

	// DDRace
	int64 _my_rtime; // reconnect time
	int DoButton_CheckBox_DontCare(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	sorted_array<CDemoItem> m_lDemos;
	void DemolistPopulate();
	bool m_Dummy;

	int64 m_RefreshTimer;

	// Ghost
	struct CGhostItem
	{
		char m_aFilename[256];
		char m_aPlayer[MAX_NAME_LENGTH];

		float m_Time;

		bool m_Active;
		int m_ID;

		bool operator<(const CGhostItem &Other) { return m_Time < Other.m_Time; }
		bool operator==(const CGhostItem &Other) { return m_ID == Other.m_ID; }
	};

	sorted_array<CGhostItem> m_lGhosts;
	CGhostItem *m_OwnGhost;
	int m_DDRacePage;
	void GhostlistPopulate();
	void setPopup(int Popup) { m_Popup = Popup; }

	int m_DemoPlayerState;
	char m_aDemoPlayerPopupHint[256];

	enum
	{
		POPUP_NONE=0,
		POPUP_FIRST_LAUNCH,
		POPUP_CONNECTING,
		POPUP_MESSAGE,
		POPUP_DISCONNECTED,
		POPUP_PURE,
		POPUP_LANGUAGE,
		POPUP_COUNTRY,
		POPUP_DELETE_DEMO,
		POPUP_RENAME_DEMO,
		POPUP_REMOVE_FRIEND,
		POPUP_SOUNDERROR,
		POPUP_PASSWORD,
		POPUP_QUIT,
		POPUP_DISCONNECT,

		// demo player states
		DEMOPLAYER_NONE=0,
		DEMOPLAYER_SLICE_SAVE,
	};

private:

	static int GhostlistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);

	// found in menus_ingame.cpp
	void RenderInGameDDRace(CUIRect MainView);
	void RenderGhost(CUIRect MainView);
	void RenderInGameBrowser(CUIRect MainView);

	// found in menus_settings.cpp
	void RenderSettingsDDRace(CUIRect MainView);
	void RenderSettingsHUD(CUIRect MainView);
};
#endif
