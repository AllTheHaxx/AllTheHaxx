/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CAMERA_H
#define GAME_CLIENT_COMPONENTS_CAMERA_H
#include <base/vmath.h>
#include <game/client/component.h>

class CCamera : public CComponent
{
	enum
	{
		CAMTYPE_UNDEFINED=-1,
		CAMTYPE_SPEC,
		CAMTYPE_PLAYER,
	};

	int m_CamType;
	vec2 m_LastPos[2];
	vec2 m_PrevCenter;

public:
	bool m_GodlikeSpec;
	vec2 m_Center;
	vec2 m_WantedCenter;
	vec2 m_SuperDynStartPos;
	vec2 m_RotationCenter;
	bool m_ZoomSet;
	float m_Zoom;
	float m_WantedZoom;

	CCamera();
	virtual void OnRender();

	// DDRace

	virtual void OnConsoleInit();
	virtual void OnReset();

	bool ZoomAllowed() const;

private:
	static void ConZoomPlus(IConsole::IResult *pResult, void *pUserData);
	static void ConZoomMinus(IConsole::IResult *pResult, void *pUserData);
	static void ConZoomReset(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleGodlikeSpec(IConsole::IResult *pResult, void *pUserData);

};

#endif
