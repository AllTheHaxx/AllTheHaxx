/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#undef luaL_dostring
#define luaL_dostring(L,s)	\
	(luaL_loadstring(L, s) || lua_pcall(L, 0, LUA_MULTRET, 0))

#include <base/tl/sorted_array.h>

#include <math.h>

#include <game/generated/client_data.h>

#include <base/system.h>

#include <engine/serverbrowser.h>
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
#include <algorithm>

#include <game/client/ui.h>

#include <game/version.h>

#include <game/client/lineinput.h>
#include <game/client/render.h>
#include <engine/client/luabinding.h>

#include "controls.h"
#include "binds.h"
#include "chat.h"
#include "menus.h"
#include "irc.h"
#include "fontmgr.h"
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

	m_aUser[0] = '\0';
	m_UseUser = false;

	m_IsCommand = false;
}

void CGameConsole::CInstance::Init(CGameConsole *pGameConsole)
{
	m_pGameConsole = pGameConsole;

	InitLua();
};

void CGameConsole::CInstance::InitLua()
{
	#if defined(FEATURE_LUA)
	if(m_Type == CONSOLETYPE_LUA)
	{
		m_LuaHandler.m_pLuaState = luaL_newstate();

		//lua_atpanic(m_pLuaState, CLua::Panic);
		//lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

		// load everything for the lua console
		luaL_openlibs(m_LuaHandler.m_pLuaState);
		luaopen_base(m_LuaHandler.m_pLuaState);
		luaopen_math(m_LuaHandler.m_pLuaState);
		luaopen_string(m_LuaHandler.m_pLuaState);
		luaopen_table(m_LuaHandler.m_pLuaState);
		luaopen_io(m_LuaHandler.m_pLuaState);
		luaopen_os(m_LuaHandler.m_pLuaState);
		luaopen_debug(m_LuaHandler.m_pLuaState);
		luaopen_bit(m_LuaHandler.m_pLuaState);
		luaopen_jit(m_LuaHandler.m_pLuaState);
		luaopen_ffi(m_LuaHandler.m_pLuaState); // don't know about this yet. could be a sand box leak.

		m_LuaHandler.m_Inited = false;
		m_LuaHandler.m_ScopeCount = 0;

		LoadLuaFile("data/luabase/events.lua");

		CGameConsole::m_pStatLuaConsole = this;
	}
	#endif

}

void CGameConsole::CInstance::ClearBacklog()
{
	m_Backlog.Init();
	m_BacklogActLine = 0;
	m_SearchFound = 0;
}

void CGameConsole::CInstance::ClearHistory()
{
	m_History.Init();
	m_pHistoryEntry = 0;
}

