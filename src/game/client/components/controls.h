/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CONTROLS_H
#define GAME_CLIENT_COMPONENTS_CONTROLS_H
#include <SDL_joystick.h>
#include <base/vmath.h>
#include <base/system.h>
#include <game/client/component.h>

class CControls : public CComponent
{
	bool m_DiscardMouseMove;
public:
	vec2 m_MousePos[2];
	vec2 m_TargetPos[2];
	float m_OldMouseX;
	float m_OldMouseY;
	SDL_Joystick *m_Joystick;
	bool m_JoystickFirePressed;
	bool m_JoystickRunPressed;
	int64 m_JoystickTapTime;

	SDL_Joystick *m_Gamepad;
	bool m_UsingGamepad;

	int m_AmmoCount[NUM_WEAPONS];

	CNetObj_PlayerInput m_InputData[2];
	CNetObj_PlayerInput m_LastData[2];
	int m_InputDirectionLeft[2];
	int m_InputDirectionRight[2];
	int m_ShowHookColl[2];
	int m_SuperDyncam[2];
	int m_LastDummy;
	int m_OtherFire;

	CControls();

	virtual void OnReset();
	virtual void OnRelease();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);
	virtual void OnConsoleInit();
	virtual void OnPlayerDeath();

	int SnapInput(int *pData);
	void ClampMousePos();
	void ResetInput(int dummy);

	// lua
	int GetDirection() const { return m_InputData[g_Config.m_ClDummy].m_ViewDir; }
	void SetDirection(int n) { m_InputData[g_Config.m_ClDummy].m_ViewDir = n; }
	int GetFire() const { return m_InputData[g_Config.m_ClDummy].m_FCount; }
	void SetFire(int n) { m_InputData[g_Config.m_ClDummy].m_FCount = n; }
	int GetHook() const { return m_InputData[g_Config.m_ClDummy].m_Hook; }
	void SetHook(int n) { m_InputData[g_Config.m_ClDummy].m_Hook = n; }
	int GetJump() const { return m_InputData[g_Config.m_ClDummy].m_Jump; }
	void SetJump(int n) { m_InputData[g_Config.m_ClDummy].m_Jump = n; }
	int GetWantedWeapon() const { return m_InputData[g_Config.m_ClDummy].m_WantedWeapon; }
	void SetWantedWeapon(int n) { m_InputData[g_Config.m_ClDummy].m_WantedWeapon = n; }
	int GetTargetX() const { return m_InputData[g_Config.m_ClDummy].m_AimX; }
	void SetTargetX(int n) { m_InputData[g_Config.m_ClDummy].m_AimX = n; }
	int GetTargetY() const { return m_InputData[g_Config.m_ClDummy].m_AimY; }
	void SetMouseX(int n) { m_MousePos[g_Config.m_ClDummy].x = n; }
	int GetMouseX() const { return m_MousePos[g_Config.m_ClDummy].x; }
	void SetMouseY(int n) { m_MousePos[g_Config.m_ClDummy].y = n; }
	int GetMouseY() const { return m_MousePos[g_Config.m_ClDummy].y; }
	void SetTargetY(int n) { m_InputData[g_Config.m_ClDummy].m_AimY = n; }
	void SetDirRight(int n) { m_InputDirectionRight[g_Config.m_ClDummy] = n; }
	void SetDirLeft(int n) { m_InputDirectionLeft[g_Config.m_ClDummy] = n; }
	int GetDirRight() const { return m_InputDirectionRight[g_Config.m_ClDummy]; }
	int GetDirLeft() const { return m_InputDirectionLeft[g_Config.m_ClDummy]; }

	static CNetObj_PlayerInput* LuaGetInputData(lua_State *L);

	int m_NextHiddenCharCounter;
	int m_HiddenCharSerialCount;
};
#endif
