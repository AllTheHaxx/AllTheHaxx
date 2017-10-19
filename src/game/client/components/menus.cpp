/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/tl/array.h>
#include <sstream>
#include <string>

#include <math.h>

#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/config.h>
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/irc.h>
#include <engine/updater.h>
#include <engine/shared/config.h>

#include <game/version.h>
#include <game/generated/protocol.h>

#include <game/generated/client_data.h>
#include <game/client/components/camera.h>
#include <game/client/components/sounds.h>
#include <game/client/components/console.h>
#include <game/client/gameclient.h>
#include <game/client/lineinput.h>
#include <game/localization.h>
#include <mastersrv/mastersrv.h>
#include <versionsrv/versionsrv.h>
#include <base/color.h>

#include "countryflags.h"
#include "console.h"
#include "mapimages.h"
#include "menus.h"
#include "skins.h"
#include "spoofremote.h"
#include "controls.h"
#include "irc.h"

vec4 CMenus::ms_GuiColor;
vec4 CMenus::ms_ColorTabbarInactiveOutgame;
vec4 CMenus::ms_ColorTabbarActiveOutgame;
vec4 CMenus::ms_ColorTabbarInactive;
vec4 CMenus::ms_ColorTabbarActive = vec4(0,0,0,0.5f);
vec4 CMenus::ms_ColorTabbarInactiveIngame;
vec4 CMenus::ms_ColorTabbarActiveIngame;

#if defined(__ANDROID__)
float CMenus::ms_ButtonHeight = 50.0f;
float CMenus::ms_ListheaderHeight = 17.0f;
float CMenus::ms_ListitemAdditionalHeight = 33.0f;
#else
float CMenus::ms_ButtonHeight = 25.0f;
float CMenus::ms_ListheaderHeight = 17.0f;
#endif
float CMenus::ms_FontmodHeight = 0.8f;

IInput::CEvent CMenus::m_aInputEvents[MAX_INPUTEVENTS];
int CMenus::m_NumInputEvents;


CMenus::CMenus()
{
	m_Popup = POPUP_NONE;
	m_ActivePage = PAGE_BROWSER;
	m_GamePage = PAGE_GAME;

	m_pfnAppearanceSubpage = 0;

	m_NeedRestartGraphics = false;
	m_NeedRestartSound = false;
	m_NeedSendinfo = false;
	m_NeedSendDummyinfo = false;
	m_MenuActive = true;
	m_HotbarActive = false;
	m_HotbarWasActive = false;
	m_UseMouseButtons = true;
	m_MouseSlow = false;
	m_InitSkinlist = true;

	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;
	m_NumInputEvents = 0;

	m_LastInput = time_get();

	str_copy(m_aCurrentDemoFolder, "demos", sizeof(m_aCurrentDemoFolder));
	m_aCallvoteReason[0] = 0;
	m_aCallvoteFilterString[0] = 0;
	m_VoteCalled = 0;

	m_aIdentFilterString[0] = 0;

	m_FriendlistSelectedIndex = -1;
	m_DoubleClickIndex = -1;

	m_DDRacePage = PAGE_BROWSER;

	m_DemoPlayerState = DEMOPLAYER_NONE;
	m_Dummy = false;
	mem_zerob(m_Nalf);

	m_SpoofSelectedPlayer = -1;

	m_LoadCurrent = 0;
	m_LoadTotal = 100; // some approx number so that we don't divide m_LoadCurrent by zero

}

float CMenus::ButtonFade(CButtonContainer *pBC, float Seconds, int Checked)
{
	if (UI()->HotItem() == pBC->GetID() || Checked)
	{
		pBC->m_FadeStartTime = Client()->SteadyTimer();
		return Seconds;
	}
	else if (pBC->m_FadeStartTime + Seconds > Client()->SteadyTimer())
	{
		return pBC->m_FadeStartTime + Seconds - Client()->SteadyTimer();
	}
	return 0.0f;
}

void CMenusTooltip::OnRender()
{
	if(m_aTooltip[0] == '\0')
		return;

	const float FONT_SIZE = 13.0f;
	const float LINE_WIDTH = 300.0f * UI()->Scale();
	const int LineCount = TextRender()->TextLineCount(0, FONT_SIZE, m_aTooltip, LINE_WIDTH);

	CUIRect Rect;
	Rect.x = UI()->MouseX() + 30.0f;
	Rect.y = UI()->MouseY() + 5.0f;
	Rect.h = (FONT_SIZE * (float)LineCount + 2.0f+1.0f + (float)(LineCount-5)*0.25f);
	Rect.w = TextRender()->TextWidth(0, FONT_SIZE, m_aTooltip, -1, LINE_WIDTH);
	Rect.Margin(-3.0f, &Rect); // outsize it a bit to give it a little padding to the text

	// make sure that the thing doesn't go out of view (right)
	Rect.x = clamp(Rect.x, 0.0f, UI()->Screen()->w - Rect.w);

	// make sure we don't hide the cursor -> move the tooltip down if we would
	if(UI()->MouseX() + 20.0f > Rect.x)
		Rect.y += 20.0f * UI()->Scale();

	// make sure that the thing doesn't go out of view (bottom)
	Rect.y = clamp(Rect.y, 0.0f, UI()->Screen()->h - Rect.h);

	RenderTools()->DrawUIRect(&Rect, vec4(0,0,0.2f,0.8f), CUI::CORNER_ALL, 2.5f);
	TextRender()->Text(0, Rect.x+1.5f, Rect.y, FONT_SIZE, m_aTooltip, LINE_WIDTH);

	m_aTooltip[0] = 0;

}

void CMenus::OnConsoleInit()
{
	CALLSTACK_ADD();

	Console()->Register("toggle_hotbar", "", CFGFLAG_CLIENT, ConKeyToggleHotbar, this, "Access the hotbar");
	Console()->Register("toggle_irc", "", CFGFLAG_CLIENT, ConKeyToggleIRC, this, "Toggle the IRC");

	Console()->Register("unlock_mouse", "", CFGFLAG_CLIENT, ConKeyShortcutRelMouse, this, "Release the mouse");
}

vec4 CMenus::ButtonColorMul(CButtonContainer *pBC)
{
	if(UI()->ActiveItem() == pBC->GetID())
		return vec4(1,1,1,0.5f);
	else if(UI()->HotItem() == pBC->GetID())
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

bool CMenus::KeyEvent(int Key, int FlagMask)
{
	for(int i = 0; i < m_NumInputEvents; i++)
		if(m_aInputEvents[i].m_Key == Key && (m_aInputEvents[i].m_Flags & FlagMask))
			return true;
	return false;
}

bool CMenus::KeyMods(int Keymod)
{
	switch(Keymod)
	{
		case KEYMOD_CTRL:
			return Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL);
		case KEYMOD_SHIFT:
			return Input()->KeyIsPressed(KEY_LSHIFT) || Input()->KeyIsPressed(KEY_RSHIFT);
		default:
			return false;
	}
}

int CMenus::DoButton_Icon(int ImageId, int SpriteId, const CUIRect *pRect)
{
	CALLSTACK_ADD();

	Graphics()->TextureSet(g_pData->m_aImages[ImageId].m_Id);

	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SpriteId);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return 0;
}

int CMenus::DoButton_Toggle(CButtonContainer *pBC, int Checked, const CUIRect *pRect, bool Active, const char *pTooltip)
{
	CALLSTACK_ADD();

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIBUTTONS].m_Id);
	Graphics()->QuadsBegin();
	if(!Active)
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	RenderTools()->SelectSprite(Checked?SPRITE_GUIBUTTON_ON:SPRITE_GUIBUTTON_OFF);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	if(UI()->HotItem() == pBC->GetID() && Active)
	{
		RenderTools()->SelectSprite(SPRITE_GUIBUTTON_HOVER);
		IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);

		if(pTooltip && pTooltip[0] != '\0')
			m_pClient->m_pTooltip->SetTooltip(pTooltip);
	}
	Graphics()->QuadsEnd();

	return Active ? UI()->DoButtonLogic(pBC->GetID(), "", Checked, pRect) : 0;
}

int CMenus::DoButton_Menu(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip, int Corner, vec4 Color)
{
	CALLSTACK_ADD();

	float Seconds = 0.6f; //  0.6 seconds for fade
	float Fade = ButtonFade(pBC, Seconds, Checked);
	float FadeVal = Fade/Seconds;

	vec4 FinalColor = mix(vec4(0.0f, 0.0f, 0.0f, 0.25f), Color, FadeVal);
	RenderTools()->DrawUIRect(pRect, FinalColor, Corner, 5.0f);
	CUIRect Temp;
	pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
#if defined(__ANDROID__)
	float TextH = min(22.0f, Temp.h);
	Temp.y += (Temp.h - TextH) / 2;
	UI()->DoLabel(&Temp, pText, TextH*ms_FontmodHeight, 0);
#else
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
#endif
	if(UI()->HotItem() == pBC->GetID() && pTooltip)
	{
		m_pClient->m_pTooltip->SetTooltip(pTooltip);
	}

	return UI()->DoButtonLogic(pBC->GetID(), pText, Checked, pRect);
	//int ret = UI()->DoButtonLogic(pBC->GetID(), pText, Checked, pRect);
	//if(ret) *const_cast<float*>(reinterpret_cast<const float*>(pBC->GetID())) = 0.0f; // hacky...? But it's sexy so idc :P
	//return ret;
}

void CMenus::DoButton_KeySelect(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip)
{
	CALLSTACK_ADD();

	float Seconds = 0.6f; //  0.6 seconds for fade
	float Fade = ButtonFade(pBC, Seconds, Checked);
	float FadeVal = Fade/Seconds;

	RenderTools()->DrawUIRect(pRect, vec4(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f+FadeVal*0.5f), CUI::CORNER_ALL, 5.0f);
	CUIRect Temp;
	pRect->HMargin(1.0f, &Temp);
	TextRender()->TextColor(1.0f-FadeVal, 1.0f-FadeVal, 1.0f-FadeVal, 1.0f);
	TextRender()->TextOutlineColor(0.0f+FadeVal, 0.0f+FadeVal, 0.0f+FadeVal, 0.25f);
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);

	if(UI()->MouseInside(&Temp) && pTooltip && pTooltip[0] != '\0')
		m_pClient->m_pTooltip->SetTooltip(pTooltip);

}

int CMenus::DoButton_MenuTab(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, int Corners, vec4 ColorActive, vec4 ColorInactive, const char *pTooltip)
{
	CALLSTACK_ADD();

	if(Checked)
		RenderTools()->DrawUIRect(pRect, ColorActive, Corners, 10.0f);
	else if(UI()->MouseInside(pRect))
	{
		RenderTools()->DrawUIRect(pRect, mix(ColorInactive, ColorActive, 0.5f), Corners, 10.0f);
		if(pTooltip && pTooltip[0] != '\0')
			m_pClient->m_pTooltip->SetTooltip(pTooltip);
	}
	else
		RenderTools()->DrawUIRect(pRect, ColorInactive, Corners, 10.0f);

	CUIRect Temp;
	pRect->HMargin(2.0f, &Temp);
#if defined(__ANDROID__)
	float TextH = min(22.0f, Temp.h);
	Temp.y += (Temp.h - TextH) / 2;
	UI()->DoLabel(&Temp, pText, TextH*ms_FontmodHeight, 0);
#else
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
#endif

	return UI()->DoButtonLogic(pBC->GetID(), pText, Checked, pRect);
}