void CGameConsole::CInstance::ExecuteLine(const char *pLine)
{
	if(m_Type == CGameConsole::CONSOLETYPE_LOCAL)
	{
#if defined(FEATURE_LUA)
		{
			bool DiscardCommand = false;
			if(g_Config.m_ClLua)
			{
				for(int ijdfg = 0; ijdfg < CLua::Client()->Lua()->GetLuaFiles().size(); ijdfg++)
				{
					CLuaFile *pLF = CLua::Client()->Lua()->GetLuaFiles()[ijdfg];
					if(pLF->State() != CLuaFile::STATE_LOADED)
						continue;
					LuaRef lfunc = pLF->GetFunc("OnConsoleCommand");
					if(lfunc) try { if(lfunc(pLine)) DiscardCommand = true; CLua::Client()->LuaCheckDrawingState(pLF->L(), "OnConsoleCommand"); } catch(std::exception &e) { CLua::Client()->Lua()->HandleException(e, pLF); }
				}
				LuaRef confunc = getGlobal(CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pLuaState, "OnConsoleCommand");
				if(confunc) try { if(confunc(pLine)) DiscardCommand = true; } catch(std::exception &e) { printf("LUA EXCEPTION: console: %s\n", e.what()); }
			}
			if(DiscardCommand)
				return;
		}
#endif
		m_pGameConsole->m_pConsole->ExecuteLine(pLine);
	}
	else if(m_Type == CGameConsole::CONSOLETYPE_REMOTE)
	{
		if(m_pGameConsole->Client()->RconAuthed())
			m_pGameConsole->Client()->Rcon(pLine);
		else
		{
			// handle rcon login information
			if(UsingUserAuth() && !UserGot())
			{
				if(str_length(pLine) > 0)
					str_copyb(m_aUser, pLine);
				else
					m_aUser[0] = 1; // hack: set the empty-user flag
			}
			else
			{
				m_pGameConsole->Client()->RconAuth(GetUser(), pLine);
				ResetRconLogin();
			}
		}
	}
	else if(m_Type == CGameConsole::CONSOLETYPE_LUA && g_Config.m_ClLua)
	{
#if defined(FEATURE_LUA)
		if(str_comp(pLine, "!help") == 0)
		{
			PrintLine("> ---------------------------[ LUACONSOLE HELP ]---------------------------");
			PrintLine("> In this console you can execute Lua code!");
			PrintLine("> Try it by typing 'print(\"Hello World!\")', you'll see the result right on the screen.");
			PrintLine("> ");
			PrintLine("> Meta-Commands:");
			PrintLine(">     !help - view this help");
			PrintLine(">     !reset - reset the current multi-line entry");
			PrintLine(">     !reload - re-init the lua console; everything you did here will be lost!");
			PrintLine("");
			return;
		}
		else if(str_comp(pLine, "!reset") == 0)
		{
			m_LuaHandler.m_ScopeCount = 0;
			m_LuaHandler.m_FullLine = "";
			m_LuaHandler.m_pDebugChild = 0;
			PrintLine("Reset complete");
			return;
		}
		else if(str_comp(pLine, "!reload") == 0)
		{
			m_LuaHandler.m_ScopeCount = 0;
			m_LuaHandler.m_FullLine = "";
			m_LuaHandler.m_Inited = false;
			m_LuaHandler.m_pDebugChild = 0;
			lua_close(m_LuaHandler.m_pLuaState);
			InitLua();
			PrintLine("Reload complete");
			return;
		}
		int Status = 0;
		bool ScopeIncreased = false;

		if(!m_LuaHandler.m_Inited)  // this is yet quite retarded!
		{
			CLuaFile::RegisterLuaCallbacks(m_LuaHandler.m_pLuaState);

			// override stuff that has to be different for the lua console
			lua_register(m_LuaHandler.m_pLuaState, "print", CGameConsole::PrintLuaLine);

			m_LuaHandler.m_Inited = true;
		}

		// SCOPING DETECT!
		std::string ActLine(pLine); // cuz after an elseif is no extra end!
		if(ActLine.find("while") == 0 || ActLine.find("function") == 0 || (ActLine.find("if") == 0 && ActLine.find("elseif") == std::string::npos) || ActLine.find("for") == 0)
		{
			m_LuaHandler.m_ScopeCount++;
			ScopeIncreased = true;
		}
		if(ActLine.find("end") != std::string::npos)  //NO ELSE IF HERE
		{
			// this is a bit tricky D: because e.g. Game.Emote:Send will also trigger the search for 'end' :D
			// so we remove all whitespaces and check again
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

				if(m_LuaHandler.m_ScopeCount == 0 &&
						m_LuaHandler.m_FullLine.c_str()[0] != '\0' && str_comp(aBuf, m_LuaHandler.m_FullLine.c_str()) != 0)  //if we are now at zero after decreasing => print new line! (but only if it was multiline entry)
				{
					PrintLine(pLine);
					//PrintLine(aBuf, true);
					//PrintLine("", true);
				}
			}
		}

		m_LuaHandler.m_FullLine.append(pLine);
		m_LuaHandler.m_FullLine.append(" ");

		if(m_LuaHandler.m_ScopeCount == 0)
		{
			lua_State *L = m_LuaHandler.m_pLuaState;
			if(m_LuaHandler.m_pDebugChild != NULL)
				L = m_LuaHandler.m_pDebugChild;

			PrintLine(("> " + m_LuaHandler.m_FullLine).c_str(), true);
			if(m_LuaHandler.m_FullLine.c_str()[0] == '=')
				m_LuaHandler.m_FullLine = std::string("return ") + std::string(m_LuaHandler.m_FullLine.c_str()+1);

			try
			{
				lua_pushcclosure(L, CLua::ErrorFunc, 0);
				if(luaL_loadstring(L, m_LuaHandler.m_FullLine.c_str()) != 0)
					m_pGameConsole->m_pClient->Client()->Lua()->HandleException(lua_tostring(L, -1), L);
				else
				{
					int OriginalStackSize = lua_gettop(L);
					Status = lua_pcall(L, 0, LUA_MULTRET, OriginalStackSize-1);

					if(Status == 0)
					{
						/*
						 * at this point, the stack will have 1 additional function (errorfunc) and all return values on top.
						 */
						int nresults = lua_gettop(L)+1 - OriginalStackSize;
						int FirstResult = lua_gettop(L)+1 - nresults;
						//for(int i = 1; i <= lua_gettop(L); i++) // this one is for debugging purposes (prints the whole stack)
						for(int i = FirstResult; i < FirstResult+nresults; i++)
						{
							if(lua_isstring(L, i))
							{
								char aBuf[512];
								if(!lua_isnumber(L, i))
									str_formatb(aBuf, "\"%s\"", lua_tostring(L, i)); // real fancy!
								else
									str_formatb(aBuf, "%s", lua_tostring(L, i));
								PrintLine(aBuf);
							}
							else
							{
								int StackTopBefore = lua_gettop(L);
								lua_getglobal(L, "tostring"); // push the function we want to call
								lua_pushvalue(L, i); // copy the current result onto the top of the stack as the argument to 'tostring'
								if(lua_pcall(L, 1, LUA_MULTRET, 0) == 0) // multi-return to support custom tostring functions (will only use the last return; but otherwise our stack would go nuts)
								{
									int StackTopAfter = lua_gettop(L);
									PrintLine(lua_tostring(L, StackTopAfter));
									if(StackTopAfter - StackTopBefore > 0)
										lua_pop(L, StackTopAfter - StackTopBefore); // pop the results of tostring() -> stack is back to its previous state
								}
								else // if calling tostring fails, fall back to the old output
								{
									int StackTopAfter = lua_gettop(L);
									if(StackTopAfter - StackTopBefore > 0)
										lua_pop(L, StackTopAfter - StackTopBefore); // remove the error message of pcall if any
									if(lua_isstring(L, i) || lua_isnumber(L, i))
										PrintLine(lua_tostring(L, i));
									else
										PrintLine(luaL_typename(L, i));
								}
							}
						}
						lua_pop(L, nresults+1); // clean up the stack
					}
				}
			}
			catch(std::exception &e)  //just to be sure...
			{
				if(m_LuaHandler.m_pDebugChild)
					m_pGameConsole->m_pClient->Client()->Lua()->HandleException(e, m_LuaHandler.m_pDebugChild);
				PrintLine(e.what());
			}
			catch(...)
			{
				PrintLine("An unknown error occured!");
			}

			m_LuaHandler.m_FullLine = "";
			m_LuaHandler.m_ScopeCount = 0;

		}
		else if(m_LuaHandler.m_ScopeCount < 0)
		{
			PrintLine("Please don't end your stuff before even having started.");
			m_LuaHandler.m_ScopeCount = 0;
			m_LuaHandler.m_FullLine = "";
		}

		if(ScopeIncreased || m_LuaHandler.m_ScopeCount > 0)   //bigger 0
		{
			char aBuf[512] = { 0 };

			int Limit = ScopeIncreased ? m_LuaHandler.m_ScopeCount-1 : m_LuaHandler.m_ScopeCount;

			if(ActLine.find("else") != std::string::npos)
				Limit--;

			for(int i = 0; i < Limit; i++)
				str_append(aBuf, "…     ", sizeof(aBuf));

			str_append(aBuf, pLine, sizeof(aBuf));
			if(Limit >= 0)
				PrintLine(aBuf);
		}
#endif // defined(FEATURE_LUA)
	}
}

void CGameConsole::CInstance::PossibleCommandsCompleteCallback(const char *pStr, void *pUser)
{
	CGameConsole::CInstance *pInstance = (CGameConsole::CInstance *)pUser;
	if(pInstance->m_CompletionChosen == pInstance->m_CompletionEnumerationCount)
		pInstance->m_Input.Set(pStr);
	pInstance->m_CompletionEnumerationCount++;
}

