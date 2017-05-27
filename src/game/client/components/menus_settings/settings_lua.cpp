#include "../menus.h"

#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include "../console.h"


#if defined(FEATURE_LUA)
void CMenus::RenderLoadingLua()
{
	return;
	CALLSTACK_ADD();

	Graphics()->Swap();

	CUIRect Bar, Rect = *UI()->Screen();

	Rect.Margin(Rect.w*0.23f, &Rect);

	Rect.HSplitTop(15.0f, &Bar, &Rect);
	RenderTools()->DrawUIRect(&Bar, vec4(0.2f, 0.7f, 0.2f, 0.8f), CUI::CORNER_T, 5.0f);
	RenderTools()->DrawUIRect(&Rect, vec4(0,0,0, 0.8f), CUI::CORNER_B, 5.0f);
	UI()->DoLabelScaled(&Bar, Localize("Please wait..."), 12.0f, 0, -1, 0);
	Rect.y += Rect.h/3; Rect.h *= 2/3;
	UI()->DoLabelScaled(&Rect, Localize("Loading Lua Script"), 18.0f, 0, -1, 0);

	Graphics()->Swap();
}

void CMenus::RenderSettingsLuaExceptions(CUIRect MainView, CLuaFile *L)
{
	static CButtonContainer s_BCListbox;
	static float s_ScrollVal = 1.0f;

	char aBuf[256];
	char aTitle[256];
	char aBottomText[32];
	str_format(aTitle, sizeof(aTitle), Localize("Exceptions thrown by script '%s'"), L->GetFilename());
	if(L->State() == CLuaFile::STATE_LOADED)
	{
		str_format(aBuf, sizeof(aBuf), " [%s]", Localize("Running"));
		str_append(aTitle, aBuf, sizeof(aTitle));
	}
	else if(L->State() == CLuaFile::STATE_ERROR)
	{
		str_format(aBuf, sizeof(aBuf), " [%s]", Localize("Aborted"));
		str_append(aTitle, aBuf, sizeof(aTitle));
	}
	str_format(aBottomText, sizeof(aBottomText), "%i/100", L->m_Exceptions.size());
	UiDoListboxStart(&s_BCListbox, &MainView, 20.0f, aTitle, aBottomText, L->m_Exceptions.size(), 1, -1, s_ScrollVal);

	CButtonContainer s_aButtonIDs[100];
	for(int i = 0; i < L->m_Exceptions.size(); i++)
	{
#if defined(CONF_DEBUG)
		dbg_assert(i < 100, "Increased the max number of exceptions? Increase it here too, please!");
#else
		if(i >= 100)
			break;
#endif

		CListboxItem Item = UiDoListboxNextItem(&s_aButtonIDs[i]);
		if(!Item.m_Visible)
			continue;

		if(UI()->MouseInside(&Item.m_Rect))
		{
			float gb = Input()->KeyIsPressed(KEY_MOUSE_1) ? 0.7f : 1.0f;
			RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1,gb,gb,0.25f), CUI::CORNER_ALL, 8.0f);
		}

		CUIRect Button;
		str_format(aBuf, sizeof(aBuf), "#%i", i+1);
		//const float tw = TextRender()->TextWidth(0, 16.0f, aBuf, -1);
		Item.m_Rect.VSplitLeft(35.0f, &Button, &Item.m_Rect);
		UI()->DoLabelScaled(&Button, aBuf, 16.0f, -1);

		Item.m_Rect.VSplitLeft(10.0f, 0, &Item.m_Rect);
		str_format(aBuf, sizeof(aBuf), "@@ %s", L->m_Exceptions[i].c_str());
		UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, -1);
	}

	int Clicked = UiDoListboxEnd(&s_ScrollVal, 0);
	if(Clicked >= 0)
		Input()->SetClipboardText(L->m_Exceptions[Clicked].c_str());
}


