/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_INPUT_H
#define ENGINE_INPUT_H

#include "kernel.h"

#include <string>

const int g_MaxKeys = 512;
extern const char g_aaKeyStrings[g_MaxKeys][20];

class IInput : public IInterface
{
	MACRO_INTERFACE("input", 0)
public:
	class CEvent
	{
	public:
		int m_Flags;
		int m_Key;
		char m_aText[32];
		int m_InputCount;
	};

protected:
	enum
	{
		INPUT_BUFFER_SIZE=32
	};

	// quick access to events
	int m_NumEvents;
	IInput::CEvent m_aInputEvents[INPUT_BUFFER_SIZE];

public:
	enum
	{
		FLAG_PRESS=1,
		FLAG_RELEASE=2,
		FLAG_REPEAT=4,
		FLAG_TEXT=8,
	};

	// events
	int NumEvents() const { return m_NumEvents; }
	virtual bool IsEventValid(CEvent *pEvent) const = 0;
	CEvent GetEvent(int Index) const
	{
		if(Index < 0 || Index >= m_NumEvents)
		{
			IInput::CEvent e = {0,0};
			return e;
		}
		return m_aInputEvents[Index];
	}

	// keys
/*<<<! HEAD
	int KeyPressed(int Key) { return m_aInputState[m_InputCurrent][Key]; }
	int KeyReleases(int Key) { return m_aInputCount[m_InputCurrent][Key].m_Releases; }
	int KeyPresses(int Key) { return m_aInputCount[m_InputCurrent][Key].m_Presses; }
	int KeyDown(int Key) { return KeyPressed(Key)&&!KeyWasPressed(Key); }
=======*/
	virtual bool KeyIsPressed(int Key) const = 0;
	bool KeyIsPressedLua(std::string KeyName) const
	{
		int id = GetKeyID(KeyName);
		if(id < 0)
			return false;
		return KeyIsPressed(id);
		
	}
	virtual bool KeyPress(int Key, bool CheckCounter=false) const = 0;
	const char *KeyName(int Key) const { return (Key >= 0 && Key < g_MaxKeys) ? g_aaKeyStrings[Key] : g_aaKeyStrings[0]; }
	std::string KeyNameSTD(int Key) const { return std::string(KeyName(Key)); }
	int GetKeyID(std::string KeyName) const
	{
		for(int i = 0; i < g_MaxKeys; i++)
			if(KeyNameSTD(i) == KeyName)
				return i;
		return -1;
	}
	virtual void Clear() = 0;

	//
	virtual void MouseModeRelative() = 0;
	virtual void MouseModeAbsolute() = 0;
	virtual bool InputGrabbed() const = 0;
	virtual void ConvertMousePos(float *x, float *y) const = 0;
	virtual void NativeMousePos(int *mx, int *my) const = 0;
	virtual bool NativeMousePressed(int index) = 0;
	virtual int64 MouseDoubleClick(float tl=-1, float tr=-1, float bl=-1, float br=-1) const = 0;
	virtual int64 MouseDoubleClickReset(float tl=-1, float tr=-1, float bl=-1, float br=-1) = 0;
	virtual int64 MouseDoubleClickNative(float tl=-1, float tr=-1, float bl=-1, float br=-1) const = 0;
	virtual int64 MouseDoubleClickNativeReset(float tl=-1, float tr=-1, float bl=-1, float br=-1) = 0;
	virtual int64 MouseDoubleClickCurrent(float tl=-1, float tr=-1, float bl=-1, float br=-1) const = 0;
	virtual int64 MouseDoubleClickCurrentReset(float tl=-1, float tr=-1, float bl=-1, float br=-1) = 0;

	virtual const char* GetClipboardText() = 0;
	virtual void SetClipboardText(const char *Text) = 0;
	// used for lua
	std::string GetClipboardTextSTD() { return std::string(GetClipboardText()); }
	void SetClipboardTextSTD(std::string Text) { SetClipboardText(Text.c_str()); }

	virtual void MouseRelative(float *x, float *y) = 0;

	virtual void SimulateKeyPress(int Key) = 0;
	virtual void SimulateKeyPressSTD(std::string Key) = 0;
	virtual void SimulateKeyRelease(int Key) = 0;
	virtual void SimulateKeyReleaseSTD(std::string Key) = 0;

	virtual bool GetIMEState() = 0;
	virtual void SetIMEState(bool Activate) = 0;
	virtual const char* GetIMECandidate() = 0;
	virtual int GetEditingCursor() = 0;

	virtual void HookRelativeMouse(const float *x, const float *y) = 0;
	virtual void CurrentMousePos(int *pOutX, int *pOutY) const = 0;
};


class IEngineInput : public IInput
{
	MACRO_INTERFACE("engineinput", 0)
public:
	virtual void Init() = 0;
	virtual int Update() = 0;
	virtual void NextFrame() = 0;
	virtual int VideoRestartNeeded() = 0;
};

extern IEngineInput *CreateEngineInput();

#endif