int CMenus::DoButton_GridHeader(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, int Corners)
{
	CALLSTACK_ADD();

	if(Checked)
		RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f), Corners, 5.0f);
	CUIRect t;
	pRect->VSplitLeft(5.0f, 0, &t);
#if defined(__ANDROID__)
	float TextH = min(20.0f, pRect->h);
	UI()->DoLabel(&t, pText, TextH*ms_FontmodHeight, -1);
#else
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight, -1);
#endif
	return UI()->DoButtonLogic(pBC->GetID(), pText, Checked, pRect);
}

int CMenus::DoButton_CheckBox_Common(CButtonContainer *pBC, const char *pText, const char *pBoxText, const CUIRect *pRect, const char *pTooltip, bool Checked, int Corner)
{
	CALLSTACK_ADD();

	RenderTools()->DrawUIRect(pRect, vec4(0.0f, 0.0f, 0.0f, 0.25f), Corner, 5.0f);

	CUIRect c = *pRect;
	CUIRect t = *pRect;
	c.w = c.h;
	t.x += c.w;
	t.w -= c.w;
	t.VSplitLeft(5.0f, 0, &t);

	c.Margin(2.0f, &c);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, UI()->HotItem() == pBC->GetID() ? 1.0f : 0.6f);
	if(Checked)
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_ACTIVE);
	else
		RenderTools()->SelectSprite(SPRITE_MENU_CHECKBOX_INACTIVE);
	IGraphics::CQuadItem QuadItem(c.x, c.y, c.w, c.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// lame fixes
	t.y += 2.0f;
	c.y += 2.0f;
	c.x -= 0.3f;
	UI()->DoLabel(&c, pBoxText, pRect->h*ms_FontmodHeight*0.6f, 0);
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight*0.8f, -1);

	if(UI()->HotItem() == pBC->GetID() && pTooltip)
	{
		m_pClient->m_pTooltip->SetTooltip(pTooltip);
	}

	return UI()->DoButtonLogic(pBC->GetID(), pText, 0, pRect);
}

int CMenus::DoButton_CheckBox(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip, int Corner)
{
	CALLSTACK_ADD();

	return DoButton_CheckBox_Common(pBC, pText, "", pRect, pTooltip, Checked, Corner);
}
int CMenus::DoButton_CheckBox_Direct(CButtonContainer *pBC, const char *pText, int *pVar, const CUIRect *pRect, const char *pTooltip, int Corner)
{
	CALLSTACK_ADD();

	int result = DoButton_CheckBox_Common(pBC, pText, "", pRect, pTooltip, *pVar, Corner);
	if(result)
		*pVar ^= 1;
	return result;
}


int CMenus::DoButton_CheckBox_Number(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect, const char *pTooltip, int Corner)
{
	CALLSTACK_ADD();

	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d", Checked);
	return DoButton_CheckBox_Common(pBC, pText, aBuf, pRect, pTooltip, false, Corner);
}
int CMenus::DoButton_CheckBox_Number_Direct(CButtonContainer *pBC, const char *pText, int *pVar, int Min, int Max, const CUIRect *pRect, const char *pTooltip, int Corner)
{
	CALLSTACK_ADD();

	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d", *pVar);
	int result = DoButton_CheckBox_Common(pBC, pText, aBuf, pRect, pTooltip, false, Corner);
	if(result == 1)
	{
//		*pVar = (*pVar + 1) % (Max + 1 - Min) + Min;
		(*pVar)++;
		if(*pVar > Max) // wrap around
			*pVar = Min;
	}
	else if(result == 2)
	{
		(*pVar)--;
		if(*pVar < Min) // wrap around
			*pVar = Max;
	}
	return result;
}

int CMenus::DoEditBox(CButtonContainer *pBC, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *pOffset, bool Hidden, int Corners, const char *pEmptyText, int Align, const char *pTooltip)
{
	CALLSTACK_ADD();

	int Inside = UI()->MouseInside(pRect);
	bool ReturnValue = false;
	bool UpdateOffset = false;
	static int s_AtIndex = 0;
	static bool s_DoScroll = false;
	static float s_ScrollStart = 0.0f;

	FontSize *= UI()->Scale();

	if(UI()->LastActiveItem() == pBC->GetID())
	{
		int Len = str_length(pStr);
		if(Len == 0)
			s_AtIndex = 0;

		for(int iEvent = 0; iEvent < m_NumInputEvents; iEvent++)
		{
			const IInput::CEvent& Event = m_aInputEvents[iEvent];
			bool Handled = false;

			if(Event.m_Flags&IInput::FLAG_PRESS)
			{
				if(KeyMods(KEYMOD_CTRL))
				{
					Handled = true;

					CLineInput LineInput; // working with a CLineInput here is easier
					LineInput.Set(pStr);
					LineInput.SetCursorOffset(s_AtIndex);

					if(Event.m_Key == KEY_V) // paste
					{
						const char *pText = Input()->GetClipboardText();
						if(pText && pText[0])
						{
							char aLine[512];
							str_copyb(aLine, pText);
							str_replace_char_num(aLine, -1, '\n', ' ');

							char aRightPart[256];
							str_copy(aRightPart, LineInput.GetString() + LineInput.GetCursorOffset(), sizeof(aRightPart));
							str_copy(aLine, LineInput.GetString(), min(LineInput.GetCursorOffset() + 1, (int)sizeof(aLine)));
							str_append(aLine, pText, sizeof(aLine));
							str_append(aLine, aRightPart, sizeof(aLine));
							LineInput.Set(aLine);
							LineInput.SetCursorOffset(str_length(aLine) - str_length(aRightPart));
						}
						else if(g_Config.m_Debug)
							dbg_msg("menus", "paste failed: got no text from clipboard");
					}
					else if(Event.m_Key == KEY_C || Event.m_Key == KEY_X) // copy/cut
					{
						Input()->SetClipboardText(LineInput.GetString());
						if(Event.m_Key == KEY_X)
							LineInput.Clear();
					}
					else // handle skipping: jump to spaces and special ASCII characters
					{
						CLineInput::HandleSkipping(Event, &LineInput);
					}

					// apply the changes
					str_copy(pStr, LineInput.GetString(), StrSize);
					s_AtIndex = min((int)StrSize, LineInput.GetCursorOffset());
				}
			}

			// process input
			if(!Handled)
			{
				Len = str_length(pStr);
				int NumChars = Len;
				ReturnValue |= CLineInput::Manipulate(Event, pStr, StrSize, StrSize, &Len, &s_AtIndex, &NumChars);
			}

		}

		// handle scrolling
		if(Inside && UI()->MouseButton(0) && m_pClient->m_pGameConsole->IsClosed())
		{
			s_DoScroll = true;
			s_ScrollStart = UI()->MouseX();
			int MxRel = (int)(UI()->MouseX() - pRect->x);
			float dOffset = 0.0f;

			if(Align == 0)
				dOffset = pRect->w/2.0f-TextRender()->TextWidth(0, FontSize, pStr, -1)/2.0f;
			else if(Align > 0)
				dOffset = pRect->w-TextRender()->TextWidth(0, FontSize, pStr, -1)/2.0f;

			for(int i = 1; i <= Len; i++)
			{
				if(dOffset + TextRender()->TextWidth(0, FontSize, pStr, i) - *pOffset > MxRel)
				{
					s_AtIndex = i - 1;
					break;
				}

				if(i == Len)
					s_AtIndex = Len;
			}
		}
		else if(!UI()->MouseButton(0) || m_pClient->m_pGameConsole->IsClosed())
			s_DoScroll = false;
		else if(s_DoScroll)
		{
			// do scrolling
			if(UI()->MouseX() < pRect->x && s_ScrollStart-UI()->MouseX() > 10.0f)
			{
				s_AtIndex = max(0, s_AtIndex-1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
			else if(UI()->MouseX() > pRect->x+pRect->w && UI()->MouseX()-s_ScrollStart > 10.0f)
			{
				s_AtIndex = min(Len, s_AtIndex+1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
		}
	}

	bool JustGotActive = false;

	if(UI()->ActiveItem() == pBC->GetID())
	{
		if(!UI()->MouseButton(0))
		{
			s_AtIndex = min(s_AtIndex, str_length(pStr));
			s_DoScroll = false;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pBC->GetID())
	{
		if(UI()->MouseButton(0))
		{
			if (UI()->LastActiveItem() != pBC->GetID())
				JustGotActive = true;
			UI()->SetActiveItem(pBC->GetID());
		}
	}

	if(Inside)
	{
		UI()->SetHotItem(pBC->GetID());
#if defined(__ANDROID__)
		if(UI()->ActiveItem() == pBC->GetID() && UI()->MouseButtonClicked(0))
		{
			s_AtIndex = 0;
			UI()->AndroidBlockAndGetTextInput(pStr, StrSize, "");
		}
#endif
	}

	CUIRect Textbox = *pRect;
	RenderTools()->DrawUIRect(&Textbox, vec4(0.0f, 0.0f, 0.0f, 0.25f), Corners, 3.0f);
	Textbox.VMargin(2.0f, &Textbox);
	Textbox.HMargin(2.0f, &Textbox);

	const char *pDisplayStr = pStr;
	char aStars[128];

	if(pDisplayStr[0] == '\0' && pEmptyText)
	{
		pDisplayStr = pEmptyText;
		TextRender()->TextColor(1, 1, 1, 0.75f);
	}

	if(Hidden && !(pEmptyText && *pEmptyText && str_length(pStr) == 0))
	{
		unsigned s = (unsigned int)str_length(pDisplayStr);
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		for(unsigned int i = 0; i < s; ++i)
			aStars[i] = '*';
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	char aInputing[32] = {0};
	if(UI()->HotItem() == pBC->GetID() && Input()->GetIMEState())
	{
		str_copyb(aInputing, pStr);
		const char *pText = Input()->GetIMECandidate();
		if (str_length(pText))
		{
			int NewTextLen = str_length(pText);
			int CharsLeft = StrSize - str_length(aInputing) - 1;
			int FillCharLen = min(NewTextLen, CharsLeft);
			//Push Char Backward
			for(int i = str_length(aInputing); i >= s_AtIndex; i--)
				aInputing[i + FillCharLen] = aInputing[i];
			for(int i = 0; i < FillCharLen; i++)
			{
				if(pText[i] == '\n')
					aInputing[s_AtIndex + i] = ' ';
				else
					aInputing[s_AtIndex + i] = pText[i];
			}
			//s_AtIndex = s_AtIndex+FillCharLen;
			pDisplayStr = aInputing;
		}
	}

	// check if the text has to be moved
	if(UI()->LastActiveItem() == pBC->GetID() && !JustGotActive && (UpdateOffset || m_NumInputEvents))
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		if(w-*pOffset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(0, FontSize, pDisplayStr);
			do
			{
				*pOffset += min(wt-*pOffset-Textbox.w, Textbox.w/3.0f);
			}
			while(w-*pOffset > Textbox.w);
		}
		else if(w-*pOffset < 0.0f)
		{
			// move to the right
			do
			{
				*pOffset = max(0.0f, *pOffset-Textbox.w/3.0f);
			}
			while(w-*pOffset < 0.0f);
		}
	}
	UI()->ClipEnable(pRect);
	Textbox.x -= *pOffset;

	UI()->DoLabel(&Textbox, pDisplayStr, FontSize, Align);

	TextRender()->TextColor(1, 1, 1, 1);

	// render the cursor
	if(UI()->LastActiveItem() == pBC->GetID() && !JustGotActive)
	{
		const float cw = TextRender()->TextWidth(0, FontSize, "|")/2;

		// wtf is this good for?
		if (str_length(aInputing))
		{
			float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex + Input()->GetEditingCursor());
			Textbox = *pRect;
			Textbox.VSplitLeft(2.0f, 0, &Textbox);
			Textbox.x += (w-*pOffset-cw);

			UI()->DoLabel(&Textbox, "|", FontSize, CUI::ALIGN_LEFT); // TODO: the alignment here might not be correct (like this entire block ._.)
		}

		const float tw = TextRender()->TextWidth(0, FontSize, pDisplayStr);
		const float twTillCursor = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		Textbox = *pRect;
		Textbox.x += twTillCursor;
		Textbox.x -= cw;
		Textbox.x -= *pOffset;
		if(Align == CUI::ALIGN_CENTER)
		{
			// move it into the middle
			Textbox.x += Textbox.w/2.0f - tw/2.0f;
		}
		else if(Align == CUI::ALIGN_RIGHT)
		{
			// move it to the right and add a little margin
			Textbox.VSplitRight(2.0f, &Textbox, 0);
			Textbox.x += Textbox.w - tw;
		}
		else //if(Align == CUI::ALIGN_LEFT)
		{
			// a little margin from the left
			Textbox.VSplitLeft(2.0f, 0, &Textbox);
		}

		// make it blink
		if((2*time_get()/time_freq()) % 2)
			UI()->DoLabel(&Textbox, "|", FontSize, CUI::ALIGN_LEFT); // the correct alignment is being calculated above
	}
	UI()->ClipDisable();

	if(Inside && pTooltip && pTooltip[0] != '\0')
		m_pClient->m_pTooltip->SetTooltip(pTooltip);


	return ReturnValue;
}

int CMenus::DoEditBoxLua(lua::CEditboxContainer *pBC, const CUIRect *pRect, float FontSize, bool Hidden, int Corners, const char *pEmptyText, const char *pTooltip)
{
	static char *pBuffer = NULL;
	static unsigned int BufferSize = 0;
	const unsigned int MAX_STR_LEN = max(16U, (unsigned int)(pBC->GetString().length() * 2)); // always go with
	if(!pBuffer || BufferSize < MAX_STR_LEN)
	{
		mem_free(pBuffer);
		pBuffer = mem_allocb(char, MAX_STR_LEN);
		BufferSize = MAX_STR_LEN;
	}
	str_copy(pBuffer, pBC->GetString().c_str(), MAX_STR_LEN);

	//dbg_msg("DBG!", "pBuffer before (SIZE=%i, CAN INPUT=%x): '%s'", MAX_STR_LEN, Input()->GetIMEState(), pBuffer);
	int result = DoEditBox(pBC, pRect, pBuffer, (unsigned)MAX_STR_LEN, FontSize, &pBC->m_Offset, Hidden, Corners, pEmptyText, 0, pTooltip);
	//dbg_msg("DBG!", "pBuffer danach (RSLT=%i): '%s'", result, pBuffer);

	// copy it back
	pBC->SetString(std::string(pBuffer));
	return result;
}

float CMenus::DoScrollbarV(CButtonContainer *pBC, const CUIRect *pRect, float Current, const char *pTooltip, int Value, int LenPercent)
{
	CALLSTACK_ADD();

	CUIRect Handle;
	static float OffsetY;

	{
#if defined(__ANDROID__)
		const float FixedLen = 50.0f;
#else
		const float FixedLen = 33.0f;
#endif

		// calculate the handle length
		float Len = FixedLen;
		if(LenPercent != ~0)
		{
			LenPercent = clamp(LenPercent, 0, 100);
			Len = clamp(pRect->h * ((float)LenPercent / 100.0f),
						FixedLen/2.0f,
						pRect->h-1.0f);
		}
		pRect->HSplitTop(Len, &Handle, 0);
	}

	Handle.y += (pRect->h-Handle.h)*Current;

	// logic
	float ReturnValue = Current;
	int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pBC->GetID())
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);

		if(KeyMods(KEYMOD_SHIFT))
			m_MouseSlow = true;

		float Min = pRect->y;
		float Max = pRect->h-Handle.h;
		float Cur = UI()->MouseY()-OffsetY;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pBC->GetID())
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pBC->GetID());
			OffsetY = UI()->MouseY()-Handle.y;
		}
	}

	if(Inside)
		UI()->SetHotItem(pBC->GetID());

	// render
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	if(Inside)
	{
		Slider.w = Rail.x-Slider.x;
		RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_L, 2.5f);
		Slider.x = Rail.x+Rail.w;
		RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_R, 2.5f);
	}

	Slider = Handle;
	Slider.VMargin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.3f)*ButtonColorMul(pBC), 0, 2.5f);


	if(Inside && pTooltip && pTooltip[0] != '\0')
		m_pClient->m_pTooltip->SetTooltip(pTooltip);

	if(UI()->MouseInside(&Rail) || Inside)
	{
		if(KeyEvent(KEY_MOUSE_WHEEL_UP))
			ReturnValue -= Input()->KeyPress(KEY_LSHIFT) ? 0.01f : 0.05f;
		else if(KeyEvent(KEY_MOUSE_WHEEL_DOWN))
			ReturnValue += Input()->KeyPress(KEY_LSHIFT) ? 0.01f : 0.05f;
	}

	return clamp(ReturnValue, 0.0f, 1.0f);
}

