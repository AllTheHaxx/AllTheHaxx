#include <engine/keys.h>

#include "menus.h"

static CUIRect gs_ListBoxOriginalView;
static CUIRect gs_ListBoxView;
static float gs_ListBoxRowHeight;
static int gs_ListBoxItemIndex;
static int gs_ListBoxSelectedIndex;
static int gs_ListBoxNewSelected;
static int gs_ListBoxDoneEvents;
static int gs_ListBoxNumItems;
static int gs_ListBoxItemsPerRow;
static float gs_ListBoxScrollValue;
static bool gs_ListBoxItemActivated;

static inline int ItemsPerPage()
{
	float PageHeight = gs_ListBoxOriginalView.h;
	int NumRows = round_to_int(PageHeight / gs_ListBoxRowHeight);
	int ItemsPerPage = NumRows * gs_ListBoxItemsPerRow;

	return ItemsPerPage;
}

void CMenus::UiDoListboxStart(CButtonContainer *pBC, const CUIRect *pRect, float RowHeight, const char *pTitle, const char *pBottomText, int NumItems,
							  int ItemsPerRow, int SelectedIndex, float ScrollValue, int CornerTop, int CornerBottom)
{
	CUIRect Scroll, Row;
	CUIRect View = *pRect;
	CUIRect Header, Footer;

	if(!pTitle)
		pTitle = "";
	if(!pBottomText)
		pBottomText = "";

	// draw header
	View.HSplitTop(ms_ListheaderHeight, &Header, &View);
	RenderTools()->DrawUIRect(&Header, vec4(1,1,1,0.25f), CornerTop, 5.0f);
	UI()->DoLabel(&Header, pTitle, Header.h*ms_FontmodHeight, 0);

	// draw footers
	View.HSplitBottom(ms_ListheaderHeight, &View, &Footer);
	RenderTools()->DrawUIRect(&Footer, vec4(1,1,1,0.25f), CornerBottom, 5.0f);
	Footer.VSplitLeft(10.0f, 0, &Footer);
	UI()->DoLabel(&Footer, pBottomText, Header.h*ms_FontmodHeight, 0);

	// background
	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.15f), 0, 0);

	// prepare the scroll
#if defined(__ANDROID__)
	View.VSplitRight(50, &View, &Scroll);
#else
	View.VSplitRight(15, &View, &Scroll);
#endif

	// setup the variables
	gs_ListBoxOriginalView = View;
	gs_ListBoxSelectedIndex = SelectedIndex;
	gs_ListBoxNewSelected = SelectedIndex;
	gs_ListBoxItemIndex = 0;
	gs_ListBoxRowHeight = RowHeight;
	gs_ListBoxNumItems = NumItems;
	gs_ListBoxItemsPerRow = ItemsPerRow;
	gs_ListBoxDoneEvents = 0;
	gs_ListBoxScrollValue = ScrollValue;
	gs_ListBoxItemActivated = false;

	// do the scrollbar
	View.HSplitTop(gs_ListBoxRowHeight, &Row, 0);

	int NumViewable = (int)(gs_ListBoxOriginalView.h/Row.h) + 1;
	int Num = (NumItems+gs_ListBoxItemsPerRow-1)/gs_ListBoxItemsPerRow-NumViewable+1;
	int ViewablePercent = round_to_int(100.0f * ((float)NumViewable / (float)max(1, NumItems)));
	if(Num < 0)
		Num = 0;
	if(Num > 0)
	{
		static std::map<const void*, std::pair<float, float> > s_NewVals; // maybe a map seems hacky, but it's a convenient way to get it to work

		// initialize
		if(s_NewVals.find(pBC->GetID()) == s_NewVals.end())
		{
			s_NewVals[pBC->GetID()] = std::pair<float, float>(0.0f, 0.0f);
		}

		float& s_NewVal = s_NewVals[pBC->GetID()].first;
		if(UI()->ActiveItem() == pBC->GetID() || UI()->MouseInside(&Scroll)) // do not intervene the scrollbar's own logic
			s_NewVal = gs_ListBoxScrollValue;
		else
		{
			for(int i = 0; i < m_NumInputEvents; i++)
			{
				if(m_aInputEvents[i].m_Key == KEY_MOUSE_WHEEL_UP && UI()->MouseInside(&View))
					s_NewVal -= ((float)ViewablePercent/100.0f) * 0.85f;
				if(m_aInputEvents[i].m_Key == KEY_MOUSE_WHEEL_DOWN && UI()->MouseInside(&View))
					s_NewVal += ((float)ViewablePercent/100.0f) * 0.85f;
			}

			if(s_NewVal < 0.0f) s_NewVal = 0.0f;
			if(s_NewVal > 1.0f) s_NewVal = 1.0f;
			if(gs_ListBoxScrollValue != s_NewVals[pBC->GetID()].second) // move freely
			{
				float old = s_NewVal;
				s_NewVal = gs_ListBoxScrollValue;
				gs_ListBoxScrollValue = old;
			}
			smooth_set(&gs_ListBoxScrollValue, s_NewVal, 23.0f, Client()->RenderFrameTime());
			s_NewVals[pBC->GetID()].second = gs_ListBoxScrollValue;
		}
	}

	Scroll.HMargin(5.0f, &Scroll);
	gs_ListBoxScrollValue = DoScrollbarV(pBC, &Scroll, gs_ListBoxScrollValue, 0, ~0, ViewablePercent);

	// the list
	gs_ListBoxView = gs_ListBoxOriginalView;
	gs_ListBoxView.VMargin(5.0f, &gs_ListBoxView);
	UI()->ClipEnable(&gs_ListBoxView);
	gs_ListBoxView.y -= gs_ListBoxScrollValue*Num*Row.h;
}

