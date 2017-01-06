/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/color.h>
#include <base/tl/array.h>

#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/storage.h>

#include "editor.h"


// popup menu handling
static struct
{
	CUIRect m_Rect;
	void *m_pId;
	int (*m_pfnFunc)(CEditor *pEditor, CUIRect Rect);
	int m_IsMenu;
	void *m_pExtra;
} s_UiPopups[8];

static int g_UiNumPopups = 0;

void CEditor::UiInvokePopupMenu(void *pID, int Flags, float x, float y, float Width, float Height, int (*pfnFunc)(CEditor *pEditor, CUIRect Rect), void *pExtra)
{
	if(g_UiNumPopups > 7)
		return;
	Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "editor", "invoked");
	if(x + Width > UI()->Screen()->w)
		x -= Width;
	if(y + Height > UI()->Screen()->h)
		y -= Height;
	s_UiPopups[g_UiNumPopups].m_pId = pID;
	s_UiPopups[g_UiNumPopups].m_IsMenu = Flags;
	s_UiPopups[g_UiNumPopups].m_Rect.x = x;
	s_UiPopups[g_UiNumPopups].m_Rect.y = y;
	s_UiPopups[g_UiNumPopups].m_Rect.w = Width;
	s_UiPopups[g_UiNumPopups].m_Rect.h = Height;
	s_UiPopups[g_UiNumPopups].m_pfnFunc = pfnFunc;
	s_UiPopups[g_UiNumPopups].m_pExtra = pExtra;
	g_UiNumPopups++;
}

void CEditor::UiDoPopupMenu()
{
	for(int i = 0; i < g_UiNumPopups; i++)
	{
		bool Inside = UI()->MouseInside(&s_UiPopups[i].m_Rect);
		UI()->SetHotItem(&s_UiPopups[i].m_pId);

		if(UI()->ActiveItem() == &s_UiPopups[i].m_pId)
		{
			if(!UI()->MouseButton(0))
			{
				if(!Inside)
				{
					g_UiNumPopups--;
					m_PopupEventWasActivated = false;
				}
				UI()->SetActiveItem(0);
			}
		}
		else if(UI()->HotItem() == &s_UiPopups[i].m_pId)
		{
			if(UI()->MouseButton(0))
				UI()->SetActiveItem(&s_UiPopups[i].m_pId);
		}

		int Corners = CUI::CORNER_ALL;
		if(s_UiPopups[i].m_IsMenu)
			Corners = CUI::CORNER_R|CUI::CORNER_B;

		CUIRect r = s_UiPopups[i].m_Rect;
		RenderTools()->DrawUIRect(&r, vec4(0.5f,0.5f,0.5f,0.75f), Corners, 3.0f);
		r.Margin(1.0f, &r);
		RenderTools()->DrawUIRect(&r, vec4(0,0,0,0.75f), Corners, 3.0f);
		r.Margin(4.0f, &r);

		if(s_UiPopups[i].m_pfnFunc(this, r))
		{
			m_LockMouse = false;
			UI()->SetActiveItem(0);
			g_UiNumPopups--;
			m_PopupEventWasActivated = false;
		}

		if(Input()->KeyPress(KEY_ESCAPE))
		{
			m_LockMouse = false;
			UI()->SetActiveItem(0);
			g_UiNumPopups--;
			m_PopupEventWasActivated = false;
		}
	}
}