void CGameConsole::CInstance::OnInput(IInput::CEvent Event)
{
	bool Handled = false;

	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		if(m_pGameConsole->Input()->KeyIsPressed(KEY_LCTRL) || m_pGameConsole->Input()->KeyIsPressed(KEY_RCTRL))
		{
			Handled = true;

			// handle paste
			if(Event.m_Key == KEY_V)
			{
				const char *pText = m_pGameConsole->Input()->GetClipboardText();
				if(pText)
				{
					char aLine[256];
					int i, Begin = 0;
					for(i = 0; i < str_length(pText); i++)
					{
						if(pText[i] == '\n')
						{
							int max = min(i - Begin + 1, (int)sizeof(aLine));
							str_copy(aLine, pText + Begin, max);
							Begin = i+1;
							ExecuteLine(aLine);
							while(pText[i] == '\n') i++;
						}
					}
					pText += Begin;

					char aRightPart[256];
					str_copy(aRightPart, m_Input.GetString() + m_Input.GetCursorOffset(), sizeof(aRightPart));
					str_copy(aLine, m_Input.GetString(), min(m_Input.GetCursorOffset()+1, (int)sizeof(aLine)));
					str_append(aLine, pText, sizeof(aLine));
					str_append(aLine, aRightPart, sizeof(aLine));
					m_Input.Set(aLine);
					m_Input.SetCursorOffset(str_length(aLine)-str_length(aRightPart));
				}
			}
			else if(Event.m_Key == KEY_C || Event.m_Key == KEY_X) // copy/cut
			{
				m_pGameConsole->Input()->SetClipboardText(m_Input.GetString());
				if(Event.m_Key == KEY_X)
					m_Input.Clear();
			}
			else if(Event.m_Key == KEY_F) // handle searching
			{
				if(!m_pSearchString)
					m_pSearchString = m_Input.GetString();
				else
					m_pSearchString = 0;
			}
			else if(Event.m_Key == KEY_LEFT || Event.m_Key == KEY_RIGHT || Event.m_Key == KEY_BACKSPACE || Event.m_Key == KEY_DELETE)
			{
				// handle skipping: jump to spaces and special ASCII characters
				CLineInput::HandleSkipping(Event, &m_Input);
			}
			else if(Event.m_Key == KEY_T && m_Type == CONSOLETYPE_REMOTE && !m_pGameConsole->Client()->RconAuthed()) // toggle rcon login mode
			{
				if(UserAuthAvailable() || m_UseUser || (!m_UseUser && m_pGameConsole->Input()->KeyIsPressed(KEY_LSHIFT)))
				{
					m_UseUser ^= true;
					m_Input.Clear();
					mem_zerob(m_aUser);
				}
				else
				{
					m_UseUser = false;
					PrintLine("User specific auth is only available on DDNet (use CTRL-SHIFT-T to force it)");
				}
			}
			else if(Event.m_Key == KEY_L && m_Type == CONSOLETYPE_REMOTE) // logout from rcon
			{
				if(m_pGameConsole->Client()->RconAuthed())
				{
					m_pGameConsole->Client()->Rcon("logout");
					ResetRconLogin();
					if(!m_pSearchString)
						m_Input.Clear();
				}
				else
					PrintLine("Not logged in!");
			}
			else if(Event.m_Key == KEY_O) // quick clear
			{
				ClearBacklog();
				if(m_pSearchString)
					m_Input.Clear();
			}
			else if(Event.m_Key == KEY_H) // handle searching
			{
				PrintLine("");
				PrintLine("-------------- BEGIN KEY COMMAND HELP --------------");
				PrintLine("These commands are in combination with CTRL.");
				PrintLine("");
				if(m_pGameConsole->Input()->KeyIsPressed(KEY_LSHIFT)) // verbose mode
				{
					PrintLine("H: Show this help text");
					PrintLine("C: copy command line to clipboard");
					PrintLine("X: cut commandline to clipboard");
					PrintLine("V: paste clipboard text");
				}
				PrintLine("F: Toggle search mode");
				PrintLine("O: Clear the current console");
				if(m_Type == CONSOLETYPE_REMOTE)
				{
					PrintLine("L: logout from rcon");
					PrintLine("T: toggle between user-login (new) and masterpw-login (old) - only available on DDNet server");
				}
				if(m_Type == CONSOLETYPE_LUA)
					PrintLine("Type '!help' to get an overview of special lua console commands");
				else
					PrintLine("");
				PrintLine("--------------- END KEY COMMAND HELP ---------------");
				PrintLine("");
			}
			else if(Event.m_Key == KEY_MOUSE_WHEEL_UP) // make fontsize larger
			{
				g_Config.m_ClMonoFontSize = clamp(g_Config.m_ClMonoFontSize + 1, 6, 16);
			}
			else if(Event.m_Key == KEY_MOUSE_WHEEL_DOWN) // make fontsize smaller
			{
				g_Config.m_ClMonoFontSize = clamp(g_Config.m_ClMonoFontSize - 1, 6, 16);
			}
			else if(Event.m_Key != KEY_LCTRL && Event.m_Key != KEY_RCTRL && Event.m_Key != KEY_LSHIFT)
			{
				char aLine[256];
				std::string KeyName(m_pGameConsole->Input()->KeyName(Event.m_Key));
				std::transform(KeyName.begin(), KeyName.end(), KeyName.begin(), ::toupper);
				str_format(aLine, sizeof(aLine), "*** unknown key command 'CTRL-%s'. Try CTRL-H", KeyName.c_str());
				PrintLine(aLine);
			}

			// end CTRL-keycombos
		}
		else if(Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER)
		{
			//if search is actice, skip to next position
			if(m_pSearchString)
			{
				if(m_AtEnd == 2)
					m_AtEnd = 0;

				m_SearchFound = false;
			}
			else
			{
				if(m_Input.GetString()[0] || (UsingUserAuth() && !m_pGameConsole->Client()->RconAuthed() && !UserGot())) // the second part allows for empty user names
				{
					if(m_Type == CONSOLETYPE_LOCAL || (m_Type == CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed()))
					{
						// don't clutter up our history with history entries of repeated lines (usually by using the up arrow)
						if(m_History.Last() == NULL || str_comp(m_History.Last(), m_Input.GetString()) != 0)
						{
							char *pEntry = m_History.Allocate(m_Input.GetLength() + 1);
							mem_copy(pEntry, m_Input.GetString(), (unsigned int)(m_Input.GetLength() + 1));
						}
					}
					else if(m_Type == CONSOLETYPE_LUA)
					{
						//if(m_LuaHandler.m_FullLine.size())
						//	m_LuaHandler.m_FullLine.resize(m_LuaHandler.m_FullLine.size()-1);  //remove the last " "

						std::string Complete = m_LuaHandler.m_FullLine;
						Complete.append(m_Input.GetString());

						//add this to the history :3
						char *pEntry = m_History.Allocate((int)(Complete.size() + 1));
						mem_copy(pEntry, Complete.c_str(), (unsigned int)(Complete.size() + 1));
					}
					ExecuteLine(m_Input.GetString());
					m_Input.Clear();
					m_pHistoryEntry = 0x0;
				}
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
		else if(Event.m_Key == KEY_TAB && !m_pSearchString)
		{
			if(m_Type == CGameConsole::CONSOLETYPE_LOCAL || (m_Type == CGameConsole::CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed()))
			{
				if(m_ReverseTAB)
					m_CompletionChosen--;
				else
					m_CompletionChosen++;
				m_CompletionEnumerationCount = 0;
				m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask,
															 m_Type == CGameConsole::CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands(),
															 PossibleCommandsCompleteCallback, this);

				// handle wrapping
				if(m_CompletionEnumerationCount && (m_CompletionChosen >= m_CompletionEnumerationCount || m_CompletionChosen <0))
				{
					m_CompletionChosen= (m_CompletionChosen + m_CompletionEnumerationCount) %  m_CompletionEnumerationCount;
					m_CompletionEnumerationCount = 0;
					m_pGameConsole->m_pConsole->PossibleCommands(m_aCompletionBuffer, m_CompletionFlagmask,
																 m_Type == CGameConsole::CONSOLETYPE_REMOTE && m_pGameConsole->Client()->RconAuthed() && m_pGameConsole->Client()->UseTempRconCommands(),
																 PossibleCommandsCompleteCallback, this);
				}
			}
		}
		else if(Event.m_Key == KEY_PAGEUP)
		{
			m_BacklogActLine += 27;
			Handled = true;
		}
		else if(Event.m_Key == KEY_PAGEDOWN)
		{
			m_BacklogActLine -= 27;

			if(m_BacklogActLine < 0)
				m_BacklogActLine = 0;
			Handled = true;
		}
		else if(Event.m_Key == KEY_MOUSE_WHEEL_UP)
		{
			++m_BacklogActLine;
			Handled = true;
		}
		else if(Event.m_Key == KEY_MOUSE_WHEEL_DOWN)
		{
			--m_BacklogActLine;
			if(m_BacklogActLine < 0)
				m_BacklogActLine = 0;
			Handled = true;
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
		if(Event.m_Key != KEY_TAB && Event.m_Key != KEY_LSHIFT && Event.m_Key != KEY_LCTRL && Event.m_Key != KEY_RCTRL)
		{
			m_CompletionChosen = -1;
			str_copy(m_aCompletionBuffer, m_Input.GetString(), sizeof(m_aCompletionBuffer));
			m_CompletionRenderOffset = 0.0f;
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
		if(!m_pSearchString)
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

		if(m_pSearchString && !Handled)
		{
			m_BacklogActLine = 0;
			m_SearchFound = false;
			m_NoFound = false;
			m_AtEnd = 0;
		}
	}
}

void CGameConsole::CInstance::PrintLine(const char *pLine, bool Highlighted)
{
	if(!pLine)
		return;

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
#if defined(FEATURE_LUA)
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
#else
	return false;
#endif
}

bool CGameConsole::CInstance::UserAuthAvailable() const
{
	return IsDDNet(m_pGameConsole->Client()->GetServerInfo());
}

void CGameConsole::CInstance::ResetRconLogin()
{
	mem_zerob(m_aUser);
	m_UseUser = false;
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
	if(m_pSearchString)
		return;

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

	static float s_OverlayAlpha = 0.0f;
	if(m_pClient->m_pMenus->MouseUnlocked())
	{
		smooth_set(&s_OverlayAlpha, 0.15f, 40.0f, Client()->RenderFrameTime());
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		CUIRect Warning, Text;
		const float Fade = (const float)((sin(Client()->LocalTime()) + 1.0) / 4.0 + 0.5);
		Screen.HSplitTop(200.0f, &Warning, 0);
		Warning.Margin(Warning.w/3.2f, &Warning);
		Warning.HMargin(Warning.h/2.85f, &Warning);
		Warning.h -= Warning.h*2.0f;
		Warning.y -= Warning.h;
		RenderTools()->DrawUIRect(&Warning, vec4(0,0,0,Fade), CUI::CORNER_ALL, 3.0f);
		TextRender()->TextColor(1,1,1,Fade);

		//Warning.HMargin(-10.0f, &Text);
		Warning.HSplitTop(20.0f, 0, &Text);
		//UI()->DoLabelScaled(&Text, Localize("MOUSE UNLOCKED"), 17.0f, 0, Warning.w-5.0f);
		{
			const float SIZE = 27.0f;
			const char *pText = Localize("MOUSE UNLOCKED");
			const float w = TextRender()->TextWidth(0, SIZE, pText, str_length(pText));
			TextRender()->Text(0, Text.x+Text.w/2-w/2, Text.y, SIZE, pText, Text.w);
		}

		Warning.HSplitMid(&Text, &Warning);
		//RenderTools()->DrawUIRect(&Text, vec4(1,0,0,Fade), 0, 0.0f);
		{
			const float SIZE = 17.0f;

			char aBuf[256];
			const char *pBoundKey = m_pClient->m_pBinds->GetKey("+unlock_mouse");
			if(pBoundKey && pBoundKey[0] != '\0')
				str_format(aBuf, sizeof(aBuf), Localize("Press %s to switch back to ingame mouse"), pBoundKey);
			else
				str_format(aBuf, sizeof(aBuf), Localize("Use the command 'unlock_mouse' in f1"), pBoundKey/*, m_pClient->m_pBinds->GetKey("toggle_local_console")*/);
			//UI()->DoLabelScaled(&Text, aBuf, 17.0f, 0, Warning.w-5.0f);

			const float w = TextRender()->TextWidth(0, SIZE, aBuf, str_length(aBuf));
			TextRender()->Text(0, Text.x+Text.w/2-w/2, Text.y+Text.h+SIZE/2, SIZE, aBuf, Text.w);
		}
	}
	else
		smooth_set(&s_OverlayAlpha, 0.0f, 40.0f, Client()->RenderFrameTime());

	if(s_OverlayAlpha > 0.0f)
		RenderTools()->DrawUIRect(&Screen, vec4(0,0,0,s_OverlayAlpha), 0, 0);
	TextRender()->TextColor(1,1,1,1);

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
	{
		float FontSize = (float)g_Config.m_ClMonoFontSize;
		float RowHeight = FontSize*1.25f;
		float x = 3;
		float y = ConsoleHeight - RowHeight - 5.0f;
		CFont * const pConFont = m_pClient->m_pFontMgrMono->GetFont(FONT_REGULAR);

		// clipboard selection
		static bool mousePress = false;
		static float copyStart = -1.0f;
		static float copyEnd = -1.0f;
		static int copyIndexStart = -1;
		static int copyIndexEnd = -1;
		static int selectedLine = -1;
		static CUIRect textRect;
		textRect.h = FontSize+3.0f;
		static std::string sText;

		CRenderInfo Info;
		Info.m_pSelf = this;
		Info.m_WantedCompletion = pConsole->m_CompletionChosen;
		Info.m_EnumCount = 0;
		Info.m_Offset = pConsole->m_CompletionRenderOffset;
		Info.m_Width = Screen.w;
		Info.m_pCurrentCmd = pConsole->m_aCompletionBuffer;
		TextRender()->SetCursor(&Info.m_Cursor, x+Info.m_Offset, y+RowHeight+2.0f, FontSize, TEXTFLAG_RENDER, pConFont);

		// render prompt
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER, pConFont);
		char aPrompt[128] = { ">" };
		if(m_ConsoleType == CONSOLETYPE_REMOTE)
		{
			if(Client()->State() == IClient::STATE_ONLINE)
			{
				if(Client()->RconAuthed())
					str_copyb(aPrompt, "rcon>");
				else
				{
					if(pConsole->UsingUserAuth())
					{
						if(!pConsole->UserGot())
							str_copyb(aPrompt, "[CTRL-T] Enter Username>");
						else
							str_formatb(aPrompt, "[CTRL-T] Enter Password for '%s'>", pConsole->GetUser());
					}
					else
						str_copyb(aPrompt, "Enter Password>");
				}
			}
			else
				str_copyb(aPrompt, "NOT CONNECTED!");
		}
		else if(m_ConsoleType == CONSOLETYPE_LUA)
		{
			if(g_Config.m_ClLua)
			{
				if(m_LuaConsole.m_LuaHandler.m_pDebugChild)
				{
					CLuaFile *pLF = CLuaBinding::GetLuaFile(m_LuaConsole.m_LuaHandler.m_pDebugChild);
					str_format(aPrompt, sizeof(aPrompt), "(%s) DEBUGGER>", pLF->GetFilename()+4);
					str_copyb(aPrompt, aPrompt);
				}
				else
					str_copyb(aPrompt, "Lua>");
			}
			else
				str_copyb(aPrompt, "Lua disabled. Please enable Lua first.");
		}
		if(m_pSearchString)
			str_copyb(aPrompt, "[CTRL+F] SEARCHING»");
		str_appendb(aPrompt, " ");

		// notify the user nothing can be found
		if(m_pSearchString && (pConsole->m_NoFound || pConsole->m_AtEnd == 2))
		{
			vec3 crgb = HslToRgb(vec3(g_Config.m_ClMessageHighlightHue / 255.0f, g_Config.m_ClMessageHighlightSat / 255.0f, g_Config.m_ClMessageHighlightLht / 255.0f));
			TextRender()->TextColor(crgb.r, crgb.g, crgb.b, 1);
		}
		TextRender()->TextEx(&Cursor, aPrompt, -1);

		x = Cursor.m_X;


		// console text editing
		bool Editing = false;
		int EditingCursor = Input()->GetEditingCursor();
		if (Input()->GetIMEState())
		{
			if(str_length(Input()->GetIMECandidate()))
			{
				pConsole->m_Input.Editing(Input()->GetIMECandidate(), EditingCursor);
				Editing = true;
			}
		}

		// hide rcon password
		char aInputString[512];
		str_copy(aInputString, pConsole->m_Input.GetString(Editing), sizeof(aInputString));
		if(m_ConsoleType == CONSOLETYPE_REMOTE)
			if(Client()->State() == IClient::STATE_ONLINE && !Client()->RconAuthed() && !m_pSearchString &&
				(pConsole->UsingUserAuth() ? pConsole->UserGot() : true))
		{
			for(int i = 0; i < pConsole->m_Input.GetLength(Editing); ++i)
				aInputString[i] = '*';
		}

		// clipboard selection
		if (copyIndexStart != -1)
		{
			textRect.x = copyStart;
			textRect.w = copyEnd-copyStart;
			RenderTools()->DrawUIRect(&textRect, vec4(0.0f,0.0f,1.0f,1.0f), 0, 0.0f);
		}

		// render console input (wrap line)
		TextRender()->SetCursor(&Cursor, x, y, FontSize, 0, pConFont);
		Cursor.m_LineWidth = Screen.w - 10.0f - x;
		TextRender()->TextEx(&Cursor, aInputString, pConsole->m_Input.GetCursorOffset(Editing));
		TextRender()->TextEx(&Cursor, aInputString+pConsole->m_Input.GetCursorOffset(Editing), -1);
		int Lines = Cursor.m_LineCount;

		y -= (Lines - 1) * FontSize;
		TextRender()->SetCursor(&Cursor, x, y, FontSize, TEXTFLAG_RENDER, pConFont);
		Cursor.m_LineWidth = Screen.w - 10.0f - x;

		TextRender()->TextEx(&Cursor, aInputString, pConsole->m_Input.GetCursorOffset(Editing));
		static float MarkerOffset = TextRender()->TextWidth(pConFont, FontSize, "|", -1)/3;
		CTextCursor Marker = Cursor;
		Marker.m_X -= MarkerOffset;
		Marker.m_LineWidth = -1;
		TextRender()->TextEx(&Marker, "|", -1);
		TextRender()->TextEx(&Cursor, aInputString+pConsole->m_Input.GetCursorOffset(Editing), -1);

		// render possible commands
		if(!m_pSearchString && m_ConsoleType != CONSOLETYPE_LUA && (m_ConsoleType == CONSOLETYPE_LOCAL || (m_ConsoleType == CONSOLETYPE_REMOTE && Client()->RconAuthed())))
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


		// new console rendering
		CInstance::CBacklogEntry *pEntry = pConsole->m_Backlog.Last();

		// scrolling
		if(pConsole->m_SearchFound)
		{
			pConsole->m_BacklogActLineOld = 0; // reset var
			for(int i = 0; i < pConsole->m_BacklogActLine; i++)
			{
				if(!pEntry || ((pEntry == pConsole->m_Backlog.First()) && str_length(pEntry->m_aText) >= 3))
				{
					pConsole->m_BacklogActLine = pConsole->m_BacklogActLineOld;
					if(m_pSearchString)
						pConsole->m_AtEnd = 1;
					break;//stop at first entry
				}
				else
				{
					pConsole->m_BacklogActLineOld++;
					pEntry = pConsole->m_Backlog.Prev(pEntry);
				}
			}
		}
		else
		{
			if(m_pSearchString && m_pSearchString[0] != 0)
			{
				for(int i = 0; i < 1024; i++)
				{
					if(pEntry && pEntry != pConsole->m_Backlog.First() && str_length(pEntry->m_aText) >= 3)
					{
						if(!str_find_nocase(pEntry->m_aText, m_pSearchString))
						{
							pEntry = pConsole->m_Backlog.Prev(pEntry);
							pConsole->m_BacklogActLine++;
							continue;
						}
						else
						{
							pConsole->m_BacklogActLineOld = pConsole->m_BacklogActLine;
							break;
						}
					}
					else
					{
						//skips to last position if nothing can be found.
						pConsole->m_BacklogActLine = pConsole->m_BacklogActLineOld;
						pConsole->m_NoFound = true;
					}
				}
				//lets reset at end
				pConsole->m_SearchFound = true;
			}
			else
			{
				//reset
				pConsole->m_SearchFound = true;
				pConsole->m_NoFound = false;
			}
		}

		if(pConsole->m_AtEnd == 1)
		{
			pConsole->m_BacklogActLine = 0;
			pConsole->m_NoFound = false;
			pConsole->m_AtEnd = 2;
			pConsole->m_SearchFound = false;
			pEntry = pConsole->m_Backlog.Last();
		}

		float OffsetY = 0.0f;
		float LineOffset = 1.0f;
		int lineNum = 0;
		for(int iter = 0; iter < 32; iter++)
		{
			if(pEntry)
			{
				vec3 rgb(0.7f, 0.7f, 0.7f);
				if(m_ConsoleType == CONSOLETYPE_LOCAL)
				{
					#define StartsWith(TAG) (str_comp_num(pEntry->m_aText, TAG": ", str_length(TAG)+2) == 0)
					if (StartsWith("[serv]")) // system message
						rgb = HslToRgb(vec3(g_Config.m_ClMessageSystemHue / 255.0f, g_Config.m_ClMessageSystemSat / 255.0f, g_Config.m_ClMessageSystemLht / 255.0f));
					else if (StartsWith("[chat]")) // translator
						rgb = vec3(1.0f, 1.0f, 1.0f);
					else if (StartsWith("[chat]: [*Translator*]")) // translator
						rgb = vec3(0.45f, 0.45f, 1.0f);
					else if (StartsWith("[teamchat]: [*Lua*]")) // lua
						rgb = vec3(1.0f, 0.45f, 0.45f);
					else if (StartsWith("[teamchat]"))
						rgb = HslToRgb(vec3(g_Config.m_ClMessageTeamHue / 255.0f, g_Config.m_ClMessageTeamSat / 255.0f, g_Config.m_ClMessageTeamLht / 255.0f));
					else if (pEntry->m_Highlighted)
						rgb = HslToRgb(vec3(g_Config.m_ClMessageHighlightHue / 255.0f, g_Config.m_ClMessageHighlightSat / 255.0f, g_Config.m_ClMessageHighlightLht / 255.0f));
//					else
//						rgb = HslToRgb(vec3(g_Config.m_ClMessageHue / 255.0f, g_Config.m_ClMessageSat / 255.0f, g_Config.m_ClMessageLht / 255.0f));
					#undef StartsWith
				}

				// get y offset (calculate it if we haven't yet)
				if(pEntry->m_YOffset < 0.0f)
				{
					TextRender()->SetCursor(&Cursor, 0.0f, 0.0f, FontSize, 0, pConFont);
					Cursor.m_LineWidth = Screen.w-10;
					TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1);
					TextRender()->TextEx(&Cursor, pEntry->m_aText, -1);
					pEntry->m_YOffset = Cursor.m_Y+Cursor.m_FontSize+LineOffset;
				}

				TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1);

				OffsetY += pEntry->m_YOffset;

				//	next page when lines reach the top
				if(y-OffsetY <= RowHeight)
					break;

				// clipboard selection
				int mx, my;
				Input()->NativeMousePos(&mx, &my);
				Graphics()->MapScreen(UI()->Screen()->x, UI()->Screen()->y, UI()->Screen()->w, UI()->Screen()->h);
				mx = (mx / (float)Graphics()->ScreenWidth()) * Screen.w;
				my = (my / (float)Graphics()->ScreenHeight()) * Screen.h;

				int strWidth = round_to_int(TextRender()->TextWidth(Cursor.m_pFont, FontSize, sText.c_str(), sText.length()));
				CUIRect seltextRect(0, y - OffsetY, strWidth, FontSize + 3.0f);

				if(my > seltextRect.y && my < seltextRect.y + seltextRect.h)
				{
					if(selectedLine == -1)
						sText = pEntry->m_aText;

					float offacumx = 0;
					float charwi = 0;
					unsigned i = 0;
					for(i = 0; i < sText.length(); i++)
					{
						char toAn[2] = {sText.at(i), '\0'};
						charwi = TextRender()->TextWidth(pConFont, FontSize, toAn, 1);
						if(mx >= offacumx && mx <= offacumx + charwi)
							break;

						offacumx += charwi;
					}

					if(mousePress && copyStart == -1)
					{
						textRect.y = seltextRect.y;
						textRect.h = seltextRect.h;
						copyStart = offacumx;
						copyEnd = offacumx + charwi;
						copyIndexStart = i;
						copyIndexEnd = i + 1;
						selectedLine = lineNum;
					}
					if(mousePress && copyStart != -1)
					{
						copyEnd = offacumx;
						if(copyEnd < copyStart)
						{
							copyEnd = copyStart;
							copyIndexEnd = copyIndexStart;
						}
						else
							copyIndexEnd = i;
					}

					if(Input()->NativeMousePressed(1))
					{
						mousePress = true;
					}
				}

				if(!Input()->NativeMousePressed(1) && mousePress)
				{
					if(copyIndexStart != -1 && !sText.empty() && copyIndexEnd <= (signed)sText.length())
					{
						Input()->SetClipboardTextSTD(sText.substr(copyIndexStart, copyIndexEnd - copyIndexStart));
						//dbg_msg("Clipboard", "Copied '%s' to clipboard...", sText.substr(copyIndexStart, copyIndexEnd-copyIndexStart).c_str());
					}

					copyStart = -1.0f;
					copyEnd = -1.0f;
					copyIndexStart = -1;
					copyIndexEnd = -1;
					selectedLine = -1;
					sText.clear();
					mousePress = false;
				}
				// -------------------- end clipboard selection code -------------------

				//url highlighting
				TextRender()->SetCursor(&Cursor, 0.0f, y-OffsetY, FontSize, TEXTFLAG_RENDER, pConFont);
				Cursor.m_LineWidth = Screen.w-10.0f;

				const char *pCursor = pEntry->m_aText;
				const char *pUrlBeginning = pEntry->m_aText;
				const char *pUrlEnding;

				char aUrl[64];
				int UrlSize;

				CUIRect TextRect;

				bool Found = false;

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
						pUrlEnding++;
					}

					UrlSize = pUrlEnding - pUrlBeginning;
					str_copy(aUrl, pUrlBeginning, UrlSize + 1);

					// url rect
					TextRect.x = TextRender()->TextWidth(pConFont, FontSize, pEntry->m_aText, pUrlBeginning - pEntry->m_aText);
					TextRect.y = y - OffsetY+3.0f;
					TextRect.w = TextRender()->TextWidth(pConFont, FontSize, pUrlBeginning, UrlSize);
					TextRect.h = FontSize;

					// render the first part
					if(pUrlBeginning - pCursor > 0)
					{
						TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
						TextRender()->TextEx(&Cursor, pCursor, pUrlBeginning - pCursor);
					}

					// set the color and check if pressed
					if(g_Config.m_Debug)
						RenderTools()->DrawUIRect(&TextRect, vec4(1,0,1,0.4f), 0, 0);
					if(UI()->MouseInsideNative(mx, my, &TextRect))
					{
						TextRender()->TextColor(0.0f, 1.0f, 0.39f, 1.0f);
						static float s_LastClicked = 0.0f;
						if(Input()->MouseDoubleClickNative() && Client()->SteadyTimer() > s_LastClicked + 1.0f)
						{
							TextRender()->TextColor(1.0f, 0.0f, 0.39f, 1.0f);
							s_LastClicked = Client()->SteadyTimer();
							Input()->Clear();
							//((IEngineGraphics *)Kernel()->RequestInterface<IEngineGraphics>())->Minimize();
							open_default_browser(aUrl);
						}
					}
					else
						TextRender()->TextColor(0.15f, 0.48f, 0.87f, 1.0f);

					// render the link
					TextRender()->TextEx(&Cursor, pUrlBeginning, UrlSize);

					// render the rest
					TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
					TextRender()->TextEx(&Cursor, pUrlEnding, str_length(pUrlEnding));

					pCursor = pUrlEnding;
				}

				//search function
				if(!Found)//no url to highlight
				{
					// highlight the parts that matches
					if(m_pSearchString && m_pSearchString[0] != '\0')
					{
						const char *pText = pEntry->m_aText;

						for(int i = 0; i < 2048; i++) // makes sure we doesn't get stuck in an infinite loop
						{
							if(pText)
							{
								const char *pFoundStr = str_find_nocase(pText, m_pSearchString);
								if(pFoundStr)
								{
									TextRender()->TextEx(&Cursor, pText, (int)(pFoundStr - pText));
									TextRender()->TextColor(0.8f, 0.7f, 0.15f, 1);
									TextRender()->TextEx(&Cursor, pFoundStr, str_length(m_pSearchString));
									TextRender()->TextColor(rgb.r, rgb.g, rgb.b, 1.0f);
									//TextRender()->TextEx(&Cursor, pFoundStr+str_length(m_pSearchString), -1);
									pText = pFoundStr + str_length(m_pSearchString);
								}
								else
								{
									TextRender()->TextEx(&Cursor, pText, -1);
									break;
								}

								if(pText > pEntry->m_aText + str_length(pEntry->m_aText) - 1 || pText < pEntry->m_aText)
									pText = 0;
							}
							else
								break;
						}
					}
					else
					{
						TextRender()->TextEx(&Cursor, pEntry->m_aText, -1);
						//dbg_msg("console-color-debug", "(%.2f|%.2f|%.2f) %s", rgb.r, rgb.g, rgb.b, pEntry->m_aText);
					}
				}

				if(pEntry != pConsole->m_Backlog.First())
					pEntry = pConsole->m_Backlog.Prev(pEntry);
				else
					break;
			}

			lineNum++;

			// reset color
			TextRender()->TextColor(1,1,1,1);

			if(g_Config.m_ClConsoleLowCPU)
				thread_sleep(1); //be nice to our cpu!
		}

		// render page
		char aBuf[128];
		TextRender()->TextColor(1,1,1,1);
		str_format(aBuf, sizeof(aBuf), Localize("Page %i, Line %d"), pConsole->m_BacklogActLine/27+1, pConsole->m_BacklogActLine%27+1);
		TextRender()->Text(0, 10.0f, 0.0f, FontSize, aBuf, -1);

		// render version
		str_format(aBuf, sizeof(aBuf), "v%s - AllTheHaxx %s", GAME_VERSION, ALLTHEHAXX_VERSION);
		float Width = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->Text(0, Screen.w-Width-10.0f, 0.0f, FontSize, aBuf, -1);
		str_format(aBuf, sizeof(aBuf), "built on %s", BUILD_DATE);
		Width = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->Text(0, Screen.w-Width-10.0f, 10.0f, FontSize, aBuf, -1);
	}
}