CMenus::CListboxItem CMenus::UiDoListboxNextRow()
{
	static CUIRect s_RowView;
	CListboxItem Item = {0};
	if(gs_ListBoxItemIndex%gs_ListBoxItemsPerRow == 0)
		gs_ListBoxView.HSplitTop(gs_ListBoxRowHeight /*-2.0f*/, &s_RowView, &gs_ListBoxView);

	s_RowView.VSplitLeft(s_RowView.w/(gs_ListBoxItemsPerRow-gs_ListBoxItemIndex%gs_ListBoxItemsPerRow)/(UI()->Scale()), &Item.m_Rect, &s_RowView);

	Item.m_Visible = 1;
	//item.rect = row;

	Item.m_HitRect = Item.m_Rect;

	//CUIRect select_hit_box = item.rect;

	if(gs_ListBoxSelectedIndex == gs_ListBoxItemIndex)
		Item.m_Selected = 1;

	// make sure that only those in view can be selected
	if(Item.m_Rect.y+Item.m_Rect.h > gs_ListBoxOriginalView.y)
	{
		if(Item.m_HitRect.y < gs_ListBoxOriginalView.y)
		{
			Item.m_HitRect.h -= gs_ListBoxOriginalView.y-Item.m_HitRect.y;
			Item.m_HitRect.y = gs_ListBoxOriginalView.y;
		}
	}
	else
		Item.m_Visible = 0;

	// check if we need to do more
	if(Item.m_Rect.y > gs_ListBoxOriginalView.y+gs_ListBoxOriginalView.h)
		Item.m_Visible = 0;

	gs_ListBoxItemIndex++;
	return Item;
}