int CEditor::PopupGroup(CEditor *pEditor, CUIRect View)
{
	// remove group button
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;

	// don't allow deletion of game group
	if(pEditor->m_Map.m_pGameGroup != pEditor->GetSelectedGroup())
	{
		if(pEditor->DoButton_Editor(&s_DeleteButton, "Delete group", 0, &Button, 0, "Delete group"))
		{
			pEditor->m_Map.DeleteGroup(pEditor->m_SelectedGroup);
			pEditor->m_SelectedGroup = max(0, pEditor->m_SelectedGroup-1);
			return 1;
		}
	}
	else
	{
		if(pEditor->DoButton_Editor(&s_DeleteButton, "Clean up game tiles", 0, &Button, 0, "Removes game tiles that aren't based on a layer"))
		{
			// gather all tile layers
			array<CLayerTiles*> Layers;
			for(int i = 0; i < pEditor->m_Map.m_pGameGroup->m_lLayers.size(); ++i)
			{
				if(pEditor->m_Map.m_pGameGroup->m_lLayers[i] != pEditor->m_Map.m_pGameLayer && pEditor->m_Map.m_pGameGroup->m_lLayers[i]->m_Type == LAYERTYPE_TILES)
					Layers.add(static_cast<CLayerTiles *>(pEditor->m_Map.m_pGameGroup->m_lLayers[i]));
			}

			// search for unneeded game tiles
			CLayerTiles *gl = pEditor->m_Map.m_pGameLayer;
			for(int y = 0; y < gl->m_Height; ++y)
				for(int x = 0; x < gl->m_Width; ++x)
				{
					if(gl->m_pTiles[y*gl->m_Width+x].m_Index > static_cast<unsigned char>(TILE_NOHOOK))
						continue;

					bool Found = false;
					for(int i = 0; i < Layers.size(); ++i)
					{
						if(x < Layers[i]->m_Width && y < Layers[i]->m_Height && Layers[i]->m_pTiles[y*Layers[i]->m_Width+x].m_Index)
						{
							Found = true;
							break;
						}
					}

					if(!Found)
					{
						gl->m_pTiles[y*gl->m_Width+x].m_Index = TILE_AIR;
						pEditor->m_Map.m_Modified = true;
					}
				}

			return 1;
		}
	}

	if(pEditor->GetSelectedGroup()->m_GameGroup && !pEditor->m_Map.m_pTeleLayer)
	{
		// new tele layer
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static int s_NewSwitchLayerButton = 0;
		if(pEditor->DoButton_Editor(&s_NewSwitchLayerButton, "Add tele layer", 0, &Button, 0, "Creates a new tele layer"))
		{
			CLayer *l = new CLayerTele(pEditor->m_Map.m_pGameLayer->m_Width, pEditor->m_Map.m_pGameLayer->m_Height);
			pEditor->m_Map.MakeTeleLayer(l);
			pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
			pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
			pEditor->m_Brush.Clear();
			return 1;
		}
	}

	if(pEditor->GetSelectedGroup()->m_GameGroup && !pEditor->m_Map.m_pSpeedupLayer)
	{
		// new speedup layer
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static int s_NewSwitchLayerButton = 0;
		if(pEditor->DoButton_Editor(&s_NewSwitchLayerButton, "Add speedup layer", 0, &Button, 0, "Creates a new speedup layer"))
		{
			CLayer *l = new CLayerSpeedup(pEditor->m_Map.m_pGameLayer->m_Width, pEditor->m_Map.m_pGameLayer->m_Height);
			pEditor->m_Map.MakeSpeedupLayer(l);
			pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
			pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
			pEditor->m_Brush.Clear();
			return 1;
		}
	}

	if(pEditor->GetSelectedGroup()->m_GameGroup && !pEditor->m_Map.m_pTuneLayer)
		{
			// new tune layer
			View.HSplitBottom(5.0f, &View, &Button);
			View.HSplitBottom(12.0f, &View, &Button);
			static int s_NewSwitchLayerButton = 0;
			if(pEditor->DoButton_Editor(&s_NewSwitchLayerButton, "Add tune layer", 0, &Button, 0, "Creates a new tuning layer"))
			{
				CLayer *l = new CLayerTune(pEditor->m_Map.m_pGameLayer->m_Width, pEditor->m_Map.m_pGameLayer->m_Height);
				pEditor->m_Map.MakeTuneLayer(l);
				pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
				pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
				pEditor->m_Brush.Clear();
				return 1;
			}
		}

	if(pEditor->GetSelectedGroup()->m_GameGroup && !pEditor->m_Map.m_pFrontLayer)
	{
		// new force layer
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static int s_NewFrontLayerButton = 0;
		if(pEditor->DoButton_Editor(&s_NewFrontLayerButton, "Add front layer", 0, &Button, 0, "Creates a new item layer"))
		{
			CLayer *l = new CLayerFront(pEditor->m_Map.m_pGameLayer->m_Width, pEditor->m_Map.m_pGameLayer->m_Height);
			pEditor->m_Map.MakeFrontLayer(l);
			pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
			pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
			pEditor->m_Brush.Clear();
			return 1;
		}
	}

	if(pEditor->GetSelectedGroup()->m_GameGroup && !pEditor->m_Map.m_pSwitchLayer)
	{
		// new Switch layer
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static int s_NewSwitchLayerButton = 0;
		if(pEditor->DoButton_Editor(&s_NewSwitchLayerButton, "Add switch layer", 0, &Button, 0, "Creates a new switch layer"))
		{
			CLayer *l = new CLayerSwitch(pEditor->m_Map.m_pGameLayer->m_Width, pEditor->m_Map.m_pGameLayer->m_Height);
			pEditor->m_Map.MakeSwitchLayer(l);
			pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
			pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
			pEditor->m_Brush.Clear();
			return 1;
		}
	}

	// new quad layer
	View.HSplitBottom(5.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_NewQuadLayerButton = 0;
	if(pEditor->DoButton_Editor(&s_NewQuadLayerButton, "Add quads layer", 0, &Button, 0, "Creates a new quad layer"))
	{
		CLayer *l = new CLayerQuads;
		l->m_pEditor = pEditor;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
		pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_Collapse = false;
		return 1;
	}

	// new tile layer
	View.HSplitBottom(5.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_NewTileLayerButton = 0;
	if(pEditor->DoButton_Editor(&s_NewTileLayerButton, "Add tile layer", 0, &Button, 0, "Creates a new tile layer"))
	{
		CLayer *l = new CLayerTiles(pEditor->m_Map.m_pGameLayer->m_Width, pEditor->m_Map.m_pGameLayer->m_Height);
		l->m_pEditor = pEditor;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
		pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_Collapse = false;
		return 1;
	}

	// new sound layer
	View.HSplitBottom(5.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_NewSoundLayerButton = 0;
	if(pEditor->DoButton_Editor(&s_NewSoundLayerButton, "Add sound layer", 0, &Button, 0, "Creates a new sound layer"))
	{
		CLayer *l = new CLayerSounds;
		l->m_pEditor = pEditor;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->AddLayer(l);
		pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_lLayers.size()-1;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_Collapse = false;
		return 1;
	}

	// group name
	if(!pEditor->GetSelectedGroup()->m_GameGroup)
	{
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static float s_Name = 0;
		pEditor->UI()->DoLabel(&Button, "Name:", 10.0f, -1, -1);
		Button.VSplitLeft(40.0f, 0, &Button);
		if(pEditor->DoEditBox(&s_Name, &Button, pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_aName, sizeof(pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_aName), 10.0f, &s_Name))
			pEditor->m_Map.m_Modified = true;
	}

	enum
	{
		PROP_ORDER=0,
		PROP_POS_X,
		PROP_POS_Y,
		PROP_PARA_X,
		PROP_PARA_Y,
		PROP_USE_CLIPPING,
		PROP_CLIP_X,
		PROP_CLIP_Y,
		PROP_CLIP_W,
		PROP_CLIP_H,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Order", pEditor->m_SelectedGroup, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lGroups.size()-1},
		{"Pos X", -pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", -pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetY, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Para X", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Para Y", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxY, PROPTYPE_INT_SCROLL, -1000000, 1000000},

		{"Use Clipping", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_UseClipping, PROPTYPE_BOOL, 0, 1},
		{"Clip X", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipX, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip Y", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipY, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Clip W", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipW, PROPTYPE_INT_SCROLL, 0, 1000000},
		{"Clip H", pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipH, PROPTYPE_INT_SCROLL, 0, 1000000},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;

	// cut the properties that isn't needed
	if(pEditor->GetSelectedGroup()->m_GameGroup)
		aProps[PROP_POS_X].m_pName = 0;

	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_ORDER)
		pEditor->m_SelectedGroup = pEditor->m_Map.SwapGroups(pEditor->m_SelectedGroup, NewVal);

	// these can not be changed on the game group
	if(!pEditor->GetSelectedGroup()->m_GameGroup)
	{
		if(Prop == PROP_PARA_X) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxX = NewVal;
		else if(Prop == PROP_PARA_Y) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ParallaxY = NewVal;
		else if(Prop == PROP_POS_X) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetX = -NewVal;
		else if(Prop == PROP_POS_Y) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_OffsetY = -NewVal;
		else if(Prop == PROP_USE_CLIPPING) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_UseClipping = NewVal;
		else if(Prop == PROP_CLIP_X) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipX = NewVal;
		else if(Prop == PROP_CLIP_Y) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipY = NewVal;
		else if(Prop == PROP_CLIP_W) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipW = NewVal;
		else if(Prop == PROP_CLIP_H) pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->m_ClipH = NewVal;
	}

	return 0;
}

int CEditor::PopupLayer(CEditor *pEditor, CUIRect View)
{
	// remove layer button
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;

	// don't allow deletion of game layer
	if(pEditor->m_Map.m_pGameLayer != pEditor->GetSelectedLayer(0) &&
		pEditor->DoButton_Editor(&s_DeleteButton, "Delete layer", 0, &Button, 0, "Deletes the layer"))
	{
		if(pEditor->GetSelectedLayer(0) == pEditor->m_Map.m_pFrontLayer)
			pEditor->m_Map.m_pFrontLayer = 0x0;
		if(pEditor->GetSelectedLayer(0) == pEditor->m_Map.m_pTeleLayer)
			pEditor->m_Map.m_pTeleLayer = 0x0;
		if(pEditor->GetSelectedLayer(0) == pEditor->m_Map.m_pSpeedupLayer)
			pEditor->m_Map.m_pSpeedupLayer = 0x0;
		if(pEditor->GetSelectedLayer(0) == pEditor->m_Map.m_pSwitchLayer)
			pEditor->m_Map.m_pSwitchLayer = 0x0;
		if(pEditor->GetSelectedLayer(0) == pEditor->m_Map.m_pTuneLayer)
			pEditor->m_Map.m_pTuneLayer = 0x0;
		pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup]->DeleteLayer(pEditor->m_SelectedLayer);
		return 1;
	}

	// layer name
	// if(pEditor->m_Map.m_pGameLayer != pEditor->GetSelectedLayer(0))
	if(pEditor->m_Map.m_pGameLayer != pEditor->GetSelectedLayer(0) && pEditor->m_Map.m_pTeleLayer != pEditor->GetSelectedLayer(0) && pEditor->m_Map.m_pSpeedupLayer != pEditor->GetSelectedLayer(0) && pEditor->m_Map.m_pFrontLayer != pEditor->GetSelectedLayer(0) && pEditor->m_Map.m_pSwitchLayer != pEditor->GetSelectedLayer(0) && pEditor->m_Map.m_pTuneLayer != pEditor->GetSelectedLayer(0))
	{
		View.HSplitBottom(5.0f, &View, &Button);
		View.HSplitBottom(12.0f, &View, &Button);
		static float s_Name = 0;
		pEditor->UI()->DoLabel(&Button, "Name:", 10.0f, -1, -1);
		Button.VSplitLeft(40.0f, 0, &Button);
		if(pEditor->DoEditBox(&s_Name, &Button, pEditor->GetSelectedLayer(0)->m_aName, sizeof(pEditor->GetSelectedLayer(0)->m_aName), 10.0f, &s_Name))
			pEditor->m_Map.m_Modified = true;
	}

	View.HSplitBottom(10.0f, &View, 0);

	CLayerGroup *pCurrentGroup = pEditor->m_Map.m_lGroups[pEditor->m_SelectedGroup];
	CLayer *pCurrentLayer = pEditor->GetSelectedLayer(0);

	enum
	{
		PROP_GROUP=0,
		PROP_ORDER,
		PROP_HQ,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Group", pEditor->m_SelectedGroup, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lGroups.size()-1},
		{"Order", pEditor->m_SelectedLayer, PROPTYPE_INT_STEP, 0, pCurrentGroup->m_lLayers.size()},
		{"Detail", pCurrentLayer->m_Flags&LAYERFLAG_DETAIL, PROPTYPE_BOOL, 0, 1},
		{0},
	};

	// if(pEditor->m_Map.m_pGameLayer == pEditor->GetSelectedLayer(0)) // dont use Group and Detail from the selection if this is the game layer
	if(pEditor->m_Map.m_pGameLayer == pEditor->GetSelectedLayer(0) || pEditor->m_Map.m_pTeleLayer == pEditor->GetSelectedLayer(0) || pEditor->m_Map.m_pSpeedupLayer == pEditor->GetSelectedLayer(0) || pEditor->m_Map.m_pFrontLayer == pEditor->GetSelectedLayer(0) || pEditor->m_Map.m_pSwitchLayer == pEditor->GetSelectedLayer(0) || pEditor->m_Map.m_pTuneLayer == pEditor->GetSelectedLayer(0)) // dont use Group and Detail from the selection if this is the game layer
	{
		aProps[0].m_Type = PROPTYPE_NULL;
		aProps[2].m_Type = PROPTYPE_NULL;
	}

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_ORDER)
		pEditor->m_SelectedLayer = pCurrentGroup->SwapLayers(pEditor->m_SelectedLayer, NewVal);
	else if(Prop == PROP_GROUP && pCurrentLayer->m_Type != LAYERTYPE_GAME)
	{
		if(NewVal >= 0 && NewVal < pEditor->m_Map.m_lGroups.size())
		{
			pCurrentGroup->m_lLayers.remove(pCurrentLayer);
			pEditor->m_Map.m_lGroups[NewVal]->m_lLayers.add(pCurrentLayer);
			pEditor->m_SelectedGroup = NewVal;
			pEditor->m_SelectedLayer = pEditor->m_Map.m_lGroups[NewVal]->m_lLayers.size()-1;
		}
	}
	else if(Prop == PROP_HQ)
	{
		pCurrentLayer->m_Flags &= ~LAYERFLAG_DETAIL;
		if(NewVal)
			pCurrentLayer->m_Flags |= LAYERFLAG_DETAIL;
	}

	return pCurrentLayer->RenderProperties(&View);
}

int CEditor::PopupQuad(CEditor *pEditor, CUIRect View)
{
	CQuad *pQuad = pEditor->GetSelectedQuad();

	CUIRect Button;

	// delete button
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;
	if(pEditor->DoButton_Editor(&s_DeleteButton, "Delete", 0, &Button, 0, "Deletes the current quad"))
	{
		CLayerQuads *pLayer = (CLayerQuads *)pEditor->GetSelectedLayerType(0, LAYERTYPE_QUADS);
		if(pLayer)
		{
			pEditor->m_Map.m_Modified = true;
			pLayer->m_lQuads.remove_index(pEditor->m_SelectedQuad);
			pEditor->m_SelectedQuad--;
		}
		return 1;
	}

	// aspect ratio button
	View.HSplitBottom(10.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	CLayerQuads *pLayer = (CLayerQuads *)pEditor->GetSelectedLayerType(0, LAYERTYPE_QUADS);
	if(pLayer && pLayer->m_Image >= 0 && pLayer->m_Image < pEditor->m_Map.m_lImages.size())
	{
		static int s_AspectRatioButton = 0;
		if(pEditor->DoButton_Editor(&s_AspectRatioButton, "Aspect ratio", 0, &Button, 0, "Resizes the current Quad based on the aspect ratio of the image"))
		{
			int Top = pQuad->m_aPoints[0].y;
			int Left = pQuad->m_aPoints[0].x;
			int Right = pQuad->m_aPoints[0].x;

			for(int k = 1; k < 4; k++)
			{
				if(pQuad->m_aPoints[k].y < Top) Top = pQuad->m_aPoints[k].y;
				if(pQuad->m_aPoints[k].x < Left) Left = pQuad->m_aPoints[k].x;
				if(pQuad->m_aPoints[k].x > Right) Right = pQuad->m_aPoints[k].x;
			}

			int Height = (Right-Left)*pEditor->m_Map.m_lImages[pLayer->m_Image]->m_Height/pEditor->m_Map.m_lImages[pLayer->m_Image]->m_Width;

			pQuad->m_aPoints[0].x = Left; pQuad->m_aPoints[0].y = Top;
			pQuad->m_aPoints[1].x = Right; pQuad->m_aPoints[1].y = Top;
			pQuad->m_aPoints[2].x = Left; pQuad->m_aPoints[2].y = Top+Height;
			pQuad->m_aPoints[3].x = Right; pQuad->m_aPoints[3].y = Top+Height;
			pEditor->m_Map.m_Modified = true;
			return 1;
		}
	}

	// align button
	View.HSplitBottom(6.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_AlignButton = 0;
	if(pEditor->DoButton_Editor(&s_AlignButton, "Align", 0, &Button, 0, "Aligns coordinates of the quad points"))
	{
		for(int k = 1; k < 4; k++)
		{
			pQuad->m_aPoints[k].x = 1000.0f * (int(pQuad->m_aPoints[k].x) / 1000);
			pQuad->m_aPoints[k].y = 1000.0f * (int(pQuad->m_aPoints[k].y) / 1000);
		}
		pEditor->m_Map.m_Modified = true;
		return 1;
	}

	// square button
	View.HSplitBottom(6.0f, &View, &Button);
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_Button = 0;
	if(pEditor->DoButton_Editor(&s_Button, "Square", 0, &Button, 0, "Squares the current quad"))
	{
		int Top = pQuad->m_aPoints[0].y;
		int Left = pQuad->m_aPoints[0].x;
		int Bottom = pQuad->m_aPoints[0].y;
		int Right = pQuad->m_aPoints[0].x;

		for(int k = 1; k < 4; k++)
		{
			if(pQuad->m_aPoints[k].y < Top) Top = pQuad->m_aPoints[k].y;
			if(pQuad->m_aPoints[k].x < Left) Left = pQuad->m_aPoints[k].x;
			if(pQuad->m_aPoints[k].y > Bottom) Bottom = pQuad->m_aPoints[k].y;
			if(pQuad->m_aPoints[k].x > Right) Right = pQuad->m_aPoints[k].x;
		}

		pQuad->m_aPoints[0].x = Left; pQuad->m_aPoints[0].y = Top;
		pQuad->m_aPoints[1].x = Right; pQuad->m_aPoints[1].y = Top;
		pQuad->m_aPoints[2].x = Left; pQuad->m_aPoints[2].y = Bottom;
		pQuad->m_aPoints[3].x = Right; pQuad->m_aPoints[3].y = Bottom;
		pEditor->m_Map.m_Modified = true;
		return 1;
	}


	enum
	{
		PROP_POS_X=0,
		PROP_POS_Y,
		PROP_POS_ENV,
		PROP_POS_ENV_OFFSET,
		PROP_COLOR_ENV,
		PROP_COLOR_ENV_OFFSET,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Pos X", pQuad->m_aPoints[4].x/1000, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", pQuad->m_aPoints[4].y/1000, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos. Env", pQuad->m_PosEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{"Pos. TO", pQuad->m_PosEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Color Env", pQuad->m_ColorEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{"Color TO", pQuad->m_ColorEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},

		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_POS_X)
	{
		float Offset = NewVal*1000-pQuad->m_aPoints[4].x;
		for(int k = 0; k < 5; ++k)
			pQuad->m_aPoints[k].x += Offset;
	}
	if(Prop == PROP_POS_Y)
	{
		float Offset = NewVal*1000-pQuad->m_aPoints[4].y;
		for(int k = 0; k < 5; ++k)
			pQuad->m_aPoints[k].y += Offset;
	}
	if(Prop == PROP_POS_ENV)
	{
		int Index = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
		int Step = (Index-pQuad->m_PosEnv)%2;
		if(Step != 0)
		{
			for(; Index >= -1 && Index < pEditor->m_Map.m_lEnvelopes.size(); Index += Step)
				if(Index == -1 || pEditor->m_Map.m_lEnvelopes[Index]->m_Channels == 3)
				{
					pQuad->m_PosEnv = Index;
					break;
				}
		}
	}
	if(Prop == PROP_POS_ENV_OFFSET) pQuad->m_PosEnvOffset = NewVal;
	if(Prop == PROP_COLOR_ENV)
	{
		int Index = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
		int Step = (Index-pQuad->m_ColorEnv)%2;
		if(Step != 0)
		{
			for(; Index >= -1 && Index < pEditor->m_Map.m_lEnvelopes.size(); Index += Step)
				if(Index == -1 || pEditor->m_Map.m_lEnvelopes[Index]->m_Channels == 4)
				{
					pQuad->m_ColorEnv = Index;
					break;
				}
		}
	}
	if(Prop == PROP_COLOR_ENV_OFFSET) pQuad->m_ColorEnvOffset = NewVal;

	return 0;
}

int CEditor::PopupSource(CEditor *pEditor, CUIRect View)
{
	CSoundSource *pSource = pEditor->GetSelectedSource();

	CUIRect Button;

	// delete button
	View.HSplitBottom(12.0f, &View, &Button);
	static int s_DeleteButton = 0;
	if(pEditor->DoButton_Editor(&s_DeleteButton, "Delete", 0, &Button, 0, "Deletes the current source"))
	{
		CLayerSounds *pLayer = (CLayerSounds *)pEditor->GetSelectedLayerType(0, LAYERTYPE_SOUNDS);
		if(pLayer)
		{
			pEditor->m_Map.m_Modified = true;
			pLayer->m_lSources.remove_index(pEditor->m_SelectedSource);
			pEditor->m_SelectedSource--;
		}
		return 1;
	}

	// Sound shape button
	CUIRect ShapeButton;
	View.HSplitBottom(3.0f, &View, 0x0);
	View.HSplitBottom(12.0f, &View, &ShapeButton);
	static int s_ShapeTypeButton = 0;

	static const char *s_aShapeNames[] = {
		"Rectangle",
		"Circle"
	};

	pSource->m_Shape.m_Type = pSource->m_Shape.m_Type%CSoundShape::NUM_SHAPES; // prevent out of array errors

	if(pEditor->DoButton_Editor(&s_ShapeTypeButton, s_aShapeNames[pSource->m_Shape.m_Type], 0, &ShapeButton, 0, "Change shape"))
	{
		pSource->m_Shape.m_Type = (pSource->m_Shape.m_Type+1)%CSoundShape::NUM_SHAPES;

		// set default values
		switch(pSource->m_Shape.m_Type)
		{
		case CSoundShape::SHAPE_CIRCLE:
			{
				pSource->m_Shape.m_Circle.m_Radius = 1000.0f;
				break;
			}
		case CSoundShape::SHAPE_RECTANGLE:
			{
				pSource->m_Shape.m_Rectangle.m_Width = f2fx(1000.0f);
				pSource->m_Shape.m_Rectangle.m_Height = f2fx(800.0f);
				break;
			}
		}
	}


	enum
	{
		PROP_POS_X=0,
		PROP_POS_Y,
		PROP_LOOP,
		PROP_PAN,
		PROP_TIME_DELAY,
		PROP_FALLOFF,
		PROP_POS_ENV,
		PROP_POS_ENV_OFFSET,
		PROP_SOUND_ENV,
		PROP_SOUND_ENV_OFFSET,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Pos X", pSource->m_Position.x/1000, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", pSource->m_Position.y/1000, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Loop", pSource->m_Loop, PROPTYPE_BOOL, 0, 1},
		{"Pan", pSource->m_Pan, PROPTYPE_BOOL, 0, 1},
		{"Delay", pSource->m_TimeDelay, PROPTYPE_INT_SCROLL, 0, 1000000},
		{"Falloff", pSource->m_Falloff, PROPTYPE_INT_SCROLL, 0, 255},
		{"Pos. Env", pSource->m_PosEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{"Pos. TO", pSource->m_PosEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Sound Env", pSource->m_SoundEnv+1, PROPTYPE_INT_STEP, 0, pEditor->m_Map.m_lEnvelopes.size()+1},
		{"Sound. TO", pSource->m_PosEnvOffset, PROPTYPE_INT_SCROLL, -1000000, 1000000},

		{0},
	};



	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_POS_X) pSource->m_Position.x = NewVal*1000;
	if(Prop == PROP_POS_Y) pSource->m_Position.y = NewVal*1000;
	if(Prop == PROP_LOOP) pSource->m_Loop = NewVal;
	if(Prop == PROP_PAN) pSource->m_Pan = NewVal;
	if(Prop == PROP_TIME_DELAY) pSource->m_TimeDelay = NewVal;
	if(Prop == PROP_FALLOFF) pSource->m_Falloff = NewVal;
	if(Prop == PROP_POS_ENV)
	{
		int Index = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
		int Step = (Index-pSource->m_PosEnv)%2;
		if(Step != 0)
		{
			for(; Index >= -1 && Index < pEditor->m_Map.m_lEnvelopes.size(); Index += Step)
				if(Index == -1 || pEditor->m_Map.m_lEnvelopes[Index]->m_Channels == 3)
				{
					pSource->m_PosEnv = Index;
					break;
				}
		}
	}
	if(Prop == PROP_POS_ENV_OFFSET) pSource->m_PosEnvOffset = NewVal;
	if(Prop == PROP_SOUND_ENV)
	{
		int Index = clamp(NewVal-1, -1, pEditor->m_Map.m_lEnvelopes.size()-1);
		int Step = (Index-pSource->m_SoundEnv)%2;
		if(Step != 0)
		{
			for(; Index >= -1 && Index < pEditor->m_Map.m_lEnvelopes.size(); Index += Step)
				if(Index == -1 || pEditor->m_Map.m_lEnvelopes[Index]->m_Channels == 1)
				{
					pSource->m_SoundEnv = Index;
					break;
				}
		}
	}
	if(Prop == PROP_SOUND_ENV_OFFSET) pSource->m_SoundEnvOffset = NewVal;

	// source shape properties
	switch(pSource->m_Shape.m_Type)
	{
	case CSoundShape::SHAPE_CIRCLE:
		{
			enum
			{
				PROP_CIRCLE_RADIUS=0,
				NUM_CIRCLE_PROPS,
			};

			CProperty aCircleProps[] = {
				{"Radius", pSource->m_Shape.m_Circle.m_Radius, PROPTYPE_INT_SCROLL, 0, 1000000},

				{0},
			};

			static int s_aCircleIds[NUM_CIRCLE_PROPS] = {0};

			NewVal = 0;
			Prop = pEditor->DoProperties(&View, aCircleProps, s_aCircleIds, &NewVal);
			if(Prop != -1)
				pEditor->m_Map.m_Modified = true;

			if(Prop == PROP_CIRCLE_RADIUS) pSource->m_Shape.m_Circle.m_Radius = NewVal;

			break;
		}

	case CSoundShape::SHAPE_RECTANGLE:
		{
			enum
			{
				PROP_RECTANGLE_WIDTH=0,
				PROP_RECTANGLE_HEIGHT,
				NUM_RECTANGLE_PROPS,
			};

			CProperty aRectangleProps[] = {
				{"Width", pSource->m_Shape.m_Rectangle.m_Width/1024, PROPTYPE_INT_SCROLL, 0, 1000000},
				{"Height", pSource->m_Shape.m_Rectangle.m_Height/1024, PROPTYPE_INT_SCROLL, 0, 1000000},

				{0},
			};

			static int s_aRectangleIds[NUM_RECTANGLE_PROPS] = {0};

			NewVal = 0;
			Prop = pEditor->DoProperties(&View, aRectangleProps, s_aRectangleIds, &NewVal);
			if(Prop != -1)
				pEditor->m_Map.m_Modified = true;

			if(Prop == PROP_RECTANGLE_WIDTH) pSource->m_Shape.m_Rectangle.m_Width = NewVal*1024;
			if(Prop == PROP_RECTANGLE_HEIGHT) pSource->m_Shape.m_Rectangle.m_Height = NewVal*1024;

			break;
		}
	}


	return 0;
}

int CEditor::PopupPoint(CEditor *pEditor, CUIRect View)
{
	CQuad *pQuad = pEditor->GetSelectedQuad();

	enum
	{
		PROP_POS_X=0,
		PROP_POS_Y,
		PROP_COLOR,
		NUM_PROPS,
	};

	int Color = 0;
	int x = 0, y = 0;

	for(int v = 0; v < 4; v++)
	{
		if(pEditor->m_SelectedPoints&(1<<v))
		{
			Color = 0;
			Color |= pQuad->m_aColors[v].r<<24;
			Color |= pQuad->m_aColors[v].g<<16;
			Color |= pQuad->m_aColors[v].b<<8;
			Color |= pQuad->m_aColors[v].a;

			x = pQuad->m_aPoints[v].x/1000;
			y = pQuad->m_aPoints[v].y/1000;
		}
	}


	CProperty aProps[] = {
		{"Pos X", x, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Pos Y", y, PROPTYPE_INT_SCROLL, -1000000, 1000000},
		{"Color", Color, PROPTYPE_COLOR, -1, pEditor->m_Map.m_lEnvelopes.size()},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);
	if(Prop != -1)
		pEditor->m_Map.m_Modified = true;

	if(Prop == PROP_POS_X)
	{
		for(int v = 0; v < 4; v++)
			if(pEditor->m_SelectedPoints&(1<<v))
				pQuad->m_aPoints[v].x = NewVal*1000;
	}
	if(Prop == PROP_POS_Y)
	{
		for(int v = 0; v < 4; v++)
			if(pEditor->m_SelectedPoints&(1<<v))
				pQuad->m_aPoints[v].y = NewVal*1000;
	}
	if(Prop == PROP_COLOR)
	{
		for(int v = 0; v < 4; v++)
		{
			if(pEditor->m_SelectedPoints&(1<<v))
			{
				pQuad->m_aColors[v].r = (NewVal>>24)&0xff;
				pQuad->m_aColors[v].g = (NewVal>>16)&0xff;
				pQuad->m_aColors[v].b = (NewVal>>8)&0xff;
				pQuad->m_aColors[v].a = NewVal&0xff;
			}
		}
	}

	return 0;
}

int CEditor::PopupNewFolder(CEditor *pEditor, CUIRect View)
{
	CUIRect Label, ButtonBar;

	// title
	View.HSplitTop(10.0f, 0, &View);
	View.HSplitTop(30.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Create new folder", 20.0f, 0);

	View.HSplitBottom(10.0f, &View, 0);
	View.HSplitBottom(20.0f, &View, &ButtonBar);

	if(pEditor->m_FileDialogErrString[0] == 0)
	{
		// interaction box
		View.HSplitBottom(40.0f, &View, 0);
		View.VMargin(40.0f, &View);
		View.HSplitBottom(20.0f, &View, &Label);
		static float s_FolderBox = 0;
		pEditor->DoEditBox(&s_FolderBox, &Label, pEditor->m_FileDialogNewFolderName, sizeof(pEditor->m_FileDialogNewFolderName), 15.0f, &s_FolderBox);
		View.HSplitBottom(20.0f, &View, &Label);
		pEditor->UI()->DoLabel(&Label, "Name:", 10.0f, -1);

		// button bar
		ButtonBar.VSplitLeft(30.0f, 0, &ButtonBar);
		ButtonBar.VSplitLeft(110.0f, &Label, &ButtonBar);
		static int s_CreateButton = 0;
		if(pEditor->DoButton_Editor(&s_CreateButton, "Create", 0, &Label, 0, 0))
		{
			// create the folder
			if(*pEditor->m_FileDialogNewFolderName)
			{
				char aBuf[512];
				str_format(aBuf, sizeof(aBuf), "%s/%s", pEditor->m_pFileDialogPath, pEditor->m_FileDialogNewFolderName);
				if(pEditor->Storage()->CreateFolder(aBuf, IStorageTW::TYPE_SAVE))
				{
					pEditor->FilelistPopulate(IStorageTW::TYPE_SAVE);
					return 1;
				}
				else
					str_copy(pEditor->m_FileDialogErrString, "Unable to create the folder", sizeof(pEditor->m_FileDialogErrString));
			}
		}
		ButtonBar.VSplitRight(30.0f, &ButtonBar, 0);
		ButtonBar.VSplitRight(110.0f, &ButtonBar, &Label);
		static int s_AbortButton = 0;
		if(pEditor->DoButton_Editor(&s_AbortButton, "Abort", 0, &Label, 0, 0))
			return 1;
	}
	else
	{
		// error text
		View.HSplitTop(30.0f, 0, &View);
		View.VMargin(40.0f, &View);
		View.HSplitTop(20.0f, &Label, &View);
		pEditor->UI()->DoLabel(&Label, "Error:", 10.0f, -1);
		View.HSplitTop(20.0f, &Label, &View);
		pEditor->UI()->DoLabel(&Label, "Unable to create the folder", 10.0f, -1, View.w);

		// button
		ButtonBar.VMargin(ButtonBar.w/2.0f-55.0f, &ButtonBar);
		static int s_CreateButton = 0;
		if(pEditor->DoButton_Editor(&s_CreateButton, "Ok", 0, &ButtonBar, 0, 0))
			return 1;
	}

	return 0;
}

int CEditor::PopupMapInfo(CEditor *pEditor, CUIRect View)
{
	CUIRect Label, ButtonBar, Button;

	// title
	View.HSplitTop(10.0f, 0, &View);
	View.HSplitTop(30.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Map details", 20.0f, 0);

	View.HSplitBottom(10.0f, &View, 0);
	View.HSplitBottom(20.0f, &View, &ButtonBar);

	View.VMargin(40.0f, &View);

	// author box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Author:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_AuthorBox = 0;
	pEditor->DoEditBox(&s_AuthorBox, &Button, pEditor->m_Map.m_MapInfo.m_aAuthorTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aAuthorTmp), 10.0f, &s_AuthorBox);

	// version box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Version:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_VersionBox = 0;
	pEditor->DoEditBox(&s_VersionBox, &Button, pEditor->m_Map.m_MapInfo.m_aVersionTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aVersionTmp), 10.0f, &s_VersionBox);

	// credits box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "Credits:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_CreditsBox = 0;
	pEditor->DoEditBox(&s_CreditsBox, &Button, pEditor->m_Map.m_MapInfo.m_aCreditsTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aCreditsTmp), 10.0f, &s_CreditsBox);

	// license box
	View.HSplitTop(20.0f, &Label, &View);
	pEditor->UI()->DoLabel(&Label, "License:", 10.0f, -1);
	Label.VSplitLeft(40.0f, 0, &Button);
	Button.HSplitTop(12.0f, &Button, 0);
	static float s_LicenseBox = 0;
	pEditor->DoEditBox(&s_LicenseBox, &Button, pEditor->m_Map.m_MapInfo.m_aLicenseTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aLicenseTmp), 10.0f, &s_LicenseBox);

	// button bar
	ButtonBar.VSplitLeft(30.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(110.0f, &Label, &ButtonBar);
	static int s_CreateButton = 0;
	if(pEditor->DoButton_Editor(&s_CreateButton, "Ok", 0, &Label, 0, 0))
	{
		str_copy(pEditor->m_Map.m_MapInfo.m_aAuthor, pEditor->m_Map.m_MapInfo.m_aAuthorTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aAuthor));
		str_copy(pEditor->m_Map.m_MapInfo.m_aVersion, pEditor->m_Map.m_MapInfo.m_aVersionTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aVersion));
		str_copy(pEditor->m_Map.m_MapInfo.m_aCredits, pEditor->m_Map.m_MapInfo.m_aCreditsTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aCredits));
		str_copy(pEditor->m_Map.m_MapInfo.m_aLicense, pEditor->m_Map.m_MapInfo.m_aLicenseTmp, sizeof(pEditor->m_Map.m_MapInfo.m_aLicense));
		return 1;
	}

	ButtonBar.VSplitRight(30.0f, &ButtonBar, 0);
	ButtonBar.VSplitRight(110.0f, &ButtonBar, &Label);
	static int s_AbortButton = 0;
	if(pEditor->DoButton_Editor(&s_AbortButton, "Abort", 0, &Label, 0, 0))
		return 1;

	return 0;
}

int CEditor::PopupEvent(CEditor *pEditor, CUIRect View)
{
	CUIRect Label, ButtonBar;

	// title
	View.HSplitTop(10.0f, 0, &View);
	View.HSplitTop(30.0f, &Label, &View);
	if(pEditor->m_PopupEventType == POPEVENT_EXIT)
		pEditor->UI()->DoLabel(&Label, "Exit the editor", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_LOAD)
		pEditor->UI()->DoLabel(&Label, "Load map", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_NEW)
		pEditor->UI()->DoLabel(&Label, "New map", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_SAVE)
		pEditor->UI()->DoLabel(&Label, "Save map", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_LARGELAYER)
		pEditor->UI()->DoLabel(&Label, "Large layer", 20.0f, 0);
	else if(pEditor->m_PopupEventType == POPEVENT_PREVENTUNUSEDTILES)
		pEditor->UI()->DoLabel(&Label, "Unused tiles disabled", 20.0f, 0);

	View.HSplitBottom(10.0f, &View, 0);
	View.HSplitBottom(20.0f, &View, &ButtonBar);

	// notification text
	View.HSplitTop(30.0f, 0, &View);
	View.VMargin(40.0f, &View);
	View.HSplitTop(20.0f, &Label, &View);
	if(pEditor->m_PopupEventType == POPEVENT_EXIT)
		pEditor->UI()->DoLabel(&Label, "The map contains unsaved data, you might want to save it before you exit the editor.\nContinue anyway?", 10.0f, -1, Label.w-10.0f);
	else if((pEditor->m_PopupEventType == POPEVENT_LOAD)
	||(pEditor->m_PopupEventType == POPEVENT_LOADCURRENT))
		pEditor->UI()->DoLabel(&Label, "The map contains unsaved data, you might want to save it before you load a new map.\nContinue anyway?", 10.0f, -1, Label.w-10.0f);
	else if(pEditor->m_PopupEventType == POPEVENT_NEW)
		pEditor->UI()->DoLabel(&Label, "The map contains unsaved data, you might want to save it before you create a new map.\nContinue anyway?", 10.0f, -1, Label.w-10.0f);
	else if(pEditor->m_PopupEventType == POPEVENT_SAVE)
		pEditor->UI()->DoLabel(&Label, "The file already exists.\nDo you want to overwrite the map?", 10.0f, -1);
	else if(pEditor->m_PopupEventType == POPEVENT_LARGELAYER)
		pEditor->UI()->DoLabel(&Label, "You are trying to set the height or width of a layer to more than 1000 tiles. This is actually possible, but only rarely necessary. It may cause the editor to work slower, larger file size as well as higher memory usage for client and server.", 10.0f, -1, Label.w-10.0f);
	else if(pEditor->m_PopupEventType == POPEVENT_PREVENTUNUSEDTILES)
		pEditor->UI()->DoLabel(&Label, "Unused tiles can't be placed by default because they could get a use later and then destroy your map. If you are mapping for a different gametype you can activate the 'Unused' switch to be able to place every tile.", 10.0f, -1, Label.w-10.0f);

	// button bar
	ButtonBar.VSplitLeft(30.0f, 0, &ButtonBar);
	ButtonBar.VSplitLeft(110.0f, &Label, &ButtonBar);
	static int s_OkButton = 0;
	if(pEditor->DoButton_Editor(&s_OkButton, "Ok", 0, &Label, 0, 0))
	{
		if(pEditor->m_PopupEventType == POPEVENT_EXIT)
			g_Config.m_ClEditor = 0;
		else if(pEditor->m_PopupEventType == POPEVENT_LOAD)
			pEditor->InvokeFileDialog(IStorageTW::TYPE_ALL, FILETYPE_MAP, "Load map", "Load", "maps", "", pEditor->CallbackOpenMap, pEditor);
		else if(pEditor->m_PopupEventType == POPEVENT_LOADCURRENT)
			pEditor->LoadCurrentMap();
		else if(pEditor->m_PopupEventType == POPEVENT_NEW)
		{
			pEditor->Reset();
			pEditor->m_aFileName[0] = 0;
		}
		else if(pEditor->m_PopupEventType == POPEVENT_SAVE)
			pEditor->CallbackSaveMap(pEditor->m_aFileSaveName, IStorageTW::TYPE_SAVE, pEditor);
		pEditor->m_PopupEventWasActivated = false;
		return 1;
	}
	ButtonBar.VSplitRight(30.0f, &ButtonBar, 0);
	ButtonBar.VSplitRight(110.0f, &ButtonBar, &Label);
	if(pEditor->m_PopupEventType != POPEVENT_LARGELAYER && pEditor->m_PopupEventType != POPEVENT_PREVENTUNUSEDTILES)
	{
		static int s_AbortButton = 0;
		if(pEditor->DoButton_Editor(&s_AbortButton, "Abort", 0, &Label, 0, 0))
		{
			pEditor->m_PopupEventWasActivated = false;
			return 1;
		}
	}

	return 0;
}


static int g_SelectImageSelected = -100;
static int g_SelectImageCurrent = -100;

int CEditor::PopupSelectImage(CEditor *pEditor, CUIRect View)
{
	CUIRect ButtonBar, ImageView;
	View.VSplitLeft(80.0f, &ButtonBar, &View);
	View.Margin(10.0f, &ImageView);

	int ShowImage = g_SelectImageCurrent;

	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;
	float ImagesHeight = pEditor->m_Map.m_lImages.size() * 14;
	float ScrollDifference = ImagesHeight - ButtonBar.h;

	if(pEditor->m_Map.m_lImages.size() > 20) // Do we need a scrollbar?
	{
		CUIRect Scroll;
		ButtonBar.VSplitRight(15.0f, &ButtonBar, &Scroll);
		ButtonBar.VSplitRight(3.0f, &ButtonBar, 0);	// extra spacing
		Scroll.HMargin(5.0f, &Scroll);
		s_ScrollValue = pEditor->UiDoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

		if(pEditor->UI()->MouseInside(&Scroll) || pEditor->UI()->MouseInside(&ButtonBar))
		{
			int ScrollNum = (int)((ImagesHeight-ButtonBar.h)/14.0f)+1;
			if(ScrollNum > 0)
			{
				if(pEditor->Input()->KeyPress(KEY_MOUSE_WHEEL_UP))
					s_ScrollValue = clamp(s_ScrollValue - 1.0f/ScrollNum, 0.0f, 1.0f);
				if(pEditor->Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN))
					s_ScrollValue = clamp(s_ScrollValue + 1.0f/ScrollNum, 0.0f, 1.0f);
			}
		}
	}

	float ImageStartAt = ScrollDifference * s_ScrollValue;
	if(ImageStartAt < 0.0f)
		ImageStartAt = 0.0f;

	float ImageStopAt = ImagesHeight - ScrollDifference * (1 - s_ScrollValue);
	float ImageCur = 0.0f;
	for(int i = -1; i < pEditor->m_Map.m_lImages.size(); i++)
	{
		if(ImageCur > ImageStopAt)
			break;
		if(ImageCur < ImageStartAt)
		{
			ImageCur += 14.0f;
			continue;
		}
		ImageCur += 14.0f;

		CUIRect Button;
		ButtonBar.HSplitTop(14.0f, &Button, &ButtonBar);

		if(pEditor->UI()->MouseInside(&Button))
			ShowImage = i;

		if(i == -1)
		{
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lImages[i], "None", i==g_SelectImageCurrent, &Button))
				g_SelectImageSelected = -1;
		}
		else
		{
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lImages[i], pEditor->m_Map.m_lImages[i]->m_aName, i==g_SelectImageCurrent, &Button))
				g_SelectImageSelected = i;
		}
	}

	if(ShowImage >= 0 && ShowImage < pEditor->m_Map.m_lImages.size())
	{
		if(ImageView.h < ImageView.w)
			ImageView.w = ImageView.h;
		else
			ImageView.h = ImageView.w;
		float Max = (float)(max(pEditor->m_Map.m_lImages[ShowImage]->m_Width, pEditor->m_Map.m_lImages[ShowImage]->m_Height));
		ImageView.w *= pEditor->m_Map.m_lImages[ShowImage]->m_Width/Max;
		ImageView.h *= pEditor->m_Map.m_lImages[ShowImage]->m_Height/Max;
		pEditor->Graphics()->TextureSet(pEditor->m_Map.m_lImages[ShowImage]->m_TexID);
		pEditor->Graphics()->BlendNormal();
		pEditor->Graphics()->WrapClamp();
		pEditor->Graphics()->QuadsBegin();
		IGraphics::CQuadItem QuadItem(ImageView.x, ImageView.y, ImageView.w, ImageView.h);
		pEditor->Graphics()->QuadsDrawTL(&QuadItem, 1);
		pEditor->Graphics()->QuadsEnd();
		pEditor->Graphics()->WrapNormal();
	}

	return 0;
}

