/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#undef luaL_dostring
#define luaL_dostring(L,s)	\
	(luaL_loadstring(L, s) || lua_pcall(L, 0, LUA_MULTRET, 0))

#include <base/tl/sorted_array.h>

#include <math.h>

#include <game/generated/client_data.h>

#include <base/system.h>

#include <engine/shared/ringbuffer.h>
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/keys.h>
#include <engine/console.h>

#include <cstring>
#include <cstdio>
#include <string>

#include <game/client/ui.h>

#include <game/version.h>

#include <game/client/lineinput.h>
#include <game/client/render.h>
#include <game/client/components/controls.h>
#include <game/client/components/menus.h>

#include "menus.h"
#include "irc.h"
#include "console.h"

CGameConsole::CInstance * CGameConsole::m_pStatLuaConsole = 0;
const char * CGameConsole::m_pSearchString = 0;

CGameConsole::CInstance::CInstance(int Type)
{
	m_pHistoryEntry = 0x0;

	m_Type = Type;

	if(Type == CGameConsole::CONSOLETYPE_LOCAL)
		m_CompletionFlagmask = CFGFLAG_CLIENT;
	else
		m_CompletionFlagmask = CFGFLAG_SERVER;

	m_aCompletionBuffer[0] = 0;
	m_CompletionChosen = -1;
	m_CompletionRenderOffset = 0.0f;
	m_ReverseTAB = false;
	m_CTRLPressed = false;

	m_IsCommand = false;
}

void CGameConsole::CInstance::Init(CGameConsole *pGameConsole)
{
	m_pGameConsole = pGameConsole;
	
	if(m_Type == CONSOLETYPE_LUA)
	{
		m_LuaHandler.m_pLuaState = luaL_newstate();

		//lua_atpanic(m_pLuaState, CLua::Panic);
		//lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

		luaL_openlibs(m_LuaHandler.m_pLuaState);
		luaopen_base(m_LuaHandler.m_pLuaState);
		luaopen_math(m_LuaHandler.m_pLuaState);
		luaopen_string(m_LuaHandler.m_pLuaState);
		luaopen_table(m_LuaHandler.m_pLuaState);
		//luaopen_io(m_pLua);
		luaopen_os(m_LuaHandler.m_pLuaState);
		//luaopen_package(m_pLua); // not sure whether we should load this
		luaopen_debug(m_LuaHandler.m_pLuaState);
		luaopen_bit(m_LuaHandler.m_pLuaState);
		luaopen_jit(m_LuaHandler.m_pLuaState);
		luaopen_ffi(m_LuaHandler.m_pLuaState); // don't know about this yet. could be a sand box leak.
		
		m_LuaHandler.m_Inited = false;
		m_LuaHandler.m_ScopeCount = 0;
		
		//Some luaconsole funcs!
		getGlobalNamespace(m_LuaHandler.m_pLuaState)
			.addFunction("print", &CGameConsole::PrintLuaLine);
			
		LoadLuaFile("data/luabase/events.lua");
		
		CGameConsole::m_pStatLuaConsole = this;
	}
};

void CGameConsole::CInstance::ClearBacklog()
{
	m_Backlog.Init();
	m_BacklogActPage = 0;
	m_BacklogLineOffset = 0;
}

void CGameConsole::CInstance::ClearHistory()
{
	m_History.Init();
	m_pHistoryEntry = 0;
}