float CMenus::DoScrollbarH(CButtonContainer *pBC, const CUIRect *pRect, float Current, const char *pTooltip, int Value, int LenPercent)
{
	CALLSTACK_ADD();

	CUIRect Handle;
	static float OffsetX;

	{
#if defined(__ANDROID__)
		const float FixedLen = 50.0f;
#else
		const float FixedLen = 33.0f;
#endif

		// calculate the handle length
		float Len = FixedLen;
		if(LenPercent != ~0)
		{
			LenPercent = clamp(LenPercent, 0, 100);
			Len = clamp(pRect->w * ((float)LenPercent / 100.0f),
						FixedLen/2.0f,
						pRect->w-1.0f);
		}
		pRect->VSplitLeft(Len, &Handle, 0);
	}

	Handle.x += (pRect->w-Handle.w)*Current;

	// logic
	float ReturnValue = Current;
	int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pBC->GetID())
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);

		if(KeyMods(KEYMOD_SHIFT))
			m_MouseSlow = true;

		float Min = pRect->x;
		float Max = pRect->w-Handle.w;
		float Cur = UI()->MouseX()-OffsetX;
		ReturnValue = (Cur-Min)/Max;
	}
	else if(UI()->HotItem() == pBC->GetID())
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pBC->GetID());
			OffsetX = UI()->MouseX()-Handle.x;
		}
	}

	if(Inside)
		UI()->SetHotItem(pBC->GetID());

	// render
	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	if(Value == ~0)
	{
		CUIRect Slider = Handle;
		if(Inside || (UI()->HotItem() == pBC->GetID() && UI()->MouseButton(0)) || UI()->ActiveItem() == pBC->GetID())
		{
			Slider.h = Rail.y-Slider.y;
			RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_T, 2.5f);
			Slider.y = Rail.y+Rail.h;
			RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_B, 2.5f);
		}

		Slider = Handle;
		Slider.HMargin(5.0f, &Slider);
		RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pBC), 0, 2.5f);
	}
	else
	{
		CUIRect Slider = Handle;
		if(Inside || (UI()->HotItem() == pBC->GetID() && UI()->MouseButton(0)) || UI()->ActiveItem() == pBC->GetID())
		{
			Slider.h = 2*(Rail.y-Slider.y)+Rail.h;
			Slider.Margin(-3.0f, &Slider);
			RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_ALL, 9.5f);
			Slider.Margin(3.0f, &Slider);
			RenderTools()->DrawUIRect(&Slider, vec4(0,0,0,0.80f*clamp(ButtonColorMul(pBC).a, 0.8f, 1.2f)), CUI::CORNER_ALL, 7.3f);
			char aBuf[16];
			str_format(aBuf, sizeof(aBuf), "%i", Value);
			Slider.y = Rail.y-8.5f/4.0f;
			UI()->DoLabel(&Slider, aBuf, 8.5f, 0);
		}
		else
		{
			Slider = Handle;
			Slider.HMargin(5.0f, &Slider);
			RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.3f), 0, 2.5f);
		}
	}


	if(Inside && pTooltip && pTooltip[0] != '\0')
		m_pClient->m_pTooltip->SetTooltip(pTooltip);

	if(UI()->MouseInside(&Rail) || Inside)
	{
		if(KeyEvent(KEY_MOUSE_WHEEL_UP))
			ReturnValue += Input()->KeyPress(KEY_LSHIFT) ? 0.01f : 0.05f;
		else if(KeyEvent(KEY_MOUSE_WHEEL_DOWN))
			ReturnValue -= Input()->KeyPress(KEY_LSHIFT) ? 0.01f : 0.05f;
	}

	return clamp(ReturnValue, 0.0f, 1.0f);
}

int CMenus::DoScrollbarIntSelect(CButtonContainer *pBC, const CUIRect *pRect, int *pCurrent, int Min, int Max, const char *pTooltip)
{
	return *pCurrent = round_to_int(map_val(
			DoScrollbarH(pBC, pRect, map_val(*pCurrent, Min, Max, 0.0f, 1.0f), pTooltip, *pCurrent),
			0.0f, 1.0f,
			Min, Max
	));
}


