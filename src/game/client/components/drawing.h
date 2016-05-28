#ifndef GAME_CLIENT_COMPONENTS_DRAWING_H
#define GAME_CLIENT_COMPONENTS_DRAWING_H

#include <game/client/component.h>

class CDrawing : public CComponent
{
	enum
	{
		MODE_OFF = 0,
		MODE_DRAW,
		MODE_ERASE,
		MODE_LAST
	};

	int m_DrawingMode;
	int m_PrevDrawingMode;

public:
	void OnConsoleInit();
	void OnReset();

	static void ConDrawSet(IConsole::IResult *pResult, void *pUserData);
	static void ConDrawToggle(IConsole::IResult *pResult, void *pUserData);
	//static void Con(IConsole::IResult *pResult, void *pUserData);
	//static void Con(IConsole::IResult *pResult, void *pUserData);
};


#endif
