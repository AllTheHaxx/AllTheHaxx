#include <engine/textrender.h>
#include <engine/editor.h>
#include <engine/updater.h>

#include "menus.h"
#include "countryflags.h"

void CMenus::RenderPopups()
{
	char aTitle[256];
	char aExtraText[256];
	const char *pTitle = "";
	const char *pExtraText = "";
	const char *pButtonText = "";
	int ExtraAlign = CUI::ALIGN_CENTER;

	if(m_Popup == POPUP_MESSAGE)
	{
		pTitle = m_aMessageTopic;
		pExtraText = m_aMessageBody;
		pButtonText = m_aMessageButton;
	}
	else if(m_Popup == POPUP_CONNECTING)
	{
		pTitle = Localize("Connecting to");
		pExtraText = Client()->GetCurrentServerAddress();
		pButtonText = Localize("Abort");
		if(Client()->MapDownloadTotalsize() > 0)
		{
			str_format(aTitle, sizeof(aTitle), "%s: %s", Localize("Downloading map"), Client()->MapDownloadName());
			pTitle = aTitle;
			pExtraText = "";
		}
		else if(str_comp(Client()->MapDownloadSource(), "gameserver") != 0)
		{
			str_format(aTitle, sizeof(aTitle), "%s %s", Localize("Connecting to"), Client()->GetCurrentServerAddress());
			pTitle = aTitle;

			const int IntTime = round_to_int(Client()->LocalTime());
			static const char *s_apCoolChars[] = { "–", "\\", "|", "/"};
			const char *c = s_apCoolChars[IntTime%(sizeof(s_apCoolChars)/sizeof(s_apCoolChars[0]))];
			str_format(aExtraText, sizeof(aExtraText),
						"{%s} Waiting for map DB %i/%i:\n\n"
						"%s",
					   c,
					   Client()->MapDownloadSourceID(),
					   Client()->NumMapDBServers(),
					   Client()->MapDownloadSource());
			pExtraText = aExtraText;
			ExtraAlign = CUI::ALIGN_LEFT;
		}
	}
	else if (m_Popup == POPUP_DISCONNECTED)
	{
		pTitle = Localize("Disconnected");
		pExtraText = Client()->ErrorString();
		pButtonText = Localize("Ok");
		if(Client()->m_ReconnectTime > 0)
		{
			str_format(aExtraText, sizeof(aExtraText), Localize("\n\nReconnect in %d sec"), (Client()->m_ReconnectTime - time_get()) / time_freq());
			pTitle = Client()->ErrorString();
			pExtraText = aExtraText;
			pButtonText = Localize("Abort");
		}
		ExtraAlign = CUI::ALIGN_CENTER;
	}
	else if(m_Popup == POPUP_PURE)
	{
		pTitle = Localize("Disconnected");
		pExtraText = Localize("The server is running a non-standard tuning on a pure game type.");
		pButtonText = Localize("Ok");
		ExtraAlign = CUI::ALIGN_LEFT;
	}
	else if(m_Popup == POPUP_DELETE_DEMO)
	{
		pTitle = Localize("Delete demo");
		pExtraText = Localize("Are you sure that you want to delete the demo?");
		ExtraAlign = CUI::ALIGN_LEFT;
	}
	else if(m_Popup == POPUP_RENAME_DEMO)
	{
		pTitle = Localize("Rename demo");
		pExtraText = "";
		ExtraAlign = -1;
	}
	else if(m_Popup == POPUP_REMOVE_FRIEND)
	{
		pTitle = Localize("Remove friend");
		pExtraText = Localize("Are you sure that you want to remove the player from your friends list?");
		ExtraAlign = CUI::ALIGN_LEFT;
	}
	else if(m_Popup == POPUP_SOUNDERROR)
	{
		pTitle = Localize("Sound error");
		pExtraText = Localize("The audio device couldn't be initialised.");
		pButtonText = Localize("I don't care");
		ExtraAlign = CUI::ALIGN_LEFT;
	}
	else if(m_Popup == POPUP_PASSWORD)
	{
		pTitle = Localize("Password incorrect");
		pExtraText = "";
		pButtonText = Localize("Try again");
	}
	else if(m_Popup == POPUP_QUIT)
	{
		pTitle = Localize("Quit");
		pExtraText = Localize("Are you handsome?");
		ExtraAlign = CUI::ALIGN_LEFT;
	}
	else if(m_Popup == POPUP_DISCONNECT)
	{
		pTitle = Localize("Disconnect");
		pExtraText = Localize("Are you sure that you want to disconnect?");
		ExtraAlign = CUI::ALIGN_LEFT;
	}
	else if(m_Popup == POPUP_FIRST_LAUNCH)
	{
		pTitle = Localize("Welcome to Teeworlds");
		pExtraText = Localize("As this is the first time you launch the game, please enter your nick name below. It's recommended that you check the settings to adjust them to your liking before joining a server.");
		pButtonText = Localize("Ok");
		ExtraAlign = CUI::ALIGN_LEFT;
	}
	else if(m_Popup == POPUP_LUA_REQUEST_FULLSCREEN)
	{
#if defined(FEATURE_LUA)
		pTitle = Localize("Lua Script wants to go fullscreen");
		char aScriptName[64];
		if(str_length(m_pLuaFSModeRequester->GetScriptTitle()) > 0)
			str_formatb(aScriptName, "'%s' (\"%s\")", m_pLuaFSModeRequester->GetFilename(), m_pLuaFSModeRequester->GetScriptTitle());
		else
			str_formatb(aScriptName, "'%s'", m_pLuaFSModeRequester->GetFilename());
		str_formatb(aExtraText, "The script %s wants to enter fullscreen mode. This will disable your UI. You can always exit it again by clicking the button at the top right.", aScriptName);
		pExtraText = aExtraText;
		ExtraAlign = CUI::ALIGN_LEFT;
#else
		pTitle = "What the heck happened??";
		pExtraText = "This popup is not supposed to appear when there is no lua in the client!";
		ExtraAlign = CUI::ALIGN_CENTER;
#endif
	}
	else if(m_Popup == POPUP_UPDATE)
	{
		if(Updater()->State() == IUpdater::STATE_CLEAN)
		{
			pTitle = Localize("Update available!");
			str_formatb(aExtraText, "AllTheHaxx version %s has been released! Do you want to update your client now?", Updater()->GetLatestVersion());
			pExtraText = aExtraText;
			ExtraAlign = CUI::ALIGN_LEFT;
		}
		else
		{
			switch(Updater()->State())
			{
				case IUpdater::STATE_DOWNLOADING:
					pTitle = Localize("Downloading Update...");
					pExtraText = Updater()->GetCurrentFile();
					ExtraAlign = CUI::ALIGN_LEFT;
				break;
				case IUpdater::STATE_MOVE_FILES:
					pTitle = Localize("Installing Update...");
					pExtraText = Updater()->GetCurrentFile();
					ExtraAlign = CUI::ALIGN_LEFT;
				break;
				case IUpdater::STATE_FAIL:
					pTitle = Localize("Update Failed!");
					str_formatb(aExtraText, "%s: %s", Localize("What failed"), Updater()->GetWhatFailed());
					pExtraText = aExtraText;
					ExtraAlign = CUI::ALIGN_LEFT;
				break;
				case IUpdater::STATE_NEED_RESTART:
					pTitle = Localize("Update finished!");
					pExtraText = Localize("AllTheHaxx needs to be restarted");
					ExtraAlign = CUI::ALIGN_CENTER;
				break;
				default:
					pTitle = Localize("Performing Update");
					pExtraText = Localize("Please wait");
					ExtraAlign = CUI::ALIGN_CENTER;
			}
		}
	}

	RenderCurrentPopup(pTitle, pExtraText, pButtonText, ExtraAlign);
}