int CMenus::DoKeyReader(CButtonContainer *pBC, const CUIRect *pRect, int Key, const char *pTooltip)
{
	CALLSTACK_ADD();

	// process
	static const void *pGrabbedID = 0;
	static bool MouseReleased = true;
	static int ButtonUsed = 0;
	int Inside = UI()->MouseInside(pRect);
	int NewKey = Key;

	if(!UI()->MouseButton(0) && !UI()->MouseButton(1) && pGrabbedID == pBC->GetID())
		MouseReleased = true;

	if(UI()->ActiveItem() == pBC->GetID())
	{
		if(m_Binder.m_GotKey)
		{
			// abort with escape key
			if(m_Binder.m_Key.m_Key != KEY_ESCAPE)
				NewKey = m_Binder.m_Key.m_Key;
			m_Binder.m_GotKey = false;
			UI()->SetActiveItem(0);
			MouseReleased = false;
			pGrabbedID = pBC->GetID();
		}

		if(ButtonUsed == 1 && !UI()->MouseButton(1))
		{
			if(Inside)
				NewKey = 0;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pBC->GetID())
	{
		if(MouseReleased)
		{
			if(UI()->MouseButton(0))
			{
				m_Binder.m_TakeKey = true;
				m_Binder.m_GotKey = false;
				UI()->SetActiveItem(pBC->GetID());
				ButtonUsed = 0;
			}

			if(UI()->MouseButton(1))
			{
				UI()->SetActiveItem(pBC->GetID());
				ButtonUsed = 1;
			}
		}
	}

	if(Inside)
		UI()->SetHotItem(pBC->GetID());

	// draw
	if (UI()->ActiveItem() == pBC->GetID() && ButtonUsed == 0)
		DoButton_KeySelect(pBC, "???", 0, pRect, pTooltip);
	else
	{
		if(Key == 0)
			DoButton_KeySelect(pBC, "", 0, pRect, pTooltip);
		else
			DoButton_KeySelect(pBC, Input()->KeyName(Key), 0, pRect, pTooltip);
	}
	return NewKey;
}

int CMenus::DoColorPicker(const CButtonContainer *pBC1, const CButtonContainer *pBC2, const CUIRect *pView, vec3 *pColorHSV)
{
	CUIRect SVPicker, HuePicker;

	RenderTools()->DrawUIRect(pView, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 5.0f);

	{
		CUIRect View = *pView;
		View.Margin(5.0f, &View);
		View.VSplitRight(20.0f, &SVPicker, &HuePicker);
		HuePicker.VSplitLeft(4.0f, 0x0, &HuePicker);
	}


	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();

	// base: white - hue
	vec3 hsv = *pColorHSV;
	IGraphics::CColorVertex ColorArray[4];

	vec3 c;
	c = HsvToRgb(vec3(hsv.h, 0.0f, 1.0f));
	ColorArray[0] = IGraphics::CColorVertex(0, c.r, c.g, c.b, 1.0f); // TL
	c = HsvToRgb(vec3(hsv.h, 1.0f, 1.0f));
	ColorArray[1] = IGraphics::CColorVertex(1, c.r, c.g, c.b, 1.0f); // TR
	c = HsvToRgb(vec3(hsv.h, 1.0f, 1.0f));
	ColorArray[2] = IGraphics::CColorVertex(2, c.r, c.g, c.b, 1.0f); // BR
	c = HsvToRgb(vec3(hsv.h, 0.0f, 1.0f));
	ColorArray[3] = IGraphics::CColorVertex(3, c.r, c.g, c.b, 1.0f); // BL

	Graphics()->SetColorVertex(ColorArray, 4);

	IGraphics::CQuadItem QuadItem(SVPicker.x, SVPicker.y, SVPicker.w, SVPicker.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);

	// base: transparent - black
	ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
	ColorArray[1] = IGraphics::CColorVertex(1, 0.0f, 0.0f, 0.0f, 0.0f);
	ColorArray[2] = IGraphics::CColorVertex(2, 0.0f, 0.0f, 0.0f, 1.0f);
	ColorArray[3] = IGraphics::CColorVertex(3, 0.0f, 0.0f, 0.0f, 1.0f);

	Graphics()->SetColorVertex(ColorArray, 4);

	Graphics()->QuadsDrawTL(&QuadItem, 1);

	Graphics()->QuadsEnd();

	// marker
	vec2 Marker = vec2(vec2(hsv.y*UI()->Scale(), (1.0f - hsv.z)*UI()->Scale()).x * vec2(SVPicker.w, SVPicker.h).x,
					   vec2(hsv.y*UI()->Scale(), (1.0f - hsv.z)*UI()->Scale()).y * vec2(SVPicker.w, SVPicker.h).y);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
	IGraphics::CQuadItem aMarker[2];
	aMarker[0] = IGraphics::CQuadItem(SVPicker.x+Marker.x, SVPicker.y+Marker.y - 5.0f*UI()->PixelSize(), UI()->PixelSize(), 11.0f*UI()->PixelSize());
	aMarker[1] = IGraphics::CQuadItem(SVPicker.x+Marker.x - 5.0f*UI()->PixelSize(), SVPicker.y+Marker.y, 11.0f*UI()->PixelSize(), UI()->PixelSize());
	Graphics()->QuadsDrawTL(aMarker, 2);
	Graphics()->QuadsEnd();

	// logic
	float X, Y;
	if(UI()->DoPickerLogic(pBC1->GetID(), &SVPicker, &X, &Y))
	{
		hsv.s = X/SVPicker.w;
		hsv.v = 1.0f - Y/SVPicker.h;
	}

	// hue slider
	static const float s_aColorIndices[7][3] = {
			{1.0f, 0.0f, 0.0f}, // red
			{1.0f, 0.0f, 1.0f},	// magenta
			{0.0f, 0.0f, 1.0f}, // blue
			{0.0f, 1.0f, 1.0f}, // cyan
			{0.0f, 1.0f, 0.0f}, // green
			{1.0f, 1.0f, 0.0f}, // yellow
			{1.0f, 0.0f, 0.0f}  // red
	};

	Graphics()->QuadsBegin();
	vec4 ColorTop, ColorBottom;
	float Offset = HuePicker.h/6.0f;
	for(int j = 0; j < 6; j++)
	{
		ColorTop = vec4(s_aColorIndices[j][0], s_aColorIndices[j][1], s_aColorIndices[j][2], 1.0f);
		ColorBottom = vec4(s_aColorIndices[j+1][0], s_aColorIndices[j+1][1], s_aColorIndices[j+1][2], 1.0f);

		ColorArray[0] = IGraphics::CColorVertex(0, ColorTop.r, ColorTop.g, ColorTop.b, ColorTop.a);
		ColorArray[1] = IGraphics::CColorVertex(1, ColorTop.r, ColorTop.g, ColorTop.b, ColorTop.a);
		ColorArray[2] = IGraphics::CColorVertex(2, ColorBottom.r, ColorBottom.g, ColorBottom.b, ColorBottom.a);
		ColorArray[3] = IGraphics::CColorVertex(3, ColorBottom.r, ColorBottom.g, ColorBottom.b, ColorBottom.a);
		Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem QuadItem(HuePicker.x, HuePicker.y+Offset*j, HuePicker.w, Offset);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}

	// marker
	Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
	IGraphics::CQuadItem QuadItemMarker(HuePicker.x, HuePicker.y + (1.0f - hsv.x) * HuePicker.h * UI()->Scale(), HuePicker.w, UI()->PixelSize());
	Graphics()->QuadsDrawTL(&QuadItemMarker, 1);

	Graphics()->QuadsEnd();

	if(UI()->DoPickerLogic(pBC2->GetID(), &HuePicker, &X, &Y))
	{
		hsv.x = 1.0f - Y/HuePicker.h;
	}

	int Changed = hsv != *pColorHSV;
	*pColorHSV = hsv;

	return Changed;
}



int CMenus::RenderMenubar(CUIRect r)
{
	CALLSTACK_ADD();

	CUIRect Box = r;
	CUIRect Button;

	m_ActivePage = g_Config.m_UiPage;
	int NewPage = -1;

	if(Client()->State() != IClient::STATE_OFFLINE) // hack
		m_ActivePage = m_GamePage;

	if(Client()->State() == IClient::STATE_OFFLINE)
	{
		// offline menus
		Box.VSplitLeft(90.0f, &Button, &Box);
		static CButtonContainer s_NewsButton;
		if (DoButton_MenuTab(&s_NewsButton, Localize("News"), m_ActivePage==PAGE_NEWS_ATH || m_ActivePage==PAGE_NEWS_DDNET, &Button, CUI::CORNER_T))
		{
			NewPage = PAGE_NEWS_ATH;
			m_DoubleClickIndex = -1;
		}
		Box.VSplitLeft(10.0f, 0, &Box);

		Box.VSplitLeft(100.0f, &Button, &Box);
		static CButtonContainer s_InternetButton;
		if(DoButton_MenuTab(&s_InternetButton, Localize("Browser"), m_ActivePage==PAGE_BROWSER, &Button, CUI::CORNER_T))
		{
			if(g_Config.m_UiBrowserPage == PAGE_BROWSER_INTERNET)
			{
				if(ServerBrowser()->GetCurrentType() != IServerBrowser::TYPE_INTERNET)
				{
					if(g_Config.m_BrAutoCache && ServerBrowser()->CacheExists())
						ServerBrowser()->LoadCache();
					else
						ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
				}
			}
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_LAN)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_FAVORITES)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_DDNET)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_DDNET);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_RECENT)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_RECENT);

			NewPage = PAGE_BROWSER;
			m_DoubleClickIndex = -1;
		}

		Box.VSplitLeft(10.0f, 0, &Box);
		Box.VSplitLeft(100.0f, &Button, &Box);
		static CButtonContainer s_DemosButton;
		if(DoButton_MenuTab(&s_DemosButton, Localize("Demos"), m_ActivePage==PAGE_DEMOS, &Button, CUI::CORNER_T))
		{
			DemolistPopulate();
			NewPage = PAGE_DEMOS;
			m_DoubleClickIndex = -1;
		}
	}
	else
	{
		// online menus
		Box.VSplitLeft(90.0f, &Button, &Box);
		static CButtonContainer s_GameButton;
		if(DoButton_MenuTab(&s_GameButton, Localize("Game"), m_ActivePage==PAGE_GAME, &Button, CUI::CORNER_TL))
			NewPage = PAGE_GAME;

		Box.VSplitLeft(90.0f, &Button, &Box);
		static CButtonContainer s_PlayersButton;
		if(DoButton_MenuTab(&s_PlayersButton, Localize("Players"), m_ActivePage==PAGE_PLAYERS, &Button, 0))
			NewPage = PAGE_PLAYERS;

		Box.VSplitLeft(130.0f, &Button, &Box);
		static CButtonContainer s_ServerInfoButton;
		if(DoButton_MenuTab(&s_ServerInfoButton, Localize("Server info"), m_ActivePage==PAGE_SERVER_INFO, &Button, 0))
			NewPage = PAGE_SERVER_INFO;

		Box.VSplitLeft(100.0f, &Button, &Box);
		static CButtonContainer s_CallVoteButton;
		if(DoButton_MenuTab(&s_CallVoteButton, Localize("Call vote"), m_ActivePage==PAGE_CALLVOTE, &Button, CUI::CORNER_TR))
			NewPage = PAGE_CALLVOTE;

		Box.VSplitLeft(30.0f, &Button, &Box);
		Box.VSplitLeft(100.0f, &Button, &Box);
		static CButtonContainer s_BrowserButton; int BrowserCorners = CUI::CORNER_T;
#if defined(CONF_SPOOFING)
		BrowserCorners = CUI::CORNER_TL;
#endif
		if(DoButton_MenuTab(&s_BrowserButton, Localize("Browser"), m_ActivePage==PAGE_BROWSER, &Button, BrowserCorners))
			NewPage = PAGE_BROWSER;
#if defined(CONF_SPOOFING)
		Box.VSplitLeft(100.0f, &Button, &Box);
		Box.VSplitLeft(4.0f, 0, &Box);
		static CButtonContainer s_SpoofingButton;
		if(DoButton_MenuTab(&s_SpoofingButton, Localize("Spoofing"), m_ActivePage==PAGE_SPOOFING, &Button, CUI::CORNER_TR))
			NewPage = PAGE_SPOOFING;
#endif
	}

	/*
	box.VSplitRight(110.0f, &box, &button);
	static int system_button=0;
	if (UI()->DoButton(&system_button, "System", g_Config.m_UiPage==PAGE_SYSTEM, &button))
		g_Config.m_UiPage = PAGE_SYSTEM;

	box.VSplitRight(30.0f, &box, 0);
	*/
	char aBuf[64];
