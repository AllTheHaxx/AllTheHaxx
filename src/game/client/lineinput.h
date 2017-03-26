/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_LINEINPUT_H
#define GAME_CLIENT_LINEINPUT_H

#include <engine/input.h>

// line input helper
class CLineInput
{
	enum
	{
		MAX_SIZE=512,
		MAX_CHARS=MAX_SIZE/2,
	};
	char m_aStr[MAX_SIZE];
	int m_Len;
	int m_CursorPos;
	int m_NumChars;

	char m_DisplayStr[MAX_SIZE+34];
	int m_FakeLen;
	int m_FakeCursorPos;
public:
	static bool Manipulate(const IInput::CEvent& e, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pNumCharsPtr);
	static void HandleSkipping(const IInput::CEvent& e, CLineInput *pInput);

	class CCallback
	{
	public:
		virtual ~CCallback() {}
		virtual bool Event(IInput::CEvent e) = 0;
	};

	CLineInput();
	void Clear();
	void ProcessInput(const IInput::CEvent& e);
	void Editing(const char *pString, int Cursor);
	void Set(const char *pString);
	void Add(const char *pString);
	const char *GetString(bool Editing = false) const { return Editing ? m_DisplayStr : m_aStr; }
	int GetLength(bool Editing = false) const { return Editing ? m_FakeLen : m_Len; }
	int GetCursorOffset(bool Editing = false) const { return Editing ? m_FakeCursorPos : m_CursorPos; }
	void SetCursorOffset(int Offset) { m_CursorPos = Offset > m_Len ? m_Len : Offset < 0 ? 0 : Offset; }
};

#endif
