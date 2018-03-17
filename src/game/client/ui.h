/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_UI_H
#define GAME_CLIENT_UI_H

#include <lua.hpp>
#include <base/tl/array.h>

class CUIRect
{
	// TODO: Refactor: Redo UI scaling
	inline float Scale() const;
public:
	CUIRect() { x=0; y=0; w=0; h=0; }
	CUIRect(float v) { x=v; y=v; w=v; h=v; }
	CUIRect(const CUIRect& other) { x=other.x; y=other.y; w=other.w; h=other.h; }
	CUIRect(float _x, float _y, float _w, float _h) : x(_x), y(_y), w(_w), h(_h) {}
	float x, y, w, h;

	void HSplitMid(CUIRect *pTop, CUIRect *pBottom) const;
	void HSplitTop(float Cut, CUIRect *pTop, CUIRect *pBottom) const;
	void HSplitBottom(float Cut, CUIRect *pTop, CUIRect *pBottom) const;
	void VSplitMid(CUIRect *pLeft, CUIRect *pRight, float Offset = 0.0f) const;
	void VSplitLeft(float Cut, CUIRect *pLeft, CUIRect *pRight) const;
	void VSplitRight(float Cut, CUIRect *pLeft, CUIRect *pRight) const;

	void Margin(float Cut, CUIRect *pOtherRect) const;
	void VMargin(float Cut, CUIRect *pOtherRect) const;
	void HMargin(float Cut, CUIRect *pOtherRect) const;

	CUIRect LuaCopy() const { return CUIRect(x,y,w,h); };
};


class CUI
{
	const void *m_pHotItem;
	const void *m_pActiveItem;
	const void *m_pLastActiveItem;
	const void *m_pBecommingHotItem;
	float m_MouseX, m_MouseY; // in gui space
	float m_MouseWorldX, m_MouseWorldY; // in world space
	unsigned m_MouseButtons;
	unsigned m_LastMouseButtons;
	array<CUIRect> m_ClippingAreaStack;

	CUIRect m_Screen;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;

public:
	// TODO: Refactor: Fill this in
	void SetGraphics(class IGraphics *pGraphics, class ITextRender *pTextRender) { m_pGraphics = pGraphics; m_pTextRender = pTextRender;}
	class IGraphics *Graphics() { return m_pGraphics; }
	class ITextRender *TextRender() { return m_pTextRender; }

	CUI();

	enum
	{
		ALIGN_LEFT   = -1,
		ALIGN_CENTER =  0,
		ALIGN_RIGHT  =  1,

		CORNER_NONE=0,
		CORNER_TL=1,
		CORNER_TR=2,
		CORNER_BL=4,
		CORNER_BR=8,

		CORNER_T=CORNER_TL|CORNER_TR,
		CORNER_B=CORNER_BL|CORNER_BR,
		CORNER_R=CORNER_TR|CORNER_BR,
		CORNER_L=CORNER_TL|CORNER_BL,

		CORNER_ALL=CORNER_T|CORNER_B
	};

	int Update(float mx, float my, float Mwx, float Mwy, int m_Buttons);

	float MouseX() const { return m_MouseX; }
	float MouseY() const { return m_MouseY; }
	float MouseWorldX() const { return m_MouseWorldX; }
	float MouseWorldY() const { return m_MouseWorldY; }
	int MouseButton(int Index) const { return (m_MouseButtons>>Index)&1; }
	int MouseButtonClicked(int Index) { return MouseButton(Index) && !((m_LastMouseButtons>>Index)&1) ; }

	void HookRelativeMouse(class IInput* pInput);

	void SetHotItem(const void *pID) { m_pBecommingHotItem = pID; }
	void SetActiveItem(const void *pID) { m_pActiveItem = pID; if (pID) m_pLastActiveItem = pID; }
	void ClearLastActiveItem() { m_pLastActiveItem = 0; }
	const void *HotItem() const { return m_pHotItem; }
	const void *NextHotItem() const { return m_pBecommingHotItem; }
	const void *ActiveItem() const { return m_pActiveItem; }
	const void *LastActiveItem() const { return m_pLastActiveItem; }

	int MouseInside(const CUIRect *pRect);
	int MouseInsideImpl(const CUIRect *pRect);
	int MouseInsideNative(float mx, float my, const CUIRect *pRect);
	int MouseInsideAbsolute(float mx, float my, const CUIRect *s, const CUIRect *r);
	void ConvertMouseMove(float *x, float *y);

	CUIRect *Screen();
	float PixelSize();
	void ClipEnable(const CUIRect *pRect);
	void ClipEnableImpl(const CUIRect *pRect);
	void ClipDisable();

	// TODO: Refactor: Redo UI scaling
	void SetScale(float s);
	float Scale();

	int DoButtonLogic(const void *pID, const char *pText /* TODO: Refactor: Remove */, int Checked, const CUIRect *pRect);
	int DoButtonLogicLua(const class CButtonContainer *pBC, int Checked, const CUIRect *pRect);
	int DoPickerLogic(const void *pID, const CUIRect *pRect, float *pX, float *pY);
	int DoPickerLogicLua(const class CButtonContainer *pBC, const CUIRect *pRect, float *pX, float *pY);

	// TODO: Refactor: Remove this?
	void DoLabel(const CUIRect *pRect, const char *pText, float Size, int Align, float MaxWidth = -1.0f, const char *pHighlight = 0, class CFont *pFont = 0, bool IgnoreColorCodes = false);
	void DoLabelScaled(const CUIRect *pRect, const char *pText, float Size, int Align, float MaxWidth = -1.0f, const char *pHighlight = 0, class CFont *pFont = 0, bool IgnoreColor = false);
	void DoLabelLua(const CUIRect *pRect, const char *pText, float Size, int Align, float MaxWidth, const char *pHighlight, lua_State *L);
	void DoLabelScaledLua(const CUIRect *pRect, const char *pText, float Size, int Align, float MaxWidth, const char *pHighlight, lua_State *L);

	void AndroidShowScreenKeys(bool shown);
	void AndroidShowTextInput(const char *text, const char *hintText);
	void AndroidBlockAndGetTextInput(char *text, int textLength, const char *hintText);
	bool AndroidTextInputShown();
};


#endif
