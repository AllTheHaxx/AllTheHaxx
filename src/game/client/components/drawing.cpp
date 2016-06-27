#include "drawing.h"

void CDrawing::OnReset()
{
	m_DrawingMode = MODE_OFF;
	m_PrevDrawingMode = MODE_DRAW;
}

void CDrawing::OnConsoleInit()
{
	CALLSTACK_ADD();

	Console()->Register("draw_set", "i['0'|'1'|'2']", CFGFLAG_CLIENT, ConDrawSet, this, "Set the drawing mode (0 = off, 1 = draw, 2 = erase)");
	Console()->Register("draw_toggle", "", CFGFLAG_CLIENT, ConDrawToggle, this, "Toggle current drawing mode on/off");
}

void CDrawing::ConDrawSet(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	if(pResult->GetInteger(0) >= MODE_OFF && pResult->GetInteger(0) <= MODE_LAST)
		((CDrawing*)pUserData)->m_DrawingMode = pResult->GetInteger(0);
	else
		((CDrawing*)pUserData)->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "drawing", "Possible Modes: 0 = off, 1 = draw, 2 = erase");
}

void CDrawing::ConDrawToggle(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CDrawing *pSelf = (CDrawing*)pUserData;
	if(pSelf->m_DrawingMode)
	{
		pSelf->m_PrevDrawingMode = pSelf->m_DrawingMode;
		pSelf->m_DrawingMode = MODE_OFF;
	}
	else
		pSelf->m_DrawingMode = pSelf->m_PrevDrawingMode;
}