void CGameConsole::CInstance::ExecuteLine(const char *pLine)
{
	if(m_Type == CGameConsole::CONSOLETYPE_LOCAL)
		m_pGameConsole->m_pConsole->ExecuteLine(pLine);
	else if(m_Type == CGameConsole::CONSOLETYPE_REMOTE)
	{
		if(m_pGameConsole->Client()->RconAuthed())
			m_pGameConsole->Client()->Rcon(pLine);
		else
			m_pGameConsole->Client()->RconAuth("", pLine);
	}
	else if(m_Type == CGameConsole::CONSOLETYPE_LUA && g_Config.m_ClLua)
	{
		if(str_comp(pLine, "reset") == 0)
		{
			m_LuaHandler.m_ScopeCount = 0;
			m_LuaHandler.m_FullLine = "";
			PrintLine("Reset complete");
			return;
		}
		int Status = 0;
		char ErrorMsg[512];
		bool ScopeIncreased = false;
		
		if(!m_LuaHandler.m_Inited)  //this is yet quite retarded!
		{
			CLuaFile::RegisterLuaCallbacks(m_LuaHandler.m_pLuaState);
			m_LuaHandler.m_Inited = true;
		}
		
		//SCOPING DETECT!
		std::string ActLine(pLine);                             //cuz after an elseif is no extra end!
		if(ActLine.find("while") == 0 || ActLine.find("function") == 0 || (ActLine.find("if") == 0 && ActLine.find("elseif") == std::string::npos) || ActLine.find("for") == 0)
		{
			m_LuaHandler.m_ScopeCount++;
			ScopeIncreased = true;
		}
		if(ActLine.find("end") != std::string::npos)  //NO ELSE IF HERE
		{
			//this is a bit tricky D: because e.g. Game.Emote:Send will also trigger the search for 'end' :D
			//so we remove all whitespaces and check again
			bool RealEnd = false;
			std::string testbuf = ActLine;
			
			while(testbuf.find(" ") != std::string::npos)   //remove all whitespaces
			{
				testbuf.replace(testbuf.find(" "),testbuf.size(),"");
			}
			
			if(testbuf.compare("end") == 0)
				RealEnd = true;
			
			if(ActLine.find(" end") != std::string::npos)
				RealEnd = true;
			
			if(RealEnd)
			{
				char aBuf[512] = { 0 };
				if(m_LuaHandler.m_ScopeCount > 0)
				{	
					int RemoveElseScope = 0;
						
					if(ActLine.find("else") != std::string::npos)  //works for else and elseif
						RemoveElseScope = 1;
					
					for(int i = 0; i < m_LuaHandler.m_ScopeCount-1-RemoveElseScope; i++)
						str_append(aBuf, "     ", sizeof(aBuf));
					str_append(aBuf, pLine, sizeof(aBuf));
				}
				m_LuaHandler.m_ScopeCount--;
				
				if(m_LuaHandler.m_ScopeCount == 0)  //if we are now at zero after decreasing => print new line!
				{
					PrintLine(aBuf);
					PrintLine("");
				}
			}
		}
		
		m_LuaHandler.m_FullLine.append(pLine);
		m_LuaHandler.m_FullLine.append(" ");
		
		if(m_LuaHandler.m_ScopeCount == 0)
		{
			try
			{
				luaL_loadstring(m_LuaHandler.m_pLuaState, m_LuaHandler.m_FullLine.c_str());
				Status = lua_pcall(m_LuaHandler.m_pLuaState, 0, LUA_MULTRET, 0);
				
				if(Status)
				{
					str_format(ErrorMsg, sizeof(ErrorMsg), "%s", lua_tostring(m_LuaHandler.m_pLuaState, -1));
					
					if(!strcmp(ErrorMsg, "attempt to call a string value"))  //HACKISH SOLUTION : Try recompile pLine with a print!
					{
						Status = 0;
						
						char aBuf[256];
						str_format(aBuf, sizeof(aBuf), "print(%s)", m_LuaHandler.m_FullLine.c_str());
						
						luaL_loadstring(m_LuaHandler.m_pLuaState, aBuf);
						Status = lua_pcall(m_LuaHandler.m_pLuaState, 0, LUA_MULTRET, 0);
					}
				}			
				}
				catch(std::exception &e)  //just to be sure...
				{
					PrintLine(e.what());
				}
				catch(...)
				{
					PrintLine("An unknown error occured!");
				}
				
				/*m_LuaHandler.m_FullLine.resize(m_LuaHandler.m_FullLine.size()-1);  //remove the last " "
				//add this to the history :3
				char *pEntry = m_History.Allocate(m_LuaHandler.m_FullLine.size()+1);
				mem_copy(pEntry, m_LuaHandler.m_FullLine.c_str(), m_LuaHandler.m_FullLine.size()+1);*/

				
				m_LuaHandler.m_FullLine = "";
				m_LuaHandler.m_ScopeCount = 0;
				
				if(Status)
				{
					str_format(ErrorMsg, sizeof(ErrorMsg), "%s", lua_tostring(m_LuaHandler.m_pLuaState, -1));
					PrintLine(ErrorMsg);
				}
		}
		else if(m_LuaHandler.m_ScopeCount < 0)
		{
			PrintLine("Please don't end your stuff before you already started.");
			m_LuaHandler.m_ScopeCount = 0;
			m_LuaHandler.m_FullLine = "";
		}
		
		if(ScopeIncreased || m_LuaHandler.m_ScopeCount > 0)   //bigger 0
		{
			char aBuf[512] = { 0 };
			
			int Limit = ScopeIncreased == true ? m_LuaHandler.m_ScopeCount-1 : m_LuaHandler.m_ScopeCount;
			
			if(ActLine.find("else") != std::string::npos)
				Limit--;
			
			for(int i = 0; i < Limit; i++)
				str_append(aBuf, "     ", sizeof(aBuf));
			
			str_append(aBuf, pLine, sizeof(aBuf));
			PrintLine(aBuf);
		}
	}
}

void CGameConsole::CInstance::PossibleCommandsCompleteCallback(const char *pStr, void *pUser)
{
	CGameConsole::CInstance *pInstance = (CGameConsole::CInstance *)pUser;
	if(pInstance->m_CompletionChosen == pInstance->m_CompletionEnumerationCount)
		pInstance->m_Input.Set(pStr);
	pInstance->m_CompletionEnumerationCount++;
}

bool CGameConsole::OnMouseMove(float x, float y)
{
	//m_LastInput = time_get();

	if((m_ConsoleState != CONSOLE_OPEN && m_ConsoleState != CONSOLE_OPENING))
		return false;

#if defined(__ANDROID__) // No relative mouse on Android
	m_MousePos.x = x;
	m_MousePos.y = y;
#else
	UI()->ConvertMouseMove(&x, &y);
	m_MousePos.x += x;
	m_MousePos.y += y;
#endif
	if(m_MousePos.x < 0) m_MousePos.x = 0;
	if(m_MousePos.y < 0) m_MousePos.y = 0;
	if(m_MousePos.x > Graphics()->ScreenWidth()) m_MousePos.x = Graphics()->ScreenWidth();
	if(m_MousePos.y > Graphics()->ScreenHeight()) m_MousePos.y = Graphics()->ScreenHeight();

	return true;
}

