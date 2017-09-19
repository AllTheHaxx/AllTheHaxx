#include "../menus.h"

#include "../countryflags.h"

void CMenus::RenderSettingsPlayer(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button, Label, Dummy;
	MainView.HSplitTop(10.0f, 0, &MainView);

	char *Name = g_Config.m_PlayerName;
	char *Clan = g_Config.m_PlayerClan;
	int *Country = &g_Config.m_PlayerCountry;

	if(m_Dummy)
	{
		Name = g_Config.m_ClDummyName;
		Clan = g_Config.m_ClDummyClan;
		Country = &g_Config.m_ClDummyCountry;
	}

	// player name
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(200.0f, &Button, &Dummy);
	Button.VSplitLeft(150.0f, &Button, 0);
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Name"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetName = 0.0f;
	static CButtonContainer s_EditboxName;
	if(DoEditBox(&s_EditboxName, &Button, Name, sizeof(g_Config.m_PlayerName), 14.0f, &s_OffsetName))
	{
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}

	Dummy.w /= 2.3f;
	static CButtonContainer s_CheckboxShowKillMessages;
	if(DoButton_CheckBox(&s_CheckboxShowKillMessages, Localize("Dummy settings"), m_Dummy, &Dummy))
	{
		m_Dummy ^= 1;
	}

	// player clan
	MainView.HSplitTop(5.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	Button.VSplitLeft(80.0f, &Label, &Button);
	Button.VSplitLeft(150.0f, &Button, 0);
	str_format(aBuf, sizeof(aBuf), "%s:", Localize("Clan"));
	UI()->DoLabelScaled(&Label, aBuf, 14.0, -1);
	static float s_OffsetClan = 0.0f;
	static CButtonContainer s_EditboxClan;
	if(DoEditBox(&s_EditboxClan, &Button, Clan, sizeof(g_Config.m_PlayerClan), 14.0f, &s_OffsetClan))
	{
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}

	// country flag selector
	MainView.HSplitTop(20.0f, 0, &MainView);
	static float s_ScrollValue = 0.0f;
	int OldSelected = -1;
	static CButtonContainer s_Listbox;
	UiDoListboxStart(&s_Listbox, &MainView, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

	for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
	{
		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
		if(pEntry->m_CountryCode == *Country)
			OldSelected = i;
		CPointerContainer Container(&pEntry->m_CountryCode);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		if(Item.m_Visible)
		{
			CUIRect Label;
			Item.m_Rect.Margin(5.0f, &Item.m_Rect);
			Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);
			float OldWidth = Item.m_Rect.w;
			Item.m_Rect.w = Item.m_Rect.h*2;
			Item.m_Rect.x += (OldWidth-Item.m_Rect.w)/ 2.0f;
			m_pClient->m_pCountryFlags->Render(pEntry->m_CountryCode, vec4(1), Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
			if(pEntry->m_Texture != -1)
				UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		*Country = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;
		if(m_Dummy)
			m_NeedSendDummyinfo = true;
		else
			m_NeedSendinfo = true;
	}
}
