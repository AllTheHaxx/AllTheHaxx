#include <engine/textrender.h>
#include <engine/config.h>
#include <engine/keys.h>
#include <engine/graphics.h>
#include <game/client/gameclient.h>
#include <game/version.h>

#include "console.h"
#include "menus.h"

void CMenus::RenderManual(CUIRect MainView)
{
	//static int s_SettingsPage = 0;

	// render background
	CUIRect Temp, TabBar, RestartWarning;
	MainView.VSplitRight(150.0f, &MainView, &TabBar);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B|CUI::CORNER_TL, 10.0f);
	MainView.Margin(10.0f, &MainView);
	MainView.HSplitBottom(15.0f, &MainView, &RestartWarning);
	TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_R, 10.0f);

	MainView.HSplitTop(10.0f, 0, &MainView);

	CUIRect Button;

	const char *aTabs[] = {
			Localize("About"),
			Localize("Credits"),
			Localize("General"),
	};

	const int NumTabs = (int)(sizeof(aTabs)/sizeof(*aTabs));
	static float FadeVals[NumTabs] = {0.0f};

	for(int i = 0; i < NumTabs; i++)
	{
		TabBar.HSplitTop(i == PAGE_MANUAL_GENERAL ? 24 : 10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(UI()->MouseInside(&Button))
			smooth_set(&FadeVals[i], 5.0f, 10.0f, Client()->RenderFrameTime());
		else
			smooth_set(&FadeVals[i], 0.0f, 10.0f, Client()->RenderFrameTime());
		Button.w += FadeVals[i];
		CPointerContainer Container(&aTabs[i]);
		if(DoButton_MenuTab(&Container, aTabs[i], g_Config.m_UiManualPage == i, &Button, CUI::CORNER_R, ms_ColorTabbarActive))
		{
			g_Config.m_UiManualPage = i;
		}
	}

	MainView.Margin(10.0f, &MainView);

	if(g_Config.m_UiManualPage == PAGE_MANUAL_ABOUT)
		RenderAbout(MainView);
	else if(g_Config.m_UiManualPage == PAGE_MANUAL_CREDITS)
		RenderCredits(MainView);
	else if(g_Config.m_UiManualPage == PAGE_MANUAL_GENERAL)
		RenderManual_General(MainView);
	else
		g_Config.m_UiManualPage = PAGE_MANUAL_ABOUT;

	if(m_NeedRestartUpdate)
	{
		TextRender()->TextColor(1.0f, 0.4f, 0.4f, 1.0f);
		UI()->DoLabelScaled(&RestartWarning, Localize("AllTheHaxx needs to be restarted to complete the update!"), 14.0f, -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}
	else if(m_NeedRestartGraphics || m_NeedRestartSound || m_NeedRestartDDNet)
	{
		static CButtonContainer s_ButtonRestart;
		if(DoButton_Menu(&s_ButtonRestart, Localize("You must restart the game for all settings to take effect."), 0, &RestartWarning, 0, CUI::CORNER_ALL, vec4(0.75f, 0.18f, 0.18f, 0.83f)))
			Client()->Restart();
	}
}

#define PASTE_SCROLLCODE() \
		static float s_ScrollOffset = 0.0f; \
		if(s_TotalHeight+MainView.h > MainView.h) \
		{ \
			static float s_WantedScrollOffset = 0.0f; \
			CUIRect Scrollbar; \
			MainView.VSplitRight(15.0f, &MainView, &Scrollbar); \
			Scrollbar.HMargin(5.0f, &Scrollbar); \
			static CButtonContainer s_Scrollbar; \
			s_WantedScrollOffset = DoScrollbarV(&s_Scrollbar, &Scrollbar, s_WantedScrollOffset); \
			\
			if(m_pClient->m_pGameConsole->IsClosed()) \
			{ \
				if(Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN)) \
					s_WantedScrollOffset += 0.1f; \
				if(Input()->KeyPress(KEY_MOUSE_WHEEL_UP)) \
					s_WantedScrollOffset -= 0.1f; \
			} \
			s_WantedScrollOffset = clamp(s_WantedScrollOffset, 0.0f, 1.0f); \
			\
			smooth_set(&s_ScrollOffset, s_WantedScrollOffset, 27.0f, Client()->RenderFrameTime()); \
		}

void CMenus::RenderAbout(CUIRect MainView)
{
	RenderTools()->DrawUIRect(&MainView, vec4(0.7f, 0.7f, 0.2f, 0.3f), CUI::CORNER_ALL, 15.0f);
	MainView.Margin(5.0f, &MainView);

	static float s_TotalHeight = -1.0f;
	if(s_TotalHeight < 0)
	{
		#include "menus_manual/__defines_cht.h"
		#include "menus_manual/about.h"
		#include "menus_manual/__finish.h"
		s_TotalHeight -= MainView.h;
	}
	PASTE_SCROLLCODE()

	CUIRect Label;
	TextRender()->TextColor(1,1,1,1);

	Graphics()->ClipEnable((int)MainView.x, (int)MainView.y+20+10, (int)MainView.w*2, round_to_int(MainView.h*1.5f));
	MainView.y -= s_ScrollOffset * s_TotalHeight;
	#include "menus_manual/__defines_impl.h"
	#include "menus_manual/about.h"
	#include "menus_manual/__finish.h"
	Graphics()->ClipDisable();
}

void CMenus::RenderCredits(CUIRect MainView)
{
	RenderTools()->DrawUIRect(&MainView, vec4(0.7f, 0.7f, 0.2f, 0.3f), CUI::CORNER_ALL, 15.0f);
	MainView.Margin(5.0f, &MainView);

	static float s_TotalHeight = -1.0f;
	if(s_TotalHeight < 0)
	{
		#include "menus_manual/__defines_cht.h"
		#include "menus_manual/credits.h"
		#include "menus_manual/__finish.h"
		s_TotalHeight -= MainView.h;
	}
	PASTE_SCROLLCODE()

	CUIRect Label;
	TextRender()->TextColor(1,1,1,1);

	Graphics()->ClipEnable((int)MainView.x, (int)MainView.y+20+10, (int)MainView.w*2, round_to_int(MainView.h*1.5f));
	MainView.y -= s_ScrollOffset * s_TotalHeight;
	#include "menus_manual/__defines_impl.h"
	#include "menus_manual/credits.h"
	#include "menus_manual/__finish.h"
	Graphics()->ClipDisable();
}

void CMenus::RenderManual_General(CUIRect MainView)
{

}

#undef PUTLINE
#undef NEWLINE