void CEditor::PopupSelectImageInvoke(int Current, float x, float y)
{
	static int s_SelectImagePopupId = 0;
	g_SelectImageSelected = -100;
	g_SelectImageCurrent = Current;
	UiInvokePopupMenu(&s_SelectImagePopupId, 0, x, y, 400, 300, PopupSelectImage);
}

int CEditor::PopupSelectImageResult()
{
	if(g_SelectImageSelected == -100)
		return -100;

	g_SelectImageCurrent = g_SelectImageSelected;
	g_SelectImageSelected = -100;
	return g_SelectImageCurrent;
}

static int g_SelectSoundSelected = -100;
static int g_SelectSoundCurrent = -100;

int CEditor::PopupSelectSound(CEditor *pEditor, CUIRect View)
{
	CUIRect ButtonBar, SoundView;
	View.VSplitLeft(80.0f, &ButtonBar, &View);
	View.Margin(10.0f, &SoundView);


	static int s_ScrollBar = 0;
	static float s_ScrollValue = 0;
	float SoundsHeight = pEditor->m_Map.m_lSounds.size() * 14;
	float ScrollDifference = SoundsHeight - ButtonBar.h;

	if(pEditor->m_Map.m_lSounds.size() > 20) // Do we need a scrollbar?
	{
		CUIRect Scroll;
		ButtonBar.VSplitRight(15.0f, &ButtonBar, &Scroll);
		ButtonBar.VSplitRight(3.0f, &ButtonBar, 0);	// extra spacing
		Scroll.HMargin(5.0f, &Scroll);
		s_ScrollValue = pEditor->UiDoScrollbarV(&s_ScrollBar, &Scroll, s_ScrollValue);

		if(pEditor->UI()->MouseInside(&Scroll) || pEditor->UI()->MouseInside(&ButtonBar))
		{
			int ScrollNum = (int)((SoundsHeight-ButtonBar.h)/14.0f)+1;
			if(ScrollNum > 0)
			{
				if(pEditor->Input()->KeyPress(KEY_MOUSE_WHEEL_UP))
					s_ScrollValue = clamp(s_ScrollValue - 1.0f/ScrollNum, 0.0f, 1.0f);
				if(pEditor->Input()->KeyPress(KEY_MOUSE_WHEEL_DOWN))
					s_ScrollValue = clamp(s_ScrollValue + 1.0f/ScrollNum, 0.0f, 1.0f);
			}
		}
	}

	float SoundStartAt = ScrollDifference * s_ScrollValue;
	if(SoundStartAt < 0.0f)
		SoundStartAt = 0.0f;

	float SoundStopAt = SoundsHeight - ScrollDifference * (1 - s_ScrollValue);
	float SoundCur = 0.0f;
	for(int i = -1; i < pEditor->m_Map.m_lSounds.size(); i++)
	{
		if(SoundCur > SoundStopAt)
			break;
		if(SoundCur < SoundStartAt)
		{
			SoundCur += 14.0f;
			continue;
		}
		SoundCur += 14.0f;

		CUIRect Button;
		ButtonBar.HSplitTop(14.0f, &Button, &ButtonBar);

		//if(pEditor->UI()->MouseInside(&Button))
		//	ShowSound = i;

		if(i == -1)
		{
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lSounds[i], "None", i==g_SelectSoundCurrent, &Button))
				g_SelectSoundSelected = -1;
		}
		else
		{
			if(pEditor->DoButton_MenuItem(&pEditor->m_Map.m_lSounds[i], pEditor->m_Map.m_lSounds[i]->m_aName, i==g_SelectSoundCurrent, &Button))
				g_SelectSoundSelected = i;
		}
	}

	return 0;
}