CMenus::CListboxItem CMenus::UiDoListboxNextItem(CButtonContainer *pBC, bool Selected, bool KeyEvents)
{
	int ThisItemIndex = gs_ListBoxItemIndex;
	if(Selected)
	{
		if(gs_ListBoxSelectedIndex == gs_ListBoxNewSelected)
			gs_ListBoxNewSelected = ThisItemIndex;
		gs_ListBoxSelectedIndex = ThisItemIndex;
	}

	CListboxItem Item = UiDoListboxNextRow();

	if(Item.m_Visible)
	{
		if(UI()->DoButtonLogic(pBC->GetID(), "", gs_ListBoxSelectedIndex == gs_ListBoxItemIndex, &Item.m_HitRect))
			gs_ListBoxNewSelected = ThisItemIndex;
	}

	// process input, regard selected index
	if(gs_ListBoxSelectedIndex == ThisItemIndex)
	{
		if(!gs_ListBoxDoneEvents)
		{
			gs_ListBoxDoneEvents = 1;

			if(m_EnterPressed || (UI()->ActiveItem() == pBC->GetID() && Input()->MouseDoubleClick()))
			{
				Input()->MouseDoubleClickReset();
				gs_ListBoxItemActivated = true;
				UI()->SetActiveItem(0);
			}
			else if(KeyEvents && UI()->MouseInside(&gs_ListBoxOriginalView))
			{
				for(int i = 0; i < m_NumInputEvents; i++)
				{
					int NewIndex = -1;
					if(m_aInputEvents[i].m_Flags&IInput::FLAG_PRESS)
					{
						if(m_aInputEvents[i].m_Key == KEY_DOWN) NewIndex = clamp(gs_ListBoxNewSelected + gs_ListBoxItemsPerRow, 0, gs_ListBoxNumItems);
						if(m_aInputEvents[i].m_Key == KEY_UP) NewIndex = clamp(gs_ListBoxNewSelected - gs_ListBoxItemsPerRow, 0, gs_ListBoxNumItems);
						if(m_aInputEvents[i].m_Key == KEY_RIGHT && gs_ListBoxItemsPerRow > 1) NewIndex = clamp(gs_ListBoxNewSelected + 1, 0, gs_ListBoxNumItems);
						if(m_aInputEvents[i].m_Key == KEY_LEFT && gs_ListBoxItemsPerRow > 1) NewIndex = clamp(gs_ListBoxNewSelected - 1, 0, gs_ListBoxNumItems);
						if(m_aInputEvents[i].m_Key == KEY_PAGEUP) NewIndex = max(gs_ListBoxNewSelected - ItemsPerPage(), 0);
						if(m_aInputEvents[i].m_Key == KEY_PAGEDOWN) NewIndex = min(gs_ListBoxNewSelected + ItemsPerPage(), gs_ListBoxNumItems - 1);
						if(m_aInputEvents[i].m_Key == KEY_HOME) NewIndex = 0;
						if(m_aInputEvents[i].m_Key == KEY_END) NewIndex = gs_ListBoxNumItems - 1;

					}
					if(NewIndex > -1 && NewIndex < gs_ListBoxNumItems)
					{
						// scroll
						float Offset = (NewIndex/gs_ListBoxItemsPerRow-gs_ListBoxNewSelected/gs_ListBoxItemsPerRow)*gs_ListBoxRowHeight;
						int Scroll = gs_ListBoxOriginalView.y > Item.m_Rect.y+Offset ? -1 :
									 gs_ListBoxOriginalView.y+gs_ListBoxOriginalView.h < Item.m_Rect.y+Item.m_Rect.h+Offset ? 1 : 0;
						if(Scroll)
						{
							int NumViewable = (int)(gs_ListBoxOriginalView.h/gs_ListBoxRowHeight) + 1;
							int ScrollNum = (gs_ListBoxNumItems+gs_ListBoxItemsPerRow-1)/gs_ListBoxItemsPerRow-NumViewable+1;
							if(Scroll < 0)
							{
								int Num = (int)((gs_ListBoxOriginalView.y - Item.m_Rect.y - Offset + gs_ListBoxRowHeight - 1.0f) / gs_ListBoxRowHeight);
								gs_ListBoxScrollValue -= (1.0f/ScrollNum)*Num;
							}
							else
							{
								int Num = (int)((Item.m_Rect.y + Item.m_Rect.h + Offset - (gs_ListBoxOriginalView.y + gs_ListBoxOriginalView.h) + gs_ListBoxRowHeight - 1.0f) /
												gs_ListBoxRowHeight);
								gs_ListBoxScrollValue += (1.0f/ScrollNum)*Num;
							}
							if(gs_ListBoxScrollValue < 0.0f) gs_ListBoxScrollValue = 0.0f;
							if(gs_ListBoxScrollValue > 1.0f) gs_ListBoxScrollValue = 1.0f;
						}

						gs_ListBoxNewSelected = NewIndex;
					}
				}
			}
		}

		//selected_index = i;
		CUIRect r = Item.m_Rect;
		r.Margin(1.5f, &r);
		RenderTools()->DrawUIRect(&r, vec4(1,1,1,0.5f), CUI::CORNER_ALL, 4.0f);
	}

	return Item;
}

int CMenus::UiDoListboxEnd(float *pScrollValue, bool *pItemActivated)
{
	CALLSTACK_ADD();

	UI()->ClipDisable();
	if(pScrollValue)
		*pScrollValue = gs_ListBoxScrollValue;
	if(pItemActivated)
		*pItemActivated = gs_ListBoxItemActivated;
	return gs_ListBoxNewSelected;
}
