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

#include "../menus.h"
#include "../binds.h"
#include "../camera.h"
#include "../countryflags.h"
#include "game/client/components/fontmgr.h"
#include "../identity.h"
#include "../skins.h"
#include "../console.h"



/*	This is for scripts/update_localization.py to work, don't remove!
	Localize("Move left");Localize("Move right");Localize("Jump");Localize("Fire");Localize("Hook");Localize("Hammer");
	Localize("Pistol");Localize("Shotgun");Localize("Grenade");Localize("Rifle");Localize("Next weapon");Localize("Prev. weapon");
	Localize("Vote yes");Localize("Vote no");Localize("Chat");Localize("Team chat");Localize("Show chat");Localize("Emoticon");
	Localize("Spectator mode");Localize("Spectate next");Localize("Spectate previous");Localize("Console");Localize("Remote console");Localize("Screenshot");Localize("Scoreboard");Localize("Respawn");
	Localize("Hammerfly Dummy");Localize("Hidden Chat");Localize("Crypted Chat");Localize("Hookfly Dummy");Localize("Toggle X-Ray");Localize("Zoom in");
	Localize("Zoom out");Localize("Toggle IRC");Localize("Toggle Lua Console");Localize("Toggle Hotbar");Localize("Unlock Mouse");
*/



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
		int State = Updater()->State();

		// update button
		if(NeedUpdate && State <= IUpdater::STATE_CLEAN)
		{
			str_format(aBuf, sizeof(aBuf), Localize("New Client Version '%s' is available!"), Client()->LatestVersion());
			Label.VSplitLeft(TextRender()->TextWidth(0, 14.0f, aBuf, -1) + 10.0f, &Label, &Button);
			Button.VSplitLeft(TextRender()->TextWidth(0, Button.h*ms_FontmodHeight, Localize("Update now"), -1), &Button, 0);
			static CButtonContainer s_ButtonUpdate;
			if(DoButton_Menu(&s_ButtonUpdate, Localize("Update now"), 0, &Button))
				Updater()->PerformUpdate();
		}
		else if(State >= IUpdater::STATE_GETTING_MANIFEST && State <= IUpdater::STATE_DOWNLOADING)
			str_copyb(aBuf, Localize("Downloading update..."));
		else if(State == IUpdater::STATE_MOVE_FILES)
			str_copyb(aBuf, Localize("Installing update..."));
		else if(State == IUpdater::STATE_NEED_RESTART){
			str_copyb(aBuf, Localize("AllTheHaxx updated!"));
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