#define PREPARE_BUTTON(SYMBOL, LABEL) \
		str_format(aBuf, sizeof(aBuf), SYMBOL " %s", LABEL); \
		Box.VSplitRight(clamp(TextRender()->TextWidth(0, Box.h, aBuf, str_length(aBuf)), 90.0f, 150.0f), &Box, &Button);

	{
		PREPARE_BUTTON("×", Localize("Quit"))
		static CButtonContainer s_QuitButton;
		if(DoButton_MenuTab(&s_QuitButton, aBuf, 0, &Button, CUI::CORNER_TR))
			m_Popup = POPUP_QUIT;
	}

	//Box.VSplitRight(10.0f, &Box, &Button);
	{
		PREPARE_BUTTON("⚙", Localize("Settings"))
		static CButtonContainer s_SettingsButton;
		if(DoButton_MenuTab(&s_SettingsButton, aBuf, m_ActivePage == PAGE_SETTINGS, &Button, 0))
		{
			NewPage = PAGE_SETTINGS;
		}
	}

	//Box.VSplitRight(10.0f, &Box, &Button);
	{
		PREPARE_BUTTON("ⅈ", Localize("Manual"))
		static CButtonContainer s_InfoButton;
		if(DoButton_MenuTab(&s_InfoButton, aBuf, m_ActivePage == PAGE_MANUAL, &Button, Client()->State() == IClient::STATE_OFFLINE ? 0 : CUI::CORNER_TL))
			NewPage = PAGE_MANUAL;
	}

	if(Client()->State() == IClient::STATE_OFFLINE)
	{
		//Box.VSplitRight(10.0f, &Box, &Button);
		{
			PREPARE_BUTTON("⬀", Localize("Chat"))
			static CButtonContainer s_ChatButton;
			if(DoButton_MenuTab(&s_ChatButton, aBuf, m_IRCActive, &Button, 0))
				ToggleIRC();
		}

		//Box.VSplitRight(10.0f, &Box, &Button);
		{
			PREPARE_BUTTON("✎", Localize("Editor"))
			static CButtonContainer s_EditorButton;
			if(DoButton_MenuTab(&s_EditorButton, aBuf, g_Config.m_ClEditor, &Button, CUI::CORNER_TL))
				g_Config.m_ClEditor = 1;
		}
	}

	if(NewPage != -1)
	{
		if(Client()->State() == IClient::STATE_OFFLINE)
			g_Config.m_UiPage = NewPage;
		else
			m_GamePage = NewPage;
	}

	return 0;
}

float CMenus::DoDropdownMenu(CButtonContainer *pBC, const CUIRect *pRect, const char *pStr, float HeaderHeight, FDropdownCallback pfnCallback, void *pArgs, const char *pTooltip)
{
	CALLSTACK_ADD();

	CUIRect View = *pRect;
	CUIRect Header, Label;

	bool Active = pBC->GetID() == m_pActiveDropdown;
	int Corners = Active ? CUI::CORNER_T : CUI::CORNER_ALL;

	View.HSplitTop(HeaderHeight, &Header, &View);

	// background
	RenderTools()->DrawUIRect(&Header, vec4(0.0f, 0.0f, 0.0f, 0.25f), Corners, 5.0f);

	// render icon
	CUIRect Button;
	Header.VSplitLeft(Header.h, &Button, 0);
	Button.Margin(2.0f, &Button);
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_MENUICONS].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, UI()->HotItem() == pBC->GetID() ? 1.0f : 0.6f);
	if(Active)
		RenderTools()->SelectSprite(SPRITE_MENU_EXPANDED);
	else
		RenderTools()->SelectSprite(SPRITE_MENU_COLLAPSED);
	IGraphics::CQuadItem QuadItem(Button.x, Button.y, Button.w, Button.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	Graphics()->QuadsEnd();

	// label
	Label = Header;
	Label.y += 2.0f;
	UI()->DoLabel(&Label, pStr, Header.h*ms_FontmodHeight*0.8f, 0);

	if(UI()->DoButtonLogic(pBC->GetID(), 0, 0, &Header))
	{
		if(Active)
			m_pActiveDropdown = 0;
		else
			m_pActiveDropdown = (int*)pBC->GetID();
	}

	// tooltip
	if(UI()->HotItem() == pBC->GetID() && pTooltip && pTooltip[0] != '\0')
		m_pClient->m_pTooltip->SetTooltip(pTooltip);

	// render content of expanded menu
	if(Active)
		return HeaderHeight + pfnCallback(View, this, pArgs);

	return HeaderHeight;
}

// TODO: Add more! :D
static const char *s_apSayings[] = {
		"The client with the keck!"
		,"Project codename: D.I.C.K.E.R. D.E.N.N.I.S"
		,"KAT ZE! :3"
		,"You can keep bugs you find any!"
		,"Random splash!"
		,"LUABRIDGE!"
		,"Low on calories!"
		,"DENNIS!"
		,"KECK!"
		,"1338 - The Next Level"
		,"1338 - Off By One"
		,"1338 - Close Enough"
		,"1338 - Gone Too Far"
		,"1338 - One Step Ahead"
		,"1338 - Crossing Boarders"
		,"1338 - Eine Idee weiter"
		,"0x53A-0x539=0x1"
		,"Waiting for 0.7!"
		,"YAA, SPAß IM LEBEN!"
		,"Dennis * dennis = new Dennis()"
		,"Definitely legal!"
		,"Lua goes OOP!"
		,"SMOOTH!"
		,"This game was named 'Teewars'"
		,"long time ago..."
		,"Containing free melon-power!"
		,"Jump the gun!"
		,"øıŋĸ?"
		,"§1C§2o§3l§4o§5r§6m§7a§8t§9i§ac"
		,"Have fun!"
		,"What is a floppy disk...?"
		,"Do you expect Haxx?"
		,"!!!!11eleven"
		,"Don't be salty!"
		,"This text is stored in static const char *s_apSayings[]"
		,"Rainbows are awesome"
		,"Blue is the color of life...!"
		,"Melons can be green, yellow or a hat!"
		,"Sublime!"
		,"Try toggle_hotbar!"
		,"Try toggle_irc!"
		,"- allthehaxx.github.io -"
		,"beta no more!"
		,"The crasher got removed :c"
		,"100+ commits per week!"
		,GAME_NETVERSION
		,"Built on " BUILD_DATE
		,"Version " ALLTHEHAXX_VERSION
		,"Searching for ideas..."
		,"(mooning) (sheep) (dog)"
		,".calc 2^10*5e3"
		,"Tequila!!! DODO DODODO DODO DO!"
		,"They will not force us!"
		,"Got many splashes, pfew"
		,"Seperated by a comma."
		,"::.. xx-xx-xx ..::"
		,"Do not count 'em ;)"
		,"Guaranteed to be 420% verkeckt!"
		,"CI Status: PASSED"
		,"rm -rf /"
		,"Markdown supported"
		,"You can click links!"
		,"Keep it up-to-date all the time"
		,"Join the chat!"
		,"No forum"
		,"It has to be teeish"
		,"Don't quote liars!"
		,"(4N Y0U R34D 7H15??"
		,"Going out of splash texts...."
		,"Life is strange :o"
		,"¿¡¿¡¿¡¿¡¿¡¿¡¿¡¿¡"
		,"...REISUB!"
		,"WO\xD0\xAFK\xC6\xA7 FO\xD0\xAF M\xC6\x8E"
		,"Do you get the insiders...? :D"
		,"Ask if you need something!"
		,"Brain.exe has stopped working"
		,"Read error: Could not access data at address 0x00000000"
		,"Do you play an instrument?"
		,"Cucumber ya mei lord, cucumber ya!"
		,"git rebase -i HEAD~~"
		,"Multicoloral!"
		,"Localize(\"%s\")"
		,"func = assert(load(\"return \" .. x))"
		,"RegisterEvent(\"OnIdle\", \"fap\")"
		,"Grr Java ate my brain"
		,"May contain traces of peanuts."
		,"It\x0A is\x0A  line\x0A   break\x0A    ing!"
		,"146 ist der Boss."
		,"Würselen city, boys!"
		,"Meet me at Internetcaffee 52146."
		,"Caitlyn Jenner is stunning and beautiful."
		,"Der Junge ist jetzt einer von uns."
		,"Chef's Chocolate Salty Balls"
		,"Fat Camp"
		,"Spoiler: Kenny dies!"
		,"There are more stars in the sky than there are atoms in the universe"
		,"\"Use the force, Harry!\" - Gandalf"
		,"\"Quotations on the internet are often attributed incorrectly.\" - Abraham Lincoln"
		,"\"Bitches ain't shit but hoes and tricks.\" - Ghandi"
		,"You miss 100% of the shots you don't take"
		,"Taking shots without bullets"
		,"I'm too drunk to taste this chicken."
		,"Panamera"
		,"Hercules Prima 5"
		,"Pirate on the weekend!"
		,"Why did the chicken cross the road?"
		,"Are you even trying?"
		,"What if everyone jumped at once?"
		,"What color is a mirror?"
		,"What if the moon was a disco ball?"
		,"Is your red the same red as my red?"
		,"Is anything real?"
		,"What is the speed of dark?"
		,"What is random?"
		,"Is the 5-second rule actually true?"
		,"How much does Thor's hammer weigh?"
		,"How much does a shadow weigh?"
		,"ReD go TW!!!"
		,"At least I got me for life"
		,"SIDEKICKS!"
		,"Drunk driving is okay if you buckle up."
		,"If you get kicked out of the bars just hit the boulevard."
		,"Flipping balisong..."
		,"Shoutout to my son Fruchti!"
		,"Random != Random"
		,"The smooth loading screen isn't threadsafe :("
		,"Shaders would be awesome!"
		,"SDL2 <3"
		,"8th May 2016 – 100 issues closed! (27 to go)"
		,"Computers don't byte!"
		,"git stash pop stash@{0}"
		,"Splashtexts for teh lulz ^^"
		,"Was he drunk...?"
		,"...no."
		,"What is a 'Neologism' ??"
		,"How could you know that?!"
		,"The spelling is 'whether', NOT 'wether'!!!"
		,"\"Zwei Russen sind besser als einer\""
		,"[20:49:00] Prinzessin Meskalin: ReD + 11 min = rip"
		,"[21:00:00] ReD: auutsch"
		,"\"Red nur du ReD\""
		,"BAUM!"
		,"epppiiiiccc"
		,"Gimme dat muzik!"
		,"...it's evolving!"
		,"When will the Aliens find us?"
		,"Gotta get more sleep :("
		,"We furfill every wish!"
		,"I hate fucking script kiddies"
		,"Don't be rude!"
		,"Nobody needs nothing"
		,"[IRC] received a CTCP FINGER"
		,"xoring you out!"
		,"Text formatting would be cool"
		,"Do penguins have knees?"
		,"Did you know: You can use CTRL+F in the consoles to search for text"
		,"Did you know: In the consoles, you can mark text to copy it"
		,"\"I'm in love with my lust, burning angel wings to dust!\""
		,"\"A square doesn't fit the circle\" ¯\\_(°-^)_/¯"
		,"The cake is a lie!"
		,"Happy tears fade fast, Sad tears tend to last"
		,"No, the cake is not a lie!"
		,"Tell me, Mechanist, is it true?"
		,"Remember: the Weighted Companion Cube cannot talk!"
		,"We are here to drink your beer!"
		,"If it ain't broken, break it!"
};

