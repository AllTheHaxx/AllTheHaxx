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
public:
	vec2 m_MousePos[NUM_VIRTUAL_CLIENTS];
	vec2 m_TargetPos[NUM_VIRTUAL_CLIENTS];
	float m_OldMouseX;
	float m_OldMouseY;
	SDL_Joystick *m_Joystick;
	bool m_JoystickFirePressed;
	bool m_JoystickRunPressed;
	int64 m_JoystickTapTime;

	SDL_Joystick *m_Gamepad;
	bool m_UsingGamepad;

	int m_AmmoCount[NUM_WEAPONS];

	CNetObj_PlayerInput m_InputData[NUM_VIRTUAL_CLIENTS];
	CNetObj_PlayerInput m_LastData[NUM_VIRTUAL_CLIENTS];
	int m_InputDirectionLeft[NUM_VIRTUAL_CLIENTS];
	int m_InputDirectionRight[NUM_VIRTUAL_CLIENTS];
	int m_ShowHookColl[NUM_VIRTUAL_CLIENTS];
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
	int GetDirection() const { return m_InputData[CURR_VIRTUAL_CLIENT].m_Direction; }
	void SetDirection(int n) { m_InputData[CURR_VIRTUAL_CLIENT].m_Direction = n; }
	int GetFire() const { return m_InputData[CURR_VIRTUAL_CLIENT].m_Fire; }
	void SetFire(int n) { m_InputData[CURR_VIRTUAL_CLIENT].m_Fire = n; }
	int GetHook() const { return m_InputData[CURR_VIRTUAL_CLIENT].m_Hook; }
	void SetHook(int n) { m_InputData[CURR_VIRTUAL_CLIENT].m_Hook = n; }
	int GetJump() const { return m_InputData[CURR_VIRTUAL_CLIENT].m_Jump; }
	void SetJump(int n) { m_InputData[CURR_VIRTUAL_CLIENT].m_Jump = n; }
	int GetWantedWeapon() const { return m_InputData[CURR_VIRTUAL_CLIENT].m_WantedWeapon; }
	void SetWantedWeapon(int n) { m_InputData[CURR_VIRTUAL_CLIENT].m_WantedWeapon = n; }
	int GetTargetX() const { return m_InputData[CURR_VIRTUAL_CLIENT].m_TargetX; }
	void SetTargetX(int n) { m_InputData[CURR_VIRTUAL_CLIENT].m_TargetX = n; }
	int GetTargetY() const { return m_InputData[CURR_VIRTUAL_CLIENT].m_TargetY; }
	void SetMouseX(int n) { m_MousePos[CURR_VIRTUAL_CLIENT].x = n; }
	int GetMouseX() const { return m_MousePos[CURR_VIRTUAL_CLIENT].x; }
	void SetMouseY(int n) { m_MousePos[CURR_VIRTUAL_CLIENT].y = n; }
	int GetMouseY() const { return m_MousePos[CURR_VIRTUAL_CLIENT].y; }
	void SetTargetY(int n) { m_InputData[CURR_VIRTUAL_CLIENT].m_TargetY = n; }
	void SetDirRight(int n) { m_InputDirectionRight[CURR_VIRTUAL_CLIENT] = n; }
    void SetDirLeft(int n) { m_InputDirectionLeft[CURR_VIRTUAL_CLIENT] = n; }
    int GetDirRight() const { return m_InputDirectionRight[CURR_VIRTUAL_CLIENT]; }
    int GetDirLeft() const { return m_InputDirectionLeft[CURR_VIRTUAL_CLIENT]; }
	
	int m_NextHiddenCharCounter;
	int m_HiddenCharSerialCount;
};
#endif