void CMenus::RenderCurrentPopup(const char *pTitle, const char *pExtraText, const char *pButtonText, int ExtraAlign)
{
	char aBuf[128];
	CUIRect Screen = *UI()->Screen();

	CUIRect Box, Part;
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
	Part.VMargin(20.f/UI()->Scale(), &Part);
	if(TextRender()->TextWidth(0, 24.f, pTitle, -1) > Part.w)
		UI()->DoLabelScaled(&Part, pTitle, 24.f, -1, (int)Part.w);
	else
		UI()->DoLabelScaled(&Part, pTitle, 24.f, 0);
	Box.HSplitTop(20.f/UI()->Scale(), &Part, &Box);
	Box.HSplitTop(24.f/UI()->Scale(), &Part, &Box);
	Part.VMargin(20.f/UI()->Scale(), &Part);

	if(ExtraAlign == -1)
		UI()->DoLabelScaled(&Part, pExtraText, 20.f, -1, (int)Part.w);
	else
	{
		if(TextRender()->TextWidth(0, 20.f, pExtraText, -1) > Part.w)
			UI()->DoLabelScaled(&Part, pExtraText, 20.f, -1, (int)Part.w);
		else
			UI()->DoLabelScaled(&Part, pExtraText, 20.f, 0, -1);
	}

	if(m_Popup == POPUP_QUIT)
	{
		CUIRect Yes, No;
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif

		// additional info
		Box.HSplitTop(10.0f, 0, &Box);
		Box.VMargin(20.f/UI()->Scale(), &Box);
		if(m_pClient->Editor()->HasUnsavedData())
		{
			str_format(aBuf, sizeof(aBuf),
					   "%s\n%s",
					   Localize("There's an unsaved map in the editor, you might want to save it before you quit the game."),
					   Localize("Quit anyway?")
			);
			UI()->DoLabelScaled(&Box, aBuf, 20.f, -1, Part.w-20.0f);
		}

		// buttons
		Part.VMargin(80.0f, &Part);
		Part.VSplitMid(&No, &Yes);
		Yes.VMargin(20.0f, &Yes);
		No.VMargin(20.0f, &No);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("Yes"), 0, &No) || m_EscapePressed)
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonTryAgain;
		if(DoButton_Menu(&s_ButtonTryAgain, Localize("No"), 0, &Yes) || m_EnterPressed)
			Client()->Quit();
	}
	else if(m_Popup == POPUP_DISCONNECT)
	{
		CUIRect Yes, No;
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif

		// buttons
		Part.VMargin(80.0f, &Part);
		Part.VSplitMid(&No, &Yes);
		Yes.VMargin(20.0f, &Yes);
		No.VMargin(20.0f, &No);

		static CButtonContainer s_ButtonNo;
		if(DoButton_Menu(&s_ButtonNo, Localize("No"), 0, &No) || m_EscapePressed)
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonYes;
		if(DoButton_Menu(&s_ButtonYes, Localize("Yes"), 0, &Yes) || m_EnterPressed)
			Client()->Disconnect();
	}
	else if(m_Popup == POPUP_LUA_REQUEST_FULLSCREEN)
	{
		CUIRect Yes, No;
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif

		// buttons
		Part.VMargin(80.0f, &Part);
#if defined(FEATURE_LUA)
		Part.VSplitMid(&No, &Yes);
		Yes.VMargin(20.0f, &Yes);
		No.VMargin(20.0f, &No);

		bool Handled = false;

		static CButtonContainer s_ButtonDeny;
		if(DoButton_Menu(&s_ButtonDeny, Localize("Deny"), 0, &No) || m_EscapePressed)
			Handled = true;
		static CButtonContainer s_ButtonAllow;
		if(DoButton_Menu(&s_ButtonAllow, Localize("Allow"), 0, &Yes))
		{
			Client()->Lua()->ScriptEnterFullscreen(m_pLuaFSModeRequester);
			Handled = true;
		}

		if(Handled)
		{
			m_pLuaFSModeRequester = 0;
			m_Popup = POPUP_NONE;
		}
#else
		Part.VMargin(120.0f, &Part);
		static CButtonContainer s_ButtonOk;
		if(DoButton_Menu(&s_ButtonOk, Localize("Ok"), 0, &Part) || m_EscapePressed || m_EnterPressed)
			m_Popup = POPUP_NONE;
#endif
	}
	else if(m_Popup == POPUP_UPDATE)
	{
		if(Updater()->State() == IUpdater::STATE_CLEAN)
		{
			CUIRect Yes, No;
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);

			// buttons
			Part.VMargin(80.0f, &Part);
			Part.VSplitMid(&No, &Yes);
			Yes.VMargin(20.0f, &Yes);
			No.VMargin(20.0f, &No);

//			RenderTools()->DrawUIRect(&Box, vec4(0,1,0,1), 0, 0);
//			RenderTools()->DrawUIRect(&Yes, vec4(0,1,0,1), 0, 0);
//			RenderTools()->DrawUIRect(&No, vec4(1,0,0,1), 0, 0);

			static CButtonContainer s_ButtonNo;
			if(DoButton_Menu(&s_ButtonNo, Localize("Later"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static CButtonContainer s_ButtonYes;
			if(DoButton_Menu(&s_ButtonYes, Localize("Install Update"), 0, &Yes) || m_EnterPressed)
				Updater()->PerformUpdate();
		}
		else if(Updater()->State() == IUpdater::STATE_DOWNLOADING || Updater()->State() == IUpdater::STATE_MOVE_FILES)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitBottom(2*20.0f+5.0f, &Box, 0);

			// bottom progress bar (relative progress)
			{
				CUIRect Label;
				Box.HSplitBottom(24.0f, &Box, &Part);
				Part.VMargin(120.0f, &Part);
				Part.HSplitTop(3.0f, 0, &Label);

				// render bar background
				RenderTools()->DrawUIRect(&Part, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 5.0f);

				// render progress bar
				Part.VSplitLeft(Part.w * ((float)Updater()->GetCurrentPercent()/100.0f), &Part, 0);
				RenderTools()->DrawUIRect(&Part, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f * min(1.0f, Part.w / 20.0f));

				str_formatb(aBuf, "%i%%", Updater()->GetCurrentPercent());
				UI()->DoLabelScaled(&Label, aBuf, 12.0f, CUI::ALIGN_CENTER);
			}

			// top progress bar (absolute progress)
			{
				CUIRect Label;
				Box.HSplitBottom(5.0f, &Box, 0);
				Box.HSplitBottom(24.0f, &Box, &Part);
				Part.VMargin(120.0f, &Part);
				Part.HSplitTop(3.0f, 0, &Label);

				// render bar background
				RenderTools()->DrawUIRect(&Part, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 5.0f);

				// render progress bar
				float Progress = (float)Updater()->GetTotalProgress() / (float)Updater()->GetTotalNumJobs();
				Part.VSplitLeft(Part.w * Progress, &Part, 0);
				RenderTools()->DrawUIRect(&Part, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 5.0f * min(1.0f, Part.w / 20.0f));

				str_formatb(aBuf, "%i%%", round_to_int(Progress * 100.0f));
				UI()->DoLabelScaled(&Label, aBuf, 12.0f, CUI::ALIGN_CENTER);
			}
		}
		else if(Updater()->State() == IUpdater::STATE_FAIL)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitTop(20.f, &Part, &Box);
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(120.0f, &Part);

			static CButtonContainer s_Button;
			if(DoButton_Menu(&s_Button, Localize("Damn"), 0, &Part) || m_EscapePressed || m_EnterPressed)
				m_Popup = POPUP_NONE;
		}
		else if(Updater()->State() == IUpdater::STATE_NEED_RESTART)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitTop(20.f, &Part, &Box);
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(120.0f, &Part);

			static CButtonContainer s_Button;
			if(DoButton_Menu(&s_Button, Localize("Restart to use the new version"), 0, &Part) || m_EnterPressed)
				Client()->Restart();
		}
	}
	else if(m_Popup == POPUP_PASSWORD)
	{
		CUIRect Label, TextBox, TryAgain, Abort;

		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Part.VMargin(80.0f, &Part);

		Part.VSplitMid(&Abort, &TryAgain);

		TryAgain.VMargin(20.0f, &TryAgain);
		Abort.VMargin(20.0f, &Abort);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonTryAgain;
		if(DoButton_Menu(&s_ButtonTryAgain, Localize("Try again"), 0, &TryAgain) || m_EnterPressed)
		{
			Client()->Connect(g_Config.m_UiServerAddress);
		}

		Box.HSplitBottom(60.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif

		Part.VSplitLeft(60.0f, 0, &Label);
		Label.VSplitLeft(100.0f, 0, &TextBox);
		TextBox.VSplitLeft(20.0f, 0, &TextBox);
		TextBox.VSplitRight(60.0f, &TextBox, 0);
		UI()->DoLabel(&Label, Localize("Password"), 18.0f, -1);
		static float Offset = 0.0f;
		CPointerContainer s_PasswordEditBox(&g_Config.m_Password);
		DoEditBox(&s_PasswordEditBox, &TextBox, g_Config.m_Password, sizeof(g_Config.m_Password), 12.0f, &Offset, true);
	}
	else if(m_Popup == POPUP_CONNECTING)
	{
		Box = Screen;
		Box.VMargin(150.0f, &Box);
		Box.HMargin(150.0f, &Box);
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Part.VMargin(120.0f, &Part);

		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || m_EscapePressed || m_EnterPressed)
		{
			Client()->Disconnect();
			m_Popup = POPUP_NONE;
		}

		if(Client()->MapDownloadTotalsize() > 0)
		{
			int64 Now = time_get();
			if(Now-m_DownloadLastCheckTime >= time_freq())
			{
				if(m_DownloadLastCheckSize > Client()->MapDownloadAmount())
				{
					// map downloaded restarted
					m_DownloadLastCheckSize = 0;
				}

				// update download speed
				float Diff = (Client()->MapDownloadAmount()-m_DownloadLastCheckSize)/((int)((Now-m_DownloadLastCheckTime)/time_freq()));
				float StartDiff = m_DownloadLastCheckSize-0.0f;
				if(StartDiff+Diff > 0.0f)
					m_DownloadSpeed = (Diff/(StartDiff+Diff))*(Diff/1.0f) + (StartDiff/(Diff+StartDiff))*m_DownloadSpeed;
				else
					m_DownloadSpeed = 0.0f;
				m_DownloadLastCheckTime = Now;
				m_DownloadLastCheckSize = Client()->MapDownloadAmount();
			}

			Box.HSplitTop(64.f, 0, &Box);
			Box.HSplitTop(24.f, &Part, &Box);
			str_format(aBuf, sizeof(aBuf), "%d/%d KiB (%.1f KiB/s)", Client()->MapDownloadAmount()/1024, Client()->MapDownloadTotalsize()/1024,	m_DownloadSpeed/1024.0f);
			UI()->DoLabel(&Part, aBuf, 20.f, 0, -1);

			// time left
			const char *pTimeLeftString;
			int TimeLeft = max(1, m_DownloadSpeed > 0.0f ? static_cast<int>((Client()->MapDownloadTotalsize()-Client()->MapDownloadAmount())/m_DownloadSpeed) : 1);
			if(TimeLeft >= 60)
			{
				TimeLeft /= 60;
				pTimeLeftString = TimeLeft == 1 ? Localize("%i minute left") : Localize("%i minutes left");
			}
			else
				pTimeLeftString = TimeLeft == 1 ? Localize("%i second left") : Localize("%i seconds left");
			Box.HSplitTop(20.f, 0, &Box);
			Box.HSplitTop(24.f, &Part, &Box);
			str_format(aBuf, sizeof(aBuf), pTimeLeftString, TimeLeft);
			UI()->DoLabel(&Part, aBuf, 20.f, 0, -1);

			// progress bar background
			Box.HSplitTop(20.f, 0, &Box);
			Box.HSplitTop(24.f, &Part, &Box);
			Part.VMargin(40.0f, &Part);
			RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
			CUIRect DownloadSourceLabel = Part; // save it for later
			DownloadSourceLabel.y += 3.0f;

			// the actual progress bar
//				Part.w = max(10.0f, (Part.w*Client()->MapDownloadAmount())/Client()->MapDownloadTotalsize());
			static float s_SmoothCutPercentage = 1.0f;
			float CutPercentage = (float)Client()->MapDownloadAmount()/(float)Client()->MapDownloadTotalsize();
			if(s_SmoothCutPercentage > CutPercentage) // reset
				s_SmoothCutPercentage = CutPercentage;
			smooth_set(&s_SmoothCutPercentage, CutPercentage, 70.0f * max((1.0f - CutPercentage+0.5f), 1.0f - (CutPercentage+0.3f-s_SmoothCutPercentage)) * (1.0f - CutPercentage+0.5f), Client()->RenderFrameTime());
			CUIRect DebugRect = Part;
			Part.VMargin((Part.w/2.0f)*(1.0f-s_SmoothCutPercentage), &Part);
			if(g_Config.m_Debug)
			{
				DebugRect.VMargin((DebugRect.w/2.0f)*(1.0f-CutPercentage), &DebugRect);
				RenderTools()->DrawUIRect(&DebugRect, vec4(1.0f, 0.0f, 0.0f, 0.5f), CUI::CORNER_ALL, 5.0f * min(1.0f, DebugRect.w / 20.0f));
				RenderTools()->DrawUIRect(&Part, vec4(0.0f, 1.0f, 0.0f, 0.5f), CUI::CORNER_ALL, 5.0f * min(1.0f, Part.w / 20.0f));
			}
			else
				RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 5.0f * min(1.0f, Part.w / 20.0f));

			// map download source in the progress bar if webdl is active
			if(g_Config.m_ClHttpMapDownload)
			{
				str_formatb(aBuf, "Downloading from: %s", Client()->MapDownloadSource());
				TextRender()->TextColor(0.7f, 0.7f, 0.7f, 0.9f);
				UI()->DoLabelScaled(&DownloadSourceLabel, aBuf, 14.0f, 0);
				TextRender()->TextColor(1,1,1,1);
			}

		}
	}
	else if(m_Popup == POPUP_LANGUAGE)
	{
		Box = Screen;
		Box.VMargin(150.0f, &Box);
#if defined(__ANDROID__)
		Box.HMargin(20.0f, &Box);
#else
		Box.HMargin(150.0f, &Box);
#endif
		Box.HSplitTop(20.f, &Part, &Box);
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Box.HSplitBottom(20.f, &Box, 0);
		Box.VMargin(20.0f, &Box);
		RenderLanguageSelection(Box);
		Part.VMargin(120.0f, &Part);

		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, Localize("Ok"), 0, &Part) || m_EscapePressed || m_EnterPressed)
			m_Popup = POPUP_FIRST_LAUNCH;
	}
	else if(m_Popup == POPUP_COUNTRY)
	{
		Box = Screen;
		Box.VMargin(150.0f, &Box);
#if defined(__ANDROID__)
		Box.HMargin(20.0f, &Box);
#else
		Box.HMargin(150.0f, &Box);
#endif
		Box.HSplitTop(20.f, &Part, &Box);
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Box.HSplitBottom(20.f, &Box, 0);
		Box.VMargin(20.0f, &Box);

		static int ActSelection = -2;
		if(ActSelection == -2)
			ActSelection = g_Config.m_BrFilterCountryIndex;
		static float s_ScrollValue = 0.0f;
		int OldSelected = -1;
		static CButtonContainer s_Listbox;
		UiDoListboxStart(&s_Listbox, &Box, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

		for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
		{
			const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
			if(pEntry->m_CountryCode == ActSelection)
				OldSelected = i;

			CPointerContainer ItemContainer(&pEntry->m_CountryCode);
			CListboxItem Item = UiDoListboxNextItem(&ItemContainer, OldSelected == i);
			if(Item.m_Visible)
			{
				CUIRect Label;
				Item.m_Rect.Margin(5.0f, &Item.m_Rect);
				Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);
				float OldWidth = Item.m_Rect.w;
				Item.m_Rect.w = Item.m_Rect.h*2;
				Item.m_Rect.x += (OldWidth-Item.m_Rect.w)/ 2.0f;
				vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
				m_pClient->m_pCountryFlags->Render(pEntry->m_CountryCode, &Color, Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
				UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
			}
		}

		const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
		if(OldSelected != NewSelected)
			ActSelection = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;

		Part.VMargin(120.0f, &Part);

		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, Localize("Ok"), 0, &Part) || m_EnterPressed)
		{
			g_Config.m_BrFilterCountryIndex = ActSelection;
			Client()->ServerBrowserUpdate();
			m_Popup = POPUP_NONE;
		}

		if(m_EscapePressed)
		{
			ActSelection = g_Config.m_BrFilterCountryIndex;
			m_Popup = POPUP_NONE;
		}
	}
	else if(m_Popup == POPUP_DELETE_DEMO)
	{
		CUIRect Yes, No;
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Part.VMargin(80.0f, &Part);

		Part.VSplitMid(&No, &Yes);

		Yes.VMargin(20.0f, &Yes);
		No.VMargin(20.0f, &No);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonTryAgain;
		if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
		{
			m_Popup = POPUP_NONE;
			// delete demo
			if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
			{
				str_format(aBuf, sizeof(aBuf), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
				if(Storage()->RemoveFile(aBuf, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
				{
					DemolistPopulate();
					DemolistOnUpdate(false);
				}
				else
					PopupMessage(Localize("Error"), Localize("Unable to delete the demo"), Localize("Ok"));
			}
		}
	}
	else if(m_Popup == POPUP_RENAME_DEMO)
	{
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

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonOk;
		if(DoButton_Menu(&s_ButtonOk, Localize("Ok"), 0, &Ok) || m_EnterPressed)
		{
			m_Popup = POPUP_NONE;
			// rename demo
			if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
			{
				char aBufOld[512];
				str_format(aBufOld, sizeof(aBufOld), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
				int Length = str_length(m_aCurrentDemoFile);
				char aBufNew[512];
				if(Length <= 4 || m_aCurrentDemoFile[Length-5] != '.' || str_comp_nocase(m_aCurrentDemoFile+Length-4, "demo"))
					str_format(aBufNew, sizeof(aBufNew), "%s/%s.demo", m_aCurrentDemoFolder, m_aCurrentDemoFile);
				else
					str_format(aBufNew, sizeof(aBufNew), "%s/%s", m_aCurrentDemoFolder, m_aCurrentDemoFile);
				if(Storage()->RenameFile(aBufOld, aBufNew, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
				{
					DemolistPopulate();
					DemolistOnUpdate(false);
				}
				else
					PopupMessage(Localize("Error"), Localize("Unable to rename the demo"), Localize("Ok"));
			}
		}

		Box.HSplitBottom(60.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif

		Part.VSplitLeft(60.0f, 0, &Label);
		Label.VSplitLeft(120.0f, 0, &TextBox);
		TextBox.VSplitLeft(20.0f, 0, &TextBox);
		TextBox.VSplitRight(60.0f, &TextBox, 0);
		UI()->DoLabel(&Label, Localize("New name:"), 18.0f, -1);
		static float Offset = 0.0f;
		static CButtonContainer s_TextBox;
		DoEditBox(&s_TextBox, &TextBox, m_aCurrentDemoFile, sizeof(m_aCurrentDemoFile), 12.0f, &Offset);
	}
	else if(m_Popup == POPUP_REMOVE_FRIEND)
	{
		CUIRect Yes, No;
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Part.VMargin(80.0f, &Part);

		Part.VSplitMid(&No, &Yes);

		Yes.VMargin(20.0f, &Yes);
		No.VMargin(20.0f, &No);

		static CButtonContainer s_ButtonAbort;
		if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
			m_Popup = POPUP_NONE;

		static CButtonContainer s_ButtonTryAgain;
		if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
		{
			m_Popup = POPUP_NONE;
			// remove friend
			if(m_FriendlistSelectedIndex >= 0)
			{
				m_pClient->Friends()->RemoveFriend(m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aName,
												   m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aClan);
				FriendlistOnUpdate();
				Client()->ServerBrowserUpdate();
			}
		}
	}
	else if(m_Popup == POPUP_FIRST_LAUNCH)
	{
		CUIRect Label, TextBox;

		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Part.VMargin(80.0f, &Part);

		static CButtonContainer s_EnterButton;
		if(DoButton_Menu(&s_EnterButton, Localize("Enter"), 0, &Part) || m_EnterPressed)
			m_Popup = POPUP_NONE;

		Box.HSplitBottom(40.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif

		Part.VSplitLeft(60.0f, 0, &Label);
		Label.VSplitLeft(100.0f, 0, &TextBox);
		TextBox.VSplitLeft(20.0f, 0, &TextBox);
		TextBox.VSplitRight(60.0f, &TextBox, 0);
		UI()->DoLabel(&Label, Localize("Nickname"), 18.0f, -1);
		static float Offset = 0.0f;
		static CButtonContainer s_NameEditBox;
		DoEditBox(&s_NameEditBox, &TextBox, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), 12.0f, &Offset);
	}
	else
	{
		Box.HSplitBottom(20.f, &Box, &Part);
#if defined(__ANDROID__)
		Box.HSplitBottom(60.f, &Box, &Part);
#else
		Box.HSplitBottom(24.f, &Box, &Part);
#endif
		Part.VMargin(120.0f, &Part);

		static CButtonContainer s_Button;
		if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || m_EscapePressed || m_EnterPressed)
		{
			if(m_Popup == POPUP_DISCONNECTED && Client()->m_ReconnectTime > 0)
				Client()->m_ReconnectTime = 0;
			m_Popup = POPUP_NONE;
		}
	}

	if(m_Popup == POPUP_NONE)
		UI()->SetActiveItem(0);
}