void CMenus::RenderSettingsLua(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect ListView, Button, BottomBar;
	static int s_ActiveLuaSettings = -1;
	static int s_ActiveLuaExceptions = -1;
	static int s_SelectedScript = -1;

	if(g_Config.m_ClLua)
	{
		// render settings page if open
		if(s_ActiveLuaSettings >= 0)
		{
			if(Client()->Lua()->GetLuaFiles()[s_ActiveLuaSettings]->State() != CLuaFile::STATE_LOADED)
				s_ActiveLuaSettings = -1;
			else
			{
				try
				{
					CUIRect CloseButton;
					MainView.HSplitTop(20.0f, &CloseButton, &MainView);
					static CButtonContainer s_CloseButton;
					if(DoButton_Menu(&s_CloseButton, Localize("Close"), 0, &CloseButton, 0, CUI::CORNER_B) || !g_Config.m_ClLua)
					{
						Client()->Lua()->GetLuaFiles()[s_ActiveLuaSettings]->GetFunc("OnScriptSaveSettings")();
						s_ActiveLuaSettings = -1;
					}
					else
					{
						MainView.HSplitTop(10.0f, 0, &MainView);
						Client()->Lua()->GetLuaFiles()[s_ActiveLuaSettings]->GetFunc("OnScriptRenderSettings")(MainView);
						return;
					}
				}
				catch(std::exception& e)
				{
					Client()->Lua()->HandleException(e, Client()->Lua()->GetLuaFiles()[s_ActiveLuaSettings]);
					s_ActiveLuaSettings = -1;
				}
			}
		}

		// render exceptions page if open
		if(s_ActiveLuaExceptions >= 0)
		{
			CUIRect CloseButton;
			MainView.HSplitTop(20.0f, &CloseButton, &MainView);
			static CButtonContainer s_CloseButton;
			if(DoButton_Menu(&s_CloseButton, Localize("Close"), 0, &CloseButton, 0, CUI::CORNER_B) || !g_Config.m_ClLua)
			{
				s_ActiveLuaExceptions = -1;
			}
			else
			{
				MainView.HSplitTop(10.0f, 0, &MainView);
				RenderSettingsLuaExceptions(MainView, Client()->Lua()->GetLuaFiles()[s_ActiveLuaExceptions]);
				return;
			}
		}
	}
	else
	{
		s_ActiveLuaSettings = -1;
		s_ActiveLuaExceptions = -1;
		s_SelectedScript = -1;
	}

	// render the main selection view
	MainView.VSplitLeft(MainView.w/2.0f, &ListView, &MainView);
	ListView.HSplitBottom(50.0f+5.0f, &ListView, &BottomBar);
	ListView.HSplitTop(20.0f, &Button, &ListView);


	CUIRect RefreshButton;
	Button.VSplitRight(max(100.0f, TextRender()->TextWidth(0, Button.h*ms_FontmodHeight, Localize("Refresh"), -1)), &Button, &RefreshButton);
	static CButtonContainer s_RefreshButton, s_LuaButton;
	if(g_Config.m_ClLua)
		if(DoButton_Menu(&s_RefreshButton, Localize("Refresh"), 0, &RefreshButton, "Reload the list of files", s_SelectedScript > -1 ? 0 : CUI::CORNER_TR))
		{
			s_SelectedScript = -1;
			Client()->Lua()->LoadFolder();
		}

	if(DoButton_CheckBox(&s_LuaButton, Localize("Use Lua"), g_Config.m_ClLua, &Button, 0, g_Config.m_ClLua ? CUI::CORNER_TL : CUI::CORNER_ALL))
	{
		s_SelectedScript = -1;
		if(!(g_Config.m_ClLua ^= 1))
		{
			Client()->Lua()->GetLuaFiles().delete_all();
			Client()->Lua()->GetLuaFiles().clear();
		}
		else
			Client()->Lua()->LoadFolder();
	}
	if(!g_Config.m_ClLua)
		return;


	static int ShowActiveOnly = 0;
	{
		int NumLuaFiles = Client()->Lua()->GetLuaFiles().size();

		// display mode list
		const int MAX_SCRIPTS = 512;
		static float s_ScrollValue = 0;
		static CButtonContainer pIDItem[MAX_SCRIPTS];
		static CButtonContainer pIDButtonToggleScript[MAX_SCRIPTS];
		static CButtonContainer pIDButtonPermissions[MAX_SCRIPTS];
		static CButtonContainer pIDButtonAutoload[MAX_SCRIPTS];

		static CButtonContainer s_Listbox;
		CUIRect ListBox = ListView;
		char aHeadline[128], aBottomLine[128];
		static int NumListedFiles = 0, NumActiveScripts = 0;
		str_format(aHeadline, sizeof(aHeadline), Localize("%s (%i files listed, %i scripts active)"), Localize("Lua files"), NumListedFiles, NumActiveScripts);
		str_format(aBottomLine, sizeof(aBottomLine), Localize("(%i files found – %i filtered away)"), Client()->Lua()->GetLuaFiles().size(), Client()->Lua()->GetLuaFiles().size() - NumListedFiles);
		UiDoListboxStart(&s_Listbox, &ListBox, 50.0f, aHeadline, aBottomLine, NumListedFiles, 1, -1/*s_SelectedScript*/, s_ScrollValue, 0, 0);
		NumListedFiles = 0; NumActiveScripts = 0;
		for(int i = 0; i < NumLuaFiles; i++)
		{
			CLuaFile *L = Client()->Lua()->GetLuaFiles()[i];
			if(!L)
				continue;

			if(L->State() == CLuaFile::STATE_LOADED)
				NumActiveScripts++;

			// filter
			if(g_Config.m_ClLuaFilterString[0] != '\0' && (!str_find_nocase(L->GetFilename(), g_Config.m_ClLuaFilterString) && !str_find_nocase(L->GetScriptTitle(), g_Config.m_ClLuaFilterString)))
				continue;
			if(ShowActiveOnly == 1 && L->State() != CLuaFile::STATE_LOADED)
				continue;
			else if(ShowActiveOnly == 2 && L->State() == CLuaFile::STATE_LOADED)
				continue;

			CListboxItem Item = UiDoListboxNextItem(&pIDItem[i], 0);
			NumListedFiles++;

			if(i >= MAX_SCRIPTS-1)
			{
				if(Item.m_Visible)
				{
					char aBuf[512];
					str_formatb(aBuf, "The internal limit of scripts that can be shown here has been hit (how have you got that many?!). There are %i more scripts that couldn't be shown.", (NumLuaFiles - 1) - i);
					UI()->DoLabelScaled(&Item.m_Rect, aBuf, 12.0f, 0, Item.m_Rect.w);
				}
				break;
			}

			if(Item.m_Visible)
			{
				CUIRect Label, Buttons, Button;

				Item.m_Rect.HMargin(2.5f, &Item.m_Rect);
				Item.m_Rect.HSplitTop(5.0f, 0, &Label);

				if(Item.m_Rect.y+Item.m_Rect.h > Item.m_HitRect.y)
				{
					if(UI()->MouseInside(&Item.m_Rect) && KeyEvent(KEY_MOUSE_1))
						s_SelectedScript = i;

					// activate button
					Item.m_Rect.VSplitRight(Item.m_Rect.h*0.83f, &Item.m_Rect, &Button);
					if (DoButton_Menu(&pIDButtonToggleScript[i], L->State() == CLuaFile::STATE_LOADED ? "×" : "→", 0, &Button, L->State() == CLuaFile::STATE_LOADED ? Localize("Deactivate") : Localize("Activate"), CUI::CORNER_R))
					{
						if(L->State() == CLuaFile::STATE_LOADED)
							L->Deactivate();
						else
						{
							RenderLoadingLua();
							L->Activate();
						}
					}


					// permission indicator
					Item.m_Rect.VSplitRight(Item.m_Rect.h/2.0f, &Item.m_Rect, &Buttons);
					Buttons.HSplitMid(&Buttons, &Button); // top: permission indicator, bottom: autoload checkbox

					int PermissionFlags = L->GetPermissionFlags();
					char aTooltip[1024] = {0};
					if(PermissionFlags == 0)
						str_copyb(aTooltip, Localize("This script has no additional permissions and is thus considered safe."));
					else
					{
						str_copyb(aTooltip, Localize("This script has the following additional permission:"));
#define PERM_STR(TYPE, STR) if(PermissionFlags&CLuaFile::PERMISSION_##TYPE) { str_append(aTooltip, "\n\n- ", sizeof(aTooltip)); str_append(aTooltip, STR, sizeof(aTooltip)); }
						PERM_STR(IO, Localize("IO (Write and read files)"))
						PERM_STR(DEBUG, Localize("DEBUG (WARNING: if you are not currently debugging this script, DO NOT TO USE IT!! It may cause security and performance problems!)"))
						PERM_STR(FFI, Localize("FFI (Execution of native C code from within Lua - please be sure that this code is not malicious, as the ATH API cannot control it"))
						PERM_STR(OS, Localize("OS (Access to various operation system functionalities such as time and date"))
						PERM_STR(PACKAGE, Localize("PACKAGE (Modules)"))
#undef PERM_STR
					}
					if(DoButton_Menu(&pIDButtonPermissions[i], "!", PermissionFlags, &Buttons, aTooltip, 0, vec4(PermissionFlags > 0 ? .7f : .2f, PermissionFlags > 0 ? .2f : .7f, .2f, .8f)))
						dbg_msg("lua/permissions", "'%s' | %i (%i)", L->GetFilename(), PermissionFlags, L->GetPermissionFlags());


					// autoload button
					if(DoButton_CheckBox(&pIDButtonAutoload[i], "", L->GetScriptIsAutoload(), &Button, Localize("Autoload"), 0))
					{
						bool NewVal = !L->GetScriptIsAutoload();
						L->SetScriptIsAutoload(NewVal);
						if(NewVal)
							Client()->Lua()->AddAutoload(L);
						else
							Client()->Lua()->RemoveAutoload(L);
					}


					// nice background
					vec4 Color = L->State() == CLuaFile::STATE_ERROR ? vec4(0.7f,0,0,0.3f) :
								 L->State() == CLuaFile::STATE_LOADED ? vec4(0,0.7f,0,0.3f) : vec4(0,0,0,0.3f);
					if(i == s_SelectedScript)
						RenderTools()->DrawUIRect(&Item.m_Rect, vec4(1,1,1,0.5f), 0, 0);
					else if(i%2)
						Color.a += 0.2f;

					RenderTools()->DrawUIRect(&Item.m_Rect, Color, 0, 0);

					// script filename
					UI()->DoLabelScaled(&Label, L->GetFilename()+4, 14.0f, -1, -1/*Buttons.w-5.0f*/, g_Config.m_ClLuaFilterString);

				}

			}
		}

		UiDoListboxEnd(&s_ScrollValue, 0);

		if(NumLuaFiles == 0)
		{
			CUIRect Label;
			ListView.HSplitBottom(ListView.h/2+15.0f, 0, &Label);
			UI()->DoLabelScaled(&Label, Localize("No files listed, click \"Refresh\" to reload the list"), 15.0f, 0, -1);
		}
	}


	// render the box at the right
	if(s_SelectedScript > -1 && s_SelectedScript < Client()->Lua()->GetLuaFiles().size())
	{
		RenderTools()->DrawUIRect(&MainView, vec4(0,0,0,0.25f), CUI::CORNER_R, 5.0f);

		CLuaFile *L = Client()->Lua()->GetLuaFiles()[s_SelectedScript];
		CUIRect Label;
		MainView.HSplitTop(10.0f, 0, &MainView);
		MainView.HSplitTop(25.0f, &Label, &MainView);
		if (L->GetScriptTitle()[0] != '\0')
			UI()->DoLabelScaled(&Label, L->GetScriptTitle(), 18.0f, 0, Label.w, g_Config.m_ClLuaFilterString);
		else
			UI()->DoLabelScaled(&Label, L->GetFilename()+4, 18.0f, 0, Label.w, g_Config.m_ClLuaFilterString);

		MainView.HSplitTop(10.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Label, &MainView);
		UI()->DoLabelScaled(&Label, L->GetScriptInfo(), 14.0f, 0, Label.w);


		// button bar at the bottom right
		CUIRect Button, Bar;
		MainView.HSplitBottom(10.0f, &MainView, 0);
		MainView.HSplitBottom(35.0f, &MainView, &Bar);

		Bar.VMargin(7.5f, &Bar);
//		Bar.VSplitRight(Bar.w/4.0f-4*5.0f, &Bar, &Button);

#define PREPARE_BUTTON(TEXT) Bar.VSplitRight(5.0f, &Bar, 0); Bar.VSplitRight(max(100.0f, TextRender()->TextWidth(0, Bar.h*ms_FontmodHeight, TEXT, -1)), &Bar, &Button);
		if(L->State() == CLuaFile::STATE_LOADED)
		{
			PREPARE_BUTTON(Localize("Deactivate"));
			static CButtonContainer s_DeactivateButton;
			if (DoButton_Menu(&s_DeactivateButton, Localize("Deactivate"), 0, &Button))
			{
				L->Deactivate();
			}

			static float s_ButtonReloadColorFade = 0.0f;
			static int s_PrevScriptIndex = s_SelectedScript;
			if(s_SelectedScript != s_PrevScriptIndex) s_ButtonReloadColorFade = 0.0f;
			smooth_set(&s_ButtonReloadColorFade, 0.0f, 15.0f, Client()->RenderFrameTime());
			PREPARE_BUTTON(Localize("Reload"));
			static CButtonContainer s_ReloadButton;
			if(DoButton_Menu(&s_ReloadButton, Localize("Reload"), 0, &Button, 0, CUI::CORNER_ALL, vec4(1.0f-s_ButtonReloadColorFade, 1.0f-s_ButtonReloadColorFade, 1.0f, 0.5f)))
			{
				s_ButtonReloadColorFade = 1.0f;
				RenderLoadingLua();
				L->Activate();
			}


			if (L->GetScriptHasSettings())
			{
				PREPARE_BUTTON(Localize("Settings"));
				static CButtonContainer s_ButtonSettings;
				if (DoButton_Menu(&s_ButtonSettings, Localize("Settings"), 0, &Button))
				{
					s_ActiveLuaSettings = s_SelectedScript;
				}
			}

		}
		else
		{
			PREPARE_BUTTON(Localize("Activate"));
			static CButtonContainer s_ButtonActivate;
			if (DoButton_Menu(&s_ButtonActivate, Localize("Activate"), 0, &Button))
			{
				RenderLoadingLua();
				L->Activate();
			}
		}

#undef PREPARE_BUTTON

		MainView.HSplitBottom(5.0f, &MainView, 0);
		MainView.HSplitBottom(20.0f, &MainView, &Bar);
		Bar.VMargin(7.5f+5.0f, &Bar);

		if(L->m_Exceptions.size() > 0)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), Localize("Exceptions (%i)"), L->m_Exceptions.size());
			Bar.VSplitRight(max(100.0f, TextRender()->TextWidth(0, Bar.h*ms_FontmodHeight, aBuf, -1)), &Bar, &Button);
			static CButtonContainer s_ButtonExceptions;
			if(DoButton_Menu(&s_ButtonExceptions, aBuf, 0, &Button, "", CUI::CORNER_ALL, mix(vec4(0,1,0,0.5f), vec4(1,0,0,0.5f), (float)L->m_Exceptions.size()/100.0f)))
			{
				s_ActiveLuaExceptions = s_SelectedScript;
			}
		}


		if(L->State() == CLuaFile::STATE_LOADED && m_pClient->m_pGameConsole->GetDebuggerChild() != L->L())
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s", Localize("Debug"));
			Bar.VSplitRight(max(100.0f, TextRender()->TextWidth(0, Bar.h*ms_FontmodHeight, aBuf, -1)), &Bar, &Button);
			static CButtonContainer s_ButtonDebug;
			if(DoButton_Menu(&s_ButtonDebug, aBuf, 0, &Button, "Attach the lua console as a debugger to this script"))
			{
				m_pClient->m_pGameConsole->AttachLuaDebugger(L);
			}
		}
		else if(L->State() == CLuaFile::STATE_ERROR)
		{
			float FadeVal = sinf(Client()->SteadyTimer()*1.4f)/2.0f+0.5f;
			TextRender()->TextColor(1.0f, 0.25f+FadeVal*0.75f, 0.25f+FadeVal*0.75f, 1.0f);
			UI()->DoLabelScaled(&Bar, L->m_pErrorStr && L->m_pErrorStr[0] ? L->m_pErrorStr : Localize("An error occured"), 12.0f, -1, Bar.w);
			TextRender()->TextColor(1,1,1,1);
		}


		if(L->State() == CLuaFile::STATE_LOADED)
		{
			// let the script render stuff, if it wants to
			CUIRect View;
			MainView.VMargin(7.5f, &View);
			View.HSplitBottom(10.0f, &View, 0);
			View.HSplitTop(14.0f*(TextRender()->TextLineCount(0, 14.0f, L->GetScriptInfo(), MainView.w)-1)+5.0f, 0, &View);
			LuaRef func = Client()->Lua()->GetLuaFiles()[s_SelectedScript]->GetFunc("OnScriptRenderInfo");
			if(func.cast<bool>())
			{
				try
				{
					func(View);
				} catch(std::exception &e) { Client()->Lua()->HandleException(e, Client()->Lua()->GetLuaFiles()[s_SelectedScript]); }
			}
		}

	}


	// render the bottom bar at the left
	RenderTools()->DrawUIRect(&BottomBar, vec4(1,1,1,0.25f), s_SelectedScript > -1 ? CUI::CORNER_BL : CUI::CORNER_B, 5.0f);
	BottomBar.VMargin(10.0f, &BottomBar);
	BottomBar.HSplitBottom(5.0f, &BottomBar, 0);
	// render quick search
	{
		CUIRect QuickSearch, QuickSearchClearButton;
		BottomBar.HSplitTop(25.0f, &QuickSearch, &BottomBar);
		QuickSearch.HSplitTop(5.0f, 0, &QuickSearch);
		UI()->DoLabelScaled(&QuickSearch, "⚲", 14.0f, -1);
		float wSearch = TextRender()->TextWidth(0, 14.0f, "⚲", -1);
		QuickSearch.VSplitLeft(wSearch, 0, &QuickSearch);
		QuickSearch.VSplitLeft(5.0f, 0, &QuickSearch);
		QuickSearch.VSplitLeft(QuickSearch.w-15.0f, &QuickSearch, &QuickSearchClearButton);
		static float Offset = 0.0f;
		static CButtonContainer s_LuaFilterStringEditbox;
		DoEditBox(&s_LuaFilterStringEditbox, &QuickSearch, g_Config.m_ClLuaFilterString, sizeof(g_Config.m_ClLuaFilterString), 14.0f, &Offset, false, CUI::CORNER_L, Localize("Search"));

		// clear button
		{
			static CButtonContainer s_ClearButton;
			if(DoButton_Menu(&s_ClearButton, "×", 0, &QuickSearchClearButton, "clear", CUI::CORNER_R, vec4(1,1,1,0.33f)))
			{
				g_Config.m_ClLuaFilterString[0] = 0;
				UI()->SetActiveItem(s_LuaFilterStringEditbox.GetID());
			}
		}
	}

	// render script-activation-filter button
	{
		const char *s_apLabels[] = {
				Localize("Showing all files"),
				Localize("Showing active scripts only"),
				Localize("Showing inactive scripts only")
		};

		static float Width = -1;
		if(Width < 0)
			for(int i = 0; i < 2; i++)
				Width = max(Width, TextRender()->TextWidth(0, BottomBar.h-10.0f, s_apLabels[i], -1));

		CUIRect Checkbox;
		BottomBar.HSplitTop(5.0f, 0, &BottomBar);
		BottomBar.VSplitLeft(TextRender()->TextWidth(0, 14.0f, Localize("Quickfilter:"), -1) + 5.0f, &BottomBar, &Checkbox);
		UI()->DoLabelScaled(&BottomBar, Localize("Quickfilter:"), 14.0f, -1);

		static CButtonContainer s_Checkbox;
		int MouseButton = DoButton_CheckBox_Number(&s_Checkbox, s_apLabels[ShowActiveOnly], ShowActiveOnly, &Checkbox);
		if(MouseButton == 1)
		{
			if(++ShowActiveOnly > 2)
				ShowActiveOnly = 0;
		}
		else if(MouseButton == 2)
		{
			if(--ShowActiveOnly < 0)
				ShowActiveOnly = 2;
		}
	}
}

#endif
