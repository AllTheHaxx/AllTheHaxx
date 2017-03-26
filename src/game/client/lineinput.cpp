/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/keys.h>

#include "lineinput.h"


CLineInput::CLineInput()
{
	Clear();
}

void CLineInput::Clear()
{
	mem_zero(m_aStr, sizeof(m_aStr));
	m_Len = 0;
	m_CursorPos = 0;
	m_NumChars = 0;
}

void CLineInput::Set(const char *pString)
{
	str_copy(m_aStr, pString, sizeof(m_aStr));
	m_Len = str_length(m_aStr);
	m_CursorPos = m_Len;
	m_NumChars = 0;
	int Offset = 0;
	while(pString[Offset])
	{
		Offset = str_utf8_forward(pString, Offset);
		++m_NumChars;
	}
}

void CLineInput::Editing(const char *pString, int Cursor)
{
	str_copy(m_DisplayStr, m_aStr, sizeof(m_DisplayStr));
	char Texting[34];
	str_format(Texting, sizeof(Texting), "[%s]", pString);
	int NewTextLen = str_length(Texting);
	int CharsLeft = (int)sizeof(m_DisplayStr) - str_length(m_DisplayStr) - 1;
	int FillCharLen = NewTextLen < CharsLeft ? NewTextLen : CharsLeft;
	for(int i = str_length(m_DisplayStr) - 1; i >= m_CursorPos ; i--)
		m_DisplayStr[i+FillCharLen] = m_DisplayStr[i];
	for(int i = 0; i < FillCharLen; i++)
	{
		if(Texting[i] == 28)
			m_DisplayStr[m_CursorPos + i] = ' ';
		else
			m_DisplayStr[m_CursorPos + i] = Texting[i];
	}
	m_FakeLen = str_length(m_DisplayStr);
	m_FakeCursorPos = m_CursorPos + Cursor + 1;	
}

void CLineInput::Add(const char *pString)
{
	if((int)sizeof(m_aStr) - m_Len <= (int)str_length(pString))
		return;
	str_copy(m_aStr + m_Len, pString, sizeof(m_aStr) - m_Len);
	m_Len = str_length(m_aStr);
	m_CursorPos = m_Len;
}

bool CLineInput::Manipulate(const IInput::CEvent& Event, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pNumCharsPtr)
{
	int NumChars = *pNumCharsPtr;
	int CursorPos = *pCursorPosPtr;
	int Len = *pStrLenPtr;
	bool Changes = false;

	if(CursorPos > Len)
		CursorPos = Len;

	if(Event.m_Flags&IInput::FLAG_TEXT)
	{
		// gather string stats
		int CharCount = 0;
		int CharSize = 0;
		while(Event.m_aText[CharSize])
		{
			int NewCharSize = str_utf8_forward(Event.m_aText, CharSize);
			if(NewCharSize != CharSize)
			{
				++CharCount;
				CharSize = NewCharSize;
			}
		}

		// add new string
		if(CharCount)
		{
			if(Len+CharSize < StrMaxSize && CursorPos+CharSize < StrMaxSize && NumChars+CharCount < StrMaxChars)
			{
				mem_move(pStr + CursorPos + CharSize, pStr + CursorPos, Len-CursorPos+1); // +1 == null term
				for(int i = 0; i < CharSize; i++)
					pStr[CursorPos+i] = Event.m_aText[i];
				CursorPos += CharSize;
				Len += CharSize;
				NumChars += CharCount;
				Changes = true;
			}
		}
	}

	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		int Key = Event.m_Key;
		if(Key == KEY_BACKSPACE && CursorPos > 0)
		{
			int NewCursorPos = str_utf8_rewind(pStr, CursorPos);
			int CharSize = CursorPos-NewCursorPos;
			mem_move(pStr+NewCursorPos, pStr+CursorPos, Len - NewCursorPos - CharSize + 1); // +1 == null term
			CursorPos = NewCursorPos;
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if(Key == KEY_DELETE && CursorPos < Len)
		{
			int p = str_utf8_forward(pStr, CursorPos);
			int CharSize = p-CursorPos;
			mem_move(pStr + CursorPos, pStr + CursorPos + CharSize, Len - CursorPos - CharSize + 1); // +1 == null term
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if(Key == KEY_LEFT && CursorPos > 0)
			CursorPos = str_utf8_rewind(pStr, CursorPos);
		else if(Key == KEY_RIGHT && CursorPos < Len)
			CursorPos = str_utf8_forward(pStr, CursorPos);
		else if(Key == KEY_HOME)
			CursorPos = 0;
		else if(Key == KEY_END)
			CursorPos = Len;
	}

	*pNumCharsPtr = NumChars;
	*pCursorPosPtr = CursorPos;
	*pStrLenPtr = Len;

	return Changes;
}

void CLineInput::ProcessInput(const IInput::CEvent& e)
{
	Manipulate(e, m_aStr, MAX_SIZE, MAX_CHARS, &m_Len, &m_CursorPos, &m_NumChars);
}

void CLineInput::HandleSkipping(const IInput::CEvent& Event, CLineInput *pLineInput)
{
	 // handle skipping: jump to spaces and special ASCII characters

	int SearchDirection = 0;
	if(Event.m_Key == KEY_LEFT || Event.m_Key == KEY_BACKSPACE)
		SearchDirection = -1;
	else if(Event.m_Key == KEY_RIGHT  || Event.m_Key == KEY_DELETE)
		SearchDirection = 1;

	if(SearchDirection != 0)
	{
		int FoundAt = SearchDirection > 0 ? pLineInput->GetLength() : 0;
		for(int i = pLineInput->GetCursorOffset() + SearchDirection; SearchDirection > 0 ? i < pLineInput->GetLength() - 1 : i > 0; i += SearchDirection)
		{
			int Next = i + SearchDirection;
			if((pLineInput->GetString()[Next] == ' ') ||
			   (pLineInput->GetString()[Next] >= 32 && pLineInput->GetString()[Next] <= 47) ||
			   (pLineInput->GetString()[Next] >= 58 && pLineInput->GetString()[Next] <= 64) ||
			   (pLineInput->GetString()[Next] >= 91 && pLineInput->GetString()[Next] <= 96))
			{
				//FoundAt = i;
				FoundAt = Next;
				if(SearchDirection < 0)
					FoundAt++;
				break;
			}
		}
		if(Event.m_Key == KEY_BACKSPACE)
		{
			if(pLineInput->GetCursorOffset() != 0)
			{
				char aText[512];
				str_copy(aText, pLineInput->GetString(), FoundAt + 1);
				if(pLineInput->GetCursorOffset() != str_length(pLineInput->GetString()))
				{
					str_append(aText, pLineInput->GetString() + pLineInput->GetCursorOffset(), str_length(pLineInput->GetString()));
				}
				pLineInput->Set(aText);
			}
		}
		else if(Event.m_Key == KEY_DELETE)
		{
			int AtIndex = pLineInput->GetCursorOffset();
			if(AtIndex != pLineInput->GetLength() && FoundAt > AtIndex)
			{
				char aText[512] = {0};
				if(AtIndex != 0)
				{
					str_copy(aText, pLineInput->GetString(), AtIndex + 1);
				}
				str_appendb(aText, pLineInput->GetString() + FoundAt);
				pLineInput->Set(aText);
			}
			FoundAt = AtIndex; // store the original cursor offset as it will stay the same
		}
		pLineInput->SetCursorOffset(FoundAt);
	}

}