void CGameConsole::CInstance::OnInput(IInput::CEvent Event)
{
	bool Handled = false;

	if(m_pGameConsole->Input()->KeyIsPressed(KEY_LCTRL) && m_pGameConsole->Input()->KeyPress(KEY_V))
	{
		const char *Text = m_pGameConsole->Input()->GetClipboardText();
		if(Text)
		{
			char Line[256];
			int i, Begin = 0;
			for(i = 0; i < str_length(Text); i++)
			{
				if(Text[i] == '\n')
				{
					if(i == Begin)
					{
						Begin++;
						continue;
					}
					int max = min(i - Begin + 1, (int)sizeof(Line));
					str_copy(Line, Text + Begin, max);
					Begin = i+1;
					ExecuteLine(Line);
				}
			}
			int max = min(i - Begin + 1, (int)sizeof(Line));
			str_copy(Line, Text + Begin, max);
			Begin = i+1;
			m_Input.Add(Line);
		}
	}

	if(m_pGameConsole->Input()->KeyIsPressed(KEY_LCTRL) && m_pGameConsole->Input()->KeyPress(KEY_C))
	{
		m_pGameConsole->Input()->SetClipboardText(m_Input.GetString());
	}

	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		if(Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER)
		{
			if(m_Input.GetString()[0])
			{
				if(m_Type == CONSOLETYPE_LOCAL || (m_Type == CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed()))
				{
					char *pEntry = m_History.Allocate(m_Input.GetLength()+1);
					mem_copy(pEntry, m_Input.GetString(), m_Input.GetLength()+1);
				}
				else if(m_Type == CONSOLETYPE_LUA)
				{
					//if(m_LuaHandler.m_FullLine.size())
					//	m_LuaHandler.m_FullLine.resize(m_LuaHandler.m_FullLine.size()-1);  //remove the last " "
					
					std::string Complete = m_LuaHandler.m_FullLine;
					Complete.append(m_Input.GetString());
					
					//add this to the history :3
					char *pEntry = m_History.Allocate(Complete.size()+1);
					mem_copy(pEntry, Complete.c_str(), Complete.size()+1);
				}
				ExecuteLine(m_Input.GetString());
				m_Input.Clear();
				m_pHistoryEntry = 0x0;
			}

			Handled = true;
		}
		else if (Event.m_Key == KEY_UP)
		{
			if (m_pHistoryEntry)
			{
				char *pTest = m_History.Prev(m_pHistoryEntry);

				if (pTest)
					m_pHistoryEntry = pTest;
			}
			else
				m_pHistoryEntry = m_History.Last();

			if (m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry);
			Handled = true;
		}
		else if (Event.m_Key == KEY_DOWN)
		{
			if (m_pHistoryEntry)
				m_pHistoryEntry = m_History.Next(m_pHistoryEntry);

			if (m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry);
			else
				m_Input.Clear();
			Handled = true;
		}
		else if(Event.m_Key == KEY_TAB)
		{
			if(m_Type == CGameConsole::CONSOLETYPE_LOCAL || m_pGameConsole->Client()->RconAuthed())
			{
				if(m_ReverseTAB)
					 m_CompletionChosen--;
				else
					m_CompletionChosen++;
				m_CompletionEnumerationCount = 0;
				m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask, m_Type != CGameConsole::CONSOLETYPE_LOCAL &&
					m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands(),	PossibleCommandsCompleteCallback, this);

				// handle wrapping
				if(m_CompletionEnumerationCount && (m_CompletionChosen >= m_CompletionEnumerationCount || m_CompletionChosen <0))
				{
					m_CompletionChosen= (m_CompletionChosen + m_CompletionEnumerationCount) %  m_CompletionEnumerationCount;
					m_CompletionEnumerationCount = 0;
					m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask, m_Type != CGameConsole::CONSOLETYPE_LOCAL &&
						m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands(),	PossibleCommandsCompleteCallback, this);
				}
			}
		}
		else if(Event.m_Key == KEY_PAGEUP)
		{
			if(m_BacklogLineOffset+=26 > 27-1) // 27 lines fit onto one page
			{
				m_BacklogLineOffset = m_BacklogLineOffset%27;
				m_BacklogActPage++;
			}
		}
		else if(Event.m_Key == KEY_PAGEDOWN)
		{
			if(m_BacklogLineOffset-=27 < 0)
			{
				m_BacklogLineOffset = m_BacklogLineOffset%27;
				if(--m_BacklogActPage < 0)
				{
					m_BacklogActPage = 0;
					m_BacklogLineOffset = 0;
				}
			}
		}
		else if(Event.m_Key == KEY_MOUSE_WHEEL_UP)
		{
			if(++m_BacklogLineOffset > 27-1) // 27 lines fit onto one page
			{
				m_BacklogLineOffset = 0;
				m_BacklogActPage++;
			}
		}
		else if(Event.m_Key == KEY_MOUSE_WHEEL_DOWN)
		{
			if(--m_BacklogLineOffset < 0)
			{
				m_BacklogLineOffset = 27-1;
				if(--m_BacklogActPage < 0)
				{
					m_BacklogActPage = 0;
					m_BacklogLineOffset = 0;
				}
			}
		}
		else if(Event.m_Key == KEY_F)
		{
			if(m_CTRLPressed)
			{
				if(!m_pSearchString)
					m_pSearchString = m_Input.GetString();
				else
					m_pSearchString = 0;
			}
		}
		else if(Event.m_Key == KEY_LSHIFT)
		{
			if(!m_CTRLPressed)
				m_ReverseTAB = true;
			Handled = true;
		}
		else if(Event.m_Key == KEY_LCTRL)
		{
			if(!m_ReverseTAB)
				m_CTRLPressed = true;
			Handled = true;
		}
	}
	if(Event.m_Flags&IInput::FLAG_RELEASE)
	{
		if(Event.m_Key == KEY_LSHIFT)
		{
			m_ReverseTAB = false;
			Handled = true;
		}
		else if(Event.m_Key == KEY_LCTRL)
		{
			m_CTRLPressed = false;
			Handled = true;
		}
	}

	if(!Handled)
		m_Input.ProcessInput(Event);

	if(Event.m_Flags & (IInput::FLAG_PRESS|IInput::FLAG_TEXT))
	{
		if((Event.m_Key != KEY_TAB) && (Event.m_Key != KEY_LSHIFT))
		{
			m_CompletionChosen = -1;
			str_copy(m_aCompletionBuffer, m_Input.GetString(), sizeof(m_aCompletionBuffer));
		}

		std::string line(m_Input.GetString());
		std::size_t pos = line.find("$ADDR");
		if(pos != std::string::npos && pos < line.length()-4)
		{
			// prepare address
			std::string addr(g_Config.m_UiServerAddress);
			for(size_t i = 0; i < addr.length(); i++)
				if(addr[i] == ':') addr[i] = ' ';

			// insert the address
			std::string right = line.substr(pos+5, line.length());
			std::string left= line.substr(0, pos);
			std::string newstr = left + addr + right;

			m_Input.Set(newstr.c_str());
		}


		// find the current command
		{
			char aBuf[64] = {0};
			const char *pSrc = GetString();
			int i = 0;
			for(; i < (int)sizeof(aBuf)-1 && *pSrc && *pSrc != ' '; i++, pSrc++)
				aBuf[i] = *pSrc;
			aBuf[i] = 0;

			const IConsole::CCommandInfo *pCommand = m_pGameConsole->m_pConsole->GetCommandInfo(aBuf, m_CompletionFlagmask,
				m_Type != CGameConsole::CONSOLETYPE_LOCAL && m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands());
			if(pCommand)
			{
				m_IsCommand = true;
				str_copy(m_aCommandName, pCommand->m_pName, IConsole::TEMPCMD_NAME_LENGTH);
				str_copy(m_aCommandHelp, pCommand->m_pHelp, IConsole::TEMPCMD_HELP_LENGTH);
				str_copy(m_aCommandParams, pCommand->m_pParams, IConsole::TEMPCMD_PARAMS_LENGTH);
			}
			else
				m_IsCommand = false;
		}
	}
}

