#include "../menus.h"

#include <engine/textrender.h>
#include "game/client/components/fontmgr.h"

void CMenus::RenderSettingsAppearance(CUIRect MainView)
{
	CALLSTACK_ADD();

	if(m_pfnAppearanceSubpage)
	{
		(*this.*m_pfnAppearanceSubpage)(MainView);
		return;
	}

	CUIRect Left, Right, Button;
	//MainView.VSplitMid(&Left, &Right);
	MainView.VMargin(MainView.w/3, &Left);

#define DO_NEXT_BUTTON(BASERECT, TITLE, CALLBACK) \
	Left.HSplitTop(10.0f, 0, &BASERECT); \
	Left.HSplitTop(50.0f, &Button, &BASERECT); \
	TextRender()->Text(0, Button.x+Button.w-TextRender()->TextWidth(0, Button.h, ">", 1)-10.0f, Button.y-Button.h/3.6f, Button.h, ">", 9999); \
	{ static CButtonContainer Container;\
	if(DoButton_MenuTab(&Container, TITLE, 0, &Button, CUI::CORNER_ALL)) \
		m_pfnAppearanceSubpage = &CMenus::CALLBACK; }

	DO_NEXT_BUTTON(Left, "HUD", RenderSettingsAppearanceHUD);
	DO_NEXT_BUTTON(Left, Localize("Textures"), RenderSettingsAppearanceTexture);
	DO_NEXT_BUTTON(Left, Localize("Fonts"), RenderSettingsAppearanceFont);

//	RenderTools()->DrawUIRect(&Button, vec4(0,1,1,1), 0, 0);
}

void CMenus::RenderSettingsAppearanceHUD(CUIRect MainView) // here will be more tabs and stuff I think
{
	CALLSTACK_ADD();

	//RenderTools()->DrawUIRect(&MainView, vec4(1,0,1,1), 0, 0); // debuggi ^^
	RenderSettingsHUD(MainView);
}

void CMenus::RenderSettingsAppearanceTexture(CUIRect MainView)
{
	CALLSTACK_ADD();

	//RenderTools()->DrawUIRect(&MainView, vec4(0,1,1,1), 0, 0);
	RenderSettingsTexture(MainView);
}