void CEditor::PopupSelectSoundInvoke(int Current, float x, float y)
{
	static int s_SelectSoundPopupId = 0;
	g_SelectSoundSelected = -100;
	g_SelectSoundCurrent = Current;
	UiInvokePopupMenu(&s_SelectSoundPopupId, 0, x, y, 400, 300, PopupSelectSound);
}

int CEditor::PopupSelectSoundResult()
{
	if(g_SelectSoundSelected == -100)
		return -100;

	g_SelectSoundCurrent = g_SelectSoundSelected;
	g_SelectSoundSelected = -100;
	return g_SelectSoundCurrent;
}

static int s_GametileOpSelected = -1;

int CEditor::PopupSelectGametileOp(CEditor *pEditor, CUIRect View)
{
	static const char *s_pButtonNames[] = { "Air", "Hookable", "Death", "Unhookable", "Hookthrough", "Freeze", "Unfreeze", "Deep Freeze", "Deep Unfreeze", "Blue Check-Tele", "Red Check-Tele" };
	static unsigned s_NumButtons = sizeof(s_pButtonNames) / sizeof(char*);
	CUIRect Button;

	for(unsigned i = 0; i < s_NumButtons; ++i)
	{
		View.HSplitTop(2.0f, 0, &View);
		View.HSplitTop(12.0f, &Button, &View);
		if(pEditor->DoButton_Editor(&s_pButtonNames[i], s_pButtonNames[i], 0, &Button, 0, 0))
			s_GametileOpSelected = i;
	}

	return 0;
}