void CGameConsole::OnMessage(int MsgType, void *pRawMsg)
{
}

bool CGameConsole::OnInput(IInput::CEvent Event)
{
	// accept input when opening, but not at first frame to discard the input that caused the console to open
	if(m_ConsoleState != CONSOLE_OPEN && (m_ConsoleState != CONSOLE_OPENING || m_StateChangeEnd == TimeNow()+m_StateChangeDuration))
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
		;//Input()->MouseModeRelative();
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

		if (m_ConsoleState == CONSOLE_CLOSED || m_ConsoleState == CONSOLE_CLOSING) // WHEN OPENING
		{
			Input()->MouseModeAbsolute();
			m_pClient->m_pMenus->UseMouseButtons(false);
			m_ConsoleState = CONSOLE_OPENING;
			/*// reset controls - no don't do it because we want swag!
			m_pClient->m_pControls->OnReset();*/

			Input()->SetIMEState(true);
		}
		else // WHEN CLOSING
		{
			Input()->MouseModeRelative();
			m_pClient->m_pMenus->UseMouseButtons(m_pClient->m_pMenus->IsActive() || m_pClient->m_pChat->IsActive());
			m_pClient->OnRelease();
			m_ConsoleState = CONSOLE_CLOSING;

			if(!m_pClient->m_pMenus->IsActive() && !m_pClient->m_pChat->IsActive())
				Input()->SetIMEState(false);
		}
	}

	m_ConsoleType = Type;

	if(m_ConsoleState == CONSOLE_CLOSING && m_pClient->m_pMenus->MouseUnlocked())
		m_pClient->m_pMenus->ToggleMouseMode();
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