void CMenus::RenderSettingsAppearanceFont(CUIRect MainView)
{
	CALLSTACK_ADD();

	enum
	{
		PAGE_BASIC = 0,
		PAGE_MONO = 1
	};

	static int s_FontPageType = PAGE_BASIC;

	{
		CUIRect Button, Bar;

		MainView.HSplitTop(20.0f, &Bar, &MainView);
		Bar.VSplitMid(&Button, &Bar);

		static CButtonContainer s_ButtonPlayer;
		if(DoButton_MenuTab(&s_ButtonPlayer, Localize("Basic Font"), s_FontPageType == PAGE_BASIC, &Button, CUI::CORNER_L))
			s_FontPageType = PAGE_BASIC;

		static CButtonContainer s_ButtonTee;
		if(DoButton_MenuTab(&s_ButtonTee, Localize("Monospace Font"), s_FontPageType == PAGE_MONO, &Bar, CUI::CORNER_R))
			s_FontPageType = PAGE_MONO;
	}

	IFontMgr *pFontMgr = s_FontPageType == PAGE_BASIC ? m_pClient->m_pFontMgrBasic : m_pClient->m_pFontMgrMono;
	const int NUM_FONTS = pFontMgr->GetNumFonts();
	{
		char aBuf[64];
		CUIRect OptionsBar, Button;
		MainView.VSplitRight(MainView.w*0.4f, &MainView, &OptionsBar);
		OptionsBar.x += 5.0f;

		OptionsBar.HSplitTop(20.0f, &Button, &OptionsBar);
		str_format(aBuf, sizeof(aBuf), Localize("%i fonts installed"), NUM_FONTS);
		Button.x += 10.0f;
		UI()->DoLabelScaled(&Button, aBuf, Button.h-5.0f, -1, Button.w-3.0f);

		OptionsBar.HSplitTop(10.0f, &Button, &OptionsBar);
		OptionsBar.HSplitTop(20.0f, &Button, &OptionsBar);
		static CButtonContainer s_ReloadButton;
		if(DoButton_Menu(&s_ReloadButton, Localize("Search for newly added fonts"), 0, &Button))
			pFontMgr->ReloadFontlist();

		// render some preview text in different sizes
		for(float s = 9.0f; s <= 19.0f;
			s == 9.0f ? s = 13.0f :
				s == 13.0f ? s = 19.0f :
				s++)
		{
			OptionsBar.HSplitTop(30.0f, &Button, &OptionsBar);
			OptionsBar.HSplitTop(10.0f, &Button, &OptionsBar);
			{
				char aText[32];
				str_format(aText, sizeof(aText), "Some size %i text:", round_to_int(s));
				UI()->DoLabelScaled(&Button, aText, s, -1);
			}

			CTextCursor Cursor;
			OptionsBar.HSplitTop(5.0f+s, &Button, &OptionsBar);
			OptionsBar.HSplitTop(3.0f+s, &Button, &OptionsBar);
			TextRender()->SetCursor(&Cursor, Button.x, Button.y, s, TEXTFLAG_RENDER, pFontMgr->GetFont(FONT_BOLD));
			TextRender()->TextEx(&Cursor, s_FontPageType == PAGE_BASIC ? "Some bold text" : "Some bold monospace text", -1);

			OptionsBar.HSplitTop(5.0f, &Button, &OptionsBar);
			OptionsBar.HSplitTop(3.0f+s, &Button, &OptionsBar);
			TextRender()->SetCursor(&Cursor, Button.x, Button.y, s, TEXTFLAG_RENDER, pFontMgr->GetFont(FONT_ITALIC));
			TextRender()->TextEx(&Cursor, s_FontPageType == PAGE_BASIC ? "Some italic text" : "Some italic monospace text", -1);

			OptionsBar.HSplitTop(5.0f, &Button, &OptionsBar);
			OptionsBar.HSplitTop(3.0f+s, &Button, &OptionsBar);
			TextRender()->SetCursor(&Cursor, Button.x, Button.y, s, TEXTFLAG_RENDER, pFontMgr->GetFont(FONT_BOLD_ITALIC));
			TextRender()->TextEx(&Cursor, s_FontPageType == PAGE_BASIC ? "Bold and italic text" : "Bold and italic monospace text", -1);
		}
	}

	{
		const int OldSelected = pFontMgr->GetSelectedFontIndex();
		static int pIDItem[2][128];
		static CButtonContainer s_Listbox;
		static float s_ScrollValue[2] = {0.0f, 0.0f};
		UiDoListboxStart(&s_Listbox, &MainView, 30.0f, Localize("Font Selector - Game Font"), 0, NUM_FONTS, 2, OldSelected, s_ScrollValue[s_FontPageType], CUI::CORNER_ALL);
		for(int i = 0; i < NUM_FONTS; i++)
		{
			const char *pFilePath = pFontMgr->GetFontPath(i);
			if(!pFilePath || pFilePath[0] == '\0') continue;

			CPointerContainer Container(&pIDItem[s_FontPageType][i]);
			CListboxItem Item = UiDoListboxNextItem(&Container, 0);

			if(Item.m_Visible)
			{
				CTextCursor Cursor;
				TextRender()->SetCursor(&Cursor, Item.m_Rect.x, Item.m_Rect.y, 17.0f, TEXTFLAG_RENDER, pFontMgr->GetFont(FONT_REGULAR));
				TextRender()->TextColor(1,1,1,1);
				TextRender()->TextEx(&Cursor, pFilePath, -1);
			}
		}

		int NewSelected = UiDoListboxEnd(&s_ScrollValue[s_FontPageType], 0);
		if(NewSelected != OldSelected)
		{
			pFontMgr->ActivateFont(NewSelected);
		}
	}
}