void CEditor::PopupSelectGametileOpInvoke(float x, float y)
{
	static int s_SelectGametileOpPopupId = 0;
	s_GametileOpSelected = -1;
	UiInvokePopupMenu(&s_SelectGametileOpPopupId, 0, x, y, 120.0f, 165.0f, PopupSelectGametileOp);
}

int CEditor::PopupSelectGameTileOpResult()
{
	if(s_GametileOpSelected < 0)
		return -1;

	int Result = s_GametileOpSelected;
	s_GametileOpSelected = -1;
	return Result;
}

static int s_AutoMapConfigSelected = -1;

int CEditor::PopupSelectConfigAutoMap(CEditor *pEditor, CUIRect View)
{
	CLayerTiles *pLayer = static_cast<CLayerTiles*>(pEditor->GetSelectedLayer(0));
	CUIRect Button;
	static int s_AutoMapperConfigButtons[256];
	CAutoMapper *pAutoMapper = &pEditor->m_Map.m_lImages[pLayer->m_Image]->m_AutoMapper;

	for(int i = 0; i < pAutoMapper->ConfigNamesNum(); ++i)
	{
		View.HSplitTop(2.0f, 0, &View);
		View.HSplitTop(12.0f, &Button, &View);
		if(pEditor->DoButton_Editor(&s_AutoMapperConfigButtons[i], pAutoMapper->GetConfigName(i), 0, &Button, 0, 0))
			s_AutoMapConfigSelected = i;
	}

	return 0;
}