void CGameConsole::CInstance::PrintLine(const char *pLine, bool Highlighted)
{
	int Len = str_length(pLine);

	if (Len > 255)
		Len = 255;

	CBacklogEntry *pEntry = m_Backlog.Allocate(sizeof(CBacklogEntry)+Len);
	pEntry->m_YOffset = -1.0f;
	pEntry->m_Highlighted = Highlighted;
	mem_copy(pEntry->m_aText, pLine, Len);
	pEntry->m_aText[Len] = 0;
}

bool CGameConsole::CInstance::LoadLuaFile(const char *pFile)  //this function is for LuaConsole
{
	if(m_Type != CONSOLETYPE_LUA)
		return false;
	
	if(!pFile || pFile[0] == '\0' || !m_LuaHandler.m_pLuaState)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Error loading '%s'", pFile);
		PrintLine(aBuf);
		return false;
	}
		
	int Status = luaL_loadfile(m_LuaHandler.m_pLuaState, pFile);
    if (Status)
    {
        // does this work? -- I don't think so, Henritees.
        PrintLine(lua_tostring(m_LuaHandler.m_pLuaState, -1));
        return false;
    }

    Status = lua_pcall(m_LuaHandler.m_pLuaState, 0, LUA_MULTRET, 0);
    if (Status)
    {
    	PrintLine(lua_tostring(m_LuaHandler.m_pLuaState, -1));
        return false;
    }
	
	return true;
}

CGameConsole::CGameConsole()
: m_LocalConsole(CONSOLETYPE_LOCAL), m_RemoteConsole(CONSOLETYPE_REMOTE), m_LuaConsole(CONSOLETYPE_LUA)
{
	m_ConsoleType = CONSOLETYPE_LOCAL;
	m_ConsoleState = CONSOLE_CLOSED;
	m_StateChangeEnd = 0.0f;
	m_StateChangeDuration = 0.1f;
}

float CGameConsole::TimeNow()
{
	static long long s_TimeStart = time_get();
	return float(time_get()-s_TimeStart)/float(time_freq());
}

CGameConsole::CInstance *CGameConsole::CurrentConsole()
{
	if(m_ConsoleType == CONSOLETYPE_REMOTE)
		return &m_RemoteConsole;
	else if(m_ConsoleType == CONSOLETYPE_LUA)
		return &m_LuaConsole;
	return &m_LocalConsole;
}

void CGameConsole::OnReset()
{
}

// only defined for 0<=t<=1
static float ConsoleScaleFunc(float t)
{
	//return t;
	return sinf(acosf(1.0f-t));
}

struct CRenderInfo
{
	CGameConsole *m_pSelf;
	CTextCursor m_Cursor;
	const char *m_pCurrentCmd;
	int m_WantedCompletion;
	int m_EnumCount;
	float m_Offset;
	float m_Width;
};

void CGameConsole::PossibleCommandsRenderCallback(const char *pStr, void *pUser)
{
	CRenderInfo *pInfo = static_cast<CRenderInfo *>(pUser);

	if(pInfo->m_EnumCount == pInfo->m_WantedCompletion)
	{
		float tw = pInfo->m_pSelf->TextRender()->TextWidth(pInfo->m_Cursor.m_pFont, pInfo->m_Cursor.m_FontSize, pStr, -1);
		pInfo->m_pSelf->Graphics()->TextureSet(-1);
		pInfo->m_pSelf->Graphics()->QuadsBegin();
			pInfo->m_pSelf->Graphics()->SetColor(229.0f/255.0f,185.0f/255.0f,4.0f/255.0f,0.85f);
			pInfo->m_pSelf->RenderTools()->DrawRoundRect(pInfo->m_Cursor.m_X-3, pInfo->m_Cursor.m_Y, tw+5, pInfo->m_Cursor.m_FontSize+4, pInfo->m_Cursor.m_FontSize/3);
		pInfo->m_pSelf->Graphics()->QuadsEnd();

		// scroll when out of sight
		if(pInfo->m_Cursor.m_X < 3.0f)
			pInfo->m_Offset = 0.0f;
		else if(pInfo->m_Cursor.m_X+tw > pInfo->m_Width)
			pInfo->m_Offset -= pInfo->m_Width/2;

		pInfo->m_pSelf->TextRender()->TextColor(0.05f, 0.05f, 0.05f,1);
		pInfo->m_pSelf->TextRender()->TextEx(&pInfo->m_Cursor, pStr, -1);
	}
	else
	{
		const char *pMatchStart = str_find_nocase(pStr, pInfo->m_pCurrentCmd);

		if(pMatchStart)
		{
			pInfo->m_pSelf->TextRender()->TextColor(0.5f,0.5f,0.5f,1);
			pInfo->m_pSelf->TextRender()->TextEx(&pInfo->m_Cursor, pStr, pMatchStart-pStr);
			pInfo->m_pSelf->TextRender()->TextColor(229.0f/255.0f,185.0f/255.0f,4.0f/255.0f,1);
			pInfo->m_pSelf->TextRender()->TextEx(&pInfo->m_Cursor, pMatchStart, str_length(pInfo->m_pCurrentCmd));
			pInfo->m_pSelf->TextRender()->TextColor(0.5f,0.5f,0.5f,1);
			pInfo->m_pSelf->TextRender()->TextEx(&pInfo->m_Cursor, pMatchStart+str_length(pInfo->m_pCurrentCmd), -1);
		}
		else
		{
			pInfo->m_pSelf->TextRender()->TextColor(0.75f,0.75f,0.75f,1);
			pInfo->m_pSelf->TextRender()->TextEx(&pInfo->m_Cursor, pStr, -1);
		}
	}

	pInfo->m_EnumCount++;
	pInfo->m_Cursor.m_X += 7.0f;
}