void CMenus::RenderLoading()
{
	CALLSTACK_ADD();

	float Percent = m_LoadCurrent++/(float)m_LoadTotal;

	// need up date this here to get correct
	vec3 Rgb = HsvToRgb(vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorVal/255.0f));
	ms_GuiColor = vec4(Rgb.r, Rgb.g, Rgb.b, g_Config.m_UiColorAlpha/255.0f);

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	RenderBackground();

	float w = Screen.w;
	float h = 200;
	float x = 0;
	float y = Screen.h/2-h/2;

	Graphics()->BlendNormal();

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.50f);
	RenderTools()->DrawRoundRect(x, y, w, h, 0.0f);
	Graphics()->QuadsEnd();

#define MAGIC_NUMBER 51
#define X(N) ((N)^(MAGIC_NUMBER))
	const char aProtCaption[] = {
			X(76), X(111), X(97), X(100), X(105), X(110), X(103), X(32),
			X(65), X(108), X(108), X(84), X(104), X(101), X(72), X(97), X(120), X(120),
			0
	};
#undef X

	static char aCaption[sizeof(aProtCaption)] = {0};
	if(aCaption[0] == 0)
	{
		mem_copy(aCaption, aProtCaption, sizeof(aCaption));
		for(int i = 0; aCaption[i] != 0; i++)
		{
			aCaption[i] ^= MAGIC_NUMBER;
		}
	}
#undef MAGIC_NUMBER

	CUIRect r;
	r.x = x;
	r.y = y+20;
	r.w = w;
	r.h = h;
	UI()->DoLabel(&r, aCaption, 48.0f, 0, -1);

	if(!g_Config.m_ClSimpleLoadingScreen)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0, 0, 0, 0.50f);
		RenderTools()->DrawRoundRect(x + 40, y + h - 75, w - 80, 25, 5.0f);
		Graphics()->SetColor(1, 1, 1, 0.50f);
		RenderTools()->DrawRoundRect(x + 40, y + h - 75, (w - 80) * Percent, 25, 5.0f);
		Graphics()->QuadsEnd();


		CUIRect l;
		l.x = x;
		l.y = y + 97;
		l.w = w;
		l.h = h;

		UI()->DoLabel(&l, m_aLoadLabel, 12.0f, 0, -1);

		char aBuf[16];

		str_format(aBuf, sizeof(aBuf), "%.2f%%", Percent * 100.0f);

		CUIRect p;

		p.x = x;
		p.y = y + 125;
		p.w = w;
		p.h = h;

		UI()->DoLabel(&p, aBuf, 16.0f, 0, -1);
	}

	CUIRect c;
	c.x = x;
	c.y = y + (g_Config.m_ClSimpleLoadingScreen ? 130 : 160);
	c.w = w;
	c.h = h;

	srand((unsigned)time_get()&0x6BEEFB0B&rand());
	size_t n = rand()%(sizeof(s_apSayings)/sizeof(s_apSayings[0]));
	static const char *pSaying;
	if(!pSaying)
		pSaying = (char*)s_apSayings[n];

	{
		time_t rawtime;
		struct tm* timeinfo;

		time ( &rawtime );
		timeinfo = localtime ( &rawtime );

		// for teh lulz
		#define ISDATE(D, M) (timeinfo->tm_mday == (D) && timeinfo->tm_mon == (M-1))
		if(ISDATE(20, 12))
			pSaying = "Happy Birthday, xush' :D (December 20th)";
		else if(ISDATE(16, 10))
			pSaying = "Happy Birthday, Henritees :D (October 16th)";
		else if(ISDATE(24, 10))
			pSaying = "Happy Birthday, FuroS :D (October 24th)";
		else if(ISDATE(1, 1))
			pSaying = "Happy new Year!";
		else if(ISDATE(24, 12) || ISDATE(25, 12))
			pSaying = "Merry Christmas!";
		#undef ISDATE
	}

	UI()->DoLabel(&c, pSaying, 22.0f, 0, -1);

	// swap only twice if we are using the simple loading screen
	if(g_Config.m_ClSimpleLoadingScreen)
	{
		static int s_NumIterations = 0;
		if(s_NumIterations > 2)
			return;
		s_NumIterations++;
	}

	Graphics()->Swap();

}

void CMenus::RenderNews(CUIRect MainView)
{
	CALLSTACK_ADD();

	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_ALL, 10.0f);

	// tabbar
	{
		CUIRect Tab1, Tab2;
		MainView.HSplitTop(20.0f, &Tab1, &MainView);
		Tab1.VSplitMid(&Tab1, &Tab2);
		static CButtonContainer s_ButtonNewsATH;
		if(DoButton_MenuTab(&s_ButtonNewsATH, "AllTheHaxx News", g_Config.m_UiPage == PAGE_NEWS_ATH, &Tab1, CUI::CORNER_L, vec4(0.2f, 0.2f, 0.7f, ms_ColorTabbarActive.a), vec4(0.2f, 0.2f, 0.7f, ms_ColorTabbarInactive.a)))
			g_Config.m_UiPage = PAGE_NEWS_ATH;

		static CButtonContainer s_ButtonNewsDDNet;
		if(DoButton_MenuTab(&s_ButtonNewsDDNet, "DDNet News", g_Config.m_UiPage == PAGE_NEWS_DDNET, &Tab2, CUI::CORNER_R, vec4(0.2f, 0.2f, 0.7f, ms_ColorTabbarActive.a), vec4(0.2f, 0.2f, 0.7f, ms_ColorTabbarInactive.a)))
			g_Config.m_UiPage = PAGE_NEWS_DDNET;
	}

	MainView.HSplitTop(15.0f, 0, &MainView);
	MainView.VSplitLeft(15.0f, 0, &MainView);

	// calculate how much height we need - TODO: make this better
	const int CURRENT_NEWS_PAGE = g_Config.m_UiPage == PAGE_NEWS_ATH ? 0 : 1;
	static float s_TotalHeight[2] = {-1.0f};
	if(s_TotalHeight[CURRENT_NEWS_PAGE] < 0.0f)
	{
		std::istringstream f;
		if(g_Config.m_UiPage == PAGE_NEWS_ATH)
			f.str(Client()->News());
		else if(g_Config.m_UiPage == PAGE_NEWS_DDNET)
			f.str(Client()->m_aNewsDDNet);

		std::string line;
		while (std::getline(f, line))
		{
			if(line.size() > 0 && line.at(0) == '|' && line.at(line.size()-1) == '|')
				s_TotalHeight[CURRENT_NEWS_PAGE] += 32.0f;
			else
				s_TotalHeight[CURRENT_NEWS_PAGE] += 22.0f;
		}

		s_TotalHeight[CURRENT_NEWS_PAGE] -= MainView.h;
	}

	// scrollbar if necessary
	static float s_ScrollOffset[2] = {0.0f};
	if(s_TotalHeight[CURRENT_NEWS_PAGE]+MainView.h > MainView.h)
	{
		static float s_WantedScrollOffset[2] = {0.0f};
		CUIRect Scrollbar;
		MainView.VSplitRight(15.0f, &MainView, &Scrollbar);
		Scrollbar.HMargin(5.0f, &Scrollbar);
		static CButtonContainer s_Scrollbar;
		s_WantedScrollOffset[CURRENT_NEWS_PAGE] = DoScrollbarV(&s_Scrollbar, &Scrollbar, s_WantedScrollOffset[CURRENT_NEWS_PAGE]);

		// scroll with the mousewheel
		if(m_pClient->m_pGameConsole->IsClosed())
		{
			if(KeyEvent(KEY_MOUSE_WHEEL_DOWN))
				s_WantedScrollOffset[CURRENT_NEWS_PAGE] += 0.1f;
			else if(KeyEvent(KEY_MOUSE_WHEEL_UP))
				s_WantedScrollOffset[CURRENT_NEWS_PAGE] -= 0.1f;
		}
		s_WantedScrollOffset[CURRENT_NEWS_PAGE] = clamp(s_WantedScrollOffset[CURRENT_NEWS_PAGE], 0.0f, 1.0f);

		// smooth for the win
		smooth_set(&s_ScrollOffset[CURRENT_NEWS_PAGE], s_WantedScrollOffset[CURRENT_NEWS_PAGE], 27.0f, Client()->RenderFrameTime());
	}

	Graphics()->ClipEnable((int)MainView.x, (int)MainView.y+15, (int)MainView.w*2, round_to_int(MainView.h*1.5f));
	CUIRect Label;

	std::istringstream f;
	if(g_Config.m_UiPage == PAGE_NEWS_ATH)
		f.str(Client()->News());
	else if(g_Config.m_UiPage == PAGE_NEWS_DDNET)
		f.str(Client()->m_aNewsDDNet);

	std::string line;
	while (std::getline(f, line))
	{
		if(line.size() > 0 && line.at(0) == '|' && line.at(line.size()-1) == '|')
		{
			MainView.HSplitTop(30.0f, &Label, &MainView);
			Label.y -= s_TotalHeight[CURRENT_NEWS_PAGE] * s_ScrollOffset[CURRENT_NEWS_PAGE];
			UI()->DoLabelScaled(&Label, Localize(line.substr(1, line.size()-2).c_str()), 20.0f, -1);
		}
		else
		{
			MainView.HSplitTop(20.0f, &Label, &MainView);
			Label.y -= s_TotalHeight[CURRENT_NEWS_PAGE] * s_ScrollOffset[CURRENT_NEWS_PAGE];
			UI()->DoLabelScaled(&Label, line.c_str(), 15.f, -1, (int)(MainView.w - 30.0f));
		}
	}

	Graphics()->ClipDisable();
}