void CEditor::PopupSelectConfigAutoMapInvoke(float x, float y)
{
	static int s_AutoMapConfigSelectID = 0;
	s_AutoMapConfigSelected = -1;
	CLayerTiles *pLayer = static_cast<CLayerTiles*>(GetSelectedLayer(0));
	if(pLayer && pLayer->m_Image >= 0 && pLayer->m_Image < m_Map.m_lImages.size() &&
		m_Map.m_lImages[pLayer->m_Image]->m_AutoMapper.ConfigNamesNum())
		UiInvokePopupMenu(&s_AutoMapConfigSelectID, 0, x, y, 120.0f, 12.0f+14.0f*m_Map.m_lImages[pLayer->m_Image]->m_AutoMapper.ConfigNamesNum(), PopupSelectConfigAutoMap);
}

int CEditor::PopupSelectConfigAutoMapResult()
{
	if(s_AutoMapConfigSelected < 0)
		return -1;

	int Result = s_AutoMapConfigSelected;
	s_AutoMapConfigSelected = -1;
	return Result;
}

// DDRace

int CEditor::PopupTele(CEditor *pEditor, CUIRect View)
{
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);

	enum
	{
		PROP_TELE=0,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Number", pEditor->m_TeleNumber, PROPTYPE_INT_STEP, 0, 255},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	static vec4 s_color = vec4(1,1,1,0.5f);

	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal, s_color);

	if(Prop == PROP_TELE)
	{
		NewVal = (NewVal + 256) % 256;

		CLayerTele *gl = pEditor->m_Map.m_pTeleLayer;
		for(int y = 0; y < gl->m_Height; ++y)
		{
			for(int x = 0; x < gl->m_Width; ++x)
			{
				if(gl->m_pTeleTile[y*gl->m_Width+x].m_Number == NewVal)
				{
					s_color = vec4(1,0.5f,0.5f,0.5f);
					goto done;
				}
			}
		}

		s_color = vec4(0.5f,1,0.5f,0.5f);

		done:
		pEditor->m_TeleNumber = NewVal;
	}

	return 0;
}