void CGameConsole::OnRender()
{
	CUIRect Screen = *UI()->Screen();
	float ConsoleMaxHeight = Screen.h*3/5.0f;
	float ConsoleHeight;

	float Progress = (TimeNow()-(m_StateChangeEnd-m_StateChangeDuration))/float(m_StateChangeDuration);

	if (Progress >= 1.0f)
	{
		if (m_ConsoleState == CONSOLE_CLOSING)
			m_ConsoleState = CONSOLE_CLOSED;
		else if (m_ConsoleState == CONSOLE_OPENING)
			m_ConsoleState = CONSOLE_OPEN;

		Progress = 1.0f;
	}

	if (m_ConsoleState == CONSOLE_OPEN && g_Config.m_ClEditor)
		Toggle(CONSOLETYPE_LOCAL);

	if (m_ConsoleState == CONSOLE_CLOSED)
		return;

	float ConsoleHeightScale;

	if (m_ConsoleState == CONSOLE_OPENING)
		ConsoleHeightScale = ConsoleScaleFunc(Progress);
	else if (m_ConsoleState == CONSOLE_CLOSING)
		ConsoleHeightScale = ConsoleScaleFunc(1.0f-Progress);
	else //if (console_state == CONSOLE_OPEN)
		ConsoleHeightScale = ConsoleScaleFunc(1.0f);

	ConsoleHeight = ConsoleHeightScale*ConsoleMaxHeight;

	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	{
		// do console shadow
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		IGraphics::CColorVertex Array[4] = {
			IGraphics::CColorVertex(0, 0,0,0, 0.5f),
			IGraphics::CColorVertex(1, 0,0,0, 0.5f),
			IGraphics::CColorVertex(2, 0,0,0, 0.0f),
			IGraphics::CColorVertex(3, 0,0,0, 0.0f)};
		Graphics()->SetColorVertex(Array, 4);
		IGraphics::CQuadItem QuadItem(0, ConsoleHeight, Screen.w, 10.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		// do background
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CONSOLE_BG].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.2f, 0.2f, 0.2f,0.9f);
		if(m_ConsoleType == CONSOLETYPE_REMOTE)
			Graphics()->SetColor(0.4f, 0.2f, 0.2f,0.9f);
		else if(m_ConsoleType == CONSOLETYPE_LUA)
			Graphics()->SetColor(0.0f, 0.2f, 0.4f, 0.9f);
		Graphics()->QuadsSetSubset(0,-ConsoleHeight*0.075f,Screen.w*0.075f*0.5f,0);
		QuadItem = IGraphics::CQuadItem(0, 0, Screen.w, ConsoleHeight);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		// do small bar shadow
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Array[0] = IGraphics::CColorVertex(0, 0,0,0, 0.0f);
		Array[1] = IGraphics::CColorVertex(1, 0,0,0, 0.0f);
		Array[2] = IGraphics::CColorVertex(2, 0,0,0, 0.25f);
		Array[3] = IGraphics::CColorVertex(3, 0,0,0, 0.25f);
		Graphics()->SetColorVertex(Array, 4);
		QuadItem = IGraphics::CQuadItem(0, ConsoleHeight-20, Screen.w, 10);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		// do the lower bar
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CONSOLE_BAR].m_Id);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.9f);
		Graphics()->QuadsSetSubset(0,0.1f,Screen.w*0.015f,1-0.1f);
		QuadItem = IGraphics::CQuadItem(0,ConsoleHeight-10.0f,Screen.w,10.0f);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		ConsoleHeight -= 22.0f;
	}

	CInstance *pConsole = CurrentConsole();

	// update the ui
	float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*Screen.w;
	float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*Screen.h;
	{
		int Buttons = 0;
		if(Input()->KeyPress(KEY_MOUSE_1)) Buttons |= 1;
		if(Input()->KeyPress(KEY_MOUSE_2)) Buttons |= 2;
		if(Input()->KeyPress(KEY_MOUSE_3)) Buttons |= 4;

#if defined(__ANDROID__)
		static int ButtonsOneFrameDelay = 0; // For Android touch input

		UI()->Update(mx,my,mx*3.0f,my*3.0f,ButtonsOneFrameDelay);
		ButtonsOneFrameDelay = Buttons;
#else
		UI()->Update(mx,my,mx*3.0f,my*3.0f,Buttons);
#endif
	}

	{
		float FontSize = 10.0f;
		float RowHeight = FontSize*1.25f;
		float x = 3;
		float y = ConsoleHeight - RowHeight - 5.0f;

		CRenderInfo Info;
		Info.m_pSelf = this;
		Info.m_WantedCompletion = pConsole->m_CompletionChosen;
		Info.m_EnumCount = 0;
		Info.m_Offset = pConsole->m_CompletionRenderOffset;
		Info.m_Width = Screen.w;
		Info.m_pCurrentCmd = pConsole->m_aCompletionBuffer;
		TextRender()->SetCursor(&Info.m_Cursor, x+Info.m_Offset, y+RowHeight+2.0f, FontSize, TEXTFLAG_RENDER);

		// render prompt
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);
		const char *pPrompt = "> ";
		if(m_pSearchString)
			pPrompt = "[CTRL+F] SEARCHING» ";
		if(m_ConsoleType == CONSOLETYPE_REMOTE)
		{
			if(Client()->State() == IClient::STATE_ONLINE)
			{
				if(Client()->RconAuthed())
					pPrompt = "rcon> ";
				else
					pPrompt = "ENTER PASSWORD> ";
			}
			else
				pPrompt = "NOT CONNECTED! ";
		}
		else if(m_ConsoleType == CONSOLETYPE_LUA)
		{
			if(g_Config.m_ClLua)
				pPrompt = "Lua> ";
			else
				pPrompt = "Lua disabled. Please enable Lua first. ";
		}
		TextRender()->TextEx(&Cursor, pPrompt, -1);

		x = Cursor.m_X;

		// hide rcon password
		char aInputString[512];
		str_copy(aInputString, pConsole->m_Input.GetString(), sizeof(aInputString));
		if(m_ConsoleType == CONSOLETYPE_REMOTE && Client()->State() == IClient::STATE_ONLINE && !Client()->RconAuthed())
		{
			for(int i = 0; i < pConsole->m_Input.GetLength(); ++i)
				aInputString[i] = '*';
		}

		// render console input (wrap line)
		TextRender()->SetCursor(&Cursor, x, y, FontSize, 0);
		Cursor.m_LineWidth = Screen.w - 10.0f - x;
		TextRender()->TextEx(&Cursor, aInputString, pConsole->m_Input.GetCursorOffset());
		TextRender()->TextEx(&Cursor, aInputString+pConsole->m_Input.GetCursorOffset(), -1);
		int Lines = Cursor.m_LineCount;

		y -= (Lines - 1) * FontSize;
		TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = Screen.w - 10.0f - x;

		TextRender()->TextEx(&Cursor, aInputString, pConsole->m_Input.GetCursorOffset());
		static float MarkerOffset = TextRender()->TextWidth(0, FontSize, "|", -1)/3;
		CTextCursor Marker = Cursor;
		Marker.m_X -= MarkerOffset;
		Marker.m_LineWidth = -1;
		TextRender()->TextEx(&Marker, "|", -1);
		TextRender()->TextEx(&Cursor, aInputString+pConsole->m_Input.GetCursorOffset(), -1);

		// render possible commands
		if(m_ConsoleType == CONSOLETYPE_LOCAL || Client()->RconAuthed())
		{
			if(pConsole->m_Input.GetString()[0] != 0)
			{
				m_pConsole->PossibleCommands(pConsole->m_aCompletionBuffer, pConsole->m_CompletionFlagmask, m_ConsoleType != CGameConsole::CONSOLETYPE_LOCAL &&
					Client()->RconAuthed() && Client()->UseTempRconCommands(), PossibleCommandsRenderCallback, &Info);
				pConsole->m_CompletionRenderOffset = Info.m_Offset;

				if(Info.m_EnumCount <= 0)
				{
					if(pConsole->m_IsCommand)
					{
						char aBuf[768];
						str_format(aBuf, sizeof(aBuf), "Help: %s ", pConsole->m_aCommandHelp);
						TextRender()->TextEx(&Info.m_Cursor, aBuf, -1);
						TextRender()->TextColor(0.75f, 0.75f, 0.75f, 1);
						str_format(aBuf, sizeof(aBuf), "Usage: %s %s", pConsole->m_aCommandName, pConsole->m_aCommandParams);
						TextRender()->TextEx(&Info.m_Cursor, aBuf, -1);
					}
				}
			}
		}

		vec3 rgb = HslToRgb(vec3(g_Config.m_ClMessageHighlightHue / 255.0f, g_Config.m_ClMessageHighlightSat / 255.0f, g_Config.m_ClMessageHighlightLht / 255.0f));

		//	render log (actual page, wrap lines)
		CInstance::CBacklogEntry *pEntry = pConsole->m_Backlog.Last();
		float OffsetY = 0.0f;
		float LineOffset = 1.0f;

		for(int Page = 0; Page <= pConsole->m_BacklogActPage; ++Page, OffsetY = 0.0f)
		{
			for(int asdf = 0; asdf < pConsole->m_BacklogLineOffset; asdf++)
			{
				if(pConsole->m_Backlog.Prev(pEntry))
					pEntry = pConsole->m_Backlog.Prev(pEntry);
				else break;
			}

			while(pEntry)
			{
				if(m_pSearchString && !str_find_nocase(pEntry->m_aText, m_pSearchString))
				{
					pEntry = pConsole->m_Backlog.Prev(pEntry); // skip entries not mathing our search
					continue;
				}

				if(pEntry->m_Highlighted)
					TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1);
				else
					TextRender()->TextColor(1,1,1,1);

				// get y offset (calculate it if we haven't yet)
				if(pEntry->m_YOffset < 0.0f)
				{
					TextRender()->SetCursor(&Cursor, 0.0f, 0.0f, FontSize, 0);
					Cursor.m_LineWidth = Screen.w-10;
					TextRender()->TextEx(&Cursor, pEntry->m_aText, -1);
					pEntry->m_YOffset = Cursor.m_Y+Cursor.m_FontSize+LineOffset;
				}
				OffsetY += pEntry->m_YOffset;

				//	next page when lines reach the top
				if(y-OffsetY <= RowHeight)
					break;

				//	just render output from actual backlog page (render bottom up)
				if(Page == pConsole->m_BacklogActPage)
				{
					TextRender()->SetCursor(&Cursor, 0.0f, y-OffsetY, FontSize, TEXTFLAG_RENDER);
					Cursor.m_LineWidth = Screen.w-10.0f;
					
					const char *pCursor = pEntry->m_aText;
					const char *pUrlBeginning = pEntry->m_aText;
					const char *pUrlEnding;

					char aUrl[64];
					int UrlSize;

					CUIRect TextRect;

					bool Found = false;
					bool One = true;

					while(true)
					{
						if(!pUrlBeginning)
							break;

						mem_zero(aUrl, sizeof(aUrl));
						UrlSize = 0;

						pUrlBeginning = str_find(pCursor, "http://");
						if(!pUrlBeginning)
							pUrlBeginning = str_find(pCursor, "https://");

						if(pUrlBeginning) // found the link
						{
							Found = true;

							pUrlEnding = str_find(pUrlBeginning, " ");
							if(!pUrlEnding) // the link is till the end
								pUrlEnding = pUrlBeginning + str_length(pUrlBeginning);
							else
							{
								One = false;
								pUrlEnding++;
							}

							UrlSize = pUrlEnding - pUrlBeginning;
							str_copy(aUrl, pUrlBeginning, UrlSize + 1);

							// url rect
							TextRect.x = TextRender()->TextWidth(0, FontSize, pEntry->m_aText, pUrlBeginning - pEntry->m_aText);
							TextRect.y = y - OffsetY;
							TextRect.w = TextRender()->TextWidth(0, FontSize, pUrlBeginning, UrlSize);
							TextRect.h = FontSize;

							// render the first part
							if(pUrlBeginning - pCursor > 0)
							{
								TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
								TextRender()->TextEx(&Cursor, pCursor, pUrlBeginning - pCursor);
							}

							// set the color and check if pressed
							if(UI()->MouseInside(&TextRect))
							{
								TextRender()->TextColor(0.0f, 1.0f, 0.39f, 1.0f);
								static float LastClicked = 0;
								if(UI()->MouseButtonClicked(0) && Client()->LocalTime() > LastClicked + 1)
								{
									LastClicked = Client()->LocalTime();
									Input()->MouseModeAbsolute();
									//((IEngineGraphics *)Kernel()->RequestInterface<IEngineGraphics>())->Minimize();
									open_default_browser(aUrl);
								}
							}
							else
								TextRender()->TextColor(1.0f, 0.39f, 0.0f, 1.0f);

							// render the link
							TextRender()->TextEx(&Cursor, pUrlBeginning, UrlSize);

							// render the rest
							if(One)
							{
								TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
								TextRender()->TextEx(&Cursor, pUrlEnding, str_length(pUrlEnding));
							}
							
							pCursor = pUrlEnding;
						}
					}

					if(!One)
					{
						TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
						TextRender()->TextEx(&Cursor, pUrlEnding, str_length(pUrlEnding));
					}

					if(!Found)
					{
						// highlight the parts that matches
						if(m_pSearchString && m_pSearchString[0] != '\0')
						{
							const char *pText = pEntry->m_aText;
							while(pText)
							{
								const char *pFoundStr = str_find_nocase(pText, m_pSearchString);
								if(pFoundStr)
								{
									TextRender()->TextEx(&Cursor, pText, (int)(pFoundStr-pText));
									TextRender()->TextColor(0.8f, 0.7f, 0.15f, 1);
									TextRender()->TextEx(&Cursor, pFoundStr, str_length(m_pSearchString));
									TextRender()->TextColor(1,1,1,1);
									//TextRender()->TextEx(&Cursor, pFoundStr+str_length(m_pSearchString), -1);
									pText = pFoundStr+str_length(m_pSearchString);
								}
								else
								{
									TextRender()->TextEx(&Cursor, pText, -1);
									break;
								}

								if(pText > pEntry->m_aText + str_length(pEntry->m_aText)-1 || pText < pEntry->m_aText)
									pText = 0;
							}
						}
						else
							TextRender()->TextEx(&Cursor, pEntry->m_aText, -1);
					}
				}
				pEntry = pConsole->m_Backlog.Prev(pEntry);

				// reset color
				TextRender()->TextColor(1,1,1,1);
			}

			//	actual backlog page number is too high, render last available page (current checked one, render top down)
			if(!pEntry && Page < pConsole->m_BacklogActPage)
			{
				pConsole->m_BacklogActPage = Page;
				pConsole->m_BacklogLineOffset = 26; // <-ANZAHL DER ZEILEN AUF SEITE "Page" TODO XXX nicht 27
				pEntry = pConsole->m_Backlog.First();
				while(OffsetY > 0.0f && pEntry)
				{
					TextRender()->SetCursor(&Cursor, 0.0f, y-OffsetY, FontSize, TEXTFLAG_RENDER);
					Cursor.m_LineWidth = Screen.w-10.0f;
					TextRender()->TextEx(&Cursor, pEntry->m_aText, -1);
					OffsetY -= pEntry->m_YOffset;
					pEntry = pConsole->m_Backlog.Next(pEntry);
				}
				break;
			}
		}

		// render page
		char aBuf[128];
		TextRender()->TextColor(1,1,1,1);
		str_format(aBuf, sizeof(aBuf), Localize("-Page %d, Offset %d-"), pConsole->m_BacklogActPage+1, pConsole->m_BacklogLineOffset);
		TextRender()->Text(0, 10.0f, 0.0f, FontSize, aBuf, -1);

		// render version
		str_format(aBuf, sizeof(aBuf), "v%s - AllTheHaxx %s", GAME_VERSION, ALLTHEHAXX_VERSION);
		float Width = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->Text(0, Screen.w-Width-10.0f, 0.0f, FontSize, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "built on %s", BUILD_DATE);
		Width = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->Text(0, Screen.w-Width-10.0f, 10.0f, FontSize, aBuf, -1);
	}

	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(mx, my, 24, 24);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CGameConsole::OnMessage(int MsgType, void *pRawMsg)
{
}