void CMenus::OnInit()
{
	CALLSTACK_ADD();

	if(g_Config.m_ClShowWelcome)
	{
		m_Popup = POPUP_LANGUAGE;
		g_Config.m_ClShowWelcome = 0;
	}

	Console()->Chain("add_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("remove_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("add_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("remove_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("br_show_ddnet", ConchainDDraceNetworkFilterUpdate, this);
}

void CMenus::PopupMessage(const char *pTopic, const char *pBody, const char *pButton)
{
	CALLSTACK_ADD();

	// reset active item
	UI()->SetActiveItem(0);

	str_copy(m_aMessageTopic, pTopic, sizeof(m_aMessageTopic));
	str_copy(m_aMessageBody, pBody, sizeof(m_aMessageBody));
	str_copy(m_aMessageButton, pButton, sizeof(m_aMessageButton));
	m_Popup = POPUP_MESSAGE;
}


int CMenus::Render()
{
	CALLSTACK_ADD();

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	m_MouseSlow = false;

	static int s_Frame = 0;
	if(s_Frame == 0)
	{
		s_Frame++;
	}
	else if(s_Frame == 1)
	{
		m_pClient->m_pSounds->Enqueue(CSounds::CHN_MUSIC, SOUND_MENU);
		s_Frame++;
		m_DoubleClickIndex = -1;

		if(g_Config.m_UiPage == PAGE_BROWSER)
		{
			if(g_Config.m_UiBrowserPage == PAGE_BROWSER_INTERNET)
			{
				if(g_Config.m_BrAutoCache && m_pClient->ServerBrowser()->CacheExists())
				{
					m_pClient->ServerBrowser()->LoadCacheWait();
					ServerBrowser()->RefreshQuick();
				}
				else
					ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			} else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_LAN)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_FAVORITES)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
			else if(g_Config.m_UiBrowserPage == PAGE_BROWSER_DDNET)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_DDNET);
		}
	}

	if(Client()->State() == IClient::STATE_ONLINE)
	{
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveIngame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveIngame;
	}
	else
	{
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveOutgame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveOutgame;
	}

	if(((g_Config.m_ClMenuBackground && !Client()->MapLoaded()) || (!g_Config.m_ClMenuBackground && Client()->State() == IClient::STATE_OFFLINE)) ||
			Client()->State() == IClient::STATE_CONNECTING ||
			Client()->State() == IClient::STATE_LOADING)
		RenderBackground();

	CUIRect TabBar;
	CUIRect MainView;

	// some margin around the screen
	Screen.Margin(10.0f, &Screen);

	static bool s_SoundCheck = false;
	if(!s_SoundCheck && m_Popup == POPUP_NONE)
	{
		if(Client()->SoundInitFailed())
			m_Popup = POPUP_SOUNDERROR;
		s_SoundCheck = true;
	}

	if(m_Popup == POPUP_NONE)
	{
		// do tab bar
#if defined(__ANDROID__)
		Screen.HSplitTop(100.0f, &TabBar, &MainView);
#else
		Screen.HSplitTop(24.0f, &TabBar, &MainView);
#endif
		TabBar.VMargin(20.0f, &TabBar);
		RenderMenubar(TabBar);

		// make sure the ui page doesn't go wild
		if(g_Config.m_UiPage < PAGE_NEWS_ATH || g_Config.m_UiPage >= NUM_PAGES || (Client()->State() == IClient::STATE_OFFLINE && g_Config.m_UiPage > PAGE_NEWS_DDNET && g_Config.m_UiPage < PAGE_BROWSER))
		{
			g_Config.m_UiPage = PAGE_BROWSER;
			g_Config.m_UiBrowserPage = PAGE_BROWSER_INTERNET;

			if(m_pClient->ServerBrowser()->CacheExists())
				m_pClient->ServerBrowser()->LoadCache();
			else
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			m_DoubleClickIndex = -1;
		}

		// render current page
		if(Client()->State() != IClient::STATE_OFFLINE)
		{
			if(m_GamePage == PAGE_GAME)
				RenderGame(MainView);
			else if(m_GamePage == PAGE_PLAYERS)
				RenderPlayers(MainView);
			else if(m_GamePage == PAGE_SERVER_INFO)
				RenderServerInfo(MainView);
			else if(m_GamePage == PAGE_CALLVOTE)
				RenderServerControl(MainView);
			else if(m_GamePage == PAGE_SPOOFING)
				RenderSpoofing(MainView);
			else if(m_GamePage == PAGE_SETTINGS)
				RenderSettings(MainView);
			else if(m_GamePage == PAGE_BROWSER)
				RenderBrowser(MainView, true);
			else if(m_GamePage == PAGE_MANUAL)
				RenderManual(MainView);
		}
		else if(g_Config.m_UiPage == PAGE_NEWS_ATH || g_Config.m_UiPage == PAGE_NEWS_DDNET)
			RenderNews(MainView);
		else if(g_Config.m_UiPage == PAGE_BROWSER)
			RenderBrowser(MainView, false);
		else if(g_Config.m_UiPage == PAGE_DEMOS)
			RenderDemoList(MainView);
		else if(g_Config.m_UiPage == PAGE_SETTINGS)
			RenderSettings(MainView);
		else if(g_Config.m_UiPage == PAGE_MANUAL)
			RenderManual(MainView);
	}
	else
	{
		RenderPopups();
	}
	return 0;
}


void CMenus::SetActive(bool Active)
{
	if(Active != m_MenuActive)
		Input()->SetIMEState(Active);
	m_MenuActive = Active;
#if defined(__ANDROID__)
	UI()->AndroidShowScreenKeys(!m_MenuActive && !m_pClient->m_pControls->m_UsingGamepad);
#endif
	if(!m_MenuActive)
	{
		if(m_NeedSendinfo)
		{
			m_pClient->SendInfo(false);
			m_NeedSendinfo = false;
		}

		if(m_NeedSendDummyinfo)
		{
			m_pClient->SendDummyInfo(false);
			m_NeedSendDummyinfo = false;
		}

		if(Client()->State() == IClient::STATE_ONLINE)
		{
			m_pClient->OnRelease();
		}
	}
	else if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->OnRelease();
	}
}

void CMenus::OnReset()
{
}

bool CMenus::OnMouseMove(float x, float y)
{
	CALLSTACK_ADD();

	if(m_MouseUnlocked)
		return false;

	m_LastInput = time_get();

	if((!m_MenuActive && !m_HotbarActive && !m_IRCActive) || !m_pClient->m_pGameConsole->IsClosed())
		return false;

#if defined(__ANDROID__) // No relative mouse on Android
	m_MousePos.x = x;
	m_MousePos.y = y;
#else
	UI()->ConvertMouseMove(&x, &y);
	if(m_MouseSlow)
	{
		m_MousePos.x += x * 0.05f;
		m_MousePos.y += y * 0.05f;
	}
	else
	{
		m_MousePos.x += x;
		m_MousePos.y += y;
	}
#endif
	if(m_MousePos.x < 0) m_MousePos.x = 0;
	if(m_MousePos.y < 0) m_MousePos.y = 0;
	if(m_MousePos.x > Graphics()->ScreenWidth()) m_MousePos.x = Graphics()->ScreenWidth();
	if(m_MousePos.y > Graphics()->ScreenHeight()) m_MousePos.y = Graphics()->ScreenHeight();

	return true;
}

bool CMenus::OnInput(IInput::CEvent e)
{
	CALLSTACK_ADD();

	if(m_MouseUnlocked)
		return true;

	m_LastInput = time_get();

	// special handle esc and enter for popup purposes
	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if(e.m_Key == KEY_ESCAPE)
		{
			m_EscapePressed = true;
			if(m_HotbarActive)
				m_HotbarActive = false;
			else if(m_IRCActive)
				ToggleIRC();
			else
				SetActive(!IsActive());
			return true;
		}
	}

	if(IsActive())
	{
		if(e.m_Flags&IInput::FLAG_PRESS)
		{
			// special for popups
			if(e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER)
				m_EnterPressed = true;
			else if(e.m_Key == KEY_DELETE)
				m_DeletePressed = true;
		}

		if(m_NumInputEvents < MAX_INPUTEVENTS)
			m_aInputEvents[m_NumInputEvents++] = e;
		return true;
	}

	return LockInput(e);

}

void CMenus::OnStateChange(int NewState, int OldState)
{
	CALLSTACK_ADD();

	// reset active item
	UI()->SetActiveItem(0);
	if(m_IRCActive)
		ToggleIRC();

	if(NewState == IClient::STATE_OFFLINE)
	{
		if(OldState >= IClient::STATE_ONLINE && NewState < IClient::STATE_QUITING)
			m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU, 1.0f);
		m_Popup = POPUP_NONE;
		if(Client()->ErrorString() && Client()->ErrorString()[0] != 0)
		{
			if(str_find_nocase(Client()->ErrorString(), "password"))
			{
				m_Popup = POPUP_PASSWORD;
				UI()->SetHotItem(&g_Config.m_Password);
				UI()->SetActiveItem(&g_Config.m_Password);
			}
			else
				m_Popup = POPUP_DISCONNECTED;
		}
	}
	else if(NewState == IClient::STATE_LOADING)
	{
		m_Popup = POPUP_CONNECTING;
		m_DownloadLastCheckTime = time_get();
		m_DownloadLastCheckSize = 0;
		m_DownloadSpeed = 0.0f;
	}
	else if(NewState == IClient::STATE_CONNECTING)
		m_Popup = POPUP_CONNECTING;
	else if (NewState == IClient::STATE_ONLINE || NewState == IClient::STATE_DEMOPLAYBACK)
	{
		m_Popup = POPUP_NONE;
		SetActive(false);
	}
}

extern "C" void font_debug_render();

void CMenus::OnRender()
{
	CALLSTACK_ADD();

	if((Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK) || Client()->Lua()->GetFullscreenedScript() != NULL)
		SetActive(true);

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
		RenderDemoPlayer(Screen);
	}

	if(/*XXX !g_Config.m_ClAllowPuremod && */Client()->State() == IClient::STATE_ONLINE && m_pClient->m_ServerMode == m_pClient->SERVERMODE_PUREMOD)
	{
		Client()->Disconnect();
		SetActive(true);
		m_Popup = POPUP_PURE;
	}

	// show update popup once at client startup
#if !defined(CONF_SPOOFING)
	{
		static bool s_WarnedUpdate = false;
		if(!s_WarnedUpdate && Updater()->State() == IUpdater::STATE_CLEAN && str_comp(Client()->LatestVersion(), "0"))
		{
			m_Popup = POPUP_UPDATE;
			s_WarnedUpdate = true;
		}
	}