int CEditor::PopupSpeedup(CEditor *pEditor, CUIRect View)
{
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);

	enum
	{
		PROP_FORCE=0,
		PROP_MAXSPEED,
		PROP_ANGLE,
		NUM_PROPS
	};

	CProperty aProps[] = {
		{"Force", pEditor->m_SpeedupForce, PROPTYPE_INT_STEP, 0, 255},
		{"Max Speed", pEditor->m_SpeedupMaxSpeed, PROPTYPE_INT_STEP, 0, 255},
		{"Angle", pEditor->m_SpeedupAngle, PROPTYPE_ANGLE_SCROLL, 0, 359},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);

	if(Prop == PROP_FORCE)
		pEditor->m_SpeedupForce = clamp(NewVal, 0, 255);
	if(Prop == PROP_MAXSPEED)
		pEditor->m_SpeedupMaxSpeed = clamp(NewVal, 0, 255);
	if(Prop == PROP_ANGLE)
		pEditor->m_SpeedupAngle = clamp(NewVal, 0, 359);

	return 0;
}

int CEditor::PopupSwitch(CEditor *pEditor, CUIRect View)
{
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);

	enum
	{
		PROP_SwitchDelay=0,
		PROP_SwitchNumber,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Delay", pEditor->m_SwitchDelay, PROPTYPE_INT_STEP, 0, 255},
		{"Number", pEditor->m_SwitchNum, PROPTYPE_INT_STEP, 0, 255},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	static vec4 s_color = vec4(1,1,1,0.5f);
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal, s_color);

	if(Prop == PROP_SwitchNumber)
	{
		NewVal = (NewVal + 256) % 256;

		CLayerSwitch *gl = pEditor->m_Map.m_pSwitchLayer;
		for(int y = 0; y < gl->m_Height; ++y)
		{
			for(int x = 0; x < gl->m_Width; ++x)
			{
				if(gl->m_pSwitchTile[y*gl->m_Width+x].m_Number == NewVal)
				{
					s_color = vec4(1,0.5f,0.5f,0.5f);
					goto done;
				}
			}
		}

		s_color = vec4(0.5f,1,0.5f,0.5f);

		done:
		pEditor->m_SwitchNum = NewVal;
	}
	if(Prop == PROP_SwitchDelay)
		pEditor->m_SwitchDelay = (NewVal + 256) % 256;

	return 0;
}

