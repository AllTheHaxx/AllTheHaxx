/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_TEXTRENDER_H
#define ENGINE_TEXTRENDER_H

#include "kernel.h"

enum
{
	TEXTFLAG_RENDER=1,
	TEXTFLAG_ALLOW_NEWLINE=2,
	TEXTFLAG_STOP_AT_END=4
};

enum
{
	FONT_REGULAR = 0,
	FONT_BOLD,
	FONT_ITALIC,
	FONT_BOLD_ITALIC,
	FONT_NUM_TYPES
};


class CFont;
class CFontFile;

class CTextCursor
{
public:
	int m_Flags;
	int m_LineCount;
	int m_CharCount;
	int m_MaxLines;

	float m_StartX;
	float m_StartY;
	float m_LineWidth;
	float m_X, m_Y;

	CFont *m_pFont;
	float m_FontSize;
};

class ITextRender : public IInterface
{
	MACRO_INTERFACE("textrender", 0)
public:
	virtual void SetCursor(CTextCursor *pCursor, float x, float y, float FontSize, int Flags, class CFont *pFont = 0) = 0;
	void SetCursorLua(CTextCursor *pCursor, float x, float y, float FontSize, int Flags) { SetCursor(pCursor, x, y, FontSize, Flags, NULL); }

	virtual CFont *LoadFont(const char *pFilename) = 0;
	virtual void DestroyFont(CFont *pFont) = 0;

	virtual void SetDefaultFont(CFont *pFont) = 0;

	//
	virtual float TextEx(CTextCursor *pCursor, const char *pText, int Length) = 0;
	virtual float TextExParse(CTextCursor *pCursor, const char *pText, bool IgnoreColorCodes = false, const char *pHighlight = 0) = 0;

	// old foolish interface
	virtual void TextColor(float r, float g, float b, float a) = 0;
	virtual void GetTextColor(float *r, float *g, float *b, float *a) = 0;
	virtual void TextOutlineColor(float r, float g, float b, float a) = 0;
	virtual void Text(class CFont *pFontSetV, float x, float y, float Size, const char *pText, float MaxWidth) = 0;
	virtual float TextWidth(class CFont *pFontSetV, float Size, const char *pText, int Length = -1, float LineWidth = -1) = 0;
	virtual float TextWidthParse(class CFont *pFontSetV, float Size, const char *pText, float LineWidth = -1, const char *pHighlight = 0) = 0;
	virtual int TextLineCount(class CFont *pFontSetV, float Size, const char *pText, float LineWidth) = 0;
};

class IEngineTextRender : public ITextRender
{
	MACRO_INTERFACE("enginetextrender", 0)
public:
	virtual void Init() = 0;
};

extern IEngineTextRender *CreateEngineTextRender();

#endif