#endif


	if(m_InitSkinlist)
	{
		m_apSkinList.clear();
		for(int i = 0; i < m_pClient->m_pSkins->Num(); ++i)
		{
			const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);

			// filter quick search
			if(g_Config.m_ClSkinFilterString[0] != '\0' && !str_find_nocase(s->m_aName, g_Config.m_ClSkinFilterString))
				continue;

			// filter vanilla
			if((g_Config.m_ClSkinFilterAdvanced == 1 && !s->m_IsVanilla) || (g_Config.m_ClSkinFilterAdvanced == 2 && s->m_IsVanilla))
				continue;

			// no special skins
			if((s->m_aName[0] == 'x' && s->m_aName[1] == '_'))
				continue;

			m_apSkinList.add(s);
		}
		m_InitSkinlist = false;
	}

	if(!IsActive())
	{
		m_EscapePressed = false;
		m_EnterPressed = false;
		m_DeletePressed = false;
		m_NumInputEvents = 0;
		return;
	}
	else
		UseMouseButtons(true);

	if(Client()->State() == IClient::STATE_OFFLINE &&
			g_Config.m_ClMenuBackground &&
			(!Client()->MapLoaded() || (str_comp_filenames(Client()->GetCurrentMapPath(), g_Config.m_ClMenuBackgroundMap) != 0)))
	{
		if(Client()->LoadBackgroundMap())
		{
			m_pClient->Layers()->Init(Kernel());
			m_pClient->Collision()->Init(Layers());
			RenderTools()->RenderTilemapGenerateSkip(Layers());
			m_pClient->m_pMapimages->OnMapLoad();
		}

		m_pClient->m_pCamera->m_Center = vec2(500.0f, 1000.0f);
	}

	// update colors
	vec3 Rgb = HsvToRgb(vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorVal/255.0f));
	ms_GuiColor = vec4(Rgb.r, Rgb.g, Rgb.b, g_Config.m_UiColorAlpha/255.0f);

	ms_ColorTabbarInactiveOutgame = vec4(0,0,0,0.25f);
	ms_ColorTabbarActiveOutgame = vec4(0,0,0,0.5f);

	float ColorIngameScaleI = 0.5f;
	float ColorIngameAcaleA = 0.2f;
	ms_ColorTabbarInactiveIngame = vec4(
		ms_GuiColor.r*ColorIngameScaleI,
		ms_GuiColor.g*ColorIngameScaleI,
		ms_GuiColor.b*ColorIngameScaleI,
		ms_GuiColor.a*0.8f);

	ms_ColorTabbarActiveIngame = vec4(
		ms_GuiColor.r*ColorIngameAcaleA,
		ms_GuiColor.g*ColorIngameAcaleA,
		ms_GuiColor.b*ColorIngameAcaleA,
		ms_GuiColor.a);

	// update the ui
	CUIRect *pScreen = UI()->Screen();
	float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
	float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*pScreen->h;
	int Buttons = 0;
	if(m_pClient->m_pGameConsole->IsClosed())
	{
		if(m_UseMouseButtons)
		{
			if(Input()->KeyIsPressed(KEY_MOUSE_1)) Buttons |= 1;
			if(Input()->KeyIsPressed(KEY_MOUSE_2)) Buttons |= 2;
			if(Input()->KeyIsPressed(KEY_MOUSE_3)) Buttons |= 4;

		/*	dbg_msg("dbg", "mouse buttons: %s%s%s",
					Buttons&1 ? "x" : ".",
					Buttons&4 ? "x" : ".",
					Buttons&2 ? "x" : "."
			);	*/

		}
#if defined(__ANDROID__)
		static int ButtonsOneFrameDelay = 0; // For Android touch input

		UI()->Update(mx,my,mx*3.0f,my*3.0f,ButtonsOneFrameDelay);
		ButtonsOneFrameDelay = Buttons;
#else
		UI()->Update(mx, my, mx * 3.0f, my * 3.0f, Buttons);
#endif
	}

	// lua fullscreen
	if(Client()->Lua()->GetFullscreenedScript() != NULL)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		CUIRect Button;
		UI()->Screen()->VSplitRight(15.0f, 0, &Button);
		Button.HSplitTop(15.0f, &Button, 0);

		static CButtonContainer s_ButtonExitFS;
		if(DoButton_Menu(&s_ButtonExitFS, "X", 0, &Button, Localize("Kill the currently fullscreened script"), CUI::CORNER_BL, vec4(1,0,0,0.5f)))
		{
			Client()->Lua()->GetFullscreenedScript()->Deactivate();
		}
	}
	else
	{
		// render
		if(m_IRCActive)
		{
			if(Client()->State() != IClient::STATE_ONLINE)
				RenderBackground();
			RenderIRC(*UI()->Screen());
		}
		else if(m_HotbarActive)
			RenderHotbar(*UI()->Screen());
		else if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
			Render();
	}

	// render cursor
	if(m_pClient->m_pGameConsole->IsClosed())
	{
		float Scale = UI()->Scale();
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_HUDCURSOR].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->QuadsSetRotation((float)g_Config.m_ClMouseRotation*((2.0f*3.1415926f)/360.0f));
		Graphics()->SetColor(1,1,1,1);
		IGraphics::CQuadItem QuadItem(mx, my, 24*Scale, 24*Scale);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	// render debug information
	if(g_Config.m_Debug)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "h=%p a=%p l=%p bt=%x:%c%c%c",
				   UI()->HotItem(), UI()->ActiveItem(), UI()->LastActiveItem(),
				   m_UseMouseButtons,
				   UI()->MouseButton(0) ? 'X' : 'x',
				   UI()->MouseButton(2) ? 'X' : 'x',
				   UI()->MouseButton(1) ? 'X' : 'x'
		);
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, 10, 10, 10, TEXTFLAG_RENDER);
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}

	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;
	m_NumInputEvents = 0;
}

static int gs_TextureBlob = -1;

void CMenus::RenderBackground()
{
	CALLSTACK_ADD();

	//Graphics()->Clear(1,1,1);
	//render_sunrays(0,0);
	if(gs_TextureBlob == -1)
		gs_TextureBlob = Graphics()->LoadTexture("blob.png", IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);


	float sw = 300*Graphics()->ScreenAspect();
	float sh = 300;
	Graphics()->MapScreen(0, 0, sw, sh);

	// render background color
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	//vec4 bottom(gui_color.r*0.3f, gui_color.g*0.3f, gui_color.b*0.3f, 1.0f);
	//vec4 bottom(0, 0, 0, 1.0f);
	vec4 Bottom(ms_GuiColor.r, ms_GuiColor.g, ms_GuiColor.b, 1.0f);
	vec4 Top(ms_GuiColor.r, ms_GuiColor.g, ms_GuiColor.b, 1.0f);
	IGraphics::CColorVertex Array[4] = {
			IGraphics::CColorVertex(0, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(1, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(2, Bottom.r, Bottom.g, Bottom.b, Bottom.a),
			IGraphics::CColorVertex(3, Bottom.r, Bottom.g, Bottom.b, Bottom.a)};
	Graphics()->SetColorVertex(Array, 4);
	IGraphics::CQuadItem QuadItem(0, 0, sw, sh);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// render the tiles
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	float Size = 15.0f;
	float OffsetTime = (float)fmod(Client()->LocalTime() * 0.15f, 2.0f);
	for(int y = -2; y < (int)(sw/Size); y++)
		for(int x = -2; x < (int)(sh/Size); x++)
		{
			Graphics()->SetColor(0,0,0,0.045f);
			IGraphics::CQuadItem QuadItem((x-OffsetTime)*Size*2+(y&1)*Size, (y+OffsetTime)*Size, Size, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
		}
	Graphics()->QuadsEnd();

	// render border fade
	Graphics()->TextureSet(gs_TextureBlob);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	QuadItem = IGraphics::CQuadItem(-100, -100, sw+200, sh+200);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// EVENT CALL
	LUA_FIRE_EVENT("OnRenderBackground");

	// restore screen
	{CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);}
}

int CMenus::DoButton_CheckBox_DontCare(CButtonContainer *pBC, const char *pText, int Checked, const CUIRect *pRect)
{
	CALLSTACK_ADD();

	switch(Checked)
	{
		case 0:
			return DoButton_CheckBox_Common(pBC, pText, "", pRect);
		case 1:
			return DoButton_CheckBox_Common(pBC, pText, "X", pRect);
		case 2:
			return DoButton_CheckBox_Common(pBC, pText, "O", pRect);
		default:
			return DoButton_CheckBox_Common(pBC, pText, "", pRect);
	}
}

#if defined(FEATURE_LUA)
void CMenus::LuaRequestFullscreen(CLuaFile *pLF)
{
	m_pLuaFSModeRequester = pLF;
	m_Popup = POPUP_LUA_REQUEST_FULLSCREEN;
}

void CMenus::OnLuaScriptUnload(CLuaFile *pLF)
{
	if(m_pLuaFSModeRequester == pLF)
	{
		m_pLuaFSModeRequester = NULL;
		m_Popup = POPUP_NONE;
	}
}
#endif

void CMenus::RenderUpdating(const char *pCaption, int current, int total)
{
	CALLSTACK_ADD();

	// make sure that we don't render for each little thing we load
	// because that will slow down loading if we have vsync
	static int64 LastLoadRender = 0;
	if(time_get()-LastLoadRender < time_freq()/60)
		return;
	LastLoadRender = time_get();

	// need up date this here to get correct
	vec3 Rgb = HsvToRgb(vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorVal/255.0f));
	ms_GuiColor = vec4(Rgb.r, Rgb.g, Rgb.b, g_Config.m_UiColorAlpha/255.0f);

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	RenderBackground();

	float w = 700;
	float h = 200;
	float x = Screen.w/2-w/2;
	float y = Screen.h/2-h/2;

	Graphics()->BlendNormal();

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.50f);
	RenderTools()->DrawRoundRect(0, y, Screen.w, h, 0.0f);
	Graphics()->QuadsEnd();

	CUIRect r;
	r.x = x;
	r.y = y+20;
	r.w = w;
	r.h = h;
	UI()->DoLabel(&r, Localize(pCaption), 32.0f, 0, -1);

	if (total > 0 )
	{
		float Percent = current/(float)total;
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.15f,0.15f,0.15f,0.75f);
		RenderTools()->DrawRoundRect(x+40, y+h-75, w-80, 30, 5.0f);
		Graphics()->SetColor(1,1,1,0.75f);
		RenderTools()->DrawRoundRect(x+45, y+h-70, (w-85)*Percent, 20, 5.0f);
		Graphics()->QuadsEnd();
	}

	Graphics()->Swap();
}

bool CMenus::LockInput(IInput::CEvent e)
{
	CALLSTACK_ADD();

	if(m_HotbarActive || m_IRCActive)
	{
		if((e.m_Flags&IInput::FLAG_PRESS)/* && (e.m_Key == KEY_MOUSE_1 || e.m_Key == KEY_MOUSE_2)*/)
			return true;
	}
	return false;
}

void CMenus::ToggleMouseMode()
{
	CALLSTACK_ADD();

	SetUnlockMouseMode(!m_MouseUnlocked);
}

void CMenus::SetUnlockMouseMode(bool unlock)
{
	CALLSTACK_ADD();

	if((m_MouseUnlocked = unlock))
		Input()->MouseModeAbsolute();
	else
		Input()->MouseModeRelative();
}

void CMenus::ConKeyShortcutRelMouse(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CMenus *pSelf = (CMenus *)pUserData;
	pSelf->ToggleMouseMode();
}

vec2 CMenus::GetMousePosRel() const
{
	CUIRect *pScreen = UI()->Screen();
	float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
	float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*pScreen->h;

	return vec2(mx, my);
}

void CMenus::SetMousePosRel(const vec2& p)
{
	CUIRect *pScreen = UI()->Screen();
	m_MousePos.x = (p.x/pScreen->w)*(float)Graphics()->ScreenWidth();
	m_MousePos.y = (p.y/pScreen->h)*(float)Graphics()->ScreenHeight();
}