int CEditor::PopupTune(CEditor *pEditor, CUIRect View)
{
	CUIRect Button;
	View.HSplitBottom(12.0f, &View, &Button);

	enum
	{
		PROP_TUNE=0,
		NUM_PROPS,
	};

	CProperty aProps[] = {
		{"Zone", pEditor->m_TuningNum, PROPTYPE_INT_STEP, 1, 255},
		{0},
	};

	static int s_aIds[NUM_PROPS] = {0};
	int NewVal = 0;
	int Prop = pEditor->DoProperties(&View, aProps, s_aIds, &NewVal);

	if(Prop == PROP_TUNE)
		pEditor->m_TuningNum = (NewVal - 1 + 255) % 255 + 1;

	return 0;
}

int CEditor::PopupColorPicker(CEditor *pEditor, CUIRect View)
{
	CUIRect SVPicker, HuePicker;
	View.VSplitRight(20.0f, &SVPicker, &HuePicker);
	HuePicker.VSplitLeft(4.0f, 0x0, &HuePicker);

	pEditor->Graphics()->TextureSet(-1);
	pEditor->Graphics()->QuadsBegin();

	// base: white - hue
	vec3 hsv = pEditor->ms_PickerColor;
	IGraphics::CColorVertex ColorArray[4];

	vec3 c = HsvToRgb(vec3(hsv.x, 0.0f, 1.0f));
	ColorArray[0] = IGraphics::CColorVertex(0, c.r, c.g, c.b, 1.0f);
	c = HsvToRgb(vec3(hsv.x, 1.0f, 1.0f));
	ColorArray[1] = IGraphics::CColorVertex(1, c.r, c.g, c.b, 1.0f);
	c = HsvToRgb(vec3(hsv.x, 1.0f, 1.0f));
	ColorArray[2] = IGraphics::CColorVertex(2, c.r, c.g, c.b, 1.0f);
	c = HsvToRgb(vec3(hsv.x, 0.0f, 1.0f));
	ColorArray[3] = IGraphics::CColorVertex(3, c.r, c.g, c.b, 1.0f);

	pEditor->Graphics()->SetColorVertex(ColorArray, 4);

	IGraphics::CQuadItem QuadItem(SVPicker.x, SVPicker.y, SVPicker.w, SVPicker.h);
	pEditor->Graphics()->QuadsDrawTL(&QuadItem, 1);

	// base: transparent - black
	ColorArray[0] = IGraphics::CColorVertex(0, 0.0f, 0.0f, 0.0f, 0.0f);
	ColorArray[1] = IGraphics::CColorVertex(1, 0.0f, 0.0f, 0.0f, 0.0f);
	ColorArray[2] = IGraphics::CColorVertex(2, 0.0f, 0.0f, 0.0f, 1.0f);
	ColorArray[3] = IGraphics::CColorVertex(3, 0.0f, 0.0f, 0.0f, 1.0f);

	pEditor->Graphics()->SetColorVertex(ColorArray, 4);

	pEditor->Graphics()->QuadsDrawTL(&QuadItem, 1);

	pEditor->Graphics()->QuadsEnd();

	// marker
	vec2 Marker = vec2(vec2(hsv.y*pEditor->UI()->Scale(), (1.0f - hsv.z)*pEditor->UI()->Scale()).x * vec2(SVPicker.w, SVPicker.h).x,
					vec2(hsv.y*pEditor->UI()->Scale(), (1.0f - hsv.z)*pEditor->UI()->Scale()).y * vec2(SVPicker.w, SVPicker.h).y);
	pEditor->Graphics()->QuadsBegin();
	pEditor->Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
	IGraphics::CQuadItem aMarker[2];
	aMarker[0] = IGraphics::CQuadItem(SVPicker.x+Marker.x, SVPicker.y+Marker.y - 5.0f*pEditor->UI()->PixelSize(), pEditor->UI()->PixelSize(), 11.0f*pEditor->UI()->PixelSize());
	aMarker[1] = IGraphics::CQuadItem(SVPicker.x+Marker.x - 5.0f*pEditor->UI()->PixelSize(), SVPicker.y+Marker.y, 11.0f*pEditor->UI()->PixelSize(), pEditor->UI()->PixelSize());
	pEditor->Graphics()->QuadsDrawTL(aMarker, 2);
	pEditor->Graphics()->QuadsEnd();

	// logic
	float X, Y;
	if(pEditor->UI()->DoPickerLogic(&pEditor->ms_SVPicker, &SVPicker, &X, &Y))
	{
		hsv.y = X/SVPicker.w;
		hsv.z = 1.0f - Y/SVPicker.h;
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

	pEditor->Graphics()->QuadsBegin();
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
		pEditor->Graphics()->SetColorVertex(ColorArray, 4);
		IGraphics::CQuadItem QuadItem(HuePicker.x, HuePicker.y+Offset*j, HuePicker.w, Offset);
		pEditor->Graphics()->QuadsDrawTL(&QuadItem, 1);
	}

	// marker
	pEditor->Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
	IGraphics::CQuadItem QuadItemMarker(HuePicker.x, HuePicker.y + (1.0f - hsv.x) * HuePicker.h * pEditor->UI()->Scale(), HuePicker.w, pEditor->UI()->PixelSize());
	pEditor->Graphics()->QuadsDrawTL(&QuadItemMarker, 1);

	pEditor->Graphics()->QuadsEnd();

	if(pEditor->UI()->DoPickerLogic(&pEditor->ms_HuePicker, &HuePicker, &X, &Y))
	{
		hsv.x = 1.0f - Y/HuePicker.h;
	}

	pEditor->ms_PickerColor = hsv;

	return 0;
}