bool CGameConsole::OnInput(IInput::CEvent Event)
{
	if(m_ConsoleState != CONSOLE_OPEN)
		return false;
	if((Event.m_Key >= KEY_F1 && Event.m_Key <= KEY_F12) || (Event.m_Key >= KEY_F13 && Event.m_Key <= KEY_F24))
		return false;

	if(Event.m_Key == KEY_ESCAPE && (Event.m_Flags&IInput::FLAG_PRESS))
		Toggle(m_ConsoleType);
	else
		CurrentConsole()->OnInput(Event);

	return true;
}

void CGameConsole::Toggle(int Type)
{
	if(m_ConsoleType != Type && (m_ConsoleState == CONSOLE_OPEN || m_ConsoleState == CONSOLE_OPENING))
			Input()->MouseModeRelative();
	else
	{
		if (m_ConsoleState == CONSOLE_CLOSED || m_ConsoleState == CONSOLE_OPEN)
		{
			m_StateChangeEnd = TimeNow()+m_StateChangeDuration;
		}
		else
		{
			float Progress = m_StateChangeEnd-TimeNow();
			float ReversedProgress = m_StateChangeDuration-Progress;

			m_StateChangeEnd = TimeNow()+ReversedProgress;
		}

		if (m_ConsoleState == CONSOLE_CLOSED || m_ConsoleState == CONSOLE_CLOSING)
		{
			Input()->MouseModeAbsolute();
			m_pClient->m_pMenus->UseMouseButtons(false);
			m_ConsoleState = CONSOLE_OPENING;
			/*// reset controls - no don't do it because we want swag!
			m_pClient->m_pControls->OnReset();*/
		}
		else
		{
			Input()->MouseModeRelative();
			m_pClient->m_pMenus->UseMouseButtons(true);
			m_pClient->OnRelease();
			m_ConsoleState = CONSOLE_CLOSING;
		}
	}

	m_ConsoleType = Type;

	if(m_ConsoleState == CONSOLE_OPEN || m_ConsoleState == CONSOLE_OPENING)
	{
		Input()->MouseModeRelative();
	}

	if(m_ConsoleState == CONSOLE_CLOSED)
		Input()->MouseModeRelative();
}