void CGameConsole::PrintLine(int Type, const char *pLine)
{
	if(Type == CONSOLETYPE_REMOTE)
		m_RemoteConsole.PrintLine(pLine);
	else if(Type == CONSOLETYPE_LUA)
		m_LuaConsole.PrintLine(pLine);
	else
		m_LocalConsole.PrintLine(pLine);
}

int CGameConsole::PrintLuaLine(lua_State *L)
{
#if defined(FEATURE_LUA)
	int nargs = lua_gettop(L);
	if(nargs < 1)
		return luaL_error(L, "print expects 1 argument or more");

	// construct the message from all arguments
	char aLine[512] = {0};
	for(int i = 1; i <= nargs; i++)
	{
		if(lua_isnil(L, i)) // allow nil
			str_append(aLine, "<nil>", sizeof(aLine));
		else
		{
			argcheck(lua_isstring(L, i) || lua_isnumber(L, i), i, "string or number");
			str_append(aLine, lua_tostring(L, i), sizeof(aLine));
		}
		str_append(aLine, "    ", sizeof(aLine));
	}
	aLine[str_length(aLine)-1] = '\0'; // remove the last tab character

	// pop all to clean up the stack
	lua_pop(L, nargs);

	dbg_msg("LUA|console", "%s", aLine);

	CGameConsole::m_pStatLuaConsole->PrintLine(aLine);
#endif
	return 0;
}