void CGameConsole::Dump(int Type)
{
	CInstance *pConsole = Type == CONSOLETYPE_REMOTE ? &m_RemoteConsole : &m_LocalConsole;
	char aFilename[128];
	char aDate[20];

	str_timestamp(aDate, sizeof(aDate));
	str_format(aFilename, sizeof(aFilename), "dumps/%s/%s.txt",
			Type == CONSOLETYPE_REMOTE ? "console_remote" : "console_local", aDate);
	IOHANDLE io = Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
	if(io)
	{
		for(CInstance::CBacklogEntry *pEntry = pConsole->m_Backlog.First(); pEntry;
				pEntry = pConsole->m_Backlog.Next(pEntry))
		{
			io_write(io, pEntry->m_aText, str_length(pEntry->m_aText));
			io_write_newline(io);
		}
		io_close(io);
	}
}

void CGameConsole::ConToggleLocalConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Toggle(CONSOLETYPE_LOCAL);
}

void CGameConsole::ConToggleRemoteConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Toggle(CONSOLETYPE_REMOTE);
}

void CGameConsole::ConClearLocalConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->m_LocalConsole.ClearBacklog();
}

void CGameConsole::ConToggleLuaConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Toggle(CONSOLETYPE_LUA);
}

void CGameConsole::ConClearRemoteConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->m_RemoteConsole.ClearBacklog();
}

void CGameConsole::ConDumpLocalConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Dump(CONSOLETYPE_LOCAL);
}

void CGameConsole::ConDumpRemoteConsole(IConsole::IResult *pResult, void *pUserData)
{
	((CGameConsole *)pUserData)->Dump(CONSOLETYPE_REMOTE);
}

void CGameConsole::Con_Lua(IConsole::IResult *pResult, void *pUserData)
{
	if(!g_Config.m_ClLua)
	{
		((CGameConsole *)pUserData)->PrintLine(CONSOLETYPE_LOCAL, "Lua is disabled. Please enable Lua first.");
		return;
	}
	((CGameConsole *)pUserData)->m_LuaConsole.ExecuteLine(pResult->GetString(0));
}

void CGameConsole::ClientConsolePrintCallback(const char *pStr, void *pUserData, bool Highlighted)
{
	((CGameConsole *)pUserData)->m_LocalConsole.PrintLine(pStr, Highlighted);
}

void CGameConsole::ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments() == 1)
	{
		CGameConsole *pThis = static_cast<CGameConsole *>(pUserData);
		pThis->Console()->SetPrintOutputLevel(pThis->m_PrintCBIndex, pResult->GetInteger(0));
	}
}

// TODO: This may be moved to elsewhere
void CGameConsole::ConchainIRCNickUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	CGameConsole *pThis = static_cast<CGameConsole *>(pUserData);
	pThis->m_pClient->m_pIRCBind->OnNickChange(g_Config.m_ClIRCNick);
}


void CGameConsole::PrintLine(int Type, const char *pLine)
{
	if(Type == CONSOLETYPE_REMOTE)
		m_RemoteConsole.PrintLine(pLine);
	else if(Type == CONSOLETYPE_LUA)
		m_LuaConsole.PrintLine(pLine);
	else
		m_LocalConsole.PrintLine(pLine);
}

void CGameConsole::PrintLuaLine(const char *pLine)
{
	//if the thing is nil in lua then pLine == 0!
	if(pLine == 0)
		CGameConsole::m_pStatLuaConsole->PrintLine("nil");
	else
		CGameConsole::m_pStatLuaConsole->PrintLine(pLine);
}

void CGameConsole::OnConsoleInit()
{
	// init console instances
	m_LocalConsole.Init(this);
	m_RemoteConsole.Init(this);
	m_LuaConsole.Init(this);

	m_pConsole = Kernel()->RequestInterface<IConsole>();

	//
	m_PrintCBIndex = Console()->RegisterPrintCallback(g_Config.m_ConsoleOutputLevel, ClientConsolePrintCallback, this);

	Console()->Register("toggle_local_console", "", CFGFLAG_CLIENT, ConToggleLocalConsole, this, "Toggle local console");
	Console()->Register("toggle_remote_console", "", CFGFLAG_CLIENT, ConToggleRemoteConsole, this, "Toggle remote console");
	Console()->Register("toggle_lua_console", "", CFGFLAG_CLIENT, ConToggleLuaConsole, this, "Toggle Lua console");
	Console()->Register("clear_local_console", "", CFGFLAG_CLIENT, ConClearLocalConsole, this, "Clear local console");
	Console()->Register("clear_remote_console", "", CFGFLAG_CLIENT, ConClearRemoteConsole, this, "Clear remote console");
	Console()->Register("dump_local_console", "", CFGFLAG_CLIENT, ConDumpLocalConsole, this, "Dump local console");
	Console()->Register("dump_remote_console", "", CFGFLAG_CLIENT, ConDumpRemoteConsole, this, "Dump remote console");
	Console()->Register("lua", "r", CFGFLAG_CLIENT, Con_Lua, this, "Executes a lua line!");

	Console()->Chain("console_output_level", ConchainConsoleOutputLevelUpdate, this);
	Console()->Chain("cl_irc_nick", ConchainIRCNickUpdate, this); // TODO: This may be moved to elsewhere

}

void CGameConsole::OnStateChange(int NewState, int OldState)
{
}