void CGameConsole::AttachLuaDebugger(const CLuaFile *pLF)
{
	//m_LuaConsole.ClearBacklog();
	for(int i = 0; i < 27-10; i++)
		m_LuaConsole.PrintLine("");

	m_LuaConsole.m_LuaHandler.m_ScopeCount = 0;
	m_LuaConsole.m_LuaHandler.m_FullLine = "";
	m_LuaConsole.m_LuaHandler.m_pDebugChild = pLF->L();

#if defined(FEATURE_LUA)
	luaL_loadstring(pLF->L(), "Import(\"debug\")");
	lua_pcall(pLF->L(), 0, LUA_MULTRET, 0);
#endif

	m_LuaConsole.PrintLine("> ---------------------------[ DEBUGGER STARTED ]---------------------------");
	{ char aBuf[256]; str_format(aBuf, sizeof(aBuf), "> The lua debugger was attached to the script '%s'", pLF->GetFilename()); m_LuaConsole.PrintLine(aBuf);}
	m_LuaConsole.PrintLine("> Everything you execute from here will now be executed in the scope of the script.");
	m_LuaConsole.PrintLine("> Please note that you can break the running script by doing the wrong changes.");
	m_LuaConsole.PrintLine("> ");
	m_LuaConsole.PrintLine("> The debug library has automatically been imported.");
	m_LuaConsole.PrintLine("> You may now use locals(), globals() and upvalues() to print all variables.");
	m_LuaConsole.PrintLine("> ");
	m_LuaConsole.PrintLine("> To exit the debugger, type !reset or !reload");
	m_LuaConsole.PrintLine("> ");

	Toggle(CONSOLETYPE_LUA);

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
	Console()->Register("clear_local_console", "", CFGFLAG_CLIENT, ConClearLocalConsole, this, "Clear local console");
	Console()->Register("clear_remote_console", "", CFGFLAG_CLIENT, ConClearRemoteConsole, this, "Clear remote console");
	Console()->Register("dump_local_console", "", CFGFLAG_CLIENT, ConDumpLocalConsole, this, "Dump local console");
	Console()->Register("dump_remote_console", "", CFGFLAG_CLIENT, ConDumpRemoteConsole, this, "Dump remote console");

#if defined(FEATURE_LUA)
	Console()->Register("toggle_lua_console", "", CFGFLAG_CLIENT, ConToggleLuaConsole, this, "Toggle Lua console");
	// XXX security leak! the lua console has elevated permissions, but this one can be used by every script!
	//Console()->Register("lua", "r", CFGFLAG_CLIENT, Con_Lua, this, "Executes a lua line!");
#endif

	Console()->Chain("console_output_level", ConchainConsoleOutputLevelUpdate, this);

}

void CGameConsole::OnStateChange(int NewState, int OldState)
{
}
